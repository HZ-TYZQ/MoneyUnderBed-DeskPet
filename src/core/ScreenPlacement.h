#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>

namespace mub::core {

// 角色活动区域的几何计算。纯函数，不接触 QScreen，便于确定性测试。
//
// 规则来自 docs/Decisions.md 第 2.1 节：
// 程序启动时角色出现在鼠标所在屏幕的底部；角色平时在当前屏幕底部活动。

// 角色停在可用区域底部时的窗口左上角坐标。
// `horizontalRatio` 为 0 表示贴左，1 表示贴右，0.5 表示居中。
QPoint bottomAnchoredPosition(const QRect &availableGeometry,
                              const QSize &windowSize,
                              double horizontalRatio,
                              int bottomMargin = 0);

// 把窗口位置夹回可用区域内，保证角色不会整体移出屏幕。
QPoint clampToAvailable(const QRect &availableGeometry, const QSize &windowSize,
                        const QPoint &position);

// 窗口底边与可用区域底边的距离。用于判断松手后是留在原地还是返回底部。
int distanceFromBottom(const QRect &availableGeometry, const QSize &windowSize,
                       const QPoint &position);

} // namespace mub::core
