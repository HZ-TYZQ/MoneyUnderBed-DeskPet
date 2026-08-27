#pragma once

#include <bitset>
#include <cstddef>

namespace mub::core {

// 会话可能同时因为多个系统原因暂停。每个来源只修改自己的位，某一个恢复
// 信号不能把仍被其他原因冻结的桌宠错误唤醒。
enum class SessionSuspendReason : std::size_t
{
    Locked,
    Sleeping,
    DisplayOff, // 显示器关闭，或桌面被屏保/锁屏界面完全遮蔽
    Inactive,
    Count,
};

class SessionSuspensionState
{
public:
    // 设置单个原因；返回聚合暂停状态是否发生变化。
    bool setSuspended(SessionSuspendReason reason, bool suspended);

    bool isSuspended() const;
    bool isSuspendedFor(SessionSuspendReason reason) const;

private:
    static constexpr std::size_t kReasonCount =
        static_cast<std::size_t>(SessionSuspendReason::Count);
    std::bitset<kReasonCount> reasons_;
};

} // namespace mub::core
