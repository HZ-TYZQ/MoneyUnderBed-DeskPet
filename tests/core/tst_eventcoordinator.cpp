#include "core/EventCoordinator.h"

#include <QTest>

using namespace mub::core;

Q_DECLARE_METATYPE(EventKind)
Q_DECLARE_METATYPE(EventDecision)

class TestEventCoordinator final : public QObject
{
    Q_OBJECT

private slots:
    void startsIdle();
    void anyEventIsAcceptedWhenIdle_data();
    void anyEventIsAcceptedWhenIdle();
    void pairwisePriority_data();
    void pairwisePriority();
    void frozenPriorityOrder();
    void decisionsFromTheDecisionRecord_data();
    void decisionsFromTheDecisionRecord();
    void suppressedEventsAreNotQueued();
    void replacedEventsAreNotQueued();
    void finishOnlyClearsTheCurrentKind();
    void clearRemovesAnything();
    void repeatedClicksRestartTheirOwnFeedback();
    void repeatedFeedingRequestsAreIgnored();
    void burstOfEventsEndsInTheHighestPriority();
    void noneIsAlwaysSuppressed();
    void everyKindHasAnId_data();
    void everyKindHasAnId();
};

namespace {

const QList<EventKind> kRealKinds{
    EventKind::AutonomousChatter, EventKind::ClickFeedback,
    EventKind::Dialogue, EventKind::Feeding, EventKind::Shutdown};

// 把协调器推到指定的当前事件。
EventCoordinator coordinatorWith(const EventKind current)
{
    EventCoordinator coordinator;
    if (current != EventKind::None) {
        coordinator.request(current);
    }
    return coordinator;
}

} // namespace

void TestEventCoordinator::startsIdle()
{
    const EventCoordinator coordinator;
    QCOMPARE(coordinator.current(), EventKind::None);
    QVERIFY(!coordinator.isBusy());
    QCOMPARE(coordinator.suppressedCount(), 0);
    QCOMPARE(coordinator.replacedCount(), 0);
}

void TestEventCoordinator::anyEventIsAcceptedWhenIdle_data()
{
    QTest::addColumn<EventKind>("kind");
    for (const EventKind kind : kRealKinds) {
        QTest::newRow(qPrintable(eventKindId(kind))) << kind;
    }
}

void TestEventCoordinator::anyEventIsAcceptedWhenIdle()
{
    QFETCH(EventKind, kind);

    EventCoordinator coordinator;
    QCOMPARE(coordinator.request(kind), EventDecision::Accepted);
    QCOMPARE(coordinator.current(), kind);
    QVERIFY(coordinator.isBusy());
}

void TestEventCoordinator::pairwisePriority_data()
{
    QTest::addColumn<EventKind>("current");
    QTest::addColumn<EventKind>("incoming");
    QTest::addColumn<EventDecision>("expected");

    // 全部 25 种组合，期望值逐条写出而不是由规则推算，
    // 这样实现和期望不会一起错。顺序由 docs/Decisions.md 第 4.2 节冻结。
    const auto A = EventKind::AutonomousChatter;
    const auto C = EventKind::ClickFeedback;
    const auto D = EventKind::Dialogue;
    const auto F = EventKind::Feeding;
    const auto S = EventKind::Shutdown;
    const auto acc = EventDecision::Accepted;
    const auto rep = EventDecision::Replaced;
    const auto sup = EventDecision::Suppressed;

    struct Row
    {
        EventKind current;
        EventKind incoming;
        EventDecision expected;
    };
    const QList<Row> rows{
        {A, A, sup}, {A, C, rep}, {A, D, rep}, {A, F, rep}, {A, S, rep},
        {C, A, sup}, {C, C, acc}, {C, D, rep}, {C, F, rep}, {C, S, rep},
        {D, A, sup}, {D, C, sup}, {D, D, sup}, {D, F, rep}, {D, S, rep},
        {F, A, sup}, {F, C, sup}, {F, D, sup}, {F, F, sup}, {F, S, rep},
        {S, A, sup}, {S, C, sup}, {S, D, sup}, {S, F, sup}, {S, S, sup},
    };

    for (const Row &row : rows) {
        QTest::newRow(qPrintable(QStringLiteral("%1 <- %2")
                                     .arg(eventKindId(row.current),
                                          eventKindId(row.incoming))))
            << row.current << row.incoming << row.expected;
    }
}

void TestEventCoordinator::pairwisePriority()
{
    QFETCH(EventKind, current);
    QFETCH(EventKind, incoming);
    QFETCH(EventDecision, expected);

    EventCoordinator coordinator = coordinatorWith(current);
    QCOMPARE(coordinator.request(incoming), expected);

    switch (expected) {
    case EventDecision::Accepted:
    case EventDecision::Replaced:
        QCOMPARE(coordinator.current(), incoming);
        break;
    case EventDecision::Suppressed:
        QCOMPARE(coordinator.current(), current);
        break;
    }
}

void TestEventCoordinator::frozenPriorityOrder()
{
    // 优先级顺序由 docs/Decisions.md 第 4.2 节冻结：
    // 退出／隐藏 > 投喂 > 连续对话 > 单击反馈 > 自主闲聊。
    QVERIFY(EventKind::Shutdown > EventKind::Feeding);
    QVERIFY(EventKind::Feeding > EventKind::Dialogue);
    QVERIFY(EventKind::Dialogue > EventKind::ClickFeedback);
    QVERIFY(EventKind::ClickFeedback > EventKind::AutonomousChatter);
    QVERIFY(EventKind::AutonomousChatter > EventKind::None);
}

void TestEventCoordinator::decisionsFromTheDecisionRecord_data()
{
    QTest::addColumn<EventKind>("current");
    QTest::addColumn<EventKind>("incoming");
    QTest::addColumn<EventDecision>("expected");

    // 决策文档第 4.2 节逐条列出的具体规则。
    QTest::newRow("投喂结束当前对话")
        << EventKind::Dialogue << EventKind::Feeding << EventDecision::Replaced;
    QTest::newRow("对话期间抑制随机闲聊")
        << EventKind::Dialogue << EventKind::AutonomousChatter
        << EventDecision::Suppressed;
    QTest::newRow("对话期间单击不另触发反馈")
        << EventKind::Dialogue << EventKind::ClickFeedback
        << EventDecision::Suppressed;
    QTest::newRow("单击反馈可替换自主闲聊")
        << EventKind::AutonomousChatter << EventKind::ClickFeedback
        << EventDecision::Replaced;
    QTest::newRow("隐藏或退出立即结束对话")
        << EventKind::Dialogue << EventKind::Shutdown << EventDecision::Replaced;
    QTest::newRow("隐藏或退出立即结束投喂")
        << EventKind::Feeding << EventKind::Shutdown << EventDecision::Replaced;
}

void TestEventCoordinator::decisionsFromTheDecisionRecord()
{
    QFETCH(EventKind, current);
    QFETCH(EventKind, incoming);
    QFETCH(EventDecision, expected);

    EventCoordinator coordinator = coordinatorWith(current);
    QCOMPARE(coordinator.request(incoming), expected);
}

void TestEventCoordinator::suppressedEventsAreNotQueued()
{
    // 互相冲突的对话和行为不排队，低优先级事件被抑制后不会在稍后补播。
    EventCoordinator coordinator;
    coordinator.request(EventKind::Feeding);

    for (int attempt = 0; attempt < 20; ++attempt) {
        QCOMPARE(coordinator.request(EventKind::AutonomousChatter),
                 EventDecision::Suppressed);
        QCOMPARE(coordinator.request(EventKind::ClickFeedback),
                 EventDecision::Suppressed);
    }
    QCOMPARE(coordinator.suppressedCount(), 40);

    coordinator.finish(EventKind::Feeding);
    // 投喂结束后协调器必须是空闲的，而不是接着播被抑制的那些。
    QCOMPARE(coordinator.current(), EventKind::None);
    QVERIFY(!coordinator.isBusy());
}

void TestEventCoordinator::replacedEventsAreNotQueued()
{
    EventCoordinator coordinator;
    coordinator.request(EventKind::AutonomousChatter);
    QCOMPARE(coordinator.request(EventKind::Feeding), EventDecision::Replaced);
    QCOMPARE(coordinator.lastReplaced(), EventKind::AutonomousChatter);

    coordinator.finish(EventKind::Feeding);
    // 被替换的闲聊不会回来。
    QCOMPARE(coordinator.current(), EventKind::None);
    QCOMPARE(coordinator.replacedCount(), 1);
}

void TestEventCoordinator::finishOnlyClearsTheCurrentKind()
{
    // 迟到的结束通知不得清掉已经换上来的更高优先级事件。
    EventCoordinator coordinator;
    coordinator.request(EventKind::AutonomousChatter);
    coordinator.request(EventKind::Feeding);

    coordinator.finish(EventKind::AutonomousChatter);
    QCOMPARE(coordinator.current(), EventKind::Feeding);

    coordinator.finish(EventKind::Feeding);
    QCOMPARE(coordinator.current(), EventKind::None);
}

void TestEventCoordinator::clearRemovesAnything()
{
    EventCoordinator coordinator;
    coordinator.request(EventKind::Feeding);
    coordinator.clear();
    QCOMPARE(coordinator.current(), EventKind::None);
}

void TestEventCoordinator::repeatedClicksRestartTheirOwnFeedback()
{
    // 用户连续点击时应当得到新的反馈，而不是被自己上一次点击挡住。
    EventCoordinator coordinator;
    QCOMPARE(coordinator.request(EventKind::ClickFeedback),
             EventDecision::Accepted);
    QCOMPARE(coordinator.request(EventKind::ClickFeedback),
             EventDecision::Accepted);
    QCOMPARE(coordinator.suppressedCount(), 0);
}

void TestEventCoordinator::repeatedFeedingRequestsAreIgnored()
{
    // docs/Decisions.md 第 3.2 节：当前投喂动画结束前忽略新的投喂请求，
    // 不排队、不重播，也不引入冷却状态。
    EventCoordinator coordinator;
    QCOMPARE(coordinator.request(EventKind::Feeding), EventDecision::Accepted);
    for (int attempt = 0; attempt < 5; ++attempt) {
        QCOMPARE(coordinator.request(EventKind::Feeding),
                 EventDecision::Suppressed);
    }

    // 结束后立刻可以再次投喂，没有冷却。
    coordinator.finish(EventKind::Feeding);
    QCOMPARE(coordinator.request(EventKind::Feeding), EventDecision::Accepted);
}

void TestEventCoordinator::burstOfEventsEndsInTheHighestPriority()
{
    // 连续多事件到达时，最终状态是其中优先级最高的那一个。
    EventCoordinator coordinator;
    for (const EventKind kind : {EventKind::AutonomousChatter,
                                 EventKind::ClickFeedback, EventKind::Dialogue,
                                 EventKind::AutonomousChatter,
                                 EventKind::Feeding, EventKind::ClickFeedback,
                                 EventKind::Dialogue}) {
        coordinator.request(kind);
    }
    QCOMPARE(coordinator.current(), EventKind::Feeding);

    coordinator.request(EventKind::Shutdown);
    QCOMPARE(coordinator.current(), EventKind::Shutdown);
}

void TestEventCoordinator::noneIsAlwaysSuppressed()
{
    EventCoordinator coordinator;
    QCOMPARE(coordinator.request(EventKind::None), EventDecision::Suppressed);
    QCOMPARE(coordinator.current(), EventKind::None);
}

void TestEventCoordinator::everyKindHasAnId_data()
{
    QTest::addColumn<EventKind>("kind");
    QTest::newRow("none") << EventKind::None;
    for (const EventKind kind : kRealKinds) {
        QTest::newRow(qPrintable(eventKindId(kind))) << kind;
    }
}

void TestEventCoordinator::everyKindHasAnId()
{
    QFETCH(EventKind, kind);
    const QString id = eventKindId(kind);
    QVERIFY(!id.isEmpty());
    QVERIFY(id != QStringLiteral("unknown"));
}

QTEST_APPLESS_MAIN(TestEventCoordinator)
#include "tst_eventcoordinator.moc"
