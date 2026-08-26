#include "ui/CharacterWindow.h"

#include "character/HitMask.h"
#include "core/ScreenPlacement.h"
#include "platform/DeskPetWindowBackend.h"

#include <QCursor>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>
#include <QShowEvent>
#include <QWindow>

#include <utility>

namespace mub::ui {

namespace {

Q_LOGGING_CATEGORY(lcCharacterWindow, "mub.ui.character")

} // namespace

CharacterWindow::CharacterWindow(character::SpriteSheet sheet,
                                 const int integerScale,
                                 platform::DeskPetWindowBackend *backend,
                                 QWidget *parent)
    : QWidget(parent)
    , sheet_(std::move(sheet))
    , integerScale_(integerScale > 0 ? integerScale : 1)
    , backend_(backend)
{
    setObjectName(QStringLiteral("mub-character-window"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    // 显示时不激活窗口，配合平台层的焦点策略保证不抢走当前应用焦点。
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAutoFillBackground(false);
    setMouseTracking(true);

    const QSize frameSize = sheet_.frameSize();
    setFixedSize(frameSize.width() * integerScale_,
                 frameSize.height() * integerScale_);
}

CharacterWindow::~CharacterWindow() = default;

int CharacterWindow::integerScale() const
{
    return integerScale_;
}

void CharacterWindow::setSpriteSheet(character::SpriteSheet sheet)
{
    if (!sheet.isValid()) {
        return;
    }
    sheet_ = std::move(sheet);
    frameIndex_ = 0;
    cachedFrameIndex_ = -1;
    applyHitMask();
    update();
}

const character::SpriteSheet &CharacterWindow::spriteSheet() const
{
    return sheet_;
}

int CharacterWindow::frameIndex() const
{
    return frameIndex_;
}

void CharacterWindow::setFrameIndex(const int index)
{
    if (!sheet_.isValid()) {
        return;
    }
    const int count = sheet_.frameCount();
    const int wrapped = count > 0 ? ((index % count) + count) % count : 0;
    if (wrapped == frameIndex_) {
        return;
    }
    frameIndex_ = wrapped;
    applyHitMask();
    update();
}

void CharacterWindow::setAlwaysOnTop(const bool enabled)
{
    alwaysOnTop_ = enabled;
    if (backend_ != nullptr && windowHandle() != nullptr) {
        backend_->setAlwaysOnTop(windowHandle(), enabled);
    }
}

bool CharacterWindow::isAlwaysOnTop() const
{
    return alwaysOnTop_;
}

QRegion CharacterWindow::hitRegion() const
{
    return hitRegion_;
}

void CharacterWindow::moveToBottomOf(const QRect &availableGeometry,
                                     const double horizontalRatio)
{
    move(core::bottomAnchoredPosition(availableGeometry, size(), horizontalRatio));
}

void CharacterWindow::moveToCursorScreenBottom()
{
    // docs/Decisions.md 第 2.1 节：程序启动时角色出现在鼠标所在屏幕的底部，
    // 不恢复上次退出位置。
    const QPoint cursor = QCursor::pos();
    const QScreen *target = QGuiApplication::screenAt(cursor);
    if (target == nullptr) {
        target = QGuiApplication::primaryScreen();
    }
    if (target == nullptr) {
        qCWarning(lcCharacterWindow) << "no screen available; leaving position unchanged";
        return;
    }
    moveToBottomOf(target->availableGeometry(), 0.5);
}

void CharacterWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint global = event->globalPosition().toPoint();
    gesture_.press(global);
    // 抓取点相对窗口左上角的偏移只在按下时算一次，
    // 之后窗口会移动，再算就会跳变。
    dragOffset_ = gesture_.dragOffsetFrom(frameGeometry().topLeft());
    event->accept();
}

void CharacterWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!gesture_.isPressed() || (event->buttons() & Qt::LeftButton) == 0) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint global = event->globalPosition().toPoint();
    const bool becameDrag = gesture_.move(global);
    if (becameDrag) {
        emit dragStarted();
    }
    if (gesture_.isDragging()) {
        // 产品使用手动移动而不是 startSystemDrag。
        // 由窗口管理器接管拖动后程序收不到松开事件，也就无法按
        // docs/Decisions.md 第 3.1 节判断松手位置离屏幕底部有多远。
        // 平台接口仍保留 startSystemDrag，供后续确有需要时使用。
        move(global - dragOffset_);
    }
    event->accept();
}

void CharacterWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    switch (gesture_.release(event->globalPosition().toPoint())) {
    case core::GestureRecognizer::Release::Click:
        emit clicked();
        break;
    case core::GestureRecognizer::Release::DragEnd:
        emit dragFinished();
        break;
    case core::GestureRecognizer::Release::None:
        break;
    }
    event->accept();
}

void CharacterWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (!sheet_.isValid()) {
        return;
    }

    if (cachedFrameIndex_ != frameIndex_) {
        cachedFrame_ = QPixmap::fromImage(sheet_.frame(frameIndex_));
        cachedFrameIndex_ = frameIndex_;
    }

    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    // 像素画必须用最近邻放大，禁止平滑插值（docs/Decisions.md 第 5.1 节）。
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawPixmap(rect(), cachedFrame_);
}

void CharacterWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (configured_ || backend_ == nullptr) {
        applyHitMask();
        return;
    }

    QWindow *handle = windowHandle();
    if (handle == nullptr) {
        return;
    }

    backend_->configureAsDeskPet(handle);
    backend_->setAlwaysOnTop(handle, alwaysOnTop_);
    configured_ = true;

    const platform::BackendCapabilities caps = backend_->capabilities();
    qCInfo(lcCharacterWindow).noquote()
        << QStringLiteral("backend=%1 always_on_top=%2 hit_mask=%3 passthrough=%4 window_list_excluded=%5")
               .arg(caps.name)
               .arg(caps.alwaysOnTop)
               .arg(caps.pixelHitMask)
               .arg(caps.inputPassthrough)
               .arg(caps.excludeFromWindowList);

    applyHitMask();
}

void CharacterWindow::applyHitMask()
{
    if (!sheet_.isValid()) {
        return;
    }

    // 可见像素接收交互，透明区域穿透（docs/Decisions.md 第 3.4 节）。
    hitRegion_ = character::opaqueRegion(sheet_.frame(frameIndex_), integerScale_);
    if (backend_ == nullptr) {
        return;
    }
    QWindow *handle = windowHandle();
    if (handle == nullptr) {
        return;
    }
    if (!backend_->capabilities().pixelHitMask) {
        qCWarning(lcCharacterWindow)
            << "backend cannot set a pixel hit mask; transparent areas will block input";
        return;
    }
    backend_->setHitMask(handle, hitRegion_);
}

} // namespace mub::ui
