#pragma once

#include <QString>

#include <span>

namespace mub::core {
class RandomSource;
}

namespace mub::dialogue {

// 单页气泡的触发场景。
//
// docs/Decisions.md 第 4.4、4.5 节的「使用场景」一列是自由文本，供人工审查；
// 这里把其中**当前已经有行为钩子**的两类整理成程序可用的分组。
// 其余场景（拖动、接近鼠标、进入休息、开始／结束行走、投喂各阶段、
// 掉落的替代反应）对应的台词已经在文案库里，但触发它们的行为事件还没有实现，
// 因此暂不分组，也不会被随机选中。
enum class LineTrigger
{
    ClickFeedback,     // 单击反馈
    AutonomousChatter, // 自主闲聊
};

// 某个触发场景可用的对话标识。顺序固定，便于审查与测试。
std::span<const char *const> lineIdsFor(LineTrigger trigger);

// 从该场景的台词里随机取一条。分组为空时返回空串。
//
// 只做等概率挑选。第 4.5 节标注的「低概率」「高频」是频率调优，
// 与本函数无关：出现频率由 core::ClickFeedbackSelector 和自主行为的
// 闲聊间隔决定，不在这里再叠一层权重。
QString selectLineId(LineTrigger trigger, core::RandomSource &random);

} // namespace mub::dialogue
