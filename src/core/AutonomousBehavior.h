#pragma once

#include "core/ActivityMode.h"
#include "core/Settings.h"

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QtGlobal>

namespace mub::core {

class RandomSource;
class TimeSource;

// 自主行为状态。docs/Decisions.md 第 2.1 节与计划第 9.3 节：
// 待机、行走、休息和少量特殊动作，不做饥饿、心情、亲密度或养成数值。
enum class BehaviorState
{
    Idle,              // 站在原地播待机动画
    Walking,           // 沿底部活动区走向一个目标点
    Resting,           // 停止移动并延长停留时间，不使用新素材
    ApproachingCursor, // 仅活跃模式：走向鼠标附近但停在安全距离外
    ReturningToBottom, // 被拖离底部后先短暂停留，再自行返回
    HeldByUser,        // 用户正在拖动，冻结自主行为
};

struct AutonomousBehaviorConfig
{
    // 第 14.3 节把以下参数开放为用户设置，由 `behaviorConfigFrom()` 填入。
    // `bottomTolerancePx` 与 `timeJumpThresholdMs` 除外，见其各自的注释。
    int idleMinMs = 2000;
    int idleMaxMs = 6000;
    int walkMinMs = 1500;
    int walkMaxMs = 4000;
    int restMinMs = 4000;
    int restMaxMs = 12000;

    // 一次待机结束后进入休息的概率，其余情况进入行走。
    int restChancePercent = 25;
    // 仅活跃模式：一次待机结束后改为接近鼠标的概率。
    int approachCursorChancePercent = 12;

    double walkSpeedPxPerSec = 48.0;
    double returnSpeedPxPerSec = 90.0;

    // 被拖离底部后先停留多久再返回。
    int returnDelayMs = 1500;
    // 距底部多近算作「在底部」。第 14.7 节：判定阈值，不是体验参数，不开放。
    int bottomTolerancePx = 8;
    // 接近鼠标时停在多远之外，避免直接覆盖鼠标位置。
    int cursorSafeDistancePx = 60;
    // 单帧最大推进时间。超过视为会话中断，不补算离开期间的行为
    // （docs/Decisions.md 第 2.3 节）。
    qint64 timeJumpThresholdMs = 2000;
};

// 自主行为状态机。
//
// 纯逻辑：不接触 QScreen、窗口或任何平台 API。
// 时间与随机都由外部注入，因此在假时钟和固定种子下行为序列完全可重复
// （计划第 9 节退出门）。
class AutonomousBehavior
{
public:
    AutonomousBehavior(const TimeSource &timeSource, RandomSource &random,
                       AutonomousBehaviorConfig config = {});

    // 当前屏幕的可用区域。拖到另一块屏幕后由外部更新。
    void setActivityArea(const QRect &availableGeometry);
    QRect activityArea() const;

    void setCharacterSize(const QSize &size);
    QSize characterSize() const;

    void setPosition(const QPoint &position);
    QPoint position() const;

    void setMode(ActivityMode mode);
    ActivityMode mode() const;

    // 运行期暂停：冻结移动和自主行为。只对当前运行周期有效，不保存。
    void setPaused(bool paused);
    bool isPaused() const;

    void setCursorPosition(const QPoint &position);

    // 用户开始拖动。冻结自主行为。
    void beginDrag();
    // 用户松手。靠近底部则留在原地，否则短暂停留后返回底部。
    void endDrag(const QPoint &releasePosition);

    // 按当前时间推进一步。返回 true 表示位置或状态发生了变化。
    bool update();

    BehaviorState state() const;
    QPointF velocity() const;

    // 更新行为参数。第 14.8 节：新值只作用于**下一次**对应行为——正在进行的
    // 移动保持开始时的速度快照，当前状态的截止时间也不重算。
    void setConfig(const AutonomousBehaviorConfig &config);
    const AutonomousBehaviorConfig &config() const;

private:
    void enterIdle();
    void enterRest();
    void enterWalk();
    void enterApproachCursor();
    void enterReturnToBottom();
    void chooseNextFromIdle();

    QPoint bottomAnchorFor(int x) const;
    bool isNearBottom() const;
    // 朝 target_ 移动。返回是否已经到达。
    bool moveTowardsTarget(double speedPxPerSec, double deltaSeconds);
    void snapshotSpeed(double speedPxPerSec);
    void clampPosition();

    const TimeSource *timeSource_;
    RandomSource *random_;
    AutonomousBehaviorConfig config_;

    QRect activityArea_{0, 0, 1920, 1080};
    QSize characterSize_{69, 111};
    QPointF position_;
    QPointF velocity_;
    QPoint cursorPosition_;

    BehaviorState state_ = BehaviorState::Idle;
    ActivityMode mode_ = ActivityMode::Quiet;
    bool paused_ = false;
    // 当前移动开始时取的速度快照。改设置不影响正在进行的这一次移动。
    double activeSpeedPxPerSec_ = 0.0;

    QPointF target_;
    qint64 lastUpdateMs_ = 0;
    qint64 stateDeadlineMs_ = 0;
    qint64 pauseStartedMs_ = 0;
    bool started_ = false;
};

// 用户设置到行为参数的映射。
//
// 第 14.7 节：`bottomTolerancePx` 与 `timeJumpThresholdMs` 不开放为设置，
// 因此这里始终取内部默认值——前者是判定阈值不是体验参数，后者支撑第 2.3 节
// 「恢复会话后不补算离开期间的行为」，用户改动会破坏会话恢复语义。
AutonomousBehaviorConfig behaviorConfigFrom(const BehaviorSettings &settings);

} // namespace mub::core
