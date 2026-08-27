#pragma once

#include "core/ActivityMode.h"

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
    // 以下全部是 docs/Decisions.md 第 2.1 节所说的内部参数：
    // 在原型阶段调优，第一版不在设置界面暴露。当前取值尚未定稿。
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
    // 距底部多近算作「在底部」。
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

    // 活跃模式下本步是否请求了一次自主闲聊。读取后清除。
    // 阶段 6 的对话系统接管该请求；安静模式永远不会置位。
    bool consumeChatterRequest();

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
    bool chatterRequested_ = false;

    QPointF target_;
    qint64 lastUpdateMs_ = 0;
    qint64 stateDeadlineMs_ = 0;
    qint64 pauseStartedMs_ = 0;
    bool started_ = false;
};

} // namespace mub::core
