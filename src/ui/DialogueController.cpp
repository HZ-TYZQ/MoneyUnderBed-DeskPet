#include "ui/DialogueController.h"

#include "core/RandomSource.h"
#include "dialogue/DialogueData.h"
#include "dialogue/LineSelection.h"
#include "ui/BubbleWindow.h"
#include "ui/CharacterPresenter.h"
#include "ui/CharacterWindow.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QScreen>

namespace mub::ui {

namespace {

Q_LOGGING_CATEGORY(lcDialogue, "mub.ui.dialogue")

// 与角色节拍相同的约 60 Hz。打字推进和「面板跟随角色」都靠它。
constexpr int kTickIntervalMs = 16;

QRect availableGeometryOf(const QWidget *widget)
{
    const QScreen *screen = widget->screen();
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return {};
    }
    return screen->availableGeometry();
}

dialogue::LineTrigger triggerFor(const core::EventKind kind)
{
    return kind == core::EventKind::ClickFeedback
        ? dialogue::LineTrigger::ClickFeedback
        : dialogue::LineTrigger::AutonomousChatter;
}

} // namespace

DialogueController::DialogueController(CharacterPresenter &presenter,
                                       CharacterWindow &character,
                                       const core::TimeSource &timeSource,
                                       core::RandomSource &random,
                                       platform::DeskPetWindowBackend *backend,
                                       QObject *parent)
    : QObject(parent)
    , presenter_(&presenter)
    , character_(&character)
    , random_(&random)
    , session_(timeSource)
    , bubble_(new BubbleWindow(backend))
{
    connect(bubble_, &BubbleWindow::clicked, this,
            &DialogueController::handleBubbleClick);
    connect(presenter_, &CharacterPresenter::eventReplaced, this,
            &DialogueController::handleEventReplaced);

    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(kTickIntervalMs);
    connect(&timer_, &QTimer::timeout, this, &DialogueController::tick);
}

DialogueController::~DialogueController()
{
    delete bubble_;
}

void DialogueController::setScale(const int scale)
{
    bubble_->renderer().setScale(scale);
    if (isShowing()) {
        refreshContent();
    }
}

int DialogueController::scale() const
{
    return bubble_->renderer().scale();
}

void DialogueController::setSessionSuspended(const bool suspended)
{
    session_.setSuspended(suspended);
    if (suspended) {
        timer_.stop();
    } else if (session_.isActive()) {
        timer_.start();
    }
}

bool DialogueController::isShowing() const
{
    return session_.isActive();
}

core::EventKind DialogueController::ownedEvent() const
{
    return ownedKind_;
}

const dialogue::DialogueSession &DialogueController::session() const
{
    return session_;
}

const BubbleWindow &DialogueController::bubble() const
{
    return *bubble_;
}

bool DialogueController::consumeCharacterClick()
{
    if (!session_.isActive()) {
        return false;
    }
    handleBubbleClick();
    return true;
}

bool DialogueController::showChatterBubble(const core::EventKind kind)
{
    const QString id = dialogue::selectLineId(triggerFor(kind), *random_);
    const dialogue::Dialogue *entry = dialogue::findDialogue(id);
    if (entry == nullptr) {
        qCWarning(lcDialogue).noquote()
            << QStringLiteral("no line available for %1").arg(core::eventKindId(kind));
        return false;
    }
    return begin(*entry, kind);
}

bool DialogueController::startDialogue(const QString &dialogueId)
{
    const dialogue::Dialogue *entry = dialogue::findDialogue(dialogueId);
    if (entry == nullptr) {
        qCCritical(lcDialogue).noquote()
            << QStringLiteral("unknown dialogue id: %1").arg(dialogueId);
        return false;
    }
    return begin(*entry, core::EventKind::Dialogue);
}

bool DialogueController::begin(const dialogue::Dialogue &entry,
                               const core::EventKind kind)
{
    // 调用方已经拿到了事件，这里只登记所有权。
    ownedKind_ = kind;
    session_.start(entry);
    if (!session_.isActive()) {
        // 空页面数据。不显示气泡，也不接管事件。
        ownedKind_ = core::EventKind::None;
        return false;
    }

    // 第 4.1 节：显示连续对话期间暂停自主移动和自主行为。
    // 单页气泡不冻结，角色可以边走边说。
    presenter_->setDialogueActive(kind == core::EventKind::Dialogue);

    faceId_.clear();
    pageText_.clear();
    face_ = QImage();
    refreshContent();
    bubble_->show();
    // 第 4.8 节：角色绘制在气泡之上。
    character_->raise();
    if (!session_.isSuspended()) {
        timer_.start();
    }

    qCInfo(lcDialogue).noquote()
        << QStringLiteral("dialogue started id=%1 pages=%2 event=%3")
               .arg(QString::fromLatin1(entry.id))
               .arg(session_.pageCount())
               .arg(core::eventKindId(kind));
    return true;
}

void DialogueController::handleBubbleClick()
{
    if (!session_.click()) {
        return;
    }
    if (session_.isActive()) {
        refreshContent();
        return;
    }
    stop();
}

void DialogueController::handleEventReplaced(const core::EventKind oldKind)
{
    if (oldKind == core::EventKind::None || oldKind != ownedKind_) {
        return;
    }
    // 协调器已经切换到新事件，这里只清理自己，不再调用 finishEvent()。
    qCInfo(lcDialogue).noquote()
        << QStringLiteral("dialogue dropped: %1 was replaced")
               .arg(core::eventKindId(oldKind));
    ownedKind_ = core::EventKind::None;
    hideBubble();
}

void DialogueController::stop()
{
    const core::EventKind owned = ownedKind_;
    ownedKind_ = core::EventKind::None;
    hideBubble();
    if (owned != core::EventKind::None) {
        presenter_->finishEvent(owned);
    }
}

void DialogueController::hideBubble()
{
    timer_.stop();
    session_.stop();
    bubble_->hide();
    presenter_->setDialogueActive(false);
}

void DialogueController::tick()
{
    if (session_.update()) {
        if (!session_.isActive()) {
            // 打完最后一页后超时，或单页气泡到时自动消失。
            stop();
            return;
        }
        refreshContent();
    }
    // 对话期间仍允许拖动角色，面板跟随移动（第 4.1 节）。
    reposition();
}

void DialogueController::refreshContent()
{
    BubbleRenderer &renderer = bubble_->renderer();

    // 表情只在换页时重新加载，打字过程中每帧都读资源没有意义。
    const QString faceId = session_.faceId();
    const bool faceChanged = faceId != faceId_;
    if (faceChanged) {
        faceId_ = faceId;
        face_ = QImage(dialogue::faceAssetPath(faceId));
        if (face_.isNull() && !faceId.isEmpty()) {
            qCWarning(lcDialogue).noquote()
                << QStringLiteral("missing face asset: %1").arg(faceId);
        }
    }

    const QString pageText = session_.fullText();
    if (faceChanged || pageText != pageText_) {
        pageText_ = pageText;
        // setContent 会把可见字符数重置为整页，紧接着的 setVisibleCharacters
        // 再按打字进度收回来。
        renderer.setContent(face_, pageText);
    }

    renderer.setVisibleCharacters(static_cast<int>(session_.visibleText().size()));
    renderer.setTyping(session_.state() == dialogue::DialogueState::Typing);

    reposition();
    bubble_->update();
}

void DialogueController::reposition()
{
    if (!session_.isActive()) {
        return;
    }
    const QRect place = bubble_->renderer().placeFor(character_->frameGeometry(),
                                                     availableGeometryOf(character_));
    bubble_->applyPlacement(place);
}

} // namespace mub::ui
