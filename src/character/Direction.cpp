#include "character/Direction.h"

#include <QLatin1String>

#include <algorithm>
#include <cmath>

namespace mub::character {

namespace {

constexpr auto kUpLeft = "up-left";
constexpr auto kDownLeft = "down-left";
constexpr auto kUpRight = "up-right";
constexpr auto kDownRight = "down-right";

} // namespace

QString facingId(const Facing facing)
{
    switch (facing) {
    case Facing::UpLeft:
        return QLatin1String(kUpLeft);
    case Facing::DownLeft:
        return QLatin1String(kDownLeft);
    case Facing::UpRight:
        return QLatin1String(kUpRight);
    case Facing::DownRight:
        return QLatin1String(kDownRight);
    }
    return QLatin1String(kDownLeft);
}

QString spriteIdFor(const MotionState motion, const Facing facing)
{
    const QLatin1String prefix = motion == MotionState::Running
        ? QLatin1String("run-")
        : QLatin1String("idle-");
    return prefix + facingId(facing);
}

bool facesLeft(const Facing facing)
{
    return facing == Facing::UpLeft || facing == Facing::DownLeft;
}

bool facesUp(const Facing facing)
{
    return facing == Facing::UpLeft || facing == Facing::UpRight;
}

Facing makeFacing(const bool left, const bool up)
{
    if (left) {
        return up ? Facing::UpLeft : Facing::DownLeft;
    }
    return up ? Facing::UpRight : Facing::DownRight;
}

DirectionResolver::DirectionResolver(DirectionConfig config)
    : config_(config)
{
    config_.deadZone = std::max(0.0, config_.deadZone);
    config_.hysteresis = std::max(0.0, config_.hysteresis);
}

Facing DirectionResolver::update(const QPointF &velocity)
{
    const double dx = velocity.x();
    const double dy = velocity.y();
    const bool movingHorizontally = std::abs(dx) >= config_.deadZone;
    const bool movingVertically = std::abs(dy) >= config_.deadZone;

    if (!movingHorizontally && !movingVertically) {
        // 停止后保持最后移动方向。
        motion_ = MotionState::Idle;
        return facing_;
    }

    motion_ = MotionState::Running;

    // 水平分量落在死区内时沿用上一次的左右朝向，
    // 这也实现了「纯垂直移动沿用上一次的左右朝向」。
    const bool left = movingHorizontally ? resolveLeft(dx) : facesLeft(facing_);

    // 垂直分量落在死区内时使用正面，
    // 这实现了「纯水平移动默认使用正面方向」。
    const bool up = movingVertically ? resolveUp(dy) : false;

    facing_ = makeFacing(left, up);
    return facing_;
}

Facing DirectionResolver::facing() const
{
    return facing_;
}

MotionState DirectionResolver::motionState() const
{
    return motion_;
}

void DirectionResolver::setFacing(const Facing facing)
{
    facing_ = facing;
}

bool DirectionResolver::resolveLeft(const double dx) const
{
    // 维持当前朝向只要越过死区即可；改变朝向还需要再越过滞后量。
    const bool currentlyLeft = facesLeft(facing_);
    const double switchThreshold = config_.deadZone + config_.hysteresis;
    if (currentlyLeft) {
        return !(dx >= switchThreshold);
    }
    return dx <= -switchThreshold;
}

bool DirectionResolver::resolveUp(const double dy) const
{
    // 屏幕坐标 y 向下为正，因此 dy 为负才是向上。
    const bool currentlyUp = facesUp(facing_);
    const double switchThreshold = config_.deadZone + config_.hysteresis;
    if (currentlyUp) {
        return !(dy >= switchThreshold);
    }
    return dy <= -switchThreshold;
}

} // namespace mub::character
