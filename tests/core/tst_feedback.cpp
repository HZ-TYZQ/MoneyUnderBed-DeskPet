#include "core/ActivityMode.h"
#include "core/BubbleFrequency.h"
#include "core/ClickFeedback.h"
#include "core/Feeding.h"
#include "core/RandomSource.h"

#include <QTest>

using namespace mub::core;

Q_DECLARE_METATYPE(ActivityMode)
Q_DECLARE_METATYPE(BubbleFrequency)
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
    void bubbleOffNeverAddsText();
    void activeModeAddsTextWhenTheRollSucceeds_data();
    void activeModeAddsTextWhenTheRollSucceeds();
    void normalFrequencyIsMoreTalkativeThanLow();
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
    QTest::addColumn<BubbleFrequency>("bubble");

    for (const ActivityMode mode : {ActivityMode::Quiet, ActivityMode::Active}) {
        for (const BubbleFrequency bubble :
             {BubbleFrequency::Off, BubbleFrequency::Low, BubbleFrequency::Normal}) {
            QTest::newRow(qPrintable(
                QStringLiteral("mode%1-bubble%2")
                    .arg(static_cast<int>(mode))
                    .arg(static_cast<int>(bubble))))
                << mode << bubble;
        }
    }
}

void TestFeedback::clickAlwaysGivesAReaction()
{
    QFETCH(ActivityMode, mode);
    QFETCH(BubbleFrequency, bubble);

    // docs/Decisions.md 第 3.1 节：即使气泡设置为关闭或被安静模式抑制，
    // 单击仍至少提供动作或表情反馈。
    const ClickFeedbackSelector selector;
    ScriptedRandomSource random({}, {0.0, 0.99});
    QVERIFY(selector.select(mode, bubble, random).hasReaction);
}

void TestFeedback::quietModeNeverAddsText_data()
{
    QTest::addColumn<BubbleFrequency>("bubble");
    QTest::newRow("off") << BubbleFrequency::Off;
    QTest::newRow("low") << BubbleFrequency::Low;
    QTest::newRow("normal") << BubbleFrequency::Normal;
}

void TestFeedback::quietModeNeverAddsText()
{
    QFETCH(BubbleFrequency, bubble);

    // 安静模式完全抑制气泡，与气泡频率设置无关。
    const ClickFeedbackSelector selector;
    // 随机值全部取 0，即「一定成功」，仍然不应出现文字。
    ScriptedRandomSource random({}, {0.0});
    const ClickFeedback feedback =
        selector.select(ActivityMode::Quiet, bubble, random);
    QVERIFY(!feedback.hasText);
    QVERIFY(feedback.hasReaction);
}

void TestFeedback::bubbleOffNeverAddsText()
{
    const ClickFeedbackSelector selector;
    ScriptedRandomSource random({}, {0.0});
    const ClickFeedback feedback =
        selector.select(ActivityMode::Active, BubbleFrequency::Off, random);
    QVERIFY(!feedback.hasText);
    QVERIFY(feedback.hasReaction);
}

void TestFeedback::activeModeAddsTextWhenTheRollSucceeds_data()
{
    QTest::addColumn<BubbleFrequency>("bubble");
    QTest::addColumn<double>("roll");
    QTest::addColumn<bool>("expectedText");

    // 默认低频 20%、正常 60%。
    QTest::newRow("low, roll below") << BubbleFrequency::Low << 0.10 << true;
    QTest::newRow("low, roll above") << BubbleFrequency::Low << 0.50 << false;
    QTest::newRow("normal, roll below") << BubbleFrequency::Normal << 0.50 << true;
    QTest::newRow("normal, roll above") << BubbleFrequency::Normal << 0.80 << false;
}

void TestFeedback::activeModeAddsTextWhenTheRollSucceeds()
{
    QFETCH(BubbleFrequency, bubble);
    QFETCH(double, roll);
    QFETCH(bool, expectedText);

    const ClickFeedbackSelector selector;
    ScriptedRandomSource random({}, {roll});
    QCOMPARE(selector.select(ActivityMode::Active, bubble, random).hasText,
             expectedText);
}

void TestFeedback::normalFrequencyIsMoreTalkativeThanLow()
{
    // 同一个随机值下，正常频率出文字而低频不出，
    // 证明两档确实不同而不是共用一个阈值。
    const ClickFeedbackSelector selector;
    constexpr double roll = 0.40;

    ScriptedRandomSource lowRandom({}, {roll});
    ScriptedRandomSource normalRandom({}, {roll});
    QVERIFY(!selector.select(ActivityMode::Active, BubbleFrequency::Low, lowRandom).hasText);
    QVERIFY(selector.select(ActivityMode::Active, BubbleFrequency::Normal, normalRandom).hasText);
}

QTEST_APPLESS_MAIN(TestFeedback)
#include "tst_feedback.moc"
