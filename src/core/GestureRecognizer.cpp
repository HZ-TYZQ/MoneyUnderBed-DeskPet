#include "core/GestureRecognizer.h"

#include <algorithm>

namespace mub::core {

GestureRecognizer::GestureRecognizer(const int dragThreshold)
    : dragThreshold_(std::max(1, dragThreshold))
{
}

void GestureRecognizer::press(const QPoint &globalPosition)
{
    pressed_ = true;
    dragging_ = false;
    pressPosition_ = globalPosition;
}

bool GestureRecognizer::move(const QPoint &globalPosition)
{
    if (!pressed_ || dragging_) {
        return false;
    }
    if (!exceedsThreshold(globalPosition)) {
        return false;
    }
    dragging_ = true;
    return true;
}

GestureRecognizer::Release GestureRecognizer::release(const QPoint &globalPosition)
{
    if (!pressed_) {
        return Release::None;
    }

    // 松开时再判一次阈值：某些平台上快速拖动可能没有中间的移动事件。
    const bool draggedNow = dragging_ || exceedsThreshold(globalPosition);
    pressed_ = false;
    dragging_ = false;
    return draggedNow ? Release::DragEnd : Release::Click;
}

void GestureRecognizer::cancel()
{
    pressed_ = false;
    dragging_ = false;
}

bool GestureRecognizer::isPressed() const
{
    return pressed_;
}

bool GestureRecognizer::isDragging() const
{
    return dragging_;
}

QPoint GestureRecognizer::pressPosition() const
{
    return pressPosition_;
}

QPoint GestureRecognizer::dragOffsetFrom(const QPoint &windowTopLeft) const
{
    return pressPosition_ - windowTopLeft;
}

bool GestureRecognizer::exceedsThreshold(const QPoint &globalPosition) const
{
    const QPoint delta = globalPosition - pressPosition_;
    // 用切比雪夫距离，避免开方，也避免整数溢出。
    return std::max(std::abs(delta.x()), std::abs(delta.y())) >= dragThreshold_;
}

} // namespace mub::core
