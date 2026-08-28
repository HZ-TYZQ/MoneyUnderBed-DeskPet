#include "core/AutonomousBehavior.h"

#include "core/RandomSource.h"
#include "core/ScreenPlacement.h"
#include "core/TimeSource.h"

#include <algorithm>
#include <cmath>

namespace mub::core {

namespace {

double lengthOf(const QPointF &vector)
{
    return std::hypot(vector.x(), vector.y());
}

} // namespace

AutonomousBehavior::AutonomousBehavior(const TimeSource &timeSource,
                                       RandomSource &random,
                                       AutonomousBehaviorConfig config)
    : timeSource_(&timeSource)
    , random_(&random)
    , config_(config)
{
    lastUpdateMs_ = timeSource_->nowMs();
    position_ = QPointF(bottomAnchorFor(activityArea_.center().x()));
    target_ = position_;
}

void AutonomousBehavior::setActivityArea(const QRect &availableGeometry)
{
    if (!availableGeometry.isValid()) {
        return;
    }
    activityArea_ = availableGeometry;
    clampPosition();
}

QRect AutonomousBehavior::activityArea() const
{
    return activityArea_;
}

void AutonomousBehavior::setCharacterSize(const QSize &size)
{
    if (size.isEmpty()) {
        return;
    }
    characterSize_ = size;
    clampPosition();
}

QSize AutonomousBehavior::characterSize() const
{
    return characterSize_;
}

void AutonomousBehavior::setPosition(const QPoint &position)
{
    position_ = QPointF(position);
    clampPosition();
}

QPoint AutonomousBehavior::position() const
{
    return position_.toPoint();
}

void AutonomousBehavior::setMode(const ActivityMode mode)
{
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    if (mode_ == ActivityMode::Quiet && state_ == BehaviorState::ApproachingCursor) {
        // 安静模式不主动接近鼠标，立刻停止当前的接近动作。
        enterIdle();
    }
}

ActivityMode AutonomousBehavior::mode() const
{
    return mode_;
}

void AutonomousBehavior::setPaused(const bool paused)
{
    if (paused_ == paused) {
        return;
    }
    const qint64 now = timeSource_->nowMs();
    if (paused) {
        pauseStartedMs_ = now;
    } else {
        // 整体平移截止时间，保留暂停前的剩余等待时间。不能用「当前时刻减
        // 原截止时间」重算，否则暂停时间超过剩余时间时恢复会立即切状态。
        if (started_) {
            stateDeadlineMs_ += std::max<qint64>(0, now - pauseStartedMs_);
        }
        lastUpdateMs_ = now;
    }
    paused_ = paused;
    velocity_ = QPointF();
}

bool AutonomousBehavior::isPaused() const
{
    return paused_;
}

void AutonomousBehavior::setCursorPosition(const QPoint &position)
{
    cursorPosition_ = position;
}

void AutonomousBehavior::beginDrag()
{
    state_ = BehaviorState::HeldByUser;
    velocity_ = QPointF();
}

void AutonomousBehavior::endDrag(const QPoint &releasePosition)
{
    setPosition(releasePosition);
    if (isNearBottom()) {
        // 松手位置靠近屏幕底部时，角色留在该处继续活动。
        enterIdle();
        return;
    }
    // 松手位置远离底部时，角色短暂停留后自行返回底部活动区。
    enterReturnToBottom();
}

bool AutonomousBehavior::update()
{
    const qint64 now = timeSource_->nowMs();
    if (!started_) {
        started_ = true;
        lastUpdateMs_ = now;
        enterIdle();
        return true;
    }

    qint64 delta = now - lastUpdateMs_;
    lastUpdateMs_ = now;

    if (paused_ || state_ == BehaviorState::HeldByUser) {
        velocity_ = QPointF();
        return false;
    }
    if (delta <= 0) {
        return false;
    }
    if (delta > config_.timeJumpThresholdMs) {
        // 锁屏、睡眠或进程被挂起后恢复：不补算离开期间的行为，
        // 把当前状态的截止时间同样后移，避免一恢复就立刻切换状态。
        stateDeadlineMs_ += delta - config_.timeJumpThresholdMs;
        delta = config_.timeJumpThresholdMs;
    }

    const QPointF previousPosition = position_;
    const BehaviorState previousState = state_;
    const double seconds = static_cast<double>(delta) / 1000.0;
    velocity_ = QPointF();

    switch (state_) {
    case BehaviorState::Idle:
        if (now >= stateDeadlineMs_) {
            chooseNextFromIdle();
        }
        break;
    case BehaviorState::Resting:
        if (now >= stateDeadlineMs_) {
            enterIdle();
        }
        break;
    case BehaviorState::Walking:
    case BehaviorState::ApproachingCursor:
        if (moveTowardsTarget(activeSpeedPxPerSec_, seconds)
            || now >= stateDeadlineMs_) {
            enterIdle();
        }
        break;
    case BehaviorState::ReturningToBottom:
        // 先停留，到点后再开始返回。
        if (now >= stateDeadlineMs_) {
            target_ = QPointF(bottomAnchorFor(position_.toPoint().x()));
            if (moveTowardsTarget(activeSpeedPxPerSec_, seconds)) {
                enterIdle();
            }
        }
        break;
    case BehaviorState::HeldByUser:
        break;
    }

    clampPosition();
    return position_ != previousPosition || state_ != previousState;
}

BehaviorState AutonomousBehavior::state() const
{
    return state_;
}

QPointF AutonomousBehavior::velocity() const
{
    return velocity_;
}

void AutonomousBehavior::setConfig(const AutonomousBehaviorConfig &config)
{
    // 立即替换：新值从下一次决策和下一次进入状态开始使用。正在进行的移动
    // 用的是 `activeSpeedPxPerSec_` 快照，当前状态的截止时间早已算好，
    // 因此这里不需要、也不允许重算任何进行中的东西（第 14.8 节）。
    config_ = config;
}

const AutonomousBehaviorConfig &AutonomousBehavior::config() const
{
    return config_;
}

void AutonomousBehavior::snapshotSpeed(const double speedPxPerSec)
{
    activeSpeedPxPerSec_ = speedPxPerSec;
}

void AutonomousBehavior::enterIdle()
{
    // 进入待机前先检查是否被留在了远离底部的位置。
    if (state_ != BehaviorState::ReturningToBottom && !isNearBottom()) {
        enterReturnToBottom();
        return;
    }
    state_ = BehaviorState::Idle;
    velocity_ = QPointF();
    stateDeadlineMs_ = timeSource_->nowMs()
        + random_->nextInt(config_.idleMinMs, config_.idleMaxMs);
}

void AutonomousBehavior::enterRest()
{
    // 休息不依赖新素材：角色停止移动并继续播放待机动画，只延长停留时间
    // （docs/Decisions.md 第 2.1 节）。
    state_ = BehaviorState::Resting;
    velocity_ = QPointF();
    stateDeadlineMs_ = timeSource_->nowMs()
        + random_->nextInt(config_.restMinMs, config_.restMaxMs);
}

void AutonomousBehavior::enterWalk()
{
    state_ = BehaviorState::Walking;
    snapshotSpeed(config_.walkSpeedPxPerSec);
    const int leftBound = activityArea_.x();
    const int rightBound = std::max(leftBound,
                                    activityArea_.x() + activityArea_.width()
                                        - characterSize_.width());
    const int targetX = random_->nextInt(leftBound, rightBound);
    target_ = QPointF(bottomAnchorFor(targetX));
    stateDeadlineMs_ = timeSource_->nowMs()
        + random_->nextInt(config_.walkMinMs, config_.walkMaxMs);
}

void AutonomousBehavior::enterApproachCursor()
{
    state_ = BehaviorState::ApproachingCursor;
    snapshotSpeed(config_.walkSpeedPxPerSec);

    // 停在安全距离外，不直接覆盖鼠标位置（docs/Decisions.md 第 2.1 节）。
    const QPointF characterCentre =
        position_ + QPointF(characterSize_.width() / 2.0,
                            characterSize_.height() / 2.0);
    QPointF toCharacter = characterCentre - QPointF(cursorPosition_);
    const double distance = lengthOf(toCharacter);
    if (distance < 1.0) {
        // 已经压在鼠标上时任选一个方向退开，避免除零。
        toCharacter = QPointF(1.0, 0.0);
    } else {
        toCharacter /= distance;
    }

    const QPointF desiredCentre = QPointF(cursorPosition_)
        + toCharacter * static_cast<double>(config_.cursorSafeDistancePx);
    target_ = desiredCentre - QPointF(characterSize_.width() / 2.0,
                                      characterSize_.height() / 2.0);

    const QPoint clamped = clampToAvailable(activityArea_, characterSize_,
                                            target_.toPoint());
    target_ = QPointF(clamped);
    stateDeadlineMs_ = timeSource_->nowMs()
        + random_->nextInt(config_.walkMinMs, config_.walkMaxMs);
}

void AutonomousBehavior::enterReturnToBottom()
{
    state_ = BehaviorState::ReturningToBottom;
    velocity_ = QPointF();
    snapshotSpeed(config_.returnSpeedPxPerSec);
    target_ = QPointF(bottomAnchorFor(position_.toPoint().x()));
    // 先停留一段时间再返回。
    stateDeadlineMs_ = timeSource_->nowMs() + config_.returnDelayMs;
}

void AutonomousBehavior::chooseNextFromIdle()
{
    // 随机调用顺序固定，保证同一种子下的行为序列可重复。
    if (random_->chance(config_.restChancePercent)) {
        enterRest();
        return;
    }

    // 安静模式不主动接近鼠标（docs/Decisions.md 第 2.2 节）。
    //
    // 自主闲聊**不在这里**。第 14.4 节要求它由独立的时间调度驱动，不依赖待机、
    // 行走或休息之间的状态切换；`1.0.0` 候选把它挂在这次状态切换上，结果是
    // 「说话频率」实际由活动节奏决定。现在由 `ChatterScheduler` 负责。
    if (mode_ == ActivityMode::Active
        && random_->chance(config_.approachCursorChancePercent)) {
        enterApproachCursor();
        return;
    }
    enterWalk();
}

AutonomousBehaviorConfig behaviorConfigFrom(const BehaviorSettings &settings)
{
    AutonomousBehaviorConfig config;
    config.idleMinMs = settings.idleMinMs;
    config.idleMaxMs = settings.idleMaxMs;
    config.walkMinMs = settings.walkMinMs;
    config.walkMaxMs = settings.walkMaxMs;
    config.restMinMs = settings.restMinMs;
    config.restMaxMs = settings.restMaxMs;
    config.restChancePercent = settings.restChancePercent;
    config.approachCursorChancePercent = settings.approachCursorChancePercent;
    config.walkSpeedPxPerSec = settings.walkSpeedPxPerSec;
    config.returnSpeedPxPerSec = settings.returnSpeedPxPerSec;
    config.returnDelayMs = settings.returnDelayMs;
    config.cursorSafeDistancePx = settings.cursorSafeDistancePx;
    // bottomTolerancePx 与 timeJumpThresholdMs 保持默认值：第 14.7 节不开放。
    return config;
}

QPoint AutonomousBehavior::bottomAnchorFor(const int x) const
{
    const int y = activityArea_.y() + activityArea_.height()
        - characterSize_.height();
    return clampToAvailable(activityArea_, characterSize_, QPoint(x, y));
}

bool AutonomousBehavior::isNearBottom() const
{
    return distanceFromBottom(activityArea_, characterSize_, position_.toPoint())
        <= config_.bottomTolerancePx;
}

bool AutonomousBehavior::moveTowardsTarget(const double speedPxPerSec,
                                           const double deltaSeconds)
{
    const QPointF delta = target_ - position_;
    const double distance = lengthOf(delta);
    const double step = speedPxPerSec * deltaSeconds;

    if (distance <= step || distance < 0.5) {
        position_ = target_;
        velocity_ = QPointF();
        return true;
    }

    const QPointF direction = delta / distance;
    position_ += direction * step;
    velocity_ = direction * speedPxPerSec;
    return false;
}

void AutonomousBehavior::clampPosition()
{
    const QPoint clamped =
        clampToAvailable(activityArea_, characterSize_, position_.toPoint());
    // 只在确实越界时改写，避免每次 update 都把亚像素累积抹掉。
    if (clamped != position_.toPoint()) {
        position_ = QPointF(clamped);
    }
}

} // namespace mub::core
