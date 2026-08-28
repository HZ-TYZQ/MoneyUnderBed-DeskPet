#include "character/AnimationClip.h"
#include "character/AnimationPlayer.h"
#include "core/TimeSource.h"

#include <QTest>

using namespace mub::character;
using mub::core::ManualTimeSource;

namespace {

constexpr AnimationClip kLoop{"test-loop", ":/none", 4, AnimationCategory::Idle,
                              LoopMode::Loop};
constexpr AnimationClip kOnce{"test-once", ":/none", 3, AnimationCategory::Idle,
                              LoopMode::HoldLast};

} // namespace

class TestAnimationPlayer final : public QObject
{
    Q_OBJECT

private slots:
    void startsAtTheFirstFrame();
    void advancesOneFrameAtATime();
    void doesNotAdvanceBeforeTheFrameDuration();
    void loopsAndCountsLoops();
    void holdLastStopsOnTheFinalFrame();
    void playIsIdempotentForTheSameClip();
    void restartAlwaysGoesBackToFrameZero();
    void restartFromFrameContinuesFromTheRequestedFrame();
    void pauseFreezesTheFrame();
    void resumeDoesNotCatchUpOnMissedFrames();
    void largeTimeJumpAdvancesOnlyOneFrame();
    void everyRegisteredClipHasSaneParameters_data();
    void everyRegisteredClipHasSaneParameters();
    void timingAppliesToTheNextClipNotTheCurrentOne();
    void frameDurationFollowsTheClipCategory();
};

void TestAnimationPlayer::startsAtTheFirstFrame()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);

    QCOMPARE(player.frameIndex(), 0);
    QCOMPARE(player.loopCount(), 0);
    QVERIFY(!player.isFinished());
    QCOMPARE(player.clip(), &kLoop);
}

void TestAnimationPlayer::advancesOneFrameAtATime()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);

    for (int expected = 1; expected <= 3; ++expected) {
        clock.advance(100);
        QVERIFY(player.update());
        QCOMPARE(player.frameIndex(), expected);
    }
}

void TestAnimationPlayer::doesNotAdvanceBeforeTheFrameDuration()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);

    clock.advance(99);
    QVERIFY(!player.update());
    QCOMPARE(player.frameIndex(), 0);

    // 余量累积：再走 1 ms 就够一帧。
    clock.advance(1);
    QVERIFY(player.update());
    QCOMPARE(player.frameIndex(), 1);
}

void TestAnimationPlayer::loopsAndCountsLoops()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);

    for (int step = 0; step < 4; ++step) {
        clock.advance(100);
        player.update();
    }
    QCOMPARE(player.frameIndex(), 0);
    QCOMPARE(player.loopCount(), 1);
    QVERIFY(!player.isFinished());
}

void TestAnimationPlayer::holdLastStopsOnTheFinalFrame()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kOnce);

    for (int step = 0; step < 10; ++step) {
        clock.advance(100);
        player.update();
    }
    QCOMPARE(player.frameIndex(), 2);
    QVERIFY(player.isFinished());
    QCOMPARE(player.loopCount(), 0);

    // 播完后不再变化。
    clock.advance(1000);
    QVERIFY(!player.update());
    QCOMPARE(player.frameIndex(), 2);
}

void TestAnimationPlayer::playIsIdempotentForTheSameClip()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);
    clock.advance(200);
    player.update();
    QCOMPARE(player.frameIndex(), 2);

    // 同一段动画重复 play 不应打断当前播放。
    player.play(kLoop);
    QCOMPARE(player.frameIndex(), 2);
}

void TestAnimationPlayer::restartAlwaysGoesBackToFrameZero()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);
    clock.advance(200);
    player.update();
    QCOMPARE(player.frameIndex(), 2);

    player.restart(kLoop);
    QCOMPARE(player.frameIndex(), 0);
    QCOMPARE(player.loopCount(), 0);
}

void TestAnimationPlayer::restartFromFrameContinuesFromTheRequestedFrame()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);

    player.restartFromFrame(kLoop, 2);
    QCOMPARE(player.frameIndex(), 2);
    QCOMPARE(player.loopCount(), 0);

    clock.advance(100);
    QVERIFY(player.update());
    QCOMPARE(player.frameIndex(), 3);

    player.restartFromFrame(kLoop, 99);
    QCOMPARE(player.frameIndex(), 3);
}

void TestAnimationPlayer::pauseFreezesTheFrame()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);
    clock.advance(100);
    player.update();
    QCOMPARE(player.frameIndex(), 1);

    player.pause();
    QVERIFY(player.isPaused());
    clock.advance(1000);
    QVERIFY(!player.update());
    QCOMPARE(player.frameIndex(), 1);
}

void TestAnimationPlayer::resumeDoesNotCatchUpOnMissedFrames()
{
    // docs/Decisions.md 第 2.3 节：锁屏或睡眠恢复后不补算离开期间的行为。
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);
    clock.advance(100);
    player.update();
    QCOMPARE(player.frameIndex(), 1);

    player.pause();
    clock.advance(60 * 60 * 1000);
    player.update();
    player.resume();

    QCOMPARE(player.frameIndex(), 1);
    // 恢复后正常继续，一帧就是一帧。
    clock.advance(100);
    QVERIFY(player.update());
    QCOMPARE(player.frameIndex(), 2);
}

void TestAnimationPlayer::largeTimeJumpAdvancesOnlyOneFrame()
{
    // 没有显式暂停但进程被挂起时，时间会突然跳一大段。
    // 这种情况同样不能补播，否则一恢复就会疯狂切帧。
    ManualTimeSource clock;
    AnimationPlayer player(clock);
    player.play(kLoop);

    clock.advance(AnimationPlayer::TimeJumpThresholdMs + 5000);
    QVERIFY(player.update());
    QCOMPARE(player.frameIndex(), 1);
    QCOMPARE(player.loopCount(), 0);
}

void TestAnimationPlayer::everyRegisteredClipHasSaneParameters_data()
{
    QTest::addColumn<int>("frameCount");
    QTest::addColumn<int>("frameDurationMs");

    for (const AnimationClip &clip : registeredClips()) {
        QTest::newRow(clip.id)
            << clip.frameCount << frameDurationFor(clip, AnimationTiming{});
    }
}

void TestAnimationPlayer::everyRegisteredClipHasSaneParameters()
{
    QFETCH(int, frameCount);
    QFETCH(int, frameDurationMs);

    QVERIFY(frameCount >= 1);
    QVERIFY(frameDurationMs >= 1);
}

// 第 14.8 节：帧时长在启动动画时快照，改设置不影响正在播放的这一段。
void TestAnimationPlayer::timingAppliesToTheNextClipNotTheCurrentOne()
{
    ManualTimeSource clock;
    AnimationPlayer player(clock);

    const AnimationClip *idle = findClip(QStringLiteral("idle-up-left"));
    QVERIFY(idle != nullptr);
    const AnimationClip *run = findClip(QStringLiteral("run-up-left"));
    QVERIFY(run != nullptr);

    player.restart(*idle);
    QCOMPARE(player.activeFrameDurationMs(), AnimationTiming{}.idleFrameMs);

    // 播放途中把待机帧时长改慢一倍。
    AnimationTiming slower;
    slower.idleFrameMs = AnimationTiming{}.idleFrameMs * 2;
    slower.runFrameMs = AnimationTiming{}.runFrameMs * 2;
    player.setTiming(slower);

    // 当前这一段仍按原来的帧时长推进。
    QCOMPARE(player.activeFrameDurationMs(), AnimationTiming{}.idleFrameMs);
    clock.advance(AnimationTiming{}.idleFrameMs);
    QVERIFY(player.update());
    QCOMPARE(player.frameIndex(), 1);

    // 下一次启动动画才采用新值。
    player.restart(*run);
    QCOMPARE(player.activeFrameDurationMs(), slower.runFrameMs);
    clock.advance(AnimationTiming{}.runFrameMs);
    QVERIFY(!player.update());
    clock.advance(slower.runFrameMs - AnimationTiming{}.runFrameMs);
    QVERIFY(player.update());
    QCOMPARE(player.frameIndex(), 1);
}

// 动画速度对三个帧时长施加统一倍率，类别在登记表里显式写出。
void TestAnimationPlayer::frameDurationFollowsTheClipCategory()
{
    AnimationTiming timing;
    timing.idleFrameMs = 111;
    timing.runFrameMs = 222;
    timing.icecreamFrameMs = 333;

    for (const AnimationClip &clip : registeredClips()) {
        const int expected = clip.category == AnimationCategory::Idle ? 111
            : clip.category == AnimationCategory::Run                 ? 222
                                                                      : 333;
        QCOMPARE(frameDurationFor(clip, timing), expected);
    }
}

QTEST_APPLESS_MAIN(TestAnimationPlayer)
#include "tst_animationplayer.moc"
