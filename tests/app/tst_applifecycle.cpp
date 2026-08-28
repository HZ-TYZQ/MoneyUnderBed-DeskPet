// 应用生命周期与辅助窗口所有权。
//
// docs/Decisions.md 第 14.6 节：关闭设置或关于窗口只关闭该窗口，不退出桌宠；
// 同一种辅助窗口只允许一个实例且可反复打开；只有明确的退出请求结束进程。
//
// 这些断言取代不了真实桌面验收（第 11.1 节），但根因是一条纯粹的应用层策略，
// 可以在 offscreen 下确定性地覆盖。

#include "app/AppLifecycle.h"

#include <QDialog>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

using mub::app::AppLifecycle;
using AuxiliaryWindow = mub::app::AppLifecycle::AuxiliaryWindow;

class TestAppLifecycle final : public QObject
{
    Q_OBJECT

private slots:
    // 必须是第一个测试：它断言的是进程启动时的 Qt 默认值，
    // 后面的测试会把这条策略关掉。
    void qtDefaultQuitPolicyIsTheRootCause();

    void takeOverQuitPolicyDisablesTheDefault();
    void closingAnAuxiliaryWindowLeavesTheCharacterAlone();
    void auxiliaryWindowsReopenAsTheSameInstance();
    void duplicateRegistrationIsRejected();
    void showingAnUnregisteredWindowIsHarmless();
    void everyQuitEntryCollapsesIntoOneRequest();
};

void TestAppLifecycle::qtDefaultQuitPolicyIsTheRootCause()
{
    // 角色窗口是 Qt::Tool，不计入「最后一个窗口」；设置窗口是 QDialog，计入。
    // 只要这条默认值为真，关闭设置窗口就会连带退出进程。
    QVERIFY(QGuiApplication::quitOnLastWindowClosed());
}

void TestAppLifecycle::takeOverQuitPolicyDisablesTheDefault()
{
    AppLifecycle lifecycle;
    QVERIFY(!lifecycle.quitPolicyTakenOver());

    lifecycle.takeOverQuitPolicy();

    QVERIFY(lifecycle.quitPolicyTakenOver());
    QVERIFY(!QGuiApplication::quitOnLastWindowClosed());
}

void TestAppLifecycle::closingAnAuxiliaryWindowLeavesTheCharacterAlone()
{
    AppLifecycle lifecycle;
    lifecycle.takeOverQuitPolicy();
    const QSignalSpy quitting(&lifecycle, &AppLifecycle::quitting);

    // 角色窗口用与产品相同的窗口类型：它正是不计入退出规则的那一个。
    QWidget character;
    character.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    character.show();

    QDialog settings;
    lifecycle.setAuxiliaryWindow(AuxiliaryWindow::Settings, &settings);
    lifecycle.showAuxiliaryWindow(AuxiliaryWindow::Settings);
    QVERIFY(settings.isVisible());

    settings.close();
    QCoreApplication::processEvents();

    QVERIFY(!settings.isVisible());
    // 关闭只是隐藏：窗口对象仍然存在，没有被改写成删除或自定义隐藏。
    QCOMPARE(lifecycle.auxiliaryWindow(AuxiliaryWindow::Settings), &settings);
    QVERIFY(character.isVisible());
    QVERIFY(!lifecycle.quitRequested());
    QCOMPARE(quitting.count(), 0);
}

void TestAppLifecycle::auxiliaryWindowsReopenAsTheSameInstance()
{
    AppLifecycle lifecycle;
    lifecycle.takeOverQuitPolicy();

    QDialog settings;
    QDialog about;
    lifecycle.setAuxiliaryWindow(AuxiliaryWindow::Settings, &settings);
    lifecycle.setAuxiliaryWindow(AuxiliaryWindow::About, &about);

    for (int round = 0; round < 3; ++round) {
        lifecycle.showAuxiliaryWindow(AuxiliaryWindow::Settings);
        lifecycle.showAuxiliaryWindow(AuxiliaryWindow::About);
        QVERIFY(settings.isVisible());
        QVERIFY(about.isVisible());
        QCOMPARE(lifecycle.auxiliaryWindow(AuxiliaryWindow::Settings), &settings);
        QCOMPARE(lifecycle.auxiliaryWindow(AuxiliaryWindow::About), &about);

        settings.close();
        about.close();
        QCoreApplication::processEvents();
        QVERIFY(!settings.isVisible());
        QVERIFY(!about.isVisible());
    }

    QVERIFY(!lifecycle.quitRequested());
}

void TestAppLifecycle::duplicateRegistrationIsRejected()
{
    AppLifecycle lifecycle;
    QDialog first;
    QDialog second;

    lifecycle.setAuxiliaryWindow(AuxiliaryWindow::Settings, &first);
    QTest::ignoreMessage(
        QtWarningMsg,
        "refusing to replace an already registered auxiliary window 0");
    lifecycle.setAuxiliaryWindow(AuxiliaryWindow::Settings, &second);

    QCOMPARE(lifecycle.auxiliaryWindow(AuxiliaryWindow::Settings), &first);
}

void TestAppLifecycle::showingAnUnregisteredWindowIsHarmless()
{
    AppLifecycle lifecycle;

    QTest::ignoreMessage(QtWarningMsg,
                         "no auxiliary window registered for 1");
    lifecycle.showAuxiliaryWindow(AuxiliaryWindow::About);

    QCOMPARE(lifecycle.auxiliaryWindow(AuxiliaryWindow::About), nullptr);
    QVERIFY(!lifecycle.quitRequested());
}

void TestAppLifecycle::everyQuitEntryCollapsesIntoOneRequest()
{
    AppLifecycle lifecycle;
    const QSignalSpy quitting(&lifecycle, &AppLifecycle::quitting);

    // 角色右键菜单与托盘可能在同一轮里都发出退出请求。
    lifecycle.requestQuit();
    lifecycle.requestQuit();
    lifecycle.requestQuit();

    QVERIFY(lifecycle.quitRequested());
    QCOMPARE(quitting.count(), 1);
}

QTEST_MAIN(TestAppLifecycle)
#include "tst_applifecycle.moc"
