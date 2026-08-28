#include "core/ActivityMode.h"
#include "core/ClickFeedback.h"
#include "core/SettingsPresets.h"
#include "core/Feeding.h"
#include "core/RandomSource.h"

#include <QTest>

using namespace mub::core;

Q_DECLARE_METATYPE(ActivityMode)
Q_DECLARE_METATYPE(FeedingOutcome)

class TestFeedback final : public QObject
{
    Q_OBJECT

private slots:
    void feedingHonoursTheInjectedProbability_data();
    void feedingHonoursTheInjectedProbability();
    void feedingProbabilityIsClamped_data();
    void feedingProbabilityIsClamped();
    void feedingOutcomesHaveIds();
    void dropDialogueIdIsStable();
    void clickAlwaysGivesAReaction_data();
    void clickAlwaysGivesAReaction();
    void quietModeNeverAddsText_data();
    void quietModeNeverAddsText();
    void zeroChanceNeverAddsText();
    void activeModeAddsTextWhenTheRollSucceeds_data();
    void activeModeAddsTextWhenTheRollSucceeds();
    void clickChanceIsIndependentOfSpeechFrequency();
    void clickChanceFollowsItsOwnPreset();
};

void TestFeedback::feedingHonoursTheInjectedProbability_data()
{
    QTest::addColumn<int>("dropChancePercent");
    QTest::addColumn<double>("roll");
    QTest::addColumn<FeedingOutcome>("expected");

    // chance(p) 的判定是 roll * 100 < p。
    QTest::newRow("15% roll 0.05 -> drop") << 15 << 0.05 << FeedingOutcome::Drop;
    QTest::newRow("15% roll 0.50 -> eat") << 15 << 0.50 << FeedingOutcome::Eat;
    QTest::newRow("boundary just below") << 15 << 0.1499 << FeedingOutcome::Drop;
    QTest::newRow("boundary at") << 15 << 0.15 << FeedingOutcome::Eat;
}

void TestFeedback::feedingHonoursTheInjectedProbability()
{
    QFETCH(int, dropChancePercent);
    QFETCH(double, roll);
    QFETCH(FeedingOutcome, expected);

    FeedingConfig config;
    config.dropChancePercent = dropChancePercent;
    const FeedingSelector selector(config);
    ScriptedRandomSource random({}, {roll});

    QCOMPARE(selector.select(random), expected);
}

void TestFeedback::feedingProbabilityIsClamped_data()
{
    QTest::addColumn<int>("dropChancePercent");
    QTest::addColumn<FeedingOutcome>("expected");

    // 测试可以强制分支，这正是计划第 10.3 节要求的「概率可通过测试注入强制分支」。
    QTest::newRow("never") << 0 << FeedingOutcome::Eat;
    QTest::newRow("negative is treated as never") << -50 << FeedingOutcome::Eat;
    QTest::newRow("always") << 100 << FeedingOutcome::Drop;
    QTest::newRow("above 100 is treated as always") << 500 << FeedingOutcome::Drop;
}

void TestFeedback::feedingProbabilityIsClamped()
{
    QFETCH(int, dropChancePercent);
    QFETCH(FeedingOutcome, expected);

    FeedingConfig config;
    config.dropChancePercent = dropChancePercent;
    const FeedingSelector selector(config);

    // 0 与 100 两端不消耗随机数，因此空序列也能得到确定结果。
    ScriptedRandomSource random({}, {});
    QCOMPARE(selector.select(random), expected);
}

void TestFeedback::feedingOutcomesHaveIds()
{
    QCOMPARE(feedingOutcomeId(FeedingOutcome::Eat), QStringLiteral("eat"));
    QCOMPARE(feedingOutcomeId(FeedingOutcome::Drop), QStringLiteral("drop"));
}

void TestFeedback::dropDialogueIdIsStable()
{
    // 掉落事件结束后进入对应连续对话；标识由阶段 6 的对话数据使用。
    QVERIFY(!FeedingSelector::dropDialogueId().isEmpty());
}

void TestFeedback::clickAlwaysGivesAReaction_data()
{
    QTest::addColumn<ActivityMode>("mode");
    QTest::addColumn<int>("chancePercent");

    for (const ActivityMode mode : {ActivityMode::Quiet, ActivityMode::Active}) {
        for (const int chance : {0, 20, 100}) {
            QTest::newRow(qPrintable(QStringLiteral("mode%1-chance%2")
                                         .arg(static_cast<int>(mode))
                                         .arg(chance)))
                << mode << chance;
        }
    }
}

void TestFeedback::clickAlwaysGivesAReaction()
{
    QFETCH(ActivityMode, mode);
    QFETCH(int, chancePercent);

    // docs/Decisions.md 第 3.1 节：即使概率为 `0` 或被安静模式抑制，
    // 单击仍至少提供动作或表情反馈。
    const ClickFeedbackSelector selector;
    ScriptedRandomSource random({}, {0.0, 0.99});
    QVERIFY(selector.select(mode, chancePercent, random).hasReaction);
}

void TestFeedback::quietModeNeverAddsText_data()
{
    QTest::addColumn<int>("chancePercent");
    QTest::newRow("zero") << 0;
    QTest::newRow("default") << 20;
    QTest::newRow("certain") << 100;
}

void TestFeedback::quietModeNeverAddsText()
{
    QFETCH(int, chancePercent);

    // 安静模式完全抑制气泡，与单击台词概率无关。
    const ClickFeedbackSelector selector;
    // 随机值全部取 0，即「一定成功」，仍然不应出现文字。
    ScriptedRandomSource random({}, {0.0});
    const ClickFeedback feedback =
        selector.select(ActivityMode::Quiet, chancePercent, random);
    QVERIFY(!feedback.hasText);
    QVERIFY(feedback.hasReaction);
}

void TestFeedback::zeroChanceNeverAddsText()
{
    // 第 14.4 节：「不带台词」由概率 `0%` 表达，不需要另一个开关。
    const ClickFeedbackSelector selector;
    ScriptedRandomSource random({}, {0.0});
    const ClickFeedback feedback = selector.select(ActivityMode::Active, 0, random);
    QVERIFY(!feedback.hasText);
    QVERIFY(feedback.hasReaction);
}

void TestFeedback::activeModeAddsTextWhenTheRollSucceeds_data()
{
    QTest::addColumn<int>("chancePercent");
    QTest::addColumn<double>("roll");
    QTest::addColumn<bool>("expectedText");

    // 第 14.4 节：单击台词低档默认 `20%`。
    QTest::newRow("20%, roll below") << 20 << 0.10 << true;
    QTest::newRow("20%, roll above") << 20 << 0.50 << false;
    QTest::newRow("70%, roll below") << 70 << 0.50 << true;
    QTest::newRow("70%, roll above") << 70 << 0.80 << false;
}

void TestFeedback::activeModeAddsTextWhenTheRollSucceeds()
{
    QFETCH(int, chancePercent);
    QFETCH(double, roll);
    QFETCH(bool, expectedText);

    const ClickFeedbackSelector selector;
    ScriptedRandomSource random({}, {roll});
    QCOMPARE(selector.select(ActivityMode::Active, chancePercent, random).hasText,
             expectedText);
}

void TestFeedback::clickChanceIsIndependentOfSpeechFrequency()
{
    // 第 14.4 节：单击台词概率必须与说话频率解耦。选择器只收一个概率，
    // 连说话频率这个概念都拿不到——同一个概率在任何说话频率下结果都一样。
    const ClickFeedbackSelector selector;
    constexpr double roll = 0.40;

    for (const SpeechFrequency frequency :
         {SpeechFrequency::Off, SpeechFrequency::Low, SpeechFrequency::Normal,
          SpeechFrequency::High}) {
        DialogueSettings dialogue;
        applySpeechFrequency(dialogue, frequency);
        applyClickTextFrequency(dialogue, ClickTextFrequency::Low);

        ScriptedRandomSource random({}, {roll});
        // 低档 20%，roll 0.40 不命中；说话频率换遍四档都不改变这个结果。
        QVERIFY(!selector.select(ActivityMode::Active, dialogue.clickTextChancePercent,
                                 random)
                     .hasText);
    }
}

void TestFeedback::clickChanceFollowsItsOwnPreset()
{
    // 反过来：只改单击档位，结果必须变。
    const ClickFeedbackSelector selector;
    constexpr double roll = 0.40;

    DialogueSettings low;
    applyClickTextFrequency(low, ClickTextFrequency::Low);
    DialogueSettings high;
    applyClickTextFrequency(high, ClickTextFrequency::High);

    ScriptedRandomSource lowRandom({}, {roll});
    ScriptedRandomSource highRandom({}, {roll});
    QVERIFY(!selector.select(ActivityMode::Active, low.clickTextChancePercent, lowRandom)
                 .hasText);
    QVERIFY(selector.select(ActivityMode::Active, high.clickTextChancePercent, highRandom)
                .hasText);
}

QTEST_APPLESS_MAIN(TestFeedback)
#include "tst_feedback.moc"
