#pragma once

#include <QList>
#include <QtGlobal>

#include <memory>

class QRandomGenerator;

namespace mub::core {

// 可注入的随机源。
//
// 计划第 9.3 节要求随机源可注入固定种子，单元测试验证行为边界
// 而不是依赖概率碰运气。
class RandomSource
{
public:
    RandomSource() = default;
    virtual ~RandomSource();

    RandomSource(const RandomSource &) = delete;
    RandomSource &operator=(const RandomSource &) = delete;
    RandomSource(RandomSource &&) = delete;
    RandomSource &operator=(RandomSource &&) = delete;

    // 闭区间 [minimum, maximum]。minimum 大于 maximum 时返回 minimum。
    virtual int nextInt(int minimum, int maximum) = 0;

    // 半开区间 [0, 1)。
    virtual double nextDouble() = 0;

    // 以 percent 的百分比概率返回 true。
    // percent <= 0 恒为 false，percent >= 100 恒为 true，不消耗随机数。
    bool chance(int percent);
};

// 可指定种子的随机源。同一种子产生同一序列。
class SeededRandomSource final : public RandomSource
{
public:
    explicit SeededRandomSource(quint32 seed);
    ~SeededRandomSource() override;

    int nextInt(int minimum, int maximum) override;
    double nextDouble() override;

private:
    std::unique_ptr<QRandomGenerator> generator_;
};

// 测试用固定序列源。按给定序列循环返回，用于精确构造分支。
class ScriptedRandomSource final : public RandomSource
{
public:
    // `values` 用于 nextInt：按顺序循环返回，超出范围时夹取到 [minimum, maximum]。
    // `doubles` 用于 nextDouble 与 chance。
    ScriptedRandomSource(QList<int> values, QList<double> doubles);

    int nextInt(int minimum, int maximum) override;
    double nextDouble() override;

private:
    QList<int> values_;
    QList<double> doubles_;
    int valueIndex_ = 0;
    int doubleIndex_ = 0;
};

} // namespace mub::core
