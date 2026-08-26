#include "platform/BackendFactory.h"

#if defined(Q_OS_WIN) || defined(_WIN32)
#include "platform/WindowsWindowBackend.h"
#else
#include "platform/QtWindowBackend.h"
#endif

namespace mub::platform {

std::unique_ptr<DeskPetWindowBackend> createWindowBackend()
{
#if defined(Q_OS_WIN) || defined(_WIN32)
    return std::make_unique<WindowsWindowBackend>();
#else
    // Linux 的全部所需能力已由纯 Qt XCB 路径实测通过
    // （docs/FeasibilityResults.md），因此不另立 XCB 专用后端。
    // 阶段 7 需要「固定到全部工作区」时再增加 XCB 实现。
    return std::make_unique<QtWindowBackend>();
#endif
}

} // namespace mub::platform
