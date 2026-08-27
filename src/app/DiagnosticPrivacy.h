#pragma once

#include <QString>

namespace mub::app {

// 本地日志保留诊断意义，但隐藏用户主目录和意外传入的常见环境变量值。
// 产品日志源仍不得主动记录窗口标题、文件内容或完整环境。
QString redactDiagnosticText(QString text);

} // namespace mub::app
