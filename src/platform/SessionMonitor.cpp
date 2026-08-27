#include "platform/SessionMonitor.h"

#include <QLoggingCategory>

namespace mub::platform {

namespace {

Q_LOGGING_CATEGORY(lcSessionMonitor, "mub.platform.session")

const char *reasonId(const core::SessionSuspendReason reason)
{
    switch (reason) {
    case core::SessionSuspendReason::Locked:
        return "locked";
    case core::SessionSuspendReason::Sleeping:
        return "sleeping";
    case core::SessionSuspendReason::DisplayOff:
        return "display-off";
    case core::SessionSuspendReason::Inactive:
        return "inactive";
    case core::SessionSuspendReason::Count:
        return "invalid";
    }
    return "invalid";
}

} // namespace

SessionMonitor::SessionMonitor(QObject *parent)
    : QObject(parent)
{
}

SessionMonitor::~SessionMonitor() = default;

bool SessionMonitor::isSuspended() const
{
    return state_.isSuspended();
}

void SessionMonitor::setReason(const core::SessionSuspendReason reason,
                               const bool suspended)
{
    if (state_.isSuspendedFor(reason) == suspended) {
        return;
    }
    const bool aggregateChanged = state_.setSuspended(reason, suspended);
    qCInfo(lcSessionMonitor).noquote()
        << QStringLiteral("reason=%1 active=%2 suspended=%3")
               .arg(QLatin1String(reasonId(reason)))
               .arg(suspended)
               .arg(state_.isSuspended());
    if (aggregateChanged) {
        emit suspendedChanged(state_.isSuspended());
    }
}

} // namespace mub::platform
