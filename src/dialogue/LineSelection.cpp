#include "dialogue/LineSelection.h"

#include "core/RandomSource.h"

namespace mub::dialogue {

namespace {

// 每条后面的注释是 docs/Decisions.md 第 4.4／4.5 节「使用场景」一列的原文。
constexpr const char *kClickFeedbackIds[] = {
    "original-02",  // 单击或低频闲聊
    "original-03",  // 低概率单击反馈；不关联任何付费入口
    "original-04",  // 低概率单击反馈
    "authored-03",  // 单击反馈
    "authored-13",  // 单击反馈
    "authored-19",  // 高频单击反馈
};

// 其中四条写成两页。第 4.1 节允许随机气泡选中多页文案：显示时按连续对话
// 规则走，需要点击翻页，不套用单句气泡的自动消失时长，无人操作时 20 s 超时结束。
constexpr const char *kAutonomousChatterIds[] = {
    "original-01",  // 活跃模式随机闲聊
    "original-02",  // 单击或低频闲聊
    "authored-01",  // 活跃模式闲聊
    "authored-02",  // 待机闲聊（两页）
    "authored-08",  // 活跃模式闲聊（两页）
    "authored-10",  // 待机闲聊（两页）
    "authored-15",  // 活跃模式闲聊（两页）
    "authored-17",  // 待机闲聊
};

} // namespace

std::span<const char *const> lineIdsFor(const LineTrigger trigger)
{
    switch (trigger) {
    case LineTrigger::ClickFeedback:
        return kClickFeedbackIds;
    case LineTrigger::AutonomousChatter:
        return kAutonomousChatterIds;
    }
    return {};
}

QString selectLineId(const LineTrigger trigger, core::RandomSource &random)
{
    const std::span<const char *const> ids = lineIdsFor(trigger);
    if (ids.empty()) {
        return {};
    }
    const int index = random.nextInt(0, static_cast<int>(ids.size()) - 1);
    return QString::fromLatin1(ids[static_cast<std::size_t>(index)]);
}

} // namespace mub::dialogue
