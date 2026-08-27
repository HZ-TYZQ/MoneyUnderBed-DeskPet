#include "character/AnimationPlayer.h"

#include "core/TimeSource.h"

#include <algorithm>

namespace mub::character {

AnimationPlayer::AnimationPlayer(const core::TimeSource &timeSource)
    : timeSource_(&timeSource)
{
}

void AnimationPlayer::play(const AnimationClip &clip)
{
    if (clip_ != nullptr && clip_->id == clip.id) {
        return;
    }
    restart(clip);
}

void AnimationPlayer::restart(const AnimationClip &clip)
{
    restartFromFrame(clip, 0);
}

void AnimationPlayer::restartFromFrame(const AnimationClip &clip,
                                       const int frameIndex)
{
    clip_ = &clip;
    frameIndex_ = std::clamp(frameIndex, 0, std::max(1, clip.frameCount) - 1);
    loopCount_ = 0;
    accumulatedMs_ = 0;
    finished_ = false;
    lastAdvanceMs_ = timeSource_->nowMs();
}

bool AnimationPlayer::update()
{
    if (clip_ == nullptr || paused_ || finished_) {
        return false;
    }
    const int frameDuration = std::max(1, clip_->frameDurationMs);
    const int frameCount = std::max(1, clip_->frameCount);

    const qint64 now = timeSource_->nowMs();
    qint64 delta = now - lastAdvanceMs_;
    lastAdvanceMs_ = now;
    if (delta <= 0) {
        return false;
    }
    if (delta > TimeJumpThresholdMs) {
        // 锁屏、睡眠或进程被挂起后恢复。不补播离开期间的帧。
        delta = frameDuration;
    }

    accumulatedMs_ += delta;
    if (accumulatedMs_ < frameDuration) {
        return false;
    }

    const qint64 steps = accumulatedMs_ / frameDuration;
    accumulatedMs_ %= frameDuration;

    const int previousFrame = frameIndex_;
    const qint64 absolute = static_cast<qint64>(frameIndex_) + steps;
    if (clip_->loop == LoopMode::Loop) {
        loopCount_ += static_cast<int>(absolute / frameCount);
        frameIndex_ = static_cast<int>(absolute % frameCount);
    } else {
        if (absolute >= frameCount) {
            frameIndex_ = frameCount - 1;
            finished_ = true;
        } else {
            frameIndex_ = static_cast<int>(absolute);
        }
    }
    return frameIndex_ != previousFrame;
}

void AnimationPlayer::pause()
{
    paused_ = true;
}

void AnimationPlayer::resume()
{
    if (!paused_) {
        return;
    }
    paused_ = false;
    // 丢弃暂停期间的时间，恢复后从当前帧继续，不补播。
    lastAdvanceMs_ = timeSource_->nowMs();
    accumulatedMs_ = 0;
}

bool AnimationPlayer::isPaused() const
{
    return paused_;
}

bool AnimationPlayer::isFinished() const
{
    return finished_;
}

int AnimationPlayer::frameIndex() const
{
    return frameIndex_;
}

int AnimationPlayer::loopCount() const
{
    return loopCount_;
}

const AnimationClip *AnimationPlayer::clip() const
{
    return clip_;
}

} // namespace mub::character
