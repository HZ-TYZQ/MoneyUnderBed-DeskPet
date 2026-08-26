#include "app/DiagnosticLog.h"
#include "app/SelfTest.h"
#include "app/StartupFailureReport.h"
#include "character/CharacterAssets.h"
#include "character/SpriteSheet.h"
#include "core/AppMetadata.h"
#include "platform/BackendFactory.h"
#include "platform/DeskPetWindowBackend.h"
#include "platform/StartupProbe.h"
#include "ui/CharacterWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QString>

#include <memory>

namespace {

Q_LOGGING_CATEGORY(lcMain, "mub.app")

constexpr int kAssetFailureExitCode = 4;

// 阶段 3 只显示一个方向的待机角色。
// 完整的方向映射与动画播放在阶段 4 加入。
constexpr auto kStartupSheetId = "idle-down-left";

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
        mub::character::spriteSheetPath(QLatin1String(kStartupSheetId));
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

    return application.exec();
}
