#include "ui/ValueEditor.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSlider>

#include <algorithm>
#include <cmath>

namespace mub::ui {

ValueEditor::ValueEditor(const int minimum, const int maximum,
                         const int displayDivisor, QString suffix,
                         const bool withSlider, QWidget *parent)
    : QWidget(parent)
    , minimum_(minimum)
    , maximum_(maximum)
    , displayDivisor_(std::max(1, displayDivisor))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    if (withSlider) {
        slider_ = new QSlider(Qt::Horizontal, this);
        slider_->setRange(minimum_, maximum_);
        slider_->setSingleStep(std::max(1, (maximum_ - minimum_) / 100));
        slider_->setPageStep(std::max(1, (maximum_ - minimum_) / 10));
        layout->addWidget(slider_, 1);
    }

    spin_ = new QDoubleSpinBox(this);
    spin_->setDecimals(displayDivisor_ == 1 ? 0 : 1);
    spin_->setRange(toDisplay(minimum_), toDisplay(maximum_));
    spin_->setSingleStep(displayDivisor_ == 1 ? 1.0 : 0.1);
    spin_->setSuffix(std::move(suffix));
    // 关键：输入 `3000` 时不产生 3、30、300 这些中间值（第 14.8 节）。
    spin_->setKeyboardTracking(false);
    layout->addWidget(spin_);

    if (slider_ != nullptr) {
        connect(slider_, &QSlider::valueChanged, this, [this](const int value) {
            if (updating_) {
                return;
            }
            const QSignalBlocker blocker(spin_);
            spin_->setValue(toDisplay(value));
            // 拖动过程中的每一个值都立即生效，但不落盘。
            emit valueEdited(value);
        });
        connect(slider_, &QSlider::sliderReleased, this,
                [this] { emit editingCommitted(value()); });
    }

    connect(spin_, &QDoubleSpinBox::valueChanged, this, [this](const double display) {
        if (updating_) {
            return;
        }
        const int next = fromDisplay(display);
        if (slider_ != nullptr) {
            const QSignalBlocker blocker(slider_);
            slider_->setValue(next);
        }
        // keyboardTracking 已关闭，所以到这里的都是用户提交过的值：
        // 立即生效并立即落盘。
        emit valueEdited(next);
        emit editingCommitted(next);
    });
}

int ValueEditor::fromDisplay(const double display) const
{
    const int value = static_cast<int>(std::lround(display * displayDivisor_));
    return std::clamp(value, minimum_, maximum_);
}

double ValueEditor::toDisplay(const int value) const
{
    return static_cast<double>(value) / displayDivisor_;
}

void ValueEditor::setValue(const int value)
{
    const int clamped = std::clamp(value, minimum_, maximum_);
    updating_ = true;
    if (slider_ != nullptr) {
        slider_->setValue(clamped);
    }
    spin_->setValue(toDisplay(clamped));
    updating_ = false;
}

int ValueEditor::value() const
{
    return fromDisplay(spin_->value());
}

bool ValueEditor::hasSlider() const
{
    return slider_ != nullptr;
}

} // namespace mub::ui
