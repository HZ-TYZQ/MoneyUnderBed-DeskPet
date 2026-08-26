#include "core/TimeSource.h"

#include <QElapsedTimer>

#include <algorithm>

namespace mub::core {

TimeSource::~TimeSource() = default;

MonotonicTimeSource::MonotonicTimeSource()
    : timer_(std::make_unique<QElapsedTimer>())
{
    timer_->start();
}

MonotonicTimeSource::~MonotonicTimeSource() = default;

qint64 MonotonicTimeSource::nowMs() const
{
    return timer_->elapsed();
}

qint64 ManualTimeSource::nowMs() const
{
    return nowMs_;
}

void ManualTimeSource::advance(const qint64 milliseconds)
{
    nowMs_ += std::max<qint64>(0, milliseconds);
}

void ManualTimeSource::setNow(const qint64 milliseconds)
{
    // 时间源必须单调不减，否则依赖它的状态机会出现负的时间差。
    nowMs_ = std::max(nowMs_, milliseconds);
}

} // namespace mub::core
