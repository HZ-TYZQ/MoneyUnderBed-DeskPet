#include "core/ClickFeedback.h"

#include "core/RandomSource.h"

#include <algorithm>

namespace mub::core {

ClickFeedbackSelector::ClickFeedbackSelector(ClickFeedbackConfig config)
    : config_(config)
{
    config_.lowFrequencyTextChancePercent =
        std::clamp(config_.lowFrequencyTextChancePercent, 0, 100);
    config_.normalFrequencyTextChancePercent =
        std::clamp(config_.normalFrequencyTextChancePercent, 0, 100);
}

ClickFeedback ClickFeedbackSelector::select(const ActivityMode mode,
                                            const BubbleFrequency bubble,
                                            RandomSource &random) const
{
    ClickFeedback feedback;
    feedback.hasReaction = true;

    // 安静模式完全抑制气泡；气泡设置为关闭时同样没有文字。
    // 两种情况下仍然保留动作或表情反馈。
    if (mode == ActivityMode::Quiet || bubble == BubbleFrequency::Off) {
        feedback.hasText = false;
        return feedback;
    }

    const int chance = bubble == BubbleFrequency::Normal
        ? config_.normalFrequencyTextChancePercent
        : config_.lowFrequencyTextChancePercent;
    feedback.hasText = random.chance(chance);
    return feedback;
}

} // namespace mub::core
