#pragma once

#include "core/ActivityMode.h"
#include "core/BubbleFrequency.h"

namespace mub::core {

class RandomSource;

// 一次单击反馈的构成。
//
// docs/Decisions.md 第 3.1 节：即使气泡设置为关闭或被安静模式抑制，
// 单击仍至少提供动作或表情反馈。因此 `hasReaction` 恒为真，
// 只有 `hasText` 会被设置和模式关掉。
struct ClickFeedback
{
    // 动作或表情反馈。始终提供。
    bool hasReaction = true;
    // 是否附带台词气泡。具体台词与表情由阶段 6 的对话数据决定。
    bool hasText = false;
};

struct ClickFeedbackConfig
{
    // 低频与正常两档下附带台词的概率。属于待调优的内部参数。
    int lowFrequencyTextChancePercent = 20;
    int normalFrequencyTextChancePercent = 60;
};

// 决定一次单击给出什么反馈。
//
// 只做选择，不产生台词内容，也不接触任何素材。
class ClickFeedbackSelector
{
public:
    explicit ClickFeedbackSelector(ClickFeedbackConfig config = {});

    ClickFeedback select(ActivityMode mode, BubbleFrequency bubble,
                         RandomSource &random) const;

private:
    ClickFeedbackConfig config_;
};

} // namespace mub::core
