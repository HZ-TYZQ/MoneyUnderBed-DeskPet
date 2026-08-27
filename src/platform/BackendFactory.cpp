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
    // Linux 的其余能力都由纯 Qt XCB 路径实测通过
    // （docs/FeasibilityResults.md）。LinuxWindowBackend 只补一项 Qt 表达不了的
    // 工作区归属，其余全部继承 QtWindowBackend。
    return std::make_unique<LinuxWindowBackend>();
#else
    return std::make_unique<QtWindowBackend>();
#endif
}

} // namespace mub::platform
