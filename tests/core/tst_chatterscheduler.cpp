// 自主闲聊的独立时间调度。
//
// docs/Decisions.md 第 14.4 节：闲聊由独立的时间调度驱动，不依赖行为状态切换；
// 每轮完整间隔结束时只判定一次；未触发或被抑制时不排队、不补播；
// 安静模式与「关闭」档停止调度；设置变化在下一轮生效。

#include "core/ChatterScheduler.h"
#include "core/RandomSource.h"
#include "core/Settings.h"
#include "core/SettingsPresets.h"
#include "core/TimeSource.h"

#include <QTest>

using namespace mub::core;

namespace {

// 以固定步长推进，模拟真实的定时器节拍。返回这段时间里命中的次数。
int runFor(ManualTimeSource &clock, ChatterScheduler &scheduler, const int totalMs,
           const int stepMs = 16)
{
    int hits = 0;
    for (int elapsed = 0; elapsed < totalMs; elapsed += stepMs) {
        clock.advance(stepMs);
        if (scheduler.update()) {
            ++hits;
        }
    }
    return hits;
}

ChatterScheduleConfig configFor(const SpeechFrequency frequency)
{
    DialogueSettings dialogue;
    applySpeechFrequency(dialogue, frequency);
    return {dialogue.chatterMinIntervalMs, dialogue.chatterChancePercent};
}

} // namespace

class TestChatterScheduler final : public QObject
{
    Q_OBJECT

private slots:
    void offNeverFires();
    void aFullIntervalMustElapseBeforeTheFirstRoll();
    void oneRollPerInterval();
    void aMissRestartsAFullInterval();
    void aHitRestartsAFullInterval();
    void higherFrequenciesTalkMoreOften();
    void everyPresetIsSlowerThanConstantChatter();
    void disablingStopsAndReEnablingStartsAFreshInterval();
    void configChangesApplyToTheNextRoundOnly();
    void turningItOffMidRoundStopsImmediately();
};

// 「关闭」档由概率 0% 表达，不另设开关。
void TestChatterScheduler::offNeverFires()
{
    ManualTimeSource clock;
    // 随机值恒为 0，即「一定命中」。概率为 0 时不消耗随机数也不命中。
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, configFor(SpeechFrequency::Off));

    QCOMPARE(runFor(clock, scheduler, 60 * 60 * 1000, 1000), 0);
}

void TestChatterScheduler::aFullIntervalMustElapseBeforeTheFirstRoll()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, {10000, 100});

    // 启动后必须先走满一整轮间隔，不会一开始就说话。
    // 第一轮从第一次 update() 起算，因此首次命中在第 11 秒而不是第 10 秒。
    QCOMPARE(runFor(clock, scheduler, 10000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);
}

void TestChatterScheduler::oneRollPerInterval()
{
    ManualTimeSource clock;
    // 恒命中。10 分钟内间隔 60 s，最多 10 次，不会因为角色空闲就连续说话。
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, {60000, 100});

    // 第一轮从第一次 update() 起算：命中落在第 61、121……秒，10 分钟内 9 次。
    const int hits = runFor(clock, scheduler, 10 * 60 * 1000, 1000);
    QCOMPARE(hits, 9);
}

void TestChatterScheduler::aMissRestartsAFullInterval()
{
    ManualTimeSource clock;
    // 第一轮未命中（0.9 > 0.5），第二轮命中（0.1 < 0.5）。
    ScriptedRandomSource random({}, {0.9, 0.1});
    ChatterScheduler scheduler(clock, random, {10000, 50});

    // 第一轮到期：未命中。
    QCOMPARE(runFor(clock, scheduler, 11000, 1000), 0);
    // 未命中不会立刻重试，必须再等满一整轮。
    QCOMPARE(runFor(clock, scheduler, 9000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);
}

void TestChatterScheduler::aHitRestartsAFullInterval()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, {10000, 100});

    QCOMPARE(runFor(clock, scheduler, 10000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);
    // 命中之后同样要走满一整轮才可能再次命中。
    QCOMPARE(runFor(clock, scheduler, 9000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);
}

// 三个非关闭档必须产生**实际差异**。`1.0.0` 候选的低频与正常完全一样，
// 这条断言就是为了防止那种情况再次出现。
void TestChatterScheduler::higherFrequenciesTalkMoreOften()
{
    constexpr int kWindowMs = 4 * 60 * 60 * 1000;

    const auto countHits = [](const SpeechFrequency frequency) {
        ManualTimeSource clock;
        SeededRandomSource random(20260828);
        ChatterScheduler scheduler(clock, random, configFor(frequency));
        return runFor(clock, scheduler, kWindowMs, 1000);
    };

    const int low = countHits(SpeechFrequency::Low);
    const int normal = countHits(SpeechFrequency::Normal);
    const int high = countHits(SpeechFrequency::High);

    QVERIFY2(low < normal, qPrintable(QStringLiteral("%1 vs %2").arg(low).arg(normal)));
    QVERIFY2(normal < high, qPrintable(QStringLiteral("%1 vs %2").arg(normal).arg(high)));
    QVERIFY(low > 0);
}

// 第 4 节「不做高频陪聊」：最高档的期望间隔仍在分钟量级。
void TestChatterScheduler::everyPresetIsSlowerThanConstantChatter()
{
    for (const SpeechFrequency frequency :
         {SpeechFrequency::Low, SpeechFrequency::Normal, SpeechFrequency::High}) {
        const ChatterScheduleConfig config = configFor(frequency);
        QVERIFY(config.minIntervalMs >= 30000);
        QVERIFY(config.chancePercent <= 100);

        ManualTimeSource clock;
        SeededRandomSource random(7);
        ChatterScheduler scheduler(clock, random, config);
        // 一小时内绝不可能说满 60 次：一轮只判定一次，间隔至少 30 s。
        QVERIFY(runFor(clock, scheduler, 60 * 60 * 1000, 1000) <= 60);
    }
}

// 安静模式、隐藏与暂停期间停止调度，恢复后从一整轮重新开始。
void TestChatterScheduler::disablingStopsAndReEnablingStartsAFreshInterval()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, {10000, 100});

    // 眼看就要到期时被停掉。
    QCOMPARE(runFor(clock, scheduler, 10000, 1000), 0);
    scheduler.setEnabled(false);
    QCOMPARE(runFor(clock, scheduler, 60000, 1000), 0);

    // 恢复后不会立刻把「欠下的」闲聊补出来，而是重新计一整轮。
    scheduler.setEnabled(true);
    QCOMPARE(runFor(clock, scheduler, 10000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);
}

// 第 14.8 节：设置变化在下一轮生效，不缩短也不补算正在进行的间隔。
void TestChatterScheduler::configChangesApplyToTheNextRoundOnly()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, {10000, 100});

    runFor(clock, scheduler, 5000, 1000);
    // 中途把间隔改短：这一轮仍按 10 s 计，到第 11 秒才到期。
    scheduler.setConfig({2000, 100});
    QCOMPARE(runFor(clock, scheduler, 5000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);

    // 下一轮才用新的 2 s 间隔。
    QCOMPARE(runFor(clock, scheduler, 2000, 1000), 1);
}

void TestChatterScheduler::turningItOffMidRoundStopsImmediately()
{
    ManualTimeSource clock;
    ScriptedRandomSource random({}, {0.0});
    ChatterScheduler scheduler(clock, random, {10000, 100});

    runFor(clock, scheduler, 10000, 1000);
    // 关到 0%：这一轮不会再到期命中。
    scheduler.setConfig({10000, 0});
    QCOMPARE(runFor(clock, scheduler, 60000, 1000), 0);

    // 重新打开时从一整轮重新开始。
    scheduler.setConfig({10000, 100});
    QCOMPARE(runFor(clock, scheduler, 10000, 1000), 0);
    QCOMPARE(runFor(clock, scheduler, 1000, 1000), 1);
}

QTEST_APPLESS_MAIN(TestChatterScheduler)
#include "tst_chatterscheduler.moc"
