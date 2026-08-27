#pragma once

#include "platform/QtWindowBackend.h"

namespace mub::platform {

// Windows 专用实现。整窗穿透已经在真实 Windows 11 上确认采用继承自
// QtWindowBackend 的 Qt::WindowTransparentForInput 路径；这里只保留纯 Qt
// 无法表达的 WS_EX_TOOLWINDOW / WS_EX_NOACTIVATE 补强。
class WindowsWindowBackend final : public QtWindowBackend
{
public:
    BackendCapabilities capabilities() const override;
    void configureAsDeskPet(QWindow *window) override;
};

} // namespace mub::platform
