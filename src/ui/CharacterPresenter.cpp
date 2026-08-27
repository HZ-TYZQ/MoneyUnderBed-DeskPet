#include "ui/CharacterPresenter.h"

#include "character/AnimationClip.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"
#include "ui/BubbleHost.h"
#include "ui/CharacterWindow.h"

#include <QAction>
#include <QCursor>
#include <QLoggingCategory>
#include <QMenu>
#include <QScreen>
#include <QWindow>

namespace mub::ui {

namespace {

Q_LOGGING_CATEGORY(lcPresenter, "mub.ui.presenter")

// 约 60 Hz。移动的平滑度由这个节拍决定，动画帧率由 AnimationClip 决定，
// 两者互不影响。
constexpr int kTickIntervalMs = 16;

} // namespace

CharacterPresenter::CharacterPresenter(CharacterWindow &window,
                                       const core::TimeSource &timeSource,
                                       core::RandomSource &random,
                                       QObject *parent)
    : QObject(parent)
    , window_(&window)
    , behavior_(timeSource, random)
    , animation_(timeSource)
    , random_(&random)
{
    behavior_.setCharacterSize(window_->size());

    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(kTickIntervalMs);
    connect(&timer_, &QTimer::timeout, this, &CharacterPresenter::tick);

    connect(window_, &CharacterWindow::dragStarted, this,
            [this] { behavior_.beginDrag(); });
    connect(window_, &CharacterWindow::dragFinished, this, [this] {
        // 用户可能把角色拖到了另一块屏幕，先更新活动区域再判断落点。
        syncActivityArea();
        behavior_.endDrag(window_->pos());
    });
    connect(window_, &CharacterWindow::clicked, this,
            &CharacterPresenter::handleClick);
    connect(window_, &CharacterWindow::contextMenuRequested, this,
            &CharacterPresenter::showContextMenu);
}

void CharacterPresenter::start()
{
    syncActivityArea();
    behavior_.setPosition(window_->pos());
    applyFacing();
    timer_.start();
}

void CharacterPresenter::stop()
{
    timer_.stop();
}

void CharacterPresenter::setMode(const core::ActivityMode mode)
{
    behavior_.setMode(mode);
}

core::ActivityMode CharacterPresenter::mode() const
{
    return behavior_.mode();
}

void CharacterPresenter::setPaused(const bool paused)
{
    userPaused_ = paused;
    updateFreeze();
}

bool CharacterPresenter::isPaused() const
{
    return userPaused_;
}

void CharacterPresenter::setBubbleFrequency(const core::BubbleFrequency frequency)
{
    bubbleFrequency_ = frequency;
}

core::BubbleFrequency CharacterPresenter::bubbleFrequency() const
{
    return bubbleFrequency_;
}

void CharacterPresenter::setBubbleHost(BubbleHost *host)
{
    bubbles_ = host;
}

void CharacterPresenter::setDialogueActive(const bool active)
{
    if (dialogueFreeze_ == active) {
        return;
    }
    dialogueFreeze_ = active;
    updateFreeze();
}

bool CharacterPresenter::isDialogueActive() const
{
    return dialogueFreeze_;
}

// 事件已经批下来，交给气泡宿主。宿主接手就由它负责结束，否则立即结束。
void CharacterPresenter::handOffToBubble(const core::EventKind kind,
                                         const QString &dialogueId)
{
    bool taken = false;
    if (bubbles_ != nullptr) {
        taken = kind == core::EventKind::Dialogue
            ? bubbles_->startDialogue(dialogueId)
            : bubbles_->showChatterBubble(kind);
    }
    if (!taken) {
        finishEvent(kind);
    }
}

core::EventDecision CharacterPresenter::requestEvent(const core::EventKind kind)
{
    const core::EventDecision decision = coordinator_.request(kind);
    qCInfo(lcPresenter).noquote()
        << QStringLiteral("event request=%1 decision=%2 current=%3 replaced=%4")
               .arg(core::eventKindId(kind),
                    core::eventDecisionId(decision),
                    core::eventKindId(coordinator_.current()),
                    core::eventKindId(coordinator_.lastReplaced()));
    if (decision == core::EventDecision::Replaced) {
        emit eventReplaced(coordinator_.lastReplaced());
    }
    return decision;
}

void CharacterPresenter::finishEvent(const core::EventKind kind)
{
    coordinator_.finish(kind);
}

const core::EventCoordinator &CharacterPresenter::coordinator() const
{
    return coordinator_;
}

void CharacterPresenter::updateFreeze()
{
    const bool frozen = userPaused_ || eventFreeze_ || dialogueFreeze_;
    behavior_.setPaused(frozen);
    // 事件动画期间时钟必须继续走，否则投喂动画不会推进；
    // 只有用户主动暂停才冻结动画。
    if (userPaused_) {
        animation_.pause();
    } else {
        animation_.resume();
    }
}

void CharacterPresenter::syncActivityArea()
{
    const QScreen *screen = window_->screen();
    if (screen == nullptr) {
        return;
    }
    behavior_.setActivityArea(screen->availableGeometry());
    behavior_.setCharacterSize(window_->size());
}

core::AutonomousBehavior &CharacterPresenter::behavior()
{
    return behavior_;
}

void CharacterPresenter::handleClick()
{
    // 第 4.1 节：对话期间点击只补全或推进当前台词，不另外触发单击反馈。
    if (bubbles_ != nullptr && bubbles_->consumeCharacterClick()) {
        return;
    }

    if (requestEvent(core::EventKind::ClickFeedback)
        == core::EventDecision::Suppressed) {
        // 更高优先级的事件正在进行。被抑制的请求不排队、不补播。
        return;
    }

    const core::ClickFeedback feedback =
        clickFeedback_.select(behavior_.mode(), bubbleFrequency_, *random_);

    // 即使气泡关闭或处于安静模式，也必须给出动作或表情反馈
    // （docs/Decisions.md 第 3.1 节）。当前可用素材没有专门的反应动画，
    // 因此以重置当前待机循环作为可见反馈。
    if (feedback.hasReaction) {
        const character::AnimationClip *clip = character::findClip(currentClipId_);
        if (clip != nullptr) {
            animation_.restart(*clip);
            window_->setFrameIndex(animation_.frameIndex());
        }
    }

    if (!feedback.hasText) {
        finishEvent(core::EventKind::ClickFeedback);
        return;
    }
    emit textFeedbackRequested(core::EventKind::ClickFeedback);
    handOffToBubble(core::EventKind::ClickFeedback);
}

void CharacterPresenter::showContextMenu(const QPoint &globalPosition)
{
    // 阶段 7 才建立完整菜单。这里只提供几个临时测试入口，
    // 以及一个退出项，避免无边框窗口无法关闭。
    //
    // 两个气泡入口是为了让气泡在默认设置下也能被人工检查：默认是安静模式，
    // 第 2.2 节要求安静模式完全抑制气泡，掉落对话又只有 15% 概率。
    // 它们**刻意绕过模式与频率判定**，只走协调器，因此不能保留到正式菜单里。
    QMenu menu;
    QAction *feed = menu.addAction(tr("投喂"));
    QAction *chatter = menu.addAction(tr("说一句（测试）"));
    QAction *dialogue = menu.addAction(tr("演一段对话（测试）"));
    menu.addSeparator();
    QAction *quit = menu.addAction(tr("退出"));

    const QAction *chosen = menu.exec(globalPosition);
    if (chosen == feed) {
        this->feed();
    } else if (chosen == chatter) {
        if (requestEvent(core::EventKind::AutonomousChatter)
            != core::EventDecision::Suppressed) {
            handOffToBubble(core::EventKind::AutonomousChatter);
        }
    } else if (chosen == dialogue) {
        if (requestEvent(core::EventKind::Dialogue) != core::EventDecision::Suppressed) {
            handOffToBubble(core::EventKind::Dialogue,
                            core::FeedingSelector::dropDialogueId());
        }
    } else if (chosen == quit) {
        requestEvent(core::EventKind::Shutdown);
        emit quitRequested();
    }
}

void CharacterPresenter::feed()
{
    if (requestEvent(core::EventKind::Feeding) == core::EventDecision::Suppressed) {
        // 当前投喂动画结束前忽略新的投喂请求，不排队、不重播
        // （docs/Decisions.md 第 3.2 节）。
        return;
    }

    feedingOutcome_ = feeding_.select(*random_);
    const QString clipId = feedingOutcome_ == core::FeedingOutcome::Drop
        ? QStringLiteral("icecream-drop")
        : QStringLiteral("icecream-eat");

    const character::AnimationClip *clip = character::findClip(clipId);
    const character::SpriteSheet *sheet = sheetFor(clipId);
    if (clip == nullptr || sheet == nullptr) {
        qCCritical(lcPresenter) << "feeding animation unavailable:" << clipId;
        finishEvent(core::EventKind::Feeding);
        return;
    }

    currentClipId_ = clipId;
    window_->setSpriteSheet(*sheet);
    animation_.restart(*clip);
    window_->setFrameIndex(animation_.frameIndex());

    eventFreeze_ = true;
    updateFreeze();

    qCInfo(lcPresenter).noquote()
        << QStringLiteral("feeding started outcome=%1")
               .arg(core::feedingOutcomeId(feedingOutcome_));
}

void CharacterPresenter::finishFeeding()
{
    eventFreeze_ = false;
    updateFreeze();
    finishEvent(core::EventKind::Feeding);

    // 强制下一次 applyFacing 换回待机或跑动素材。
    currentClipId_.clear();

    if (feedingOutcome_ != core::FeedingOutcome::Drop) {
        return;
    }

    // 掉落事件结束后进入对应连续对话。对话优先级高于自主闲聊，
    // 因此协调器会自动抑制后者。
    const QString dialogueId = core::FeedingSelector::dropDialogueId();
    emit dialogueRequested(dialogueId);
    if (requestEvent(core::EventKind::Dialogue) == core::EventDecision::Suppressed) {
        return;
    }
    handOffToBubble(core::EventKind::Dialogue, dialogueId);
}

bool CharacterPresenter::advanceEventAnimation()
{
    if (coordinator_.current() != core::EventKind::Feeding) {
        return false;
    }
    if (animation_.update()) {
        window_->setFrameIndex(animation_.frameIndex());
    }
    if (animation_.isFinished()) {
        finishFeeding();
    }
    return true;
}

void CharacterPresenter::tick()
{
    // 事件动画期间由事件独占角色，自主行为和方向映射都不参与。
    if (advanceEventAnimation()) {
        return;
    }

    // 只有活跃模式才需要鼠标位置；安静模式不主动接近鼠标
    // （docs/Decisions.md 第 2.2 节）。
    if (behavior_.mode() == core::ActivityMode::Active) {
        behavior_.setCursorPosition(QCursor::pos());
    }

    if (behavior_.update()) {
        window_->move(behavior_.position());
    }

    applyFacing();

    if (animation_.update()) {
        window_->setFrameIndex(animation_.frameIndex());
    }

    // 自主闲聊同样要经过协调器裁决，不能绕开优先级。
    if (behavior_.consumeChatterRequest()
        && requestEvent(core::EventKind::AutonomousChatter)
            != core::EventDecision::Suppressed) {
        emit textFeedbackRequested(core::EventKind::AutonomousChatter);
        handOffToBubble(core::EventKind::AutonomousChatter);
    }
}

void CharacterPresenter::applyFacing()
{
    direction_.update(behavior_.velocity());
    const QString clipId =
        character::spriteIdFor(direction_.motionState(), direction_.facing());
    if (clipId == currentClipId_) {
        return;
    }

    const character::AnimationClip *clip = character::findClip(clipId);
    const character::SpriteSheet *sheet = sheetFor(clipId);
    if (clip == nullptr || sheet == nullptr) {
        qCWarning(lcPresenter) << "no clip or sprite sheet for" << clipId;
        return;
    }

    currentClipId_ = clipId;
    window_->setSpriteSheet(*sheet);
    animation_.restart(*clip);
    window_->setFrameIndex(animation_.frameIndex());
}

const character::SpriteSheet *CharacterPresenter::sheetFor(const QString &clipId)
{
    const auto cached = sheets_.constFind(clipId);
    if (cached != sheets_.constEnd()) {
        return &cached.value();
    }

    const QString path = character::clipResourcePath(clipId);
    if (path.isEmpty()) {
        return nullptr;
    }
    character::SpriteSheetError error = character::SpriteSheetError::None;
    character::SpriteSheet sheet = character::SpriteSheet::load(path, &error);
    if (!sheet.isValid()) {
        qCCritical(lcPresenter).noquote()
            << QStringLiteral("could not load %1: %2")
                   .arg(path, character::describeSpriteSheetError(error));
        return nullptr;
    }
    return &sheets_.insert(clipId, std::move(sheet)).value();
}

} // namespace mub::ui
