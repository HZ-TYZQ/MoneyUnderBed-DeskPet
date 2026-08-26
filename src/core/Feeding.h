#pragma once

#include <QString>

namespace mub::core {

class RandomSource;

// 一次投喂的结果。
enum class FeedingOutcome
{
    // 正常吃完冰淇淋。
    Eat,
    // 偶发的「冰淇淋掉落」情节。
    Drop,
};

QString feedingOutcomeId(FeedingOutcome outcome);

struct FeedingConfig
{
    // 掉落概率。docs/Decisions.md 第 3.2 节：按程序内置概率偶发触发，
    // 不提供用户设置。属于待调优的内部参数。
    int dropChancePercent = 15;
};

// 决定一次投喂是正常吃还是掉落。
//
// 只负责选分支。「投喂进行中忽略新的投喂请求」由 EventCoordinator 统一裁决，
// 不在这里重复实现，避免两处各判一次而结论不一致。
class FeedingSelector
{
public:
    explicit FeedingSelector(FeedingConfig config = {});

    FeedingOutcome select(RandomSource &random) const;

    // 掉落事件结束后要进入的连续对话标识。阶段 6 的对话数据据此查找。
    static QString dropDialogueId();

private:
    FeedingConfig config_;
};

} // namespace mub::core
