#include "core/ClickFeedback.h"

#include "core/RandomSource.h"

#include <algorithm>

namespace mub::core {

ClickFeedback ClickFeedbackSelector::select(const ActivityMode mode,
                                            const int textChancePercent,
                                            RandomSource &random) const
{
    ClickFeedback feedback;
    feedback.hasReaction = true;

    // 第 3.1 节：安静模式完全抑制气泡，但单击仍然保留动作或表情反馈。
    if (mode == ActivityMode::Quiet) {
        feedback.hasText = false;
        return feedback;
    }

    // 概率为 `0` 就是「单击不带台词」，不需要另一个开关。
    feedback.hasText = random.chance(std::clamp(textChancePercent, 0, 100));
    return feedback;
}

} // namespace mub::core
