#pragma once

#include <QString>
#include <QStringList>

namespace mub::app {

// 窗口后端不可用时的报错通道。
//
// 调用时尚未构造 QApplication。本函数会在可能的情况下用另一个平台后端
// 构造一个只用于显示错误的 QApplication，展示原因后以非零退出码结束。
//
// 该回退只用于报错，不进入运行状态，因此不属于 docs/legacy/Decisions.md
// 第 8.2 节禁止的静默回退。`reason` 是探测给出的未翻译源文本
// （见 platform::StartupProbeResult）。
//
// 返回值就是进程退出码，恒为非零。
int reportStartupFailure(int argc, char *argv[], const char *reason,
                         const QString &detail);

// `libraryPaths` 里是否存在名称含 `pluginKey` 的平台插件。
//
// 报错对话框要借用 Wayland 平台插件；正式 AppImage 只带 xcb 与 offscreen
// 两个平台插件，没有它时构造 QApplication 会直接 qFatal 终止进程。
bool hasPlatformPlugin(const QStringList &libraryPaths, const QString &pluginKey);

} // namespace mub::app
