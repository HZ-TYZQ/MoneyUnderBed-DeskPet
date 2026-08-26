#pragma once

#include <QString>

namespace mub::app {

// 窗口后端不可用时的报错通道。
//
// 调用时尚未构造 QApplication。本函数会在可能的情况下用另一个平台后端
// 构造一个只用于显示错误的 QApplication，展示原因后以非零退出码结束。
//
// 该回退只用于报错，不进入运行状态，因此不属于 docs/Decisions.md 第 8.2 节
// 禁止的静默回退。
//
// 返回值就是进程退出码，恒为非零。
int reportStartupFailure(int argc, char *argv[], const QString &reason,
                         const QString &detail);

} // namespace mub::app
