#pragma once

#include <QStringView>

namespace mub::app {

// 桌面标识通常来自 XDG_CURRENT_DESKTOP（可用冒号分隔多个值），
// 不做子串匹配，避免把无关桌面误判为 niri。
bool desktopListContainsNiri(QStringView value);
bool isNiriDesktop();

} // namespace mub::app
