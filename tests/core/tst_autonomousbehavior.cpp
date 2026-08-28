#include "core/ActivityMode.h"
#include "core/AutonomousBehavior.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"

#include <QFile>
#include <QTest>

#include <cmath>

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
    void resumePreservesTheRemainingStateTime();
    void quietModeNeverApproachesTheCursor();
    void activityModeChangeWaitsForTheCurrentBehaviour();
    void behaviourNoLongerCarriesChatter();
    void speedChangesOnlyAffectTheNextMovement();
    void durationChangesOnlyAffectTheNextState();
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

void TestAutonomousBehavior::resumePreservesTheRemainingStateTime()
{
    ManualTimeSource clock;
    // 休息判定恒为假，待机到点后必定进入行走。
    ScriptedRandomSource random(QList<int>{1000}, QList<double>{0.9});
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.update();

    clock.advance(400);
    behavior.update();
    QCOMPARE(behavior.state(), BehaviorState::Idle);

    behavior.setPaused(true);
    clock.advance(60000);
    behavior.update();
    behavior.setPaused(false);

    // 暂停前还剩 600 ms；暂停的一分钟不能吃掉这段剩余时间。
    clock.advance(599);
    behavior.update();
    QCOMPARE(behavior.state(), BehaviorState::Idle);

    clock.advance(1);
    behavior.update();
    QCOMPARE(behavior.state(), BehaviorState::Walking);
}

void TestAutonomousBehavior::quietModeNeverApproachesTheCursor()
{
    // docs/Decisions.md 第 2.2 节：安静模式不主动接近鼠标。
    // 「不主动显示气泡」现在由 ChatterScheduler 保证，见 tst_chatterscheduler。
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
    }
}

// 第 14.4 节：闲聊不再挂在行为状态切换上，状态机里没有任何闲聊概念。
void TestAutonomousBehavior::behaviourNoLongerCarriesChatter()
{
    const QString source =
        QStringLiteral(MUB_SOURCE_ROOT "/src/core/AutonomousBehavior.h");
    QFile header(source);
    QVERIFY2(header.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(source));
    const QString text = QString::fromUtf8(header.readAll());

    // `1.0.0` 候选靠 consumeChatterRequest() 把闲聊搭在待机结束上，结果是
    // 「说话频率」实际由活动节奏决定。这条断言防止它被重新引回来。
    QVERIFY(!text.contains(QStringLiteral("consumeChatterRequest")));
    QVERIFY(!text.contains(QStringLiteral("chatterRequested")));
}

// 第 14.8 节：移动速度在下一次开始对应移动时生效，不改变正在进行的这一次。
void TestAutonomousBehavior::speedChangesOnlyAffectTheNextMovement()
{
    ManualTimeSource clock;
    // nextInt 恒取 0 并夹取到区间下界：待机与行走各 1000 ms，行走目标是最左端。
    // 两个 0.99 使休息与接近鼠标判定都为假，于是待机结束后必定进入行走。
    ScriptedRandomSource random({0}, {0.99, 0.99});
    AutonomousBehaviorConfig config = fastConfig();
    config.walkSpeedPxPerSec = 50.0;
    AutonomousBehavior behavior(clock, random, config);
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    // 起点放在右侧，走向最左端的目标才会真正产生位移。
    behavior.setPosition(QPoint(1000, bottomY()));
    behavior.setMode(ActivityMode::Active);
    behavior.update();

    run(clock, behavior, 1008);
    QCOMPARE(behavior.state(), BehaviorState::Walking);

    const QPoint beforeChange = behavior.position();
    run(clock, behavior, 160);
    const int movedAtOldSpeed = std::abs(behavior.position().x() - beforeChange.x());
    QVERIFY(movedAtOldSpeed > 0);

    // 移动途中把速度提到四倍：这一次移动必须保持开始时的速度快照。
    config.walkSpeedPxPerSec = 200.0;
    behavior.setConfig(config);
    QCOMPARE(behavior.state(), BehaviorState::Walking);
    QCOMPARE(behavior.config().walkSpeedPxPerSec, 200.0);

    const QPoint afterChange = behavior.position();
    run(clock, behavior, 160);
    const int movedAfterChange = std::abs(behavior.position().x() - afterChange.x());

    // 同样的时长走出同样的距离，说明用的还是快照而不是新值。
    QVERIFY2(std::abs(movedAfterChange - movedAtOldSpeed) <= 1,
             qPrintable(QStringLiteral("%1 vs %2")
                            .arg(movedAtOldSpeed)
                            .arg(movedAfterChange)));
}

// 第 14.8 节：待机时长在下一次进入待机时生效，不重算当前这一次的截止时间。
void TestAutonomousBehavior::durationChangesOnlyAffectTheNextState()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({0}, {0.99, 0.99});
    AutonomousBehaviorConfig config = fastConfig();
    AutonomousBehavior behavior(clock, random, config);
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setPosition(QPoint(1000, bottomY()));
    behavior.setMode(ActivityMode::Active);
    behavior.update();
    QCOMPARE(behavior.state(), BehaviorState::Idle);

    // 待机刚开始就把待机时长改成十倍。当前这一次仍应在原定的 1000 ms 结束。
    config.idleMinMs = 10000;
    config.idleMaxMs = 10000;
    behavior.setConfig(config);

    run(clock, behavior, 960);
    QCOMPARE(behavior.state(), BehaviorState::Idle);
    run(clock, behavior, 96);
    QCOMPARE(behavior.state(), BehaviorState::Walking);

    // 下一次进入待机才用新值：行走 1000 ms 结束后回到待机，
    // 这一次的待机要到 10000 ms 才结束。
    run(clock, behavior, 1008);
    QCOMPARE(behavior.state(), BehaviorState::Idle);
    run(clock, behavior, 2000);
    QCOMPARE(behavior.state(), BehaviorState::Idle);
}

// 第 14.8 节：活动模式在当前行为结束后生效。
// 唯一的例外是切到安静时立刻停止正在进行的接近鼠标（第 2.2 节），
// 由 switchingToQuietStopsAnApproachInProgress 覆盖。
void TestAutonomousBehavior::activityModeChangeWaitsForTheCurrentBehaviour()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({0}, {0.99, 0.99});
    AutonomousBehavior behavior(clock, random, fastConfig());
    behavior.setActivityArea(kArea);
    behavior.setCharacterSize(kCharacter);
    behavior.setPosition(QPoint(1000, bottomY()));
    behavior.setMode(ActivityMode::Active);
    behavior.update();

    run(clock, behavior, 1008);
    QCOMPARE(behavior.state(), BehaviorState::Walking);
    const QPoint before = behavior.position();

    // 行走途中切到安静：这一次行走不被打断，位置继续按原计划推进。
    behavior.setMode(ActivityMode::Quiet);
    QCOMPARE(behavior.state(), BehaviorState::Walking);
    run(clock, behavior, 160);
    QVERIFY(behavior.position() != before);
    QCOMPARE(behavior.state(), BehaviorState::Walking);
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
