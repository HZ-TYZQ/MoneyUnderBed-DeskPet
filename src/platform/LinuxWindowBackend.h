#pragma once

#include "platform/QtWindowBackend.h"

namespace mub::platform {

// Linux 的桌宠窗口能力由纯 Qt XCB 路径提供。本类型只保留稳定的诊断名，
// 不再尝试用 EWMH 控制工作区；该能力在 XWayland 下没有可靠语义。
class LinuxWindowBackend final : public QtWindowBackend
{
public:
    BackendCapabilities capabilities() const override;
};

} // namespace mub::platform
