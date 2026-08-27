#include "core/Settings.h"

#include <algorithm>
#include <array>

namespace mub::core {

namespace {

// 第 5.1 节：只用整数倍率。`3×` 供高 DPI 屏使用，同样是整数，
// 是否正式列入设置界面仍待决策文档第 13 节确认。
constexpr std::array<int, 3> kAllowedScales{1, 2, 3};

constexpr auto kQuiet = "quiet";
constexpr auto kActive = "active";
constexpr auto kBubbleOff = "off";
constexpr auto kBubbleLow = "low";
constexpr auto kBubbleNormal = "normal";

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

QString bubbleFrequencyId(const BubbleFrequency frequency)
{
    switch (frequency) {
    case BubbleFrequency::Off:
        return QString::fromLatin1(kBubbleOff);
    case BubbleFrequency::Low:
        return QString::fromLatin1(kBubbleLow);
    case BubbleFrequency::Normal:
        return QString::fromLatin1(kBubbleNormal);
    }
    return QString::fromLatin1(kBubbleLow);
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

BubbleFrequency bubbleFrequencyFromId(const QStringView id,
                                      const BubbleFrequency fallback)
{
    if (id == QLatin1StringView(kBubbleOff)) {
        return BubbleFrequency::Off;
    }
    if (id == QLatin1StringView(kBubbleLow)) {
        return BubbleFrequency::Low;
    }
    if (id == QLatin1StringView(kBubbleNormal)) {
        return BubbleFrequency::Normal;
    }
    return fallback;
}

Settings sanitized(Settings settings)
{
    if (!isAllowedScale(settings.scale)) {
        settings.scale = Settings{}.scale;
    }
    return settings;
}

} // namespace mub::core
