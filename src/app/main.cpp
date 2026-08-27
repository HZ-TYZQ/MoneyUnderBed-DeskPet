#include "app/DiagnosticLog.h"
#include "app/SelfTest.h"
#include "app/StartupFailureReport.h"
#include "character/AnimationClip.h"
#include "character/SpriteSheet.h"
#include "core/AppMetadata.h"
#include "platform/BackendFactory.h"
#include "platform/DeskPetWindowBackend.h"
#include "platform/StartupProbe.h"
#include "core/BubbleFrequency.h"
#include "core/RandomSource.h"
#include "core/Settings.h"
#include "core/SettingsStore.h"
#include "core/TimeSource.h"
#include "ui/AboutWindow.h"
#include "ui/CharacterPresenter.h"
#include "ui/CharacterWindow.h"
#include "ui/DialogueController.h"
#include "ui/SettingsWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QRandomGenerator>
#include <QSettings>
#include <QString>

#include <memory>

namespace {

Q_LOGGING_CATEGORY(lcMain, "mub.app")

constexpr int kAssetFailureExitCode = 4;

// 启动时的初始动画。方向映射会在第一次移动后接管。
constexpr auto kStartupClipId = u"idle-down-left";

// 供 --scale 的报错信息使用，避免把允许集合在两处各写一遍。
QString allowedScaleList()
{
    QStringList values;
    for (const int scale : mub::core::allowedScales()) {
        values.append(QString::number(scale));
    }
    return values.join(QStringLiteral(", "));
}

} // namespace

int main(int argc, char *argv[])
{
    // 身份是静态设置，必须早于日志，否则日志目录路径不正确。
    mub::metadata::apply();

    // 消息处理器必须早于 QApplication：Qt 在构造期间加载平台插件，
    // 失败时直接 qFatal 终止进程，程序拿不到控制权
    // （docs/Decisions.md 第 8.2 节）。
    mub::app::installDiagnosticLog();

    const mub::platform::StartupProbeResult probe =
        mub::platform::probeWindowBackend();
    if (!probe.ok) {
        return mub::app::reportStartupFailure(argc, argv, probe.reason,
                                              probe.detail);
    }

    QApplication application(argc, argv);

    qCInfo(lcMain).noquote()
        << QStringLiteral("version=%1 qt=%2 requested_platform=%3 actual_platform=%4 probe=%5")
               .arg(mub::metadata::versionString())
               .arg(QString::fromLatin1(qVersion()))
               .arg(probe.selectedPlatform)
               .arg(QGuiApplication::platformName())
               .arg(probe.detail);

    QCommandLineParser parser;
    parser.setApplicationDescription(mub::metadata::unofficialNotice());
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption selfTestOption(
        QStringLiteral("self-test"),
        QCoreApplication::translate(
            "main", "运行无交互自检后退出，结果以退出码为准。"));
    // 命令行倍率只用于开发和排查，覆盖本次运行，不写回配置文件。
    const QCommandLineOption scaleOption(
        QStringLiteral("scale"),
        QCoreApplication::translate("main", "本次运行的角色显示倍率，覆盖已保存的设置。"),
        QCoreApplication::translate("main", "倍率"));
    parser.addOption(selfTestOption);
    parser.addOption(scaleOption);
    parser.process(application);

    if (parser.isSet(selfTestOption)) {
        return mub::app::runSelfTest();
    }

    // 设置走标准用户配置目录：默认构造的 QSettings 已经由 QStandardPaths
    // 决定位置，不会写在 EXE、AppImage 或当前工作目录旁边
    // （docs/Decisions.md 第 5.1 节）。
    QSettings settingsBackend;
    mub::core::SettingsStore settingsStore(settingsBackend);
    mub::core::Settings settings = settingsStore.load();

    if (parser.isSet(scaleOption)) {
        bool scaleOk = false;
        const int requested = parser.value(scaleOption).toInt(&scaleOk);
        if (!scaleOk || !mub::core::isAllowedScale(requested)) {
            qCCritical(lcMain).noquote()
                << QStringLiteral("invalid scale %1; allowed values are %2")
                       .arg(parser.value(scaleOption), allowedScaleList());
            return 2;
        }
        settings.scale = requested;
    }
    const int integerScale = settings.scale;

    const QString sheetPath =
        mub::character::clipResourcePath(kStartupClipId);
    mub::character::SpriteSheetError sheetError =
        mub::character::SpriteSheetError::None;
    mub::character::SpriteSheet sheet =
        mub::character::SpriteSheet::load(sheetPath, &sheetError);
    if (!sheet.isValid()) {
        qCCritical(lcMain).noquote()
            << QStringLiteral("could not load %1: %2")
                   .arg(sheetPath,
                        mub::character::describeSpriteSheetError(sheetError));
        return kAssetFailureExitCode;
    }

    const std::unique_ptr<mub::platform::DeskPetWindowBackend> backend =
        mub::platform::createWindowBackend();

    mub::ui::CharacterWindow window(std::move(sheet), integerScale,
                                    backend.get());
    window.moveToCursorScreenBottom();
    window.show();

    // 产品使用单调时钟与随机种子；测试通过注入替换两者，
    // 使行为序列在假时钟和固定种子下完全可重复。
    const mub::core::MonotonicTimeSource timeSource;
    mub::core::SeededRandomSource random(QRandomGenerator::global()->generate());

    mub::ui::CharacterPresenter presenter(window, timeSource, random);

    // 气泡与角色使用同一倍率（docs/Decisions.md 第 4.8 节）。
    mub::ui::DialogueController dialogue(presenter, window, timeSource, random,
                                         backend.get());
    presenter.setBubbleHost(&dialogue);

    // 平台是否支持固定到全部工作区由后端自述，界面据此决定是否显示该项，
    // 而不是自己判断当前运行在哪个系统上（第 5.1、8.4 节）。
    mub::ui::SettingsWindow settingsWindow(
        backend->capabilities().workspacePinning);

    const auto applySettings = [&](const mub::core::Settings &next) {
        settings = mub::core::sanitized(next);
        presenter.applySettings(settings);
        dialogue.setScale(settings.scale);
    };

    // 修改后立即生效并保存，不设「应用」阶段（第 5.1 节）。
    QObject::connect(&settingsWindow, &mub::ui::SettingsWindow::settingsChanged,
                     &application, [&](const mub::core::Settings &next) {
                         applySettings(next);
                         settingsStore.save(settings);
                     });
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::settingsChanged,
                     &application, [&](const mub::core::Settings &next) {
                         applySettings(next);
                         settingsStore.save(settings);
                         settingsWindow.setSettings(settings);
                     });
    QObject::connect(&settingsWindow,
                     &mub::ui::SettingsWindow::restoreDefaultsRequested, &application,
                     [&] {
                         settingsStore.restoreDefaults();
                         applySettings(settingsStore.load());
                         settingsWindow.setSettings(settings);
                     });
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::settingsRequested,
                     &settingsWindow, [&] {
                         settingsWindow.setSettings(settings);
                         settingsWindow.show();
                         settingsWindow.raise();
                         settingsWindow.activateWindow();
                     });
    // 托盘尚未实现，诊断信息里先如实报告为不可用。
    mub::ui::AboutWindow aboutWindow(backend->capabilities().name, false);
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::aboutRequested,
                     &aboutWindow, [&aboutWindow] {
                         aboutWindow.show();
                         aboutWindow.raise();
                         aboutWindow.activateWindow();
                     });

    QObject::connect(&presenter, &mub::ui::CharacterPresenter::quitRequested,
                     &application, [&application, &dialogue] {
                         // 退出不保留待恢复的对话页面（第 4.2 节）。
                         dialogue.stop();
                         application.quit();
                     });

    applySettings(settings);
    settingsWindow.setSettings(settings);
    presenter.start();

    return application.exec();
}
