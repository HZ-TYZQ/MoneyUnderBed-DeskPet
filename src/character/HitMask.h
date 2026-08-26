#pragma once

#include <QRegion>

class QImage;

namespace mub::character {

// 由 alpha 通道生成窗口命中区域。
//
// docs/Decisions.md 第 3.4 节要求角色可见像素接收交互、透明区域穿透鼠标输入，
// 不能形成不可见的矩形阻挡区。窗口掩码是实现该要求的可移植方式。
//
// `integerScale` 必须为正。返回的区域位于缩放后的窗口坐标系中。
// `alphaThreshold` 是判定为不透明的最小 alpha，取值 1 到 255。
QRegion opaqueRegion(const QImage &frame, int integerScale, int alphaThreshold = 1);

} // namespace mub::character
