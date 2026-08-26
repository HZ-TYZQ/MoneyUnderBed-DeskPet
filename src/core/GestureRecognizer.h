#pragma once

#include <QPoint>

namespace mub::core {

// 区分单击与拖动。
//
// docs/Plans/DevelopmentPlan.md 第 10.2 节：基于移动距离阈值区分单击与拖动，
// 按下时不提前计为单击。本类不产生任何产品反馈，只报告手势结论。
class GestureRecognizer
{
public:
    enum class Release
    {
        None,     // 没有正在进行的手势
        Click,    // 位移未超过阈值，判定为单击
        DragEnd,  // 位移超过阈值，判定为拖动结束
    };

    static constexpr int DefaultDragThreshold = 4;

    explicit GestureRecognizer(int dragThreshold = DefaultDragThreshold);

    // 按下。此时不产生任何结论。
    void press(const QPoint &globalPosition);

    // 移动。返回 true 表示本次移动使手势首次跨过阈值，从此计为拖动。
    bool move(const QPoint &globalPosition);

    // 松开并给出结论。
    Release release(const QPoint &globalPosition);

    // 手势被外部打断，例如窗口失去输入或程序进入隐藏。
    void cancel();

    bool isPressed() const;
    bool isDragging() const;
    QPoint pressPosition() const;

    // 按下点相对窗口左上角的偏移，用于手动拖动时保持抓取点不跳变。
    QPoint dragOffsetFrom(const QPoint &windowTopLeft) const;

private:
    bool exceedsThreshold(const QPoint &globalPosition) const;

    int dragThreshold_;
    bool pressed_ = false;
    bool dragging_ = false;
    QPoint pressPosition_;
};

} // namespace mub::core
