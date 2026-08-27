#include "Preview.h"

#include "character/AnimationClip.h"
#include "dialogue/DialogueData.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>

#include <utility>

namespace mub::bubbleprobe {

namespace {

constexpr int kTickMs = 16;

QRect availableGeometryOf(const QWidget *widget)
{
    const QScreen *screen = widget->screen();
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    return screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
}

} // namespace

BubbleOverlay::BubbleOverlay(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                   | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
}

BubbleRenderer &BubbleOverlay::renderer()
{
    return renderer_;
}

void BubbleOverlay::mousePressEvent(QMouseEvent *event)
{
    // 点击对话框或点击角色都可以推进台词。
    emit clicked();
    event->accept();
}

void BubbleOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    renderer_.paint(painter);
}

CharacterOverlay::CharacterOverlay(character::SpriteSheet sheet, QWidget *parent)
    : QWidget(parent)
    , sheet_(std::move(sheet))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                   | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setScale(2.0);
}

void CharacterOverlay::setFrameIndex(const int index)
{
    if (index == frameIndex_) {
        return;
    }
    frameIndex_ = index;
    update();
}

void CharacterOverlay::setScale(const double scale)
{
    scale_ = scale > 0.0 ? scale : 1.0;
    const QSize frame = sheet_.frameSize();
    setFixedSize(static_cast<int>(frame.width() * scale_),
                 static_cast<int>(frame.height() * scale_));
    update();
}

void CharacterOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    event->accept();
}

void CharacterOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) == 0) {
        return;
    }
    dragging_ = true;
    move(event->globalPosition().toPoint() - dragOffset_);
    emit moved();
    event->accept();
}

void CharacterOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !dragging_) {
        emit clicked();
    }
    dragging_ = false;
    event->accept();
}

void CharacterOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!sheet_.isValid()) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(rect(), sheet_.frame(frameIndex_));
}

PreviewController::PreviewController(QObject *parent)
    : QObject(parent)
    , session_(clock_)
    , animation_(clock_)
{
    const QString path =
        character::clipResourcePath(QStringLiteral("idle-down-left"));
    character_ = new CharacterOverlay(character::SpriteSheet::load(path));
    bubble_ = new BubbleOverlay();

    connect(character_, &CharacterOverlay::moved, this, &PreviewController::reposition);
    connect(character_, &CharacterOverlay::clicked, this, [this] {
        session_.click();
        refreshContent();
    });
    connect(bubble_, &BubbleOverlay::clicked, this, [this] {
        session_.click();
        refreshContent();
    });

    const character::AnimationClip *idle =
        character::findClip(QStringLiteral("idle-down-left"));
    if (idle != nullptr) {
        animation_.restart(*idle);
    }

    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(kTickMs);
    connect(&timer_, &QTimer::timeout, this, &PreviewController::tick);
    timer_.start();
}

PreviewController::~PreviewController()
{
    delete bubble_;
    delete character_;
}

void PreviewController::show()
{
    const QRect available = availableGeometryOf(character_);
    character_->move(available.center().x() - character_->width() / 2,
                     available.bottom() - character_->height() - 80);
    character_->show();
    reposition();
}

void PreviewController::setParameters(const BubbleParameters &parameters)
{
    parameters_ = parameters;
    bubble_->renderer().setParameters(parameters);
    character_->setScale(parameters.scale);

    dialogue::DialogueSessionConfig config;
    config.typingMsPerChar = parameters.typingMsPerChar;
    // 原型不测超时与自动消失，避免审核过程中气泡自己消失。
    config.idleTimeoutMs = 0;
    config.singlePageAutoHideMs = 0;
    session_ = dialogue::DialogueSession(clock_, config);
    if (!dialogueId_.isEmpty()) {
        playDialogue(dialogueId_);
    } else {
        refreshContent();
    }
}

void PreviewController::playDialogue(const QString &dialogueId)
{
    dialogueId_ = dialogueId;
    const dialogue::Dialogue *entry = dialogue::findDialogue(dialogueId);
    if (entry == nullptr) {
        return;
    }
    session_.start(*entry);
    refreshContent();
}

void PreviewController::tick()
{
    if (animation_.update()) {
        character_->setFrameIndex(animation_.frameIndex());
    }
    if (session_.update()) {
        refreshContent();
    }
}

void PreviewController::refreshContent()
{
    BubbleRenderer &renderer = bubble_->renderer();
    if (!session_.isActive()) {
        bubble_->hide();
        return;
    }

    const QImage face(dialogue::faceResourcePath(session_.faceId()));
    renderer.setContent(face, session_.fullText());
    renderer.setVisibleCharacters(static_cast<int>(session_.visibleText().size()));
    renderer.setTyping(session_.state() == dialogue::DialogueState::Typing);

    emit pageStatusChanged(static_cast<int>(renderer.wrappedLines().size()),
                           renderer.maxLinesThatFit(), renderer.overflowsPanel(),
                           session_.pageIndex() + 1, session_.pageCount());

    bubble_->show();
    reposition();
    bubble_->update();
    // CSS 里角色 z-index 4、气泡 z-index 3，角色画在气泡之上。
    character_->raise();
}

void PreviewController::reposition()
{
    if (!session_.isActive()) {
        return;
    }
    // 对话框跟随角色移动。
    const QRect place = bubble_->renderer().placeFor(character_->frameGeometry(),
                                                     availableGeometryOf(character_));
    bubble_->setFixedSize(place.size());
    bubble_->move(place.topLeft());
}

} // namespace mub::bubbleprobe
