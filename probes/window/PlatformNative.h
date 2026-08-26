#pragma once

#include <QString>

class QWidget;

// 窄平台查询与候选实现，只服务于探针。
// 这里的代码不会被正式产品直接复用；正式实现按阶段 3 在 src/platform/ 重新定义。
namespace platform_native {

// 当前平台是否提供原生查询与原生穿透候选路径。
bool isSupported();

// 平台名，用于日志区分。
QString platformName();

// 描述窗口的原生状态。Windows 上返回扩展样式位；其他平台返回未支持说明。
QString describeWindow(const QWidget *widget);

// 当前前台窗口标题，用于焦点检查。
QString foregroundWindowTitle();

// 只修改原生扩展样式实现输入穿透，不触碰 Qt 窗口标志。
// 返回是否实际执行了修改。
bool setNativeInputTransparent(QWidget *widget, bool enabled);

} // namespace platform_native
