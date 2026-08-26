#pragma once

#include "character/SpriteSheet.h"
#include "core/GestureRecognizer.h"

#include <QPixmap>
#include <QPoint>
#include <QWidget>

#include <memory>

class QMouseEvent;
class QPaintEvent;
class QShowEvent;

namespace mub::platform {
class DeskPetWindowBackend;
}

namespace mub::ui {

// 角色窗口。
//
// 阶段 3 的范围：透明、无边框、不进任务栏、默认置顶、按像素命中、
// 整数倍最近邻绘制、启动定位到鼠标所在屏幕底部、点击与拖动手势区分。
// 不含动画播放、自主行为和任何产品反馈，那些属于阶段 4 与阶段 5。
class CharacterWindow final : public QWidget
{
    Q_OBJECT

public:
    // `backend` 由调用方注入，窗口不拥有它。测试可以传入假实现。
    CharacterWindow(character::SpriteSheet sheet, int integerScale,
                    platform::DeskPetWindowBackend *backend,
                    QWidget *parent = nullptr);
    ~CharacterWindow() override;

    int integerScale() const;
    int frameIndex() const;
    void setFrameIndex(int index);

    void setAlwaysOnTop(bool enabled);
    bool isAlwaysOnTop() const;

    // 当前生效的命中区域。空区域表示尚未应用掩码。
    QRegion hitRegion() const;

    // 把角色放到指定屏幕可用区域的底部。
    void moveToBottomOf(const QRect &availableGeometry, double horizontalRatio);

    // 放到鼠标所在屏幕的底部。没有可用屏幕时不动。
    void moveToCursorScreenBottom();

signals:
    // 阶段 5 由事件协调器接管。阶段 3 只发出信号，不做任何反馈。
    void clicked();
    void dragFinished();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void applyHitMask();

    character::SpriteSheet sheet_;
    int integerScale_ = 1;
    int frameIndex_ = 0;
    bool alwaysOnTop_ = true;
    bool configured_ = false;

    platform::DeskPetWindowBackend *backend_ = nullptr;
    core::GestureRecognizer gesture_;
    QPoint dragOffset_;
    QRegion hitRegion_;
    QPixmap cachedFrame_;
    int cachedFrameIndex_ = -1;
};

} // namespace mub::ui
