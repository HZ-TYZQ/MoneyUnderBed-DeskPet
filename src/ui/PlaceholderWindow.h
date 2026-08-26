#pragma once

#include <QWidget>

class QPaintEvent;

namespace mub::ui {

// 阶段 2 的构建验证窗口。它只证明工程结构、Qt 链接、资源和元数据可用，
// 不是角色窗口，也不包含任何角色行为。
// 阶段 3 建立真正的角色窗口后删除本类。
class PlaceholderWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceholderWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace mub::ui
