#pragma once

#include <QString>
#include <QWidget>

class QDoubleSpinBox;
class QSlider;

namespace mub::ui {

// 高级层的一个数值输入行。
//
// docs/Decisions.md 第 14.2 节：
//
// - 单值、有界的参数和百分比配**滑块与数字框**，双向绑定；滑块用于快速粗调，
//   数字框用于精确输入。滑块永远与数字框成对出现，不单独提供滑块。
// - 成对上下限（待机、行走、休息三组时长）**只提供数字框**：两个滑块互相顶越
//   会引入额外的越界处理策略，而那正是成对校验要管的地方；区间滑块不是 Qt
//   内置控件，为此自制控件超出 `1.1.0` 范围。
// - 高级层使用面向用户的自然单位，秒级时长以秒显示，保存和运行时再转换为毫秒。
//
// 第 14.8 节要求把「接收」与「落盘」分开，因此本控件给出两个信号：
//
// - `valueEdited()`：滑块拖动过程中的每一个合法中间值，用于立即生效。
// - `editingCommitted()`：滑块释放或数字框完成编辑，用于落盘。
//
// 数字框关闭了 keyboardTracking，因此输入 `3000` 时不会先后产生 `3`、`30`、`300`
// 这些用户根本没打算提交的值——它们既不进运行时也不进配置文件。
class ValueEditor final : public QWidget
{
    Q_OBJECT

public:
    // `minimum`、`maximum`、`value` 都是**实际参数的单位**（毫秒、像素、百分比）。
    // `displayDivisor` 为 1 时直接显示；为 1000 时以秒显示，保留一位小数。
    ValueEditor(int minimum, int maximum, int displayDivisor, QString suffix,
                bool withSlider, QWidget *parent = nullptr);

    // 刷新显示。不发出任何信号，避免「套用 → 上报 → 再套用」的回环。
    void setValue(int value);
    int value() const;

    bool hasSlider() const;

signals:
    void valueEdited(int value);
    void editingCommitted(int value);

private:
    int fromDisplay(double display) const;
    double toDisplay(int value) const;

    int minimum_;
    int maximum_;
    int displayDivisor_;
    bool updating_ = false;
    QSlider *slider_ = nullptr;
    QDoubleSpinBox *spin_ = nullptr;
};

} // namespace mub::ui
