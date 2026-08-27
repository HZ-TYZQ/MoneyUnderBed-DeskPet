#include "platform/SessionMonitor.h"
#include "platform/SessionMonitorFactory.h"

#include <QSignalSpy>
#include <QTest>

#include <memory>

using mub::core::SessionSuspendReason;
using mub::platform::SessionMonitor;

class FakeSessionMonitor final : public SessionMonitor
{
public:
    using SessionMonitor::SessionMonitor;

    bool start(QWindow *) override { return true; }
    void report(const SessionSuspendReason reason, const bool suspended)
    {
        setReason(reason, suspended);
    }
};

class TestSessionMonitor final : public QObject
{
    Q_OBJECT

private slots:
    void emitsOnlyWhenTheAggregateStateChanges();
    void realMonitorStartsBestEffortWithoutCrashing();
};

void TestSessionMonitor::emitsOnlyWhenTheAggregateStateChanges()
{
    FakeSessionMonitor monitor;
    QSignalSpy changes(&monitor, &SessionMonitor::suspendedChanged);

    monitor.report(SessionSuspendReason::Locked, true);
    monitor.report(SessionSuspendReason::Sleeping, true);
    monitor.report(SessionSuspendReason::Locked, false);
    QCOMPARE(changes.count(), 1);
    QVERIFY(monitor.isSuspended());

    monitor.report(SessionSuspendReason::Sleeping, false);
    QCOMPARE(changes.count(), 2);
    QCOMPARE(changes.at(0).at(0).toBool(), true);
    QCOMPARE(changes.at(1).at(0).toBool(), false);
    QVERIFY(!monitor.isSuspended());
}

void TestSessionMonitor::realMonitorStartsBestEffortWithoutCrashing()
{
    const std::unique_ptr<SessionMonitor> monitor =
        mub::platform::createSessionMonitor();
    QVERIFY(monitor != nullptr);
    // CI 可能没有 logind，Windows 的原生实现则需要真实 QWindow；这里验证
    // 降级路径安全，真实事件由阶段 8 人工清单覆盖。
    static_cast<void>(monitor->start(nullptr));
}

QTEST_APPLESS_MAIN(TestSessionMonitor)
#include "tst_sessionmonitor.moc"
