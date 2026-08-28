#include "core/SettingsPresets.h"

#include <array>

namespace mub::core {

namespace {

// 「活动节奏」一档写七个参数。低档更久待机、更短行走、更容易休息，高档相反。
struct TempoValues
{
    int idleMinMs;
    int idleMaxMs;
    int walkMinMs;
    int walkMaxMs;
    int restMinMs;
    int restMaxMs;
    int restChancePercent;
};

constexpr TempoValues tempoValues(const ActivityTempo tempo)
{
    switch (tempo) {
    case ActivityTempo::Low:
        return {4000, 12000, 1000, 2500, 6000, 20000, 40};
    case ActivityTempo::Normal:
        // 「中」档取当前默认值（第 14.3 节）。
        return {2000, 6000, 1500, 4000, 4000, 12000, 25};
    case ActivityTempo::High:
        return {1200, 3500, 2000, 6000, 3000, 8000, 15};
    }
    return {2000, 6000, 1500, 4000, 4000, 12000, 25};
}

struct SpeedValues
{
    int walkSpeedPxPerSec;
    int returnSpeedPxPerSec;
};

constexpr SpeedValues speedValues(const MovementSpeed speed)
{
    switch (speed) {
    case MovementSpeed::Slow:
        return {30, 60};
    case MovementSpeed::Normal:
        return {48, 90};
    case MovementSpeed::Fast:
        return {75, 130};
    }
    return {48, 90};
}

constexpr int cursorAffinityPercent(const CursorAffinity affinity)
{
    switch (affinity) {
    case CursorAffinity::Off:
        return 0;
    case CursorAffinity::Occasional:
        return 12;
    case CursorAffinity::Frequent:
        return 30;
    }
    return 12;
}

struct SpeechValues
{
    int minIntervalMs;
    int chancePercent;
};

// 第 4 节「不做高频陪聊」对最高档同样成立（第 14.4 节）：即使是「高」，
// 一次判定的期望间隔也在分钟量级。
constexpr SpeechValues speechValues(const SpeechFrequency frequency)
{
    switch (frequency) {
    case SpeechFrequency::Off:
        // 「关闭」只由概率 `0%` 表达，间隔保持不变。
        return {0, 0};
    case SpeechFrequency::Low:
        return {120000, 30};
    case SpeechFrequency::Normal:
        return {90000, 50};
    case SpeechFrequency::High:
        return {60000, 70};
    }
    return {120000, 30};
}

constexpr int clickTextPercent(const ClickTextFrequency frequency)
{
    switch (frequency) {
    case ClickTextFrequency::Low:
        // 第 14.4 节明确低档默认 `20%`。
        return 20;
    case ClickTextFrequency::Normal:
        return 45;
    case ClickTextFrequency::High:
        return 70;
    }
    return 20;
}

constexpr int typingMsPerChar(const TypingSpeed speed)
{
    switch (speed) {
    case TypingSpeed::Slow:
        return 45;
    case TypingSpeed::Normal:
        // 第 4.1 节由原型审核确定的 `28 ms`。
        return 28;
    case TypingSpeed::Fast:
        return 16;
    }
    return 28;
}

struct FrameValues
{
    int idleFrameMs;
    int runFrameMs;
    int icecreamFrameMs;
};

// 第 14.5 节：动画速度对三个帧时长施加统一倍率，因此三个值一起变。
constexpr FrameValues frameValues(const AnimationSpeed speed)
{
    switch (speed) {
    case AnimationSpeed::Slow:
        return {150, 120, 150};
    case AnimationSpeed::Normal:
        return {100, 80, 100};
    case AnimationSpeed::Fast:
        return {70, 56, 70};
    }
    return {100, 80, 100};
}

constexpr std::array kActivityTempos{ActivityTempo::Low, ActivityTempo::Normal,
                                     ActivityTempo::High};
constexpr std::array kMovementSpeeds{MovementSpeed::Slow, MovementSpeed::Normal,
                                     MovementSpeed::Fast};
constexpr std::array kCursorAffinities{CursorAffinity::Off, CursorAffinity::Occasional,
                                       CursorAffinity::Frequent};
constexpr std::array kSpeechFrequencies{SpeechFrequency::Off, SpeechFrequency::Low,
                                        SpeechFrequency::Normal, SpeechFrequency::High};
constexpr std::array kClickTextFrequencies{ClickTextFrequency::Low,
                                           ClickTextFrequency::Normal,
                                           ClickTextFrequency::High};
constexpr std::array kTypingSpeeds{TypingSpeed::Slow, TypingSpeed::Normal,
                                   TypingSpeed::Fast};
constexpr std::array kAnimationSpeeds{AnimationSpeed::Slow, AnimationSpeed::Normal,
                                      AnimationSpeed::Fast};

} // namespace

std::span<const ActivityTempo> activityTempos()
{
    return kActivityTempos;
}

std::span<const MovementSpeed> movementSpeeds()
{
    return kMovementSpeeds;
}

std::span<const CursorAffinity> cursorAffinities()
{
    return kCursorAffinities;
}

std::span<const SpeechFrequency> speechFrequencies()
{
    return kSpeechFrequencies;
}

std::span<const ClickTextFrequency> clickTextFrequencies()
{
    return kClickTextFrequencies;
}

std::span<const TypingSpeed> typingSpeeds()
{
    return kTypingSpeeds;
}

std::span<const AnimationSpeed> animationSpeeds()
{
    return kAnimationSpeeds;
}

void applyActivityTempo(BehaviorSettings &behavior, const ActivityTempo tempo)
{
    const TempoValues values = tempoValues(tempo);
    behavior.idleMinMs = values.idleMinMs;
    behavior.idleMaxMs = values.idleMaxMs;
    behavior.walkMinMs = values.walkMinMs;
    behavior.walkMaxMs = values.walkMaxMs;
    behavior.restMinMs = values.restMinMs;
    behavior.restMaxMs = values.restMaxMs;
    behavior.restChancePercent = values.restChancePercent;
}

void applyMovementSpeed(BehaviorSettings &behavior, const MovementSpeed speed)
{
    const SpeedValues values = speedValues(speed);
    behavior.walkSpeedPxPerSec = values.walkSpeedPxPerSec;
    behavior.returnSpeedPxPerSec = values.returnSpeedPxPerSec;
}

void applyCursorAffinity(BehaviorSettings &behavior, const CursorAffinity affinity)
{
    behavior.approachCursorChancePercent = cursorAffinityPercent(affinity);
}

void applySpeechFrequency(DialogueSettings &dialogue, const SpeechFrequency frequency)
{
    if (frequency == SpeechFrequency::Off) {
        // 第 14.4 节：「关闭」只写概率，不动间隔——用户再打开时回到原来的节奏。
        dialogue.chatterChancePercent = 0;
        return;
    }
    const SpeechValues values = speechValues(frequency);
    dialogue.chatterMinIntervalMs = values.minIntervalMs;
    dialogue.chatterChancePercent = values.chancePercent;
}

void applyClickTextFrequency(DialogueSettings &dialogue,
                             const ClickTextFrequency frequency)
{
    dialogue.clickTextChancePercent = clickTextPercent(frequency);
}

void applyTypingSpeed(DialogueSettings &dialogue, const TypingSpeed speed)
{
    dialogue.typingMsPerChar = typingMsPerChar(speed);
}

void applyAnimationSpeed(AppearanceSettings &appearance, const AnimationSpeed speed)
{
    const FrameValues values = frameValues(speed);
    appearance.idleFrameMs = values.idleFrameMs;
    appearance.runFrameMs = values.runFrameMs;
    appearance.icecreamFrameMs = values.icecreamFrameMs;
}

std::optional<ActivityTempo> matchActivityTempo(const BehaviorSettings &behavior)
{
    for (const ActivityTempo tempo : kActivityTempos) {
        const TempoValues values = tempoValues(tempo);
        if (behavior.idleMinMs == values.idleMinMs
            && behavior.idleMaxMs == values.idleMaxMs
            && behavior.walkMinMs == values.walkMinMs
            && behavior.walkMaxMs == values.walkMaxMs
            && behavior.restMinMs == values.restMinMs
            && behavior.restMaxMs == values.restMaxMs
            && behavior.restChancePercent == values.restChancePercent) {
            return tempo;
        }
    }
    return std::nullopt;
}

std::optional<MovementSpeed> matchMovementSpeed(const BehaviorSettings &behavior)
{
    for (const MovementSpeed speed : kMovementSpeeds) {
        const SpeedValues values = speedValues(speed);
        if (behavior.walkSpeedPxPerSec == values.walkSpeedPxPerSec
            && behavior.returnSpeedPxPerSec == values.returnSpeedPxPerSec) {
            return speed;
        }
    }
    return std::nullopt;
}

std::optional<CursorAffinity> matchCursorAffinity(const BehaviorSettings &behavior)
{
    for (const CursorAffinity affinity : kCursorAffinities) {
        if (behavior.approachCursorChancePercent == cursorAffinityPercent(affinity)) {
            return affinity;
        }
    }
    return std::nullopt;
}

std::optional<SpeechFrequency> matchSpeechFrequency(const DialogueSettings &dialogue)
{
    // 概率为 `0` 就是「关闭」，此时间隔取什么值都不参与匹配（第 14.4 节）。
    if (dialogue.chatterChancePercent == 0) {
        return SpeechFrequency::Off;
    }
    for (const SpeechFrequency frequency : kSpeechFrequencies) {
        if (frequency == SpeechFrequency::Off) {
            continue;
        }
        const SpeechValues values = speechValues(frequency);
        if (dialogue.chatterMinIntervalMs == values.minIntervalMs
            && dialogue.chatterChancePercent == values.chancePercent) {
            return frequency;
        }
    }
    return std::nullopt;
}

std::optional<ClickTextFrequency> matchClickTextFrequency(const DialogueSettings &dialogue)
{
    for (const ClickTextFrequency frequency : kClickTextFrequencies) {
        if (dialogue.clickTextChancePercent == clickTextPercent(frequency)) {
            return frequency;
        }
    }
    return std::nullopt;
}

std::optional<TypingSpeed> matchTypingSpeed(const DialogueSettings &dialogue)
{
    for (const TypingSpeed speed : kTypingSpeeds) {
        if (dialogue.typingMsPerChar == typingMsPerChar(speed)) {
            return speed;
        }
    }
    return std::nullopt;
}

std::optional<AnimationSpeed> matchAnimationSpeed(const AppearanceSettings &appearance)
{
    for (const AnimationSpeed speed : kAnimationSpeeds) {
        const FrameValues values = frameValues(speed);
        if (appearance.idleFrameMs == values.idleFrameMs
            && appearance.runFrameMs == values.runFrameMs
            && appearance.icecreamFrameMs == values.icecreamFrameMs) {
            return speed;
        }
    }
    return std::nullopt;
}

} // namespace mub::core
