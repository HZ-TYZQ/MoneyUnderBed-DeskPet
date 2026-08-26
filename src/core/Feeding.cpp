#include "core/Feeding.h"

#include "core/RandomSource.h"

#include <algorithm>

namespace mub::core {

QString feedingOutcomeId(const FeedingOutcome outcome)
{
    switch (outcome) {
    case FeedingOutcome::Eat:
        return QStringLiteral("eat");
    case FeedingOutcome::Drop:
        return QStringLiteral("drop");
    }
    return QStringLiteral("unknown");
}

FeedingSelector::FeedingSelector(FeedingConfig config)
    : config_(config)
{
    config_.dropChancePercent = std::clamp(config_.dropChancePercent, 0, 100);
}

FeedingOutcome FeedingSelector::select(RandomSource &random) const
{
    return random.chance(config_.dropChancePercent) ? FeedingOutcome::Drop
                                                    : FeedingOutcome::Eat;
}

QString FeedingSelector::dropDialogueId()
{
    // 对应 docs/Decisions.md 第 4.4 节的四页冰淇淋掉落对话。
    return QStringLiteral("icecream-drop");
}

} // namespace mub::core
