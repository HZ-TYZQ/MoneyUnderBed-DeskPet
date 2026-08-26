#include "ui/CharacterPresenter.h"

#include "character/AnimationClip.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"
#include "ui/CharacterWindow.h"

#include <QCursor>
#include <QLoggingCategory>
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
    behavior_.setPaused(paused);
    if (paused) {
        animation_.pause();
    } else {
        animation_.resume();
    }
}

bool CharacterPresenter::isPaused() const
{
    return behavior_.isPaused();
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

void CharacterPresenter::tick()
{
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

    // 阶段 6 的对话系统接管该请求。当前只是消费掉，避免无限累积。
    behavior_.consumeChatterRequest();
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
