#pragma once

#include "character/AnimationClip.h"

#include <QtGlobal>

namespace mub::core {
class TimeSource;
}

namespace mub::character {

// 动画播放器。
//
// 计划第 9.1 节：只负责帧范围、循环方式和时钟，不负责选择角色行为。
// 选择播放哪段动画是行为层的事。
//
// 时间来自注入的 TimeSource，因此测试不依赖真实等待。
class AnimationPlayer
{
public:
    // 超过该阈值的时间跳跃视为会话中断（锁屏、睡眠、进程被挂起），
    // 只前进一帧，不补播离开期间的帧（docs/Decisions.md 第 2.3 节）。
    static constexpr qint64 TimeJumpThresholdMs = 2000;

    explicit AnimationPlayer(const core::TimeSource &timeSource);

    // 帧时长策略。第 14.8 节：新值在**下一次启动对应动画**时生效，
    // 正在播放的这一段保持开始时的帧时长快照，不重算已经走过的时间。
    void setTiming(const AnimationTiming &timing);
    const AnimationTiming &timing() const;
    // 当前正在播放的这一段实际使用的每帧毫秒数。
    int activeFrameDurationMs() const;

    // 开始播放。同一段动画重复调用不会重新开始。
    void play(const AnimationClip &clip);

    // 强制从第一帧重新开始，即使是同一段动画。
    void restart(const AnimationClip &clip);

    // 从指定帧重新开始。越界帧夹取到有效范围；用于需要立即产生可见变化、
    // 随后仍由同一播放器正常续播的短反馈。
    void restartFromFrame(const AnimationClip &clip, int frameIndex);

    // 按当前时间推进。返回 true 表示帧号发生了变化。
    bool update();

    // 暂停时钟。暂停期间 update() 不推进帧。
    void pause();

    // 恢复时钟。不补播暂停期间的帧。
    void resume();

    bool isPaused() const;
    bool isFinished() const;
    int frameIndex() const;
    int loopCount() const;
    const AnimationClip *clip() const;

private:
    const core::TimeSource *timeSource_;
    const AnimationClip *clip_ = nullptr;
    AnimationTiming timing_;
    int activeFrameDurationMs_ = AnimationTiming{}.idleFrameMs;
    qint64 lastAdvanceMs_ = 0;
    qint64 accumulatedMs_ = 0;
    int frameIndex_ = 0;
    int loopCount_ = 0;
    bool paused_ = false;
    bool finished_ = false;
};

} // namespace mub::character
