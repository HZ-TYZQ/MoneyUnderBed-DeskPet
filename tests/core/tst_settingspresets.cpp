// 普通层档位与实际参数之间的双向映射。
//
// docs/Decisions.md 第 14.2 节：实际参数是唯一真相来源，配置文件不另存预设名；
// 界面按实际参数反向匹配，完全匹配显示档位，否则显示「自定义」。

#include "core/Settings.h"
#include "core/SettingsPresets.h"

#include <QTest>

using namespace mub::core;

class TestSettingsPresets final : public QObject
{
    Q_OBJECT

private slots:
    void everyPresetRoundTrips();
    void defaultsMatchTheMiddlePresets();
    void editingOneAdvancedValueBreaksTheMatch_data();
    void editingOneAdvancedValueBreaksTheMatch();
    void presetsOnlyWriteTheirOwnFields();
    void speechOffIsExpressedAsZeroChance();
    void speechOffKeepsTheInterval();
    void animationSpeedMovesAllThreeFrameDurations();
    void everyPresetProducesValidSettings();
};

void TestSettingsPresets::everyPresetRoundTrips()
{
    for (const ActivityTempo tempo : activityTempos()) {
        BehaviorSettings behavior;
        applyActivityTempo(behavior, tempo);
        QCOMPARE(matchActivityTempo(behavior), tempo);
    }
    for (const MovementSpeed speed : movementSpeeds()) {
        BehaviorSettings behavior;
        applyMovementSpeed(behavior, speed);
        QCOMPARE(matchMovementSpeed(behavior), speed);
    }
    for (const CursorAffinity affinity : cursorAffinities()) {
        BehaviorSettings behavior;
        applyCursorAffinity(behavior, affinity);
        QCOMPARE(matchCursorAffinity(behavior), affinity);
    }
    for (const SpeechFrequency frequency : speechFrequencies()) {
        DialogueSettings dialogue;
        applySpeechFrequency(dialogue, frequency);
        QCOMPARE(matchSpeechFrequency(dialogue), frequency);
    }
    for (const ClickTextFrequency frequency : clickTextFrequencies()) {
        DialogueSettings dialogue;
        applyClickTextFrequency(dialogue, frequency);
        QCOMPARE(matchClickTextFrequency(dialogue), frequency);
    }
    for (const TypingSpeed speed : typingSpeeds()) {
        DialogueSettings dialogue;
        applyTypingSpeed(dialogue, speed);
        QCOMPARE(matchTypingSpeed(dialogue), speed);
    }
    for (const AnimationSpeed speed : animationSpeeds()) {
        AppearanceSettings appearance;
        applyAnimationSpeed(appearance, speed);
        QCOMPARE(matchAnimationSpeed(appearance), speed);
    }
}

// 第 14.3 至 14.5 节：「中」「正常」档取当前默认值。
void TestSettingsPresets::defaultsMatchTheMiddlePresets()
{
    const Settings defaults;

    QCOMPARE(matchActivityTempo(defaults.behavior), ActivityTempo::Normal);
    QCOMPARE(matchMovementSpeed(defaults.behavior), MovementSpeed::Normal);
    QCOMPARE(matchCursorAffinity(defaults.behavior), CursorAffinity::Occasional);
    QCOMPARE(matchClickTextFrequency(defaults.dialogue), ClickTextFrequency::Low);
    QCOMPARE(matchTypingSpeed(defaults.dialogue), TypingSpeed::Normal);
    QCOMPARE(matchAnimationSpeed(defaults.appearance), AnimationSpeed::Normal);
    // 第 4 节：气泡默认低频，对应说话频率的「低」档。
    QCOMPARE(matchSpeechFrequency(defaults.dialogue), SpeechFrequency::Low);
}

void TestSettingsPresets::editingOneAdvancedValueBreaksTheMatch_data()
{
    QTest::addColumn<QString>("field");

    QTest::newRow("idleMinMs") << QStringLiteral("idleMinMs");
    QTest::newRow("restChancePercent") << QStringLiteral("restChancePercent");
    QTest::newRow("walkSpeedPxPerSec") << QStringLiteral("walkSpeedPxPerSec");
    QTest::newRow("returnSpeedPxPerSec") << QStringLiteral("returnSpeedPxPerSec");
    QTest::newRow("chatterMinIntervalMs") << QStringLiteral("chatterMinIntervalMs");
    QTest::newRow("typingMsPerChar") << QStringLiteral("typingMsPerChar");
    QTest::newRow("runFrameMs") << QStringLiteral("runFrameMs");
}

// 反向匹配要求**整组**参数完全相等：改动其中任意一个就落到「自定义」。
void TestSettingsPresets::editingOneAdvancedValueBreaksTheMatch()
{
    QFETCH(QString, field);

    Settings settings;
    if (field == QStringLiteral("idleMinMs")) {
        settings.behavior.idleMinMs += 1;
        QVERIFY(!matchActivityTempo(settings.behavior).has_value());
    } else if (field == QStringLiteral("restChancePercent")) {
        settings.behavior.restChancePercent += 1;
        QVERIFY(!matchActivityTempo(settings.behavior).has_value());
    } else if (field == QStringLiteral("walkSpeedPxPerSec")) {
        settings.behavior.walkSpeedPxPerSec += 1;
        QVERIFY(!matchMovementSpeed(settings.behavior).has_value());
    } else if (field == QStringLiteral("returnSpeedPxPerSec")) {
        settings.behavior.returnSpeedPxPerSec += 1;
        QVERIFY(!matchMovementSpeed(settings.behavior).has_value());
    } else if (field == QStringLiteral("chatterMinIntervalMs")) {
        settings.dialogue.chatterMinIntervalMs += 1;
        QVERIFY(!matchSpeechFrequency(settings.dialogue).has_value());
    } else if (field == QStringLiteral("typingMsPerChar")) {
        settings.dialogue.typingMsPerChar += 1;
        QVERIFY(!matchTypingSpeed(settings.dialogue).has_value());
    } else {
        settings.appearance.runFrameMs += 1;
        QVERIFY(!matchAnimationSpeed(settings.appearance).has_value());
    }
}

// 一个档位只写它自己驱动的参数，不得顺手改别的。
void TestSettingsPresets::presetsOnlyWriteTheirOwnFields()
{
    BehaviorSettings behavior;
    behavior.returnDelayMs = 777;
    behavior.cursorSafeDistancePx = 99;
    behavior.mode = ActivityMode::Active;

    applyActivityTempo(behavior, ActivityTempo::High);
    // 活动节奏不碰速度、返回延迟、安全距离和活动模式。
    QCOMPARE(behavior.walkSpeedPxPerSec, BehaviorSettings{}.walkSpeedPxPerSec);
    QCOMPARE(behavior.returnDelayMs, 777);
    QCOMPARE(behavior.cursorSafeDistancePx, 99);
    QCOMPARE(behavior.mode, ActivityMode::Active);

    applyMovementSpeed(behavior, MovementSpeed::Fast);
    // 移动速度不碰节奏参数。
    QCOMPARE(matchActivityTempo(behavior), ActivityTempo::High);

    applyCursorAffinity(behavior, CursorAffinity::Frequent);
    QCOMPARE(matchActivityTempo(behavior), ActivityTempo::High);
    QCOMPARE(matchMovementSpeed(behavior), MovementSpeed::Fast);

    DialogueSettings dialogue;
    dialogue.singlePageAutoHideMs = 6000;
    applyTypingSpeed(dialogue, TypingSpeed::Fast);
    applyClickTextFrequency(dialogue, ClickTextFrequency::High);
    QCOMPARE(dialogue.singlePageAutoHideMs, 6000);
    QCOMPARE(matchTypingSpeed(dialogue), TypingSpeed::Fast);
    QCOMPARE(matchClickTextFrequency(dialogue), ClickTextFrequency::High);
}

// 第 14.4 节：「关闭」档由触发概率 `0%` 表达，不另存启用标志。
void TestSettingsPresets::speechOffIsExpressedAsZeroChance()
{
    DialogueSettings dialogue;
    applySpeechFrequency(dialogue, SpeechFrequency::Off);
    QCOMPARE(dialogue.chatterChancePercent, 0);

    // 概率为 `0` 时，间隔取任何值都仍然显示「关闭」。
    for (const int interval : {10000, 60000, 120000, 900000}) {
        dialogue.chatterMinIntervalMs = interval;
        QCOMPARE(matchSpeechFrequency(dialogue), SpeechFrequency::Off);
    }
}

void TestSettingsPresets::speechOffKeepsTheInterval()
{
    DialogueSettings dialogue;
    applySpeechFrequency(dialogue, SpeechFrequency::High);
    const int interval = dialogue.chatterMinIntervalMs;

    // 关掉再打开不应该丢掉用户选过的节奏。
    applySpeechFrequency(dialogue, SpeechFrequency::Off);
    QCOMPARE(dialogue.chatterMinIntervalMs, interval);
}

// 第 14.5 节：动画速度对三个帧时长施加统一倍率，三个值一起变。
void TestSettingsPresets::animationSpeedMovesAllThreeFrameDurations()
{
    AppearanceSettings slow;
    applyAnimationSpeed(slow, AnimationSpeed::Slow);
    AppearanceSettings normal;
    applyAnimationSpeed(normal, AnimationSpeed::Normal);
    AppearanceSettings fast;
    applyAnimationSpeed(fast, AnimationSpeed::Fast);

    QVERIFY(slow.idleFrameMs > normal.idleFrameMs);
    QVERIFY(slow.runFrameMs > normal.runFrameMs);
    QVERIFY(slow.icecreamFrameMs > normal.icecreamFrameMs);
    QVERIFY(fast.idleFrameMs < normal.idleFrameMs);
    QVERIFY(fast.runFrameMs < normal.runFrameMs);
    QVERIFY(fast.icecreamFrameMs < normal.icecreamFrameMs);

    // 倍率统一：跑动帧一直比待机帧短。
    QVERIFY(slow.runFrameMs < slow.idleFrameMs);
    QVERIFY(fast.runFrameMs < fast.idleFrameMs);

    // 显示倍率不属于动画速度。
    QCOMPARE(slow.scale, AppearanceSettings{}.scale);
}

// 任何档位组合写出来的设置都必须能通过校验，否则档位本身就是坏的。
void TestSettingsPresets::everyPresetProducesValidSettings()
{
    for (const ActivityTempo tempo : activityTempos()) {
        for (const MovementSpeed speed : movementSpeeds()) {
            for (const CursorAffinity affinity : cursorAffinities()) {
                Settings settings;
                applyActivityTempo(settings.behavior, tempo);
                applyMovementSpeed(settings.behavior, speed);
                applyCursorAffinity(settings.behavior, affinity);
                QCOMPARE(sanitized(settings), settings);
            }
        }
    }
    for (const SpeechFrequency frequency : speechFrequencies()) {
        for (const TypingSpeed speed : typingSpeeds()) {
            for (const AnimationSpeed animation : animationSpeeds()) {
                Settings settings;
                applySpeechFrequency(settings.dialogue, frequency);
                applyTypingSpeed(settings.dialogue, speed);
                applyAnimationSpeed(settings.appearance, animation);
                QCOMPARE(sanitized(settings), settings);
            }
        }
    }
}

QTEST_APPLESS_MAIN(TestSettingsPresets)
#include "tst_settingspresets.moc"
