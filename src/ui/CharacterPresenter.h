#pragma once

#include "character/AnimationPlayer.h"
#include "character/Direction.h"
#include "character/SpriteSheet.h"
#include "core/ActivityMode.h"
#include "core/AutonomousBehavior.h"
#include "core/BubbleFrequency.h"
#include "core/ClickFeedback.h"
#include "core/EventCoordinator.h"
#include "core/Feeding.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

namespace mub::core {
class RandomSource;
class TimeSource;
}

namespace mub::ui {

class CharacterWindow;

// 把自主行为、方向映射和动画播放接到角色窗口上。
//
// 这里只做接线：状态由 core 的状态机决定，帧由 AnimationPlayer 决定，
// 朝向由 DirectionResolver 决定。本类不自己判断行为，也不含平台分支。
//
// 阶段 5 引入统一事件协调器后，单击与投喂等事件改由协调器裁决，
// 本类只保留「把结果画出来」的职责。
class CharacterPresenter final : public QObject
{
    Q_OBJECT

public:
    CharacterPresenter(CharacterWindow &window, const core::TimeSource &timeSource,
                       core::RandomSource &random, QObject *parent = nullptr);

    void start();
    void stop();

    void setMode(core::ActivityMode mode);
    core::ActivityMode mode() const;

    void setPaused(bool paused);
    bool isPaused() const;

    void setBubbleFrequency(core::BubbleFrequency frequency);
    core::BubbleFrequency bubbleFrequency() const;

    // 用户主动投喂。公开为产品动作，右键菜单与后续设置／快捷入口共用，
    // 也让组件测试可以直接验证完整的事件生命周期。
    void feed();

    // 提交一次行为请求。返回协调器的裁决。
    // 所有行为请求都必须经过这里，功能模块不得直接切换全局状态
    // （docs/Decisions.md 第 4.2 节）。
    core::EventDecision requestEvent(core::EventKind kind);

    // 结束由调用方拥有的事件。只有当前事件类型匹配时才会清除，迟到的完成
    // 通知不会误清已经替换上来的更高优先级事件。
    void finishEvent(core::EventKind kind);

    const core::EventCoordinator &coordinator() const;

    // 从窗口当前所在屏幕同步活动区域。屏幕变化时应重新调用。
    void syncActivityArea();

    core::AutonomousBehavior &behavior();

signals:
    // 阶段 6 的对话系统接管这两个信号。
    void textFeedbackRequested();
    // 这里只报告触发请求，不提前占用 EventKind::Dialogue。
    // 接收者成功启动会话后再通过 requestEvent() 申请 Dialogue，结束时调用
    // finishEvent()。这样没有接收者时不会遗留永不结束的事件。
    void dialogueRequested(const QString &dialogueId);
    // 更高优先级事件替换了旧事件。旧事件的所有者必须立即清理其会话和 UI，
    // 但不再调用 finish(oldKind)，因为协调器已经切换到了新事件。
    void eventReplaced(core::EventKind oldKind);
    void quitRequested();

private:
    void tick();
    void handleClick();
    void showContextMenu(const QPoint &globalPosition);
    void finishFeeding();
    void updateFreeze();
    bool advanceEventAnimation();
    void applyFacing();
    const character::SpriteSheet *sheetFor(const QString &clipId);

    CharacterWindow *window_;
    core::AutonomousBehavior behavior_;
    character::DirectionResolver direction_;
    character::AnimationPlayer animation_;
    QTimer timer_;
    core::EventCoordinator coordinator_;
    core::ClickFeedbackSelector clickFeedback_;
    core::FeedingSelector feeding_;
    core::RandomSource *random_;
    core::BubbleFrequency bubbleFrequency_ = core::BubbleFrequency::Low;
    core::FeedingOutcome feedingOutcome_ = core::FeedingOutcome::Eat;
    bool userPaused_ = false;
    bool eventFreeze_ = false;
    QHash<QString, character::SpriteSheet> sheets_;
    QString currentClipId_;
};

} // namespace mub::ui
