#pragma once

#include "character/AnimationPlayer.h"
#include "character/Direction.h"
#include "character/SpriteSheet.h"
#include "core/ActivityMode.h"
#include "core/AutonomousBehavior.h"

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

    // 从窗口当前所在屏幕同步活动区域。屏幕变化时应重新调用。
    void syncActivityArea();

    core::AutonomousBehavior &behavior();

private:
    void tick();
    void applyFacing();
    const character::SpriteSheet *sheetFor(const QString &clipId);

    CharacterWindow *window_;
    core::AutonomousBehavior behavior_;
    character::DirectionResolver direction_;
    character::AnimationPlayer animation_;
    QTimer timer_;
    QHash<QString, character::SpriteSheet> sheets_;
    QString currentClipId_;
};

} // namespace mub::ui
