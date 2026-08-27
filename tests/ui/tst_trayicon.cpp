#include "core/ActivityMode.h"
#include "ui/TrayIcon.h"

#include <QSignalSpy>
#include <QSystemTrayIcon>
#include <QTest>

using mub::core::ActivityMode;
using mub::ui::TrayIcon;

class TestTrayIcon final : public QObject
{
    Q_OBJECT

private slots:
    void reportsWhetherTheDesktopProvidesATray();
    void staysInertWhenNoTrayIsAvailable();
    void updatesAreSafeRegardlessOfAvailability();
};

void TestTrayIcon::reportsWhetherTheDesktopProvidesATray()
{
    TrayIcon tray;
    // 是否建立起图标必须与「桌面是否提供托盘」一致，不能自称建好了却没有。
    QCOMPARE(tray.isActive(), TrayIcon::isAvailable());
}

// 第 3.3 节：托盘只作为备用入口，托盘不可用时不影响其余功能。
// 原生 GNOME 通常没有托盘，这是主要降级路径，不是错误。
void TestTrayIcon::staysInertWhenNoTrayIsAvailable()
{
    if (TrayIcon::isAvailable()) {
        QSKIP("当前环境提供托盘，无法在此验证降级路径。");
    }

    TrayIcon tray;
    QVERIFY(!tray.isActive());

    QSignalSpy shows(&tray, &TrayIcon::showCharacterRequested);
    QSignalSpy modes(&tray, &TrayIcon::modeChangeRequested);
    QSignalSpy quits(&tray, &TrayIcon::quitRequested);

    // 没有托盘时不应该凭空发出任何请求。
    tray.setMode(ActivityMode::Active);
    tray.setCharacterVisible(false);
    QCoreApplication::processEvents();

    QCOMPARE(shows.count(), 0);
    QCOMPARE(modes.count(), 0);
    QCOMPARE(quits.count(), 0);
}

// 同步状态的调用在两种环境下都必须安全：没有托盘时是空操作，不是崩溃。
void TestTrayIcon::updatesAreSafeRegardlessOfAvailability()
{
    TrayIcon tray;
    QSignalSpy modes(&tray, &TrayIcon::modeChangeRequested);

    tray.setMode(ActivityMode::Active);
    tray.setMode(ActivityMode::Quiet);
    tray.setCharacterVisible(true);
    tray.setCharacterVisible(false);
    QCoreApplication::processEvents();

    // setMode 是「把界面同步到当前状态」，不是用户操作，
    // 因此不得反过来再报告一次模式变化，否则会形成回环。
    QCOMPARE(modes.count(), 0);
}

QTEST_MAIN(TestTrayIcon)
#include "tst_trayicon.moc"
