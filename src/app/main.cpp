#include "app/DiagnosticLog.h"
#include "app/SelfTest.h"
#include "app/StartupFailureReport.h"
#include "character/AnimationClip.h"
#include "character/SpriteSheet.h"
#include "core/AppMetadata.h"
#include "platform/BackendFactory.h"
#include "platform/DeskPetWindowBackend.h"
#include "platform/StartupProbe.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"
#include "ui/CharacterPresenter.h"
#include "ui/CharacterWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QRandomGenerator>
#include <QString>

#include <memory>

namespace {

Q_LOGGING_CATEGORY(lcMain, "mub.app")

constexpr int kAssetFailureExitCode = 4;

// 启动时的初始动画。方向映射会在第一次移动后接管。
constexpr auto kStartupClipId = u"idle-down-left";

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
    const QCommandLineOption scaleOption(
        QStringLiteral("scale"),
        QCoreApplication::translate("main", "角色显示倍率，正整数。"),
        QCoreApplication::translate("main", "倍率"), QStringLiteral("2"));
    parser.addOption(selfTestOption);
    parser.addOption(scaleOption);
    parser.process(application);

    if (parser.isSet(selfTestOption)) {
        return mub::app::runSelfTest();
    }

    bool scaleOk = false;
    const int integerScale = parser.value(scaleOption).toInt(&scaleOk);
    if (!scaleOk || integerScale < 1 || integerScale > 8) {
        qCCritical(lcMain).noquote()
            << QStringLiteral("invalid scale %1; expected an integer from 1 to 8")
                   .arg(parser.value(scaleOption));
        return 2;
    }

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
    // 首次启动默认为安静模式（docs/Decisions.md 第 2.2 节）。
    // 设置界面在阶段 7 接管该取值。
    presenter.setMode(mub::core::ActivityMode::Quiet);
    presenter.start();

    return application.exec();
}
