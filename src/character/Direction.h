#pragma once

#include <QPointF>
#include <QString>

namespace mub::character {

// 逻辑方向。素材只有四个斜向，见 docs/Decisions.md 第 7 节。
enum class Facing
{
    UpLeft,
    DownLeft,
    UpRight,
    DownRight,
};

// 角色当前是站着还是在跑。决定用待机表还是跑动表。
enum class MotionState
{
    Idle,
    Running,
};

QString facingId(Facing facing);

// 由朝向与运动状态取精灵表逻辑标识，例如 `idle-down-left`。
// 这是显式映射，不从文件名推断（docs/Decisions.md 第 7 节）。
QString spriteIdFor(MotionState motion, Facing facing);

bool facesLeft(Facing facing);
bool facesUp(Facing facing);

// 由水平与垂直分量合成朝向。
Facing makeFacing(bool left, bool up);

// 速度方向到朝向的解析器。
//
// 规则（docs/Decisions.md 第 7 节）：
// - 四象限分别对应四个斜向素材。
// - 角色停止后保持最后移动方向。
// - 纯水平移动默认使用正面方向：向左为 down-left，向右为 down-right。
// - 纯垂直移动沿用上一次的左右朝向。
// - 使用速度死区与切换滞后，避免速度在零附近波动时频繁转身。
struct DirectionConfig
{
    // 以下为内部参数，按 docs/Decisions.md 第 2.1 节在原型阶段调优，
    // 第一版不在设置界面暴露。单位是像素每秒。
    double deadZone = 6.0;
    double hysteresis = 4.0;
};

class DirectionResolver
{
public:
    explicit DirectionResolver(DirectionConfig config = {});

    // 传入当前速度，返回应当使用的朝向。
    Facing update(const QPointF &velocity);

    Facing facing() const;
    MotionState motionState() const;

    // 强制设定朝向，例如从设置恢复或被外部行为指定。
    void setFacing(Facing facing);

private:
    bool resolveLeft(double dx) const;
    bool resolveUp(double dy) const;

    DirectionConfig config_;
    Facing facing_ = Facing::DownLeft;
    MotionState motion_ = MotionState::Idle;
};

} // namespace mub::character
