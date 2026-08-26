#include "core/RandomSource.h"

#include <QList>
#include <QRandomGenerator>

#include <algorithm>
#include <utility>

namespace mub::core {

RandomSource::~RandomSource() = default;

bool RandomSource::chance(const int percent)
{
    // 两端不消耗随机数，使「必然发生」和「必然不发生」的测试
    // 不会打乱后续随机序列。
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }
    return nextDouble() * 100.0 < static_cast<double>(percent);
}

SeededRandomSource::SeededRandomSource(const quint32 seed)
    : generator_(std::make_unique<QRandomGenerator>(seed))
{
}

SeededRandomSource::~SeededRandomSource() = default;

int SeededRandomSource::nextInt(const int minimum, const int maximum)
{
    if (minimum >= maximum) {
        return minimum;
    }
    return generator_->bounded(minimum, maximum + 1);
}

double SeededRandomSource::nextDouble()
{
    return generator_->generateDouble();
}

ScriptedRandomSource::ScriptedRandomSource(QList<int> values, QList<double> doubles)
    : values_(std::move(values))
    , doubles_(std::move(doubles))
{
}

int ScriptedRandomSource::nextInt(const int minimum, const int maximum)
{
    if (minimum >= maximum) {
        return minimum;
    }
    if (values_.isEmpty()) {
        return minimum;
    }
    const int value = values_.at(valueIndex_ % values_.size());
    ++valueIndex_;
    return std::clamp(value, minimum, maximum);
}

double ScriptedRandomSource::nextDouble()
{
    if (doubles_.isEmpty()) {
        return 0.0;
    }
    const double value = doubles_.at(doubleIndex_ % doubles_.size());
    ++doubleIndex_;
    return std::clamp(value, 0.0, 0.999999);
}

} // namespace mub::core
