#pragma once

#include <QString>

namespace mub::platform {

// 平台后端探测结果。
struct StartupProbeResult
{
    bool ok = true;
    // 面向用户的失败原因，会出现在错误对话框里。
    QString reason;
    // 面向日志的细节。
    QString detail;
    // 探测后实际选定的 Qt 平台插件名。
    QString selectedPlatform;
};

// 选定并探测窗口后端。
//
// **必须在构造 QApplication 或 QGuiApplication 之前调用。**
//
// Qt 在 QApplication 构造期间加载平台插件，失败时直接终止进程，
// 程序拿不到控制权，因此报错只能在构造之前处理
// （docs/Decisions.md 第 8.2 节「XCB 不可用时的报错通道」）。
//
// Linux 上本函数会把 QT_QPA_PLATFORM 设为单值 `xcb`，
// 绝不使用 `xcb;wayland` 这类候选列表，避免 Qt 自行回退；
// 然后用原生 xcb_connect 探测连接是否可用。
//
// 例外：QT_QPA_PLATFORM 已经是 `offscreen` 或 `minimal` 时保持不变。
// 这两个是无头测试平台，不是桌面回退路径，因此不违反禁止静默回退的规定。
StartupProbeResult probeWindowBackend();

} // namespace mub::platform
