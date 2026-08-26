#pragma once

#include <QString>

namespace mub::core {

// 行为事件类型。
//
// 枚举值的大小就是优先级，数值越大越优先。顺序由 docs/Decisions.md
// 第 4.2 节冻结，改动顺序等于改动产品行为，必须同时改决策文档。
enum class EventKind
{
    None = 0,
    AutonomousChatter = 1, // 自主闲聊
    ClickFeedback = 2,     // 单击反馈
    Dialogue = 3,          // 连续对话
    Feeding = 4,           // 用户主动投喂
    Shutdown = 5,          // 退出／隐藏
};

QString eventKindId(EventKind kind);

// 协调器对一次请求的裁决。
enum class EventDecision
{
    // 当前没有事件，或来者优先级更高但没有正在进行的事件。
    Accepted,
    // 来者优先级更高，替换掉正在进行的事件。被替换者不会补播。
    Replaced,
    // 来者优先级不足，或同类事件正在进行且不允许重来。被抑制者不会补播。
    Suppressed,
};

QString eventDecisionId(EventDecision decision);

// 统一行为／事件调度。
//
// docs/Decisions.md 第 4.2 节要求优先级由统一的调度逻辑实现，
// 不能分散为各功能互相覆盖的临时判断。因此：
//
// - 所有行为请求只提交给本类，功能模块不得直接强制切换全局状态。
// - 不建立延迟事件队列。被抑制或替换的低优先级事件不会在稍后补播。
// - 对外暴露可观察状态，便于日志和确定性测试。
//
// 本类是纯逻辑，不接触 Qt Widgets、平台 API 或任何素材。
class EventCoordinator
{
public:
    // 提交一次行为请求。
    EventDecision request(EventKind kind);

    // 事件自然结束。只有正在进行的那一类才会被清除，
    // 避免迟到的结束通知误清掉已经换上来的更高优先级事件。
    void finish(EventKind kind);

    // 无条件清空当前事件。用于隐藏与退出。
    void clear();

    EventKind current() const;
    bool isBusy() const;

    // 上一次裁决，供日志与测试观察。
    EventKind lastRequested() const;
    EventDecision lastDecision() const;
    EventKind lastReplaced() const;

    // 自创建以来被抑制与被替换的次数。用于确认「没有队列」。
    int suppressedCount() const;
    int replacedCount() const;

private:
    // 同类事件正在进行时是否允许从头重来。
    static bool restartsOnSameKind(EventKind kind);

    EventKind current_ = EventKind::None;
    EventKind lastRequested_ = EventKind::None;
    EventKind lastReplaced_ = EventKind::None;
    EventDecision lastDecision_ = EventDecision::Accepted;
    int suppressedCount_ = 0;
    int replacedCount_ = 0;
};

} // namespace mub::core
