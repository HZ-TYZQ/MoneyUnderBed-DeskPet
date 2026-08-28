#pragma once

#include "character/AnimationPlayer.h"
#include "character/Direction.h"
#include "character/SpriteSheet.h"
#include "core/ActivityMode.h"
#include "core/AutonomousBehavior.h"
#include "core/ChatterScheduler.h"
#include "core/ClickFeedback.h"
#include "core/EventCoordinator.h"
#include "core/Feeding.h"
#include "core/Settings.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

namespace mub::core {
class RandomSource;
class TimeSource;
}

namespace mub::ui {

class BubbleHost;
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

    // 系统会话暂停独立于用户菜单里的“暂停”。锁屏、睡眠或显示器关闭恢复时
    // 不能清掉用户主动暂停，反之亦然。
    void setSessionSuspended(bool suspended);
    bool isSessionSuspended() const;

    // 角色已被用户隐藏。
    //
    // 第 4.2 节把「退出／隐藏」定为最高优先级事件：隐藏立即结束正在进行的
    // 事件，并且在隐藏期间抑制其后的一切内容。看不见的角色不能继续自主移动、
    // 播放动画，更不能自己弹气泡。
    void setHidden(bool hidden);
    bool isHidden() const;


    // 一次性套用全部用户设置（docs/Decisions.md 第 5.1 节：修改后立即生效）。
    // 暂停不在设置里：第 2.2 节规定它只对当前运行周期有效。
    void applySettings(const core::Settings &settings);
    const core::Settings &settings() const;

    // 气泡宿主。不注入时所有事件都由本类即刻结束，产品退化为无气泡但可运行。
    // 本类不拥有它。
    void setBubbleHost(BubbleHost *host);

    // 是否存在唤回通道（托盘或单实例唤回）。
    // 第 3.3 节要求隐藏后必须能唤回，因此没有任何唤回通道时不提供「隐藏」
    // 菜单项 —— 不给一个藏了就找不回来的入口。
    void setRecallAvailable(bool available);

    // 连续对话正在显示。第 4.1 节：此期间暂停自主移动和自主行为，
    // 但继续播放当前朝向的待机动画。单页气泡不触发本状态。
    void setDialogueActive(bool active);
    bool isDialogueActive() const;

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
    // 以下两个信号只报告「发生了什么」，供日志、测试和后续功能观察。
    // 真正的气泡显示走 BubbleHost，不通过信号，因为本类需要同步知道
    // 气泡是否真的起来了，才能决定事件由谁结束。
    void textFeedbackRequested(core::EventKind kind);
    void dialogueRequested(const QString &dialogueId);
    // 更高优先级事件替换了旧事件。旧事件的所有者必须立即清理其会话和 UI，
    // 但不再调用 finish(oldKind)，因为协调器已经切换到了新事件。
    void eventReplaced(core::EventKind oldKind);

    // 菜单里直接改掉的设置。上层据此保存并同步设置窗口
    // （第 5.1 节：修改后立即生效并保存）。暂停不走这里，它不保存。
    void settingsChanged(const core::Settings &settings);

    // 右键菜单入口。窗口只报告请求，具体界面由上层组装
    // （第 3.3 节：角色右键菜单是主要控制入口）。
    void settingsRequested();
    void aboutRequested();
    // 第 2.3 节：隐藏状态不保存，下次启动角色必须出现。
    void hideRequested();
    void quitRequested();

private:
    void tick();
    void handleClick();
    void showContextMenu(const QPoint &globalPosition);
    // 申请到事件之后交给气泡宿主。宿主没接手时立即结束该事件，
    // 保证任何一条路径都不会留下永不结束的事件。
    void handOffToBubble(core::EventKind kind, const QString &dialogueId = {});
    void finishFeeding();
    void updateFreeze();
    bool advanceEventAnimation();
    void applyFacing();
    const character::SpriteSheet *sheetFor(const QString &clipId);

    CharacterWindow *window_;
    core::AutonomousBehavior behavior_;
    // 第 14.4 节：自主闲聊由独立的时间调度驱动，不搭在行为状态切换上。
    core::ChatterScheduler chatter_;
    character::DirectionResolver direction_;
    character::AnimationPlayer animation_;
    QTimer timer_;
    core::EventCoordinator coordinator_;
    core::ClickFeedbackSelector clickFeedback_;
    core::FeedingSelector feeding_;
    core::RandomSource *random_;
    BubbleHost *bubbles_ = nullptr;
    core::Settings settings_;
    core::FeedingOutcome feedingOutcome_ = core::FeedingOutcome::Eat;
    bool userPaused_ = false;
    bool sessionSuspended_ = false;
    bool hidden_ = false;
    bool eventFreeze_ = false;
    bool dialogueFreeze_ = false;
    bool recallAvailable_ = false;
    QHash<QString, character::SpriteSheet> sheets_;
    QString currentClipId_;
};

} // namespace mub::ui
