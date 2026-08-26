#include "core/ActivityMode.h"
#include "core/AutonomousBehavior.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"

#include <QTest>

using namespace mub::core;

Q_DECLARE_METATYPE(BehaviorState)

namespace {

constexpr QRect kArea(0, 0, 1920, 1080);
constexpr QSize kCharacter(138, 222);

AutonomousBehaviorConfig fastConfig()
{
    AutonomousBehaviorConfig config;
    config.idleMinMs = 1000;
    config.idleMaxMs = 1000;
    config.walkMinMs = 1000;
    config.walkMaxMs = 1000;
    config.restMinMs = 3000;
    config.restMaxMs = 3000;
    config.returnDelayMs = 500;
    return config;
}

// 以固定步长推进，模拟真实的定时器节拍。
void run(ManualTimeSource &clock, AutonomousBehavior &behavior,
         const int totalMs, const int stepMs = 16)
{
    for (int elapsed = 0; elapsed < totalMs; elapsed += stepMs) {
        clock.advance(stepMs);
        behavior.update();
    }
}

int bottomY()
{
    return kArea.y() + kArea.height() - kCharacter.height();
}

} // namespace

class TestAutonomousBehavior final : public QObject
{
    Q_OBJECT

private slots:
    void startsIdleAtTheBottom();
    void neverLeavesTheActivityArea();
    void pausedFreezesEverything();
    void quietModeNeverApproachesTheCursorOrAsksForChatter();
    void activeModeCanApproachTheCursor();
    void approachStopsOutsideTheSafeDistance();
    void switchingToQuietStopsAnApproachInProgress();
    void dragFreezesAutonomousBehaviour();
    void releaseNearTheBottomStaysPut();
    void releaseFarFromTheBottomReturnsAfterADelay();
    void returnPathEndsAtTheBottom();
    void restStopsMovement();
    void movingToAnotherScreenUpdatesTheActivityArea();
    void timeJumpDoesNotCatchUp();
    void sameSeedProducesTheSameSequence();
    void differentSeedsDiverge();
};

void TestAutonomousBehavior::startsIdleAtTheBottom()
{
    ManualTimeSource clock;
    SeededRandomSource random(1);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);

    QVERIFY(behavior.update());
    QCOMPARE(behavior.state(), BehaviorState::Idle);
    QCOMPARE(behavior.position().y(), bottomY());
    QCOMPARE(behavior.velocity(), QPointF());
}

void TestAutonomousBehavior::neverLeavesTheActivityArea()
{
    ManualTimeSource clock;
    SeededRandomSource random(7);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setMode(ActivityMode::Active);
    behavior.setCursorPosition(QPoint(1900, 20));
    behavior.update();

    // 连续跑 5 分钟，全程检查边界。
    for (int step = 0; step < 5 * 60 * 1000 / 16; ++step) {
        clock.advance(16);
        behavior.update();
        const QPoint position = behavior.position();
        QVERIFY(position.x() >= kArea.x());
        QVERIFY(position.y() >= kArea.y());
        QVERIFY(position.x() + kCharacter.width() <= kArea.right() + 1);
        QVERIFY(position.y() + kCharacter.height() <= kArea.bottom() + 1);
    }
}

void TestAutonomousBehavior::pausedFreezesEverything()
{
    ManualTimeSource clock;
    SeededRandomSource random(3);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    // 先让它走起来。
    run(clock, behavior, 3000);
    const BehaviorState stateBefore = behavior.state();

    behavior.setPaused(true);
    const QPoint positionBefore = behavior.position();
    run(clock, behavior, 60000);

    QVERIFY(behavior.isPaused());
    QCOMPARE(behavior.position(), positionBefore);
    QCOMPARE(behavior.state(), stateBefore);
    QCOMPARE(behavior.velocity(), QPointF());

    behavior.setPaused(false);
    QVERIFY(!behavior.isPaused());
}

void TestAutonomousBehavior::quietModeNeverApproachesTheCursorOrAsksForChatter()
{
    // docs/Decisions.md 第 2.2 节：安静模式不主动接近鼠标，也不主动显示气泡。
    ManualTimeSource clock;
    SeededRandomSource random(11);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setMode(ActivityMode::Quiet);
    behavior.setCursorPosition(QPoint(960, 100));
    behavior.update();

    for (int step = 0; step < 10 * 60 * 1000 / 16; ++step) {
        clock.advance(16);
        behavior.update();
        QVERIFY(behavior.state() != BehaviorState::ApproachingCursor);
        QVERIFY(!behavior.consumeChatterRequest());
    }
}

void TestAutonomousBehavior::activeModeCanApproachTheCursor()
{
    // 用固定序列强制走到接近鼠标的分支，不靠概率碰运气。
    ManualTimeSource clock;
    // chance() 用 nextDouble：0.99 使休息判定为假，0.01 使接近鼠标判定为真。
    ScriptedRandomSource random({0}, {0.99, 0.01});
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setMode(ActivityMode::Active);
    behavior.setCursorPosition(QPoint(400, 300));
    behavior.update();

    run(clock, behavior, 1100);
    QCOMPARE(behavior.state(), BehaviorState::ApproachingCursor);
}

void TestAutonomousBehavior::approachStopsOutsideTheSafeDistance()
{
    AutonomousBehaviorConfig config = fastConfig();
    config.walkMinMs = 60000;
    config.walkMaxMs = 60000;
    config.cursorSafeDistancePx = 120;

    ManualTimeSource clock;
    ScriptedRandomSource random({0}, {0.99, 0.01});
    AutonomousBehavior behavior(clock, random, config);
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setMode(ActivityMode::Active);
    const QPoint cursor(900, 600);
    behavior.setCursorPosition(cursor);
    behavior.update();

    run(clock, behavior, 60000);

    const QPointF centre = QPointF(behavior.position())
        + QPointF(kCharacter.width() / 2.0, kCharacter.height() / 2.0);
    const double distance = QLineF(centre, QPointF(cursor)).length();
    // 停在安全距离外，不直接覆盖鼠标位置。允许几像素的取整误差。
    QVERIFY2(distance > 100.0,
             qPrintable(QStringLiteral("distance was %1").arg(distance)));
}

void TestAutonomousBehavior::switchingToQuietStopsAnApproachInProgress()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({0}, {0.99, 0.01});
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setMode(ActivityMode::Active);
    behavior.setCursorPosition(QPoint(400, 300));
    behavior.update();
    run(clock, behavior, 1100);
    QCOMPARE(behavior.state(), BehaviorState::ApproachingCursor);

    behavior.setMode(ActivityMode::Quiet);
    QVERIFY(behavior.state() != BehaviorState::ApproachingCursor);
}

void TestAutonomousBehavior::dragFreezesAutonomousBehaviour()
{
    ManualTimeSource clock;
    SeededRandomSource random(5);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    behavior.beginDrag();
    QCOMPARE(behavior.state(), BehaviorState::HeldByUser);

    const QPoint before = behavior.position();
    run(clock, behavior, 30000);
    QCOMPARE(behavior.position(), before);
    QCOMPARE(behavior.state(), BehaviorState::HeldByUser);
}

void TestAutonomousBehavior::releaseNearTheBottomStaysPut()
{
    // docs/Decisions.md 第 3.1 节：松手位置靠近底部时留在该处继续活动。
    ManualTimeSource clock;
    SeededRandomSource random(5);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    behavior.beginDrag();
    const QPoint release(300, bottomY() - 4);
    behavior.endDrag(release);

    QCOMPARE(behavior.state(), BehaviorState::Idle);
    QCOMPARE(behavior.position(), release);
}

void TestAutonomousBehavior::releaseFarFromTheBottomReturnsAfterADelay()
{
    // 松手位置远离底部时先短暂停留，再自行返回。
    ManualTimeSource clock;
    SeededRandomSource random(5);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    behavior.beginDrag();
    const QPoint release(300, 100);
    behavior.endDrag(release);
    QCOMPARE(behavior.state(), BehaviorState::ReturningToBottom);

    // 停留期间不动。
    run(clock, behavior, 400);
    QCOMPARE(behavior.position(), release);
    QCOMPARE(behavior.state(), BehaviorState::ReturningToBottom);

    // 停留结束后开始下移。
    run(clock, behavior, 300);
    QVERIFY(behavior.position().y() > release.y());
}

void TestAutonomousBehavior::returnPathEndsAtTheBottom()
{
    ManualTimeSource clock;
    SeededRandomSource random(5);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    behavior.beginDrag();
    behavior.endDrag(QPoint(300, 50));
    run(clock, behavior, 30000);

    QCOMPARE(behavior.position().y(), bottomY());
    QVERIFY(behavior.state() != BehaviorState::ReturningToBottom);
}

void TestAutonomousBehavior::restStopsMovement()
{
    // 休息不依赖新素材：只是停止移动并延长停留时间。
    ManualTimeSource clock;
    // 0.01 使休息判定为真。
    ScriptedRandomSource random({0}, {0.01});
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    run(clock, behavior, 1100);
    QCOMPARE(behavior.state(), BehaviorState::Resting);

    const QPoint position = behavior.position();
    run(clock, behavior, 2000);
    QCOMPARE(behavior.state(), BehaviorState::Resting);
    QCOMPARE(behavior.position(), position);
    QCOMPARE(behavior.velocity(), QPointF());
}

void TestAutonomousBehavior::movingToAnotherScreenUpdatesTheActivityArea()
{
    // 多显示器按 best-effort 实现，本测试只覆盖纯逻辑部分。
    ManualTimeSource clock;
    SeededRandomSource random(9);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    const QRect second(1920, 40, 2560, 1400);
    behavior.beginDrag();
    behavior.setActivityArea(second);
    behavior.endDrag(QPoint(2000, 200));

    QCOMPARE(behavior.activityArea(), second);
    run(clock, behavior, 30000);
    QCOMPARE(behavior.position().y(),
             second.y() + second.height() - kCharacter.height());
    QVERIFY(behavior.position().x() >= second.x());
}

void TestAutonomousBehavior::timeJumpDoesNotCatchUp()
{
    // 锁屏或睡眠恢复后不补算离开期间的行为。
    ManualTimeSource clock;
    SeededRandomSource random(13);
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();
    run(clock, behavior, 1100);

    const QPoint before = behavior.position();
    clock.advance(6 * 60 * 60 * 1000);
    behavior.update();

    // 单步位移不得超过一次正常节拍能走的距离。
    const int moved = (behavior.position() - before).manhattanLength();
    QVERIFY2(moved <= 200, qPrintable(QStringLiteral("moved %1 px").arg(moved)));
}

void TestAutonomousBehavior::sameSeedProducesTheSameSequence()
{
    // 计划第 9 节退出门：在假时钟和固定随机种子下，所有行为序列可重复。
    const auto record = [](const quint32 seed) {
        ManualTimeSource clock;
        SeededRandomSource random(seed);
        AutonomousBehavior behavior(clock, random, fastConfig());
        behavior.setActivityArea(kArea);
        behavior.setCharacterSize(kCharacter);
        behavior.setMode(ActivityMode::Active);
        behavior.setCursorPosition(QPoint(700, 400));
        behavior.update();

        QList<QPoint> trace;
        for (int step = 0; step < 4000; ++step) {
            clock.advance(16);
            behavior.update();
            if (step % 25 == 0) {
                trace.append(behavior.position());
            }
        }
        return trace;
    };

    const QList<QPoint> first = record(42);
    const QList<QPoint> second = record(42);
    QVERIFY(!first.isEmpty());
    QCOMPARE(first, second);
}

void TestAutonomousBehavior::differentSeedsDiverge()
{
    // 反向确认：上一个测试的相等不是因为角色根本没动。
    const auto record = [](const quint32 seed) {
        ManualTimeSource clock;
        SeededRandomSource random(seed);
        AutonomousBehavior behavior(clock, random, fastConfig());
        behavior.setActivityArea(kArea);
        behavior.setCharacterSize(kCharacter);
        behavior.update();

        QList<QPoint> trace;
        for (int step = 0; step < 4000; ++step) {
            clock.advance(16);
            behavior.update();
            if (step % 25 == 0) {
                trace.append(behavior.position());
            }
        }
        return trace;
    };

    QVERIFY(record(42) != record(4242));
}

QTEST_APPLESS_MAIN(TestAutonomousBehavior)
#include "tst_autonomousbehavior.moc"
