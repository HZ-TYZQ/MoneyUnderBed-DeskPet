#pragma once

#include <QtGlobal>

namespace mub::core {

class RandomSource;
class TimeSource;

// 自主闲聊的调度参数。对应 docs/Decisions.md 第 14.4 节的两个新参数。
struct ChatterScheduleConfig
{
    int minIntervalMs = 120000;
    // `0` 即「关闭」。第 14.4 节：关闭档只由概率 `0%` 表达，不另设开关。
    int chancePercent = 30;
};

// 自主闲聊的时间调度器。
//
// docs/Decisions.md 第 14.4 节：
//
// - 自主闲聊由**独立的时间调度**驱动，不依赖待机、行走或休息之间的状态切换。
//   `1.0.0` 候选把闲聊挂在自主行为的状态切换上，导致「说话频率」实际由活动
//   节奏决定，低频与正常没有差别。
// - 每轮完整间隔结束时**只进行一次**概率判定。
// - 未触发、角色正忙或请求被更高优先级事件抑制时，不排队也不补播，
//   直接重新开始下一轮完整间隔。
// - 安静模式和「关闭」档停止调度。
// - 设置变化在下一轮生效，不缩短也不补算正在进行的间隔（第 14.8 节）。
//
// 第 4 节「不做高频陪聊」对最高档同样成立：本类只在间隔到期时判定一次，
// 不会因为角色空闲就连续说话。
//
// 纯逻辑：时间与随机都由外部注入，因此在假时钟和固定随机源下完全可重复。
class ChatterScheduler
{
public:
    ChatterScheduler(const TimeSource &timeSource, RandomSource &random,
                     ChatterScheduleConfig config = {});

    // 下一轮生效。不影响正在计时的这一轮。
    void setConfig(const ChatterScheduleConfig &config);
    const ChatterScheduleConfig &config() const;

    // 是否允许调度。安静模式、隐藏和暂停都由调用方置为 `false`。
    // 重新允许时从一整轮间隔重新开始，不接着上次的进度。
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // 按当前时间推进。返回 true 表示本轮命中，调用方应当去申请一次闲聊事件。
    //
    // 无论命中与否，返回后都已经开始新的一轮间隔：调用方的事件申请被拒绝时
    // 什么都不用做，不排队、不补播。
    bool update();

    // 距当前这一轮到期还有多久。测试与诊断用。
    qint64 remainingMs() const;

private:
    void beginRound();
    bool isRunning() const;

    const TimeSource *timeSource_;
    RandomSource *random_;
    ChatterScheduleConfig config_;
    // 正在计时的这一轮所用的间隔与概率。改设置不影响这一轮。
    int activeIntervalMs_ = 0;
    int activeChancePercent_ = 0;
    qint64 roundDeadlineMs_ = 0;
    bool enabled_ = true;
    bool started_ = false;
};

} // namespace mub::core
