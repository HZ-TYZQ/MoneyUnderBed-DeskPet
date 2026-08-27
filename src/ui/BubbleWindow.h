#pragma once

#include "ui/BubbleRenderer.h"

#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QShowEvent;

namespace mub::platform {
class DeskPetWindowBackend;
}

namespace mub::ui {

// 气泡窗口。
//
// 独立的顶层窗口，不是角色窗口的子控件：角色窗口按像素设置了命中掩码，
// 把气泡放进去会被掩码裁掉，也会被角色的固定尺寸限制。
//
// 窗口大小始终等于面板大小，面板本身是不透明矩形，因此窗口内没有透明边距，
// 不会形成看不见的大矩形点击区域（docs/Decisions.md 第 4.1 节）。
class BubbleWindow final : public QWidget
{
    Q_OBJECT

public:
    // `backend` 由调用方注入，窗口不拥有它。测试可以传入假实现。
    explicit BubbleWindow(platform::DeskPetWindowBackend *backend,
                          QWidget *parent = nullptr);
    ~BubbleWindow() override;

    BubbleRenderer &renderer();
    const BubbleRenderer &renderer() const;

    // 按渲染器当前内容调整窗口尺寸并移动到 `place`。
    void applyPlacement(const QRect &place);

signals:
    // 第 4.1 节：点击对话框和点击角色都能补全或推进台词。
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    BubbleRenderer renderer_;
    platform::DeskPetWindowBackend *backend_ = nullptr;
    bool configured_ = false;
};

} // namespace mub::ui
