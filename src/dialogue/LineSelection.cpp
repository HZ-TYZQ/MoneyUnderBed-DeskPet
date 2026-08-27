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

constexpr const char *kAutonomousChatterIds[] = {
    "original-01",  // 活跃模式随机闲聊
    "original-02",  // 单击或低频闲聊
    "authored-01",  // 活跃模式闲聊
    "authored-17",  // 待机闲聊
};

// 另外四条闲聊文案（authored-02、08、10、15）是两页的，暂不进入随机池。
// 两页台词按第 4.1 节属于连续对话：需要点击翻页，不会「数秒后自动消失」。
// 让自主闲聊选中它们，等于让角色自己弹出一个要用户点两下才收起、
// 否则挂满 20 s 超时的面板。是否允许、以及是否改为自动翻页，
// 需要项目所有者决定并写回决策文档，这里先不做。

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
