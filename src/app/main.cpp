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
#include "core/RandomSource.h"
#include "core/Settings.h"
#include "core/SettingsStore.h"
#include "core/TimeSource.h"
#include "dialogue/DialogueData.h"
#include "app/AppLifecycle.h"
#include "app/SettingsController.h"
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

    // 第 14.6 节：进程的生命周期由应用层显式管理。必须早于任何窗口——角色与
    // 气泡是 Qt::Tool 不计入「最后一个窗口」，一旦设置或关于窗口被关闭，Qt 的
    // 默认策略就会退出整个应用。
    mub::app::AppLifecycle lifecycle;
    lifecycle.takeOverQuitPolicy();

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
    QApplication::setWindowIcon(
        QIcon(mub::dialogue::faceAssetPath(QStringLiteral("natural"))));

    // 设置走标准用户配置目录：默认构造的 QSettings 已经由 QStandardPaths
    // 决定位置，不会写在 EXE、AppImage 或当前工作目录旁边
    // （docs/Decisions.md 第 5.1 节）。
    QSettings settingsBackend;
    mub::core::SettingsStore settingsStore(settingsBackend);

    // 第 14.2 节：应用层统一持有唯一运行时设置，负责校验、套用和自动保存。
    mub::app::SettingsController controller(settingsStore);

    if (parser.isSet(scaleOption)) {
        bool scaleOk = false;
        const int requested = parser.value(scaleOption).toInt(&scaleOk);
        if (!scaleOk || !mub::core::isAllowedScale(requested)) {
            qCCritical(lcMain).noquote()
                << QStringLiteral("invalid scale %1; allowed values are %2")
                       .arg(parser.value(scaleOption), allowedScaleList());
            return 2;
        }
        // 第 5.1 节：命令行倍率只覆盖本次运行，不写回配置文件。
        mub::core::Settings overridden = controller.settings();
        overridden.appearance.scale = requested;
        controller.applyForThisRunOnly(overridden);
    }
    const int integerScale = controller.settings().appearance.scale;

    const QString sheetPath =
        mub::character::clipAssetPath(kStartupClipId);
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
    lifecycle.setAuxiliaryWindow(mub::app::AppLifecycle::AuxiliaryWindow::Settings,
                                 &settingsWindow);

    mub::ui::TrayIcon *trayForSettings = nullptr;
    mub::ui::AboutWindow *aboutWindowForSettings = nullptr;

    // 运行时消费者按领域订阅（第 6.3 节）。
    // 这里不再手工拼「套用并保存」的 lambda，也不再有第二份运行时设置。
    QObject::connect(&controller, &mub::app::SettingsController::settingsChanged,
                     &application, [&](const mub::core::Settings &next) {
                         presenter.applySettings(next);
                         if (aboutWindowForSettings != nullptr) {
                             aboutWindowForSettings->setSettings(next);
                         }
                         if (trayForSettings != nullptr) {
                             trayForSettings->setMode(next.behavior.mode);
                         }
                         settingsWindow.setSettings(next);
                     });
    QObject::connect(&controller, &mub::app::SettingsController::dialogueChanged,
                     &application, [&](const mub::core::DialogueSettings &next) {
                         dialogue.applyDialogueSettings(next);
                     });
    QObject::connect(&controller, &mub::app::SettingsController::appearanceChanged,
                     &application, [&](const mub::core::AppearanceSettings &next) {
                         dialogue.setScale(next.scale);
                     });

    // 第 14.8 节：拖动过程中立即生效但不逐帧落盘；一次编辑完成才写一次。
    QObject::connect(&settingsWindow, &mub::ui::SettingsWindow::settingsEdited,
                     &controller, &mub::app::SettingsController::apply);
    QObject::connect(&settingsWindow, &mub::ui::SettingsWindow::settingsCommitted,
                     &controller, &mub::app::SettingsController::applyAndPersist);
    // 界面已经取得用户确认，控制器收到即执行（第 14.2 节）。
    QObject::connect(&settingsWindow, &mub::ui::SettingsWindow::groupResetRequested,
                     &controller, &mub::app::SettingsController::resetGroup);
    QObject::connect(&settingsWindow, &mub::ui::SettingsWindow::resetAllRequested,
                     &controller, &mub::app::SettingsController::resetAll);

    // 角色右键菜单的活动模式切换走同一个控制器，设置窗口打开时看到的是同一份真相。
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::settingsChanged,
                     &controller, &mub::app::SettingsController::applyAndPersist);
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::settingsRequested,
                     &settingsWindow, [&] {
                         settingsWindow.setSettings(controller.settings());
                         lifecycle.showAuxiliaryWindow(
                             mub::app::AppLifecycle::AuxiliaryWindow::Settings);
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
        presenter.setHidden(false);
        window.show();
        window.raise();
        tray.setCharacterVisible(true);
    };
    const auto hideCharacter = [&] {
        // 第 4.2 节：隐藏时立即结束当前对话，不保留待恢复的页面。
        dialogue.stop();
        // 隐藏期间角色不再自主行动，也不再自己弹气泡：占住协调器的最高
        // 优先级事件并冻结自主行为，否则看不见的角色会继续在桌面上说话。
        presenter.setHidden(true);
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
                         mub::core::Settings next = controller.settings();
                         next.behavior.mode = mode;
                         controller.applyAndPersist(next);
                     });
    QObject::connect(&tray, &mub::ui::TrayIcon::quitRequested, &presenter,
                     &mub::ui::CharacterPresenter::quitRequested);

    mub::ui::AboutWindow aboutWindow(backend->capabilities().name, tray.isActive());
    aboutWindowForSettings = &aboutWindow;
    lifecycle.setAuxiliaryWindow(mub::app::AppLifecycle::AuxiliaryWindow::About,
                                 &aboutWindow);
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::aboutRequested,
                     &aboutWindow, [&lifecycle] {
                         lifecycle.showAuxiliaryWindow(
                             mub::app::AppLifecycle::AuxiliaryWindow::About);
                     });

    // 角色右键菜单与托盘都汇入同一条显式退出路径，清理只做一次。
    QObject::connect(&presenter, &mub::ui::CharacterPresenter::quitRequested,
                     &lifecycle, &mub::app::AppLifecycle::requestQuit);
    QObject::connect(&lifecycle, &mub::app::AppLifecycle::quitting, &application,
                     [&application, &dialogue, &controller] {
                         // 退出不保留待恢复的对话页面（第 4.2 节）。
                         dialogue.stop();
                         // 去抖窗口里还没落盘的最后一次修改不能因为退出而丢掉。
                         controller.flush();
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
    const QImage entryIcon(
        mub::dialogue::faceAssetPath(QStringLiteral("natural")));

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

    // 启动时把已保存的设置送到全部运行时消费者。
    presenter.applySettings(controller.settings());
    dialogue.applyDialogueSettings(controller.settings().dialogue);
    dialogue.setScale(controller.settings().appearance.scale);
    tray.setMode(controller.settings().behavior.mode);
    settingsWindow.setSettings(controller.settings());
    aboutWindow.setSettings(controller.settings());
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
