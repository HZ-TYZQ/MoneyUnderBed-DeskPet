#include "ui/BubbleWindow.h"

#include "platform/DeskPetWindowBackend.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QShowEvent>
#include <QWindow>

namespace mub::ui {

BubbleWindow::BubbleWindow(platform::DeskPetWindowBackend *backend, QWidget *parent)
    : QWidget(parent)
    , backend_(backend)
{
    setObjectName(QStringLiteral("mub-bubble-window"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    // 气泡不抢焦点：出现时不能打断用户正在输入的窗口
    // （docs/Decisions.md 第 3.4 节）。
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAutoFillBackground(false);
    configureNativeWindow();
}

BubbleWindow::~BubbleWindow() = default;

BubbleRenderer &BubbleWindow::renderer()
{
    return renderer_;
}

const BubbleRenderer &BubbleWindow::renderer() const
{
    return renderer_;
}

void BubbleWindow::applyPlacement(const QRect &place)
{
    if (size() != place.size()) {
        setFixedSize(place.size());
    }
    if (pos() != place.topLeft()) {
        move(place.topLeft());
    }
}

void BubbleWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    emit clicked();
    event->accept();
}

void BubbleWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    renderer_.paint(painter);
}

void BubbleWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 构造阶段正常情况下已经完成；保留这里作为原生句柄创建失败时的兜底。
    configureNativeWindow();
}

void BubbleWindow::configureNativeWindow()
{
    if (configured_ || backend_ == nullptr) {
        return;
    }
    static_cast<void>(winId());
    QWindow *handle = windowHandle();
    if (handle == nullptr) {
        return;
    }
    // 与角色窗口用同一套平台配置：置顶、不进任务栏和窗口列表、不抢焦点。
    backend_->configureAsDeskPet(handle);
    backend_->setAlwaysOnTop(handle, true);
    configured_ = true;
}

} // namespace mub::ui
