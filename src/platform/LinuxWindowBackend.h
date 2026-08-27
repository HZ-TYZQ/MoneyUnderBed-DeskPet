#pragma once

#include "platform/QtWindowBackend.h"

namespace mub::platform {

// Linux XCB 实现。
//
// docs/Decisions.md 第 8.4 节：优先使用 Qt，Qt 无法满足时才调用平台 API。
// Linux 所需的窗口能力已由纯 Qt XCB 路径实测通过（docs/FeasibilityResults.md），
// 因此本类**只覆盖工作区归属这一项** —— Qt 没有跨平台接口表达它，
// 必须走 EWMH 的 `_NET_WM_DESKTOP`。其余能力全部继承 QtWindowBackend。
class LinuxWindowBackend final : public QtWindowBackend
{
public:
    BackendCapabilities capabilities() const override;
    void setWorkspaceVisibility(QWindow *window, bool allWorkspaces) override;
};

} // namespace mub::platform
