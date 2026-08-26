#pragma once

#include <QString>

namespace mub::app {

// 安装 Qt 消息处理器，把消息同时写到 stderr 和本地日志文件。
//
// **必须在构造 QApplication 之前调用。**
// 平台插件加载失败时 Qt 会调用 qFatal 直接终止进程，程序无法阻止；
// 提前安装处理器至少能让这种情况留下可查记录
// （docs/Decisions.md 第 8.2 节）。
//
// 当前实现是两文件轮转的最小版本。完整的轮转策略、字段与隐私检查
// 在阶段 8 完成（docs/Decisions.md 第 11.2 节）。
void installDiagnosticLog();

// 当前日志文件路径。日志不可写时返回空串。
QString diagnosticLogPath();

} // namespace mub::app
