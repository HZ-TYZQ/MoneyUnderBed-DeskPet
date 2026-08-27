#include "core/SessionSuspension.h"

#include <QTest>

using mub::core::SessionSuspendReason;
using mub::core::SessionSuspensionState;

class TestSessionSuspension final : public QObject
{
    Q_OBJECT

private slots:
    void oneReasonSuspendsAndResumes();
    void repeatedNotificationsAreIdempotent();
    void allReasonsMustClearBeforeResuming();
};

void TestSessionSuspension::oneReasonSuspendsAndResumes()
{
    SessionSuspensionState state;
    QVERIFY(!state.isSuspended());

    QVERIFY(state.setSuspended(SessionSuspendReason::Locked, true));
    QVERIFY(state.isSuspended());
    QVERIFY(state.isSuspendedFor(SessionSuspendReason::Locked));

    QVERIFY(state.setSuspended(SessionSuspendReason::Locked, false));
    QVERIFY(!state.isSuspended());
}

void TestSessionSuspension::repeatedNotificationsAreIdempotent()
{
    SessionSuspensionState state;
    QVERIFY(state.setSuspended(SessionSuspendReason::Sleeping, true));
    QVERIFY(!state.setSuspended(SessionSuspendReason::Sleeping, true));
    QVERIFY(state.isSuspended());

    QVERIFY(state.setSuspended(SessionSuspendReason::Sleeping, false));
    QVERIFY(!state.setSuspended(SessionSuspendReason::Sleeping, false));
}

void TestSessionSuspension::allReasonsMustClearBeforeResuming()
{
    SessionSuspensionState state;
    QVERIFY(state.setSuspended(SessionSuspendReason::Locked, true));
    QVERIFY(!state.setSuspended(SessionSuspendReason::DisplayOff, true));

    // 解锁时显示器仍关闭，聚合状态不能恢复。
    QVERIFY(!state.setSuspended(SessionSuspendReason::Locked, false));
    QVERIFY(state.isSuspended());
    QVERIFY(state.isSuspendedFor(SessionSuspendReason::DisplayOff));

    QVERIFY(state.setSuspended(SessionSuspendReason::DisplayOff, false));
    QVERIFY(!state.isSuspended());
}

QTEST_APPLESS_MAIN(TestSessionSuspension)
#include "tst_sessionsuspension.moc"
