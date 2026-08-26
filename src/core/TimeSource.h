#pragma once

#include <QtGlobal>

#include <memory>

class QElapsedTimer;

namespace mub::core {

// 可注入的时间源。
//
// 计划第 9.1 节要求为动画时钟注入可控时间源，使测试不依赖真实等待。
// 产品用 MonotonicTimeSource，测试用 ManualTimeSource。
class TimeSource
{
public:
    TimeSource() = default;
    virtual ~TimeSource();

    TimeSource(const TimeSource &) = delete;
    TimeSource &operator=(const TimeSource &) = delete;
    TimeSource(TimeSource &&) = delete;
    TimeSource &operator=(TimeSource &&) = delete;

    // 自某个固定起点起的毫秒数。必须单调不减。
    virtual qint64 nowMs() const = 0;
};

// 基于 QElapsedTimer 的单调时钟。不受系统时间调整影响。
class MonotonicTimeSource final : public TimeSource
{
public:
    MonotonicTimeSource();
    ~MonotonicTimeSource() override;

    qint64 nowMs() const override;

private:
    std::unique_ptr<QElapsedTimer> timer_;
};

// 测试用手动时钟。只会因显式调用而前进。
class ManualTimeSource final : public TimeSource
{
public:
    qint64 nowMs() const override;

    void advance(qint64 milliseconds);
    void setNow(qint64 milliseconds);

private:
    qint64 nowMs_ = 0;
};

} // namespace mub::core
