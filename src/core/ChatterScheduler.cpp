#include "core/ChatterScheduler.h"

#include "core/RandomSource.h"
#include "core/TimeSource.h"

#include <algorithm>

namespace mub::core {

ChatterScheduler::ChatterScheduler(const TimeSource &timeSource, RandomSource &random,
                                   ChatterScheduleConfig config)
    : timeSource_(&timeSource)
    , random_(&random)
    , config_(config)
{
}

void ChatterScheduler::setConfig(const ChatterScheduleConfig &config)
{
    config_ = config;

    if (!isRunning()) {
        // 关到 `0%`：停止调度。下次开启时从一整轮重新开始，不接着旧进度。
        started_ = false;
        return;
    }
    // 仍在运行：正在计时的这一轮连同它的间隔与概率都保持原样，
    // 新值从下一轮开始（第 14.8 节：不缩短也不补算正在进行的间隔）。
}

const ChatterScheduleConfig &ChatterScheduler::config() const
{
    return config_;
}

void ChatterScheduler::setEnabled(const bool enabled)
{
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
    // 重新允许时从一整轮重新开始：安静或隐藏期间不积累「欠下的」闲聊。
    started_ = false;
}

bool ChatterScheduler::isEnabled() const
{
    return enabled_;
}

bool ChatterScheduler::isRunning() const
{
    return enabled_ && config_.chancePercent > 0 && config_.minIntervalMs > 0;
}

void ChatterScheduler::beginRound()
{
    // 一轮开始时把间隔与概率一起定下来，中途改设置不影响这一轮。
    activeIntervalMs_ = std::max(1, config_.minIntervalMs);
    activeChancePercent_ = config_.chancePercent;
    roundDeadlineMs_ = timeSource_->nowMs() + activeIntervalMs_;
    started_ = true;
}

bool ChatterScheduler::update()
{
    if (!isRunning()) {
        started_ = false;
        return false;
    }
    if (!started_) {
        beginRound();
        return false;
    }
    if (timeSource_->nowMs() < roundDeadlineMs_) {
        return false;
    }

    // 一轮只判定一次。命中与否都立刻开始下一轮，因此调用方的请求被拒绝时
    // 不会留下待补播的闲聊。
    const bool hit = random_->chance(activeChancePercent_);
    beginRound();
    return hit;
}

qint64 ChatterScheduler::remainingMs() const
{
    if (!isRunning() || !started_) {
        return 0;
    }
    return std::max<qint64>(0, roundDeadlineMs_ - timeSource_->nowMs());
}

} // namespace mub::core
