#include "core/EventCoordinator.h"

namespace mub::core {

QString eventKindId(const EventKind kind)
{
    switch (kind) {
    case EventKind::None:
        return QStringLiteral("none");
    case EventKind::AutonomousChatter:
        return QStringLiteral("autonomous-chatter");
    case EventKind::ClickFeedback:
        return QStringLiteral("click-feedback");
    case EventKind::Dialogue:
        return QStringLiteral("dialogue");
    case EventKind::Feeding:
        return QStringLiteral("feeding");
    case EventKind::Shutdown:
        return QStringLiteral("shutdown");
    }
    return QStringLiteral("unknown");
}

QString eventDecisionId(const EventDecision decision)
{
    switch (decision) {
    case EventDecision::Accepted:
        return QStringLiteral("accepted");
    case EventDecision::Replaced:
        return QStringLiteral("replaced");
    case EventDecision::Suppressed:
        return QStringLiteral("suppressed");
    }
    return QStringLiteral("unknown");
}

bool EventCoordinator::restartsOnSameKind(const EventKind kind)
{
    switch (kind) {
    case EventKind::ClickFeedback:
        // 用户连续点击时应当给出新的反馈，而不是被自己上一次点击挡住。
        return true;
    case EventKind::Feeding:
        // docs/Decisions.md 第 3.2 节：当前投喂动画结束前忽略新的投喂请求，
        // 不排队、不重播，也不引入冷却状态。
        return false;
    case EventKind::None:
    case EventKind::AutonomousChatter:
    case EventKind::Dialogue:
    case EventKind::Shutdown:
        return false;
    }
    return false;
}

EventDecision EventCoordinator::request(const EventKind kind)
{
    lastRequested_ = kind;
    lastReplaced_ = EventKind::None;

    if (kind == EventKind::None) {
        lastDecision_ = EventDecision::Suppressed;
        ++suppressedCount_;
        return lastDecision_;
    }

    if (current_ == EventKind::None) {
        current_ = kind;
        lastDecision_ = EventDecision::Accepted;
        return lastDecision_;
    }

    if (kind == current_) {
        if (restartsOnSameKind(kind)) {
            lastDecision_ = EventDecision::Accepted;
            return lastDecision_;
        }
        lastDecision_ = EventDecision::Suppressed;
        ++suppressedCount_;
        return lastDecision_;
    }

    if (kind > current_) {
        lastReplaced_ = current_;
        current_ = kind;
        lastDecision_ = EventDecision::Replaced;
        ++replacedCount_;
        return lastDecision_;
    }

    // 优先级不足。不排队，也不会在稍后补播。
    lastDecision_ = EventDecision::Suppressed;
    ++suppressedCount_;
    return lastDecision_;
}

void EventCoordinator::finish(const EventKind kind)
{
    if (current_ == kind) {
        current_ = EventKind::None;
    }
}

void EventCoordinator::clear()
{
    current_ = EventKind::None;
}

EventKind EventCoordinator::current() const
{
    return current_;
}

bool EventCoordinator::isBusy() const
{
    return current_ != EventKind::None;
}

EventKind EventCoordinator::lastRequested() const
{
    return lastRequested_;
}

EventDecision EventCoordinator::lastDecision() const
{
    return lastDecision_;
}

EventKind EventCoordinator::lastReplaced() const
{
    return lastReplaced_;
}

int EventCoordinator::suppressedCount() const
{
    return suppressedCount_;
}

int EventCoordinator::replacedCount() const
{
    return replacedCount_;
}

} // namespace mub::core
