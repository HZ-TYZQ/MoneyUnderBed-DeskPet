#include "app/DesktopEntry.h"
#include "app/DesktopEnvironment.h"
#include "app/DiagnosticLog.h"
#include "app/SelfTest.h"
#include "app/SingleInstance.h"
#include "app/StartupFailureReport.h"
#include "character/AnimationClip.h"
#include "character/SpriteSheet.h"
#include "core/AppMetadata.h"
#include "platform/BackendFactory.h"
#include "platform/DeskPetWindowBackend.h"
#include "platform/StartupProbe.h"
#include "platform/SessionMonitor.h"
#include "platform/SessionMonitorFactory.h"
#include "core/BubbleFrequency.h"
#include "core/RandomSource.h"
#include "core/Settings.h"
#include "core/SettingsStore.h"
#include "core/TimeSource.h"
#include "ui/AboutWindow.h"
#include "ui/CharacterPresenter.h"
#include "ui/CharacterWindow.h"
#include "ui/DialogueController.h"
#include "ui/FirstRunWindow.h"
#include "ui/SettingsWindow.h"
#include "ui/TrayIcon.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QIcon>
#include <QImage>
#include <QStandardPaths>
#include <QSettings>
#include <QScreen>
#include <QString>
#include <QTimer>
#include <QWindow>

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

void queueActivityAreaSync(mub::ui::CharacterPresenter *presenter)
{
    QTimer::singleShot(0, presenter,
                       [presenter] { presenter->syncActivityArea(); });
}

void watchScreenGeometry(QScreen *screen, mub::ui::CharacterPresenter *presenter)
{
    if (screen == nullptr) {
        return;
    }
    QObject::connect(screen, &QScreen::geometryChanged, presenter,
                     [presenter] { queueActivityAreaSync(presenter); });
    QObject::connect(screen, &QScreen::availableGeometryChanged, presenter,
                     [presenter] { queueActivityAreaSync(presenter); });
}

} // namespace

int main(int argc, char *argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();
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
    const QList<QScreen *> startupScreens = QGuiApplication::screens();
    for (qsizetype index = 0; index < startupScreens.size(); ++index) {
        const QScreen *screen = startupScreens.at(index);
        const QRect geometry = screen->geometry();
        const QRect available = screen->availableGeometry();
        qCInfo(lcMain).noquote()
            << QStringLiteral("screen=%1 geometry=%2x%3 x=%4 y=%5 available=%6x%7 ax=%8 ay=%9 dpr=%10 dpi=%11 refresh_hz=%12")
                   .arg(index)
                   .arg(geometry.width())
                   .arg(geometry.height())
                   .arg(geometry.x())
                   .arg(geometry.y())
                   .arg(available.width())
                   .arg(available.height())
                   .arg(available.x())
                   .arg(available.y())
                   .arg(screen->devicePixelRatio())
                   .arg(screen->logicalDotsPerInch())
                   .arg(screen->refreshRate());
    }

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

    // 单实例判定必须早于任何窗口：副实例的职责只是把唤回消息递出去然后退出，
    // 不应该先闪一个角色再消失（docs/Decisions.md 第 3.3 节）。
    mub::app::SingleInstance instance(mub::app::singleInstanceName());
    if (instance.acquire() == mub::app::SingleInstance::Role::Secondary) {
        qCInfo(lcMain) << "recalled the running instance; exiting";
        return 0;
    }

    if (mub::app::isNiriDesktop()) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setWindowTitle(QCoreApplication::translate("main", "未支持的桌面环境"));
        warning.setText(QCoreApplication::translate(
            "main", "检测到 niri。niri 当前不在支持范围内，角色自主移动等核心功能可能无法工作。"));
        warning.setInformativeText(QCoreApplication::translate(
            "main", "你可以退出，或以 best-effort 模式继续运行并提交 Issue / PR。"));
        QPushButton *continueButton = warning.addButton(
            QCoreApplication::translate("main", "继续运行"), QMessageBox::AcceptRole);
        warning.addButton(QCoreApplication::translate("main", "退出"),
                          QMessageBox::RejectRole);
        warning.exec();
        if (warning.clickedButton() != continueButton) {
            qCInfo(lcMain) << "the user exited after the niri compatibility warning";
            return 0;
        }
        qCWarning(lcMain) << "continuing on unsupported niri in best-effort mode";
    }

    // 应用图标同样取自作者头像素材（第 6 节）。
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/face/natural.png")));

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

    // 屏幕增删、面板尺寸变化或桌面 Shell 重启后重新取得可用区域，
    // 并把角色夹回可见范围。多显示器路径保留自动覆盖，但标为未实测。
    for (QScreen *screen : QGuiApplication::screens()) {
        watchScreenGeometry(screen, &presenter);
    }
    QObject::connect(&application, &QGuiApplication::screenAdded, &presenter,
                     [&presenter](QScreen *screen) {
                         watchScreenGeometry(screen, &presenter);
                         queueActivityAreaSync(&presenter);
                     });
    QObject::connect(&application, &QGuiApplication::screenRemoved, &presenter,
                     [&presenter](QScreen *) {
                         queueActivityAreaSync(&presenter);
                     });
    if (window.windowHandle() != nullptr) {
        QObject::connect(window.windowHandle(), &QWindow::screenChanged, &presenter,
                         [&presenter](QScreen *) {
                             queueActivityAreaSync(&presenter);
                         });
    }

    // 先连接全部消费者再启动监视器；start() 会同步发布初始锁定状态。
    const std::unique_ptr<mub::platform::SessionMonitor> sessionMonitor =
        mub::platform::createSessionMonitor();
    QObject::connect(sessionMonitor.get(),
                     &mub::platform::SessionMonitor::suspendedChanged,
                     &presenter, &mub::ui::CharacterPresenter::setSessionSuspended);
    QObject::connect(sessionMonitor.get(),
                     &mub::platform::SessionMonitor::suspendedChanged,
                     &dialogue, &mub::ui::DialogueController::setSessionSuspended);
    if (!sessionMonitor->start(window.windowHandle())) {
        qCWarning(lcMain) << "session lifecycle monitoring is unavailable";
    }

    mub::ui::SettingsWindow settingsWindow;

    mub::ui::TrayIcon *trayForSettings = nullptr;
    const auto applySettings = [&](const mub::core::Settings &next) {
        settings = mub::core::sanitized(next);
        presenter.applySettings(settings);
        dialogue.setScale(settings.scale);
        if (trayForSettings != nullptr) {
            trayForSettings->setMode(settings.mode);
        }
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
    // 第 3.3 节：托盘只作为备用入口，托盘不可用时不影响角色右键菜单。
    mub::ui::TrayIcon tray;
    trayForSettings = &tray;

    // 隐藏后必须能唤回。托盘是一条通道，再次启动程序是另一条；
    // 原生 GNOME 没有托盘时靠后者（第 3.3 节）。
    const bool recallAvailable =
        tray.isActive() || instance.role() == mub::app::SingleInstance::Role::Primary;
    presenter.setRecallAvailable(recallAvailable);

    const auto showCharacter = [&] {
        window.show();
        window.raise();
        tray.setCharacterVisible(true);
    };
    const auto hideCharacter = [&] {
        // 第 4.2 节：隐藏时立即结束当前对话，不保留待恢复的页面。
        dialogue.stop();
        window.hide();
        tray.setCharacterVisible(false);
    };

    QObject::connect(&presenter, &mub::ui::CharacterPresenter::hideRequested,
                     &application, hideCharacter);
    QObject::connect(&tray, &mub::ui::TrayIcon::showCharacterRequested, &application,
                     showCharacter);
    // 第 2.3 节：用户主动启动程序时角色必须出现，因此唤回总是显示角色。
    QObject::connect(&instance, &mub::app::SingleInstance::recallRequested,
                     &application, showCharacter);
    QObject::connect(&tray, &mub::ui::TrayIcon::modeChangeRequested, &application,
                     [&](const mub::core::ActivityMode mode) {
                         mub::core::Settings next = settings;
                         next.mode = mode;
                         applySettings(next);
                         settingsStore.save(settings);
                         settingsWindow.setSettings(settings);
                     });
    QObject::connect(&tray, &mub::ui::TrayIcon::quitRequested, &presenter,
                     &mub::ui::CharacterPresenter::quitRequested);

    mub::ui::AboutWindow aboutWindow(backend->capabilities().name, tray.isActive());
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

    // 应用菜单入口只对 AppImage 有意义。Windows 免安装 ZIP 不创建快捷方式，
    // 普通 Linux 安装包由包管理器提供入口（第 5.2 节）。
    const QString appImagePath = qEnvironmentVariable("APPIMAGE");
    const bool runningAsAppImage = !appImagePath.isEmpty();
    mub::app::DesktopEntry desktopEntry(
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/applications"),
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/icons/hicolor/256x256/apps"));
    const QImage entryIcon(QStringLiteral(":/assets/face/natural.png"));

    const auto refreshDesktopEntryState = [&] {
        settingsWindow.setDesktopEntryState(runningAsAppImage,
                                            desktopEntry.isInstalled());
    };
    QObject::connect(&settingsWindow,
                     &mub::ui::SettingsWindow::installDesktopEntryRequested,
                     &application, [&] {
                         desktopEntry.install(appImagePath, entryIcon);
                         refreshDesktopEntryState();
                     });
    QObject::connect(&settingsWindow,
                     &mub::ui::SettingsWindow::removeDesktopEntryRequested,
                     &application, [&] {
                         desktopEntry.remove();
                         refreshDesktopEntryState();
                     });

    // 用户移动 AppImage 后重新运行该文件即更新集成（第 5.2 节）。
    // 这是更新已有入口，不是静默新建：用户此前已经同意过。
    if (runningAsAppImage && desktopEntry.isInstalled()
        && desktopEntry.installedExecutable() != appImagePath) {
        qCInfo(lcMain) << "the AppImage moved; updating the application menu entry";
        desktopEntry.install(appImagePath, entryIcon);
    }

    applySettings(settings);
    settingsWindow.setSettings(settings);
    refreshDesktopEntryState();
    presenter.start();
    qCInfo(lcMain) << "startup ready elapsed_ms=" << startupTimer.elapsed();

    // 首次启动的简短提示。第 5.2 节：一页，不做多页欢迎向导。
    if (!settingsStore.firstRunNoticeShown()) {
        mub::ui::FirstRunWindow notice(runningAsAppImage);
        notice.exec();
        if (notice.wantsDesktopEntry()) {
            desktopEntry.install(appImagePath, entryIcon);
            refreshDesktopEntryState();
        }
        settingsStore.markFirstRunNoticeShown();
    }

    return application.exec();
}
