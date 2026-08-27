#include "platform/SessionMonitorFactory.h"

#include "platform/SessionMonitor.h"

#if defined(Q_OS_LINUX) || defined(__linux__)
#include "platform/LinuxSessionMonitor.h"
#elif defined(Q_OS_WIN) || defined(_WIN32)
#include "platform/WindowsSessionMonitor.h"
#endif

#include <QWindow>

namespace mub::platform {

namespace {

class NoopSessionMonitor final : public SessionMonitor
{
public:
    using SessionMonitor::SessionMonitor;

    bool start(QWindow *notificationWindow) override
    {
        Q_UNUSED(notificationWindow)
        return false;
    }
};

} // namespace

std::unique_ptr<SessionMonitor> createSessionMonitor()
{
#if defined(Q_OS_LINUX) || defined(__linux__)
    return std::make_unique<LinuxSessionMonitor>();
#elif defined(Q_OS_WIN) || defined(_WIN32)
    return std::make_unique<WindowsSessionMonitor>();
#else
    return std::make_unique<NoopSessionMonitor>();
#endif
}

} // namespace mub::platform
