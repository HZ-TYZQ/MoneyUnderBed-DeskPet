#include "platform/BackendFactory.h"

#if defined(Q_OS_WIN) || defined(_WIN32)
#include "platform/WindowsWindowBackend.h"
#elif defined(Q_OS_LINUX) || defined(__linux__)
#include "platform/LinuxWindowBackend.h"
#else
#include "platform/QtWindowBackend.h"
#endif

namespace mub::platform {

std::unique_ptr<DeskPetWindowBackend> createWindowBackend()
{
#if defined(Q_OS_WIN) || defined(_WIN32)
    return std::make_unique<WindowsWindowBackend>();
#elif defined(Q_OS_LINUX) || defined(__linux__)
    // Linux 的窗口能力由纯 Qt XCB 路径提供；专用类型只给诊断信息稳定命名。
    return std::make_unique<LinuxWindowBackend>();
#else
    return std::make_unique<QtWindowBackend>();
#endif
}

} // namespace mub::platform
