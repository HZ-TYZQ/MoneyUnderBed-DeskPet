#include "core/Settings.h"

#include <algorithm>
#include <array>

namespace mub::core {

namespace {

// 第 5.1 节与第 14.5 节：只用整数倍率，档位集合不变。
constexpr std::array<int, 3> kAllowedScales{1, 2, 3};

constexpr auto kQuiet = "quiet";
constexpr auto kActive = "active";

// 单值越界回到默认值，不做夹取：夹取会把一个坏配置悄悄变成一个用户没选过的
// 合法值，默认值至少是可预期的。
void fallbackIfOutside(int &value, const IntRange range, const int fallback)
{
    if (!range.contains(value)) {
        value = fallback;
    }
}

// 成对校验：任一端越界，或最小值大于最大值，整对回到默认值。
void fallbackPairIfInvalid(int &minValue, int &maxValue, const IntRange range,
                           const int minFallback, const int maxFallback)
{
    if (!range.contains(minValue) || !range.contains(maxValue)
        || minValue > maxValue) {
        minValue = minFallback;
        maxValue = maxFallback;
    }
}

} // namespace

std::span<const int> allowedScales()
{
    return kAllowedScales;
}

bool isAllowedScale(const int scale)
{
    return std::find(kAllowedScales.begin(), kAllowedScales.end(), scale)
        != kAllowedScales.end();
}

QString activityModeId(const ActivityMode mode)
{
    return QString::fromLatin1(mode == ActivityMode::Active ? kActive : kQuiet);
}

ActivityMode activityModeFromId(const QStringView id, const ActivityMode fallback)
{
    if (id == QLatin1StringView(kQuiet)) {
        return ActivityMode::Quiet;
    }
    if (id == QLatin1StringView(kActive)) {
        return ActivityMode::Active;
    }
    return fallback;
}

Settings sanitized(Settings settings)
{
    const Settings defaults;

    BehaviorSettings &behavior = settings.behavior;
    const BehaviorSettings &behaviorDefaults = defaults.behavior;
    fallbackPairIfInvalid(behavior.idleMinMs, behavior.idleMaxMs, ranges::kIdleMs,
                          behaviorDefaults.idleMinMs, behaviorDefaults.idleMaxMs);
    fallbackPairIfInvalid(behavior.walkMinMs, behavior.walkMaxMs, ranges::kWalkMs,
                          behaviorDefaults.walkMinMs, behaviorDefaults.walkMaxMs);
    fallbackPairIfInvalid(behavior.restMinMs, behavior.restMaxMs, ranges::kRestMs,
                          behaviorDefaults.restMinMs, behaviorDefaults.restMaxMs);
    fallbackIfOutside(behavior.restChancePercent, ranges::kPercent,
                      behaviorDefaults.restChancePercent);
    fallbackIfOutside(behavior.approachCursorChancePercent, ranges::kPercent,
                      behaviorDefaults.approachCursorChancePercent);
    fallbackIfOutside(behavior.walkSpeedPxPerSec, ranges::kSpeedPxPerSec,
                      behaviorDefaults.walkSpeedPxPerSec);
    fallbackIfOutside(behavior.returnSpeedPxPerSec, ranges::kSpeedPxPerSec,
                      behaviorDefaults.returnSpeedPxPerSec);
    fallbackIfOutside(behavior.returnDelayMs, ranges::kReturnDelayMs,
                      behaviorDefaults.returnDelayMs);
    fallbackIfOutside(behavior.cursorSafeDistancePx, ranges::kCursorSafeDistancePx,
                      behaviorDefaults.cursorSafeDistancePx);

    DialogueSettings &dialogue = settings.dialogue;
    const DialogueSettings &dialogueDefaults = defaults.dialogue;
    fallbackIfOutside(dialogue.chatterMinIntervalMs, ranges::kChatterIntervalMs,
                      dialogueDefaults.chatterMinIntervalMs);
    fallbackIfOutside(dialogue.chatterChancePercent, ranges::kPercent,
                      dialogueDefaults.chatterChancePercent);
    fallbackIfOutside(dialogue.clickTextChancePercent, ranges::kPercent,
                      dialogueDefaults.clickTextChancePercent);
    fallbackIfOutside(dialogue.singlePageAutoHideMs, ranges::kSinglePageAutoHideMs,
                      dialogueDefaults.singlePageAutoHideMs);
    fallbackIfOutside(dialogue.typingMsPerChar, ranges::kTypingMsPerChar,
                      dialogueDefaults.typingMsPerChar);

    AppearanceSettings &appearance = settings.appearance;
    const AppearanceSettings &appearanceDefaults = defaults.appearance;
    if (!isAllowedScale(appearance.scale)) {
        appearance.scale = appearanceDefaults.scale;
    }
    fallbackIfOutside(appearance.idleFrameMs, ranges::kFrameMs,
                      appearanceDefaults.idleFrameMs);
    fallbackIfOutside(appearance.runFrameMs, ranges::kFrameMs,
                      appearanceDefaults.runFrameMs);
    fallbackIfOutside(appearance.icecreamFrameMs, ranges::kFrameMs,
                      appearanceDefaults.icecreamFrameMs);

    return settings;
}

} // namespace mub::core
