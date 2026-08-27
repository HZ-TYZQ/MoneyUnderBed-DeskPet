#include "core/SessionSuspension.h"

namespace mub::core {

bool SessionSuspensionState::setSuspended(const SessionSuspendReason reason,
                                          const bool suspended)
{
    const bool before = isSuspended();
    const auto index = static_cast<std::size_t>(reason);
    if (index < kReasonCount) {
        reasons_.set(index, suspended);
    }
    return before != isSuspended();
}

bool SessionSuspensionState::isSuspended() const
{
    return reasons_.any();
}

bool SessionSuspensionState::isSuspendedFor(const SessionSuspendReason reason) const
{
    const auto index = static_cast<std::size_t>(reason);
    return index < kReasonCount && reasons_.test(index);
}

} // namespace mub::core
