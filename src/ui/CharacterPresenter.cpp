#include "ui/CharacterPresenter.h"

#include "character/AnimationClip.h"
#include "core/RandomSource.h"
#include "core/ScreenPlacement.h"
#include "core/TimeSource.h"
#include "ui/BubbleHost.h"
#include "ui/CharacterWindow.h"

#include <QAction>
#include <QCursor>
#include <QLoggingCategory>
#include <QMenu>
#include <QScreen>
#include <QWindow>

#include <algorithm>

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
    settings_.mode = mode;
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

void CharacterPresenter::setSessionSuspended(const bool suspended)
{
    if (sessionSuspended_ == suspended) {
        return;
    }
    sessionSuspended_ = suspended;
    updateFreeze();
}

bool CharacterPresenter::isSessionSuspended() const
{
    return sessionSuspended_;
}

void CharacterPresenter::setHidden(const bool hidden)
{
    if (hidden_ == hidden) {
        return;
    }
    hidden_ = hidden;

    if (hidden_) {
        // 第 4.2 节：隐藏与退出同属最高优先级。占住协调器即可让其后的自主
        // 闲聊、单击反馈和对话全部被抑制，被抑制的请求不排队、不补播。
        requestEvent(core::EventKind::Shutdown);
        // 正在播放的投喂动画被放弃，冻结位随之释放，否则恢复显示后
        // 自主行为会一直停在事件冻结状态。
        eventFreeze_ = false;
        // 强制下一次 applyFacing 重新装载待机或跑动素材。
        currentClipId_.clear();
    } else {
        finishEvent(core::EventKind::Shutdown);
    }

    updateFreeze();
    qCInfo(lcPresenter) << "character" << (hidden_ ? "hidden" : "shown");
}

bool CharacterPresenter::isHidden() const
{
    return hidden_;
}

void CharacterPresenter::setBubbleFrequency(const core::BubbleFrequency frequency)
{
    settings_.bubble = frequency;
}

core::BubbleFrequency CharacterPresenter::bubbleFrequency() const
{
    return settings_.bubble;
}

void CharacterPresenter::applySettings(const core::Settings &settings)
{
    settings_ = core::sanitized(settings);

    behavior_.setMode(settings_.mode);
    window_->setAlwaysOnTop(settings_.alwaysOnTop);

    if (window_->integerScale() != settings_.scale) {
        window_->setIntegerScale(settings_.scale);
        // 窗口尺寸变了，活动区域内的可移动范围随之改变。
        syncActivityArea();
        behavior_.setPosition(window_->pos());
    }

}

const core::Settings &CharacterPresenter::settings() const
{
    return settings_;
}

void CharacterPresenter::setBubbleHost(BubbleHost *host)
{
    bubbles_ = host;
}

void CharacterPresenter::setRecallAvailable(const bool available)
{
    recallAvailable_ = available;
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
    const bool frozen = userPaused_ || sessionSuspended_ || hidden_ || eventFreeze_
        || dialogueFreeze_;
    behavior_.setPaused(frozen);
    // 事件动画期间时钟必须继续走，否则投喂动画不会推进；
    // 用户暂停、系统会话暂停与隐藏冻结动画；事件与连续对话只冻结自主行为，
    // 否则投喂动画无法推进、对话期间待机动画也会停住。
    if (userPaused_ || sessionSuspended_ || hidden_) {
        animation_.pause();
    } else {
        animation_.resume();
    }
}

void CharacterPresenter::syncActivityArea()
{
    QScreen *screen = window_->screen();
    if (screen == nullptr || !QGuiApplication::screens().contains(screen)) {
        screen = QGuiApplication::screenAt(window_->frameGeometry().center());
    }
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return;
    }
    const QRect available = screen->availableGeometry();
    const QPoint visiblePosition =
        core::clampToAvailable(available, window_->size(), window_->pos());
    if (visiblePosition != window_->pos()) {
        qCInfo(lcPresenter) << "screen geometry changed; clamping character from"
                            << window_->pos() << "to" << visiblePosition;
        window_->move(visiblePosition);
    }
    behavior_.setActivityArea(available);
    behavior_.setCharacterSize(window_->size());
    behavior_.setPosition(window_->pos());
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
        clickFeedback_.select(behavior_.mode(), settings_.bubble, *random_);

    // 即使气泡关闭或处于安静模式，也必须给出动作或表情反馈
    // （docs/Decisions.md 第 3.1 节）。当前可用素材没有专门的反应动画，
    // 因此跳到当前循环的另一半再正常续播。与固定重置到第 0 帧不同，
    // 这保证每次点击都立刻改变画面，不会因为恰好已在第 0 帧而看不见。
    if (feedback.hasReaction) {
        const character::AnimationClip *clip = character::findClip(currentClipId_);
        if (clip != nullptr) {
            const int frameCount = std::max(1, clip->frameCount);
            const int reactionFrame =
                (window_->frameIndex() + std::max(1, frameCount / 2)) % frameCount;
            animation_.restartFromFrame(*clip, reactionFrame);
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
    // 第 3.3 节：角色右键菜单是主要控制入口。菜单状态与当前模式同步；
    // 明确选择「退出」后直接结束，不再弹确认框。
    QMenu menu;

    QAction *feed = menu.addAction(tr("投喂"));

    QAction *active = menu.addAction(tr("活跃模式"));
    active->setCheckable(true);
    active->setChecked(settings_.mode == core::ActivityMode::Active);

    QAction *pause = menu.addAction(tr("暂停"));
    pause->setCheckable(true);
    pause->setChecked(userPaused_);

    // 只有存在唤回通道时才提供隐藏（第 3.3 节）。
    QAction *hide = recallAvailable_ ? menu.addAction(tr("隐藏")) : nullptr;

    menu.addSeparator();
    QAction *settings = menu.addAction(tr("设置…"));
    QAction *about = menu.addAction(tr("关于…"));
    menu.addSeparator();
    QAction *quit = menu.addAction(tr("退出"));

    const QAction *chosen = menu.exec(globalPosition);
    if (chosen == feed) {
        this->feed();
    } else if (chosen == active) {
        setMode(active->isChecked() ? core::ActivityMode::Active
                                    : core::ActivityMode::Quiet);
        emit settingsChanged(settings_);
    } else if (chosen == pause) {
        // 第 2.2 节：暂停只对当前运行周期有效，不保存。
        setPaused(pause->isChecked());
    } else if (hide != nullptr && chosen == hide) {
        emit hideRequested();
    } else if (chosen == settings) {
        emit settingsRequested();
    } else if (chosen == about) {
        emit aboutRequested();
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

    // 关闭气泡时仍要消费自主行为产生的请求，但不能申请事件或把它交给气泡；
    // 否则活跃模式会绕过点击反馈里的频率判断继续说话。
    const bool chatterRequested = behavior_.consumeChatterRequest();
    if (chatterRequested && settings_.bubble != core::BubbleFrequency::Off
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

    const QString path = character::clipAssetPath(clipId);
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
