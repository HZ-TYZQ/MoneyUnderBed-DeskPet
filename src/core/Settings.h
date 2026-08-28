#pragma once

#include "core/ActivityMode.h"

#include <QMetaType>
#include <QString>
#include <QStringView>

#include <span>

namespace mub::core {

// 合法输入区间。校验与设置界面共用同一份定义，避免两处各写一遍。
struct IntRange
{
    int min = 0;
    int max = 0;

    constexpr bool contains(const int value) const
    {
        return value >= min && value <= max;
    }

    friend bool operator==(const IntRange &, const IntRange &) = default;
};

// 用户设置，按 docs/Decisions.md 第 14.2 节分成四个领域。
//
// **本结构仍然只保存用户设置。** 第 2.3 节：不保存角色位置、互动记录、最近行为
// 或长期记忆；暂停与隐藏不保存。第 14.9 节重申，设置项变多不是把运行时状态写进
// 配置的理由——下面新增的全部是用户可调参数，没有一项是角色状态。
//
// 第 14.2 节还规定：由档位驱动的设置，**实际参数是唯一真相来源**，配置文件不另存
// 一份「当前预设」。因此这里没有任何档位字段，档位由 `SettingsPresets.h` 的纯函数
// 正向写入、反向匹配。

// 行为参数（第 14.3 节）。
struct BehaviorSettings
{
    // 第 2.2 节：首次启动默认安静模式。
    ActivityMode mode = ActivityMode::Quiet;

    // 「活动节奏」档位驱动的七个参数。
    int idleMinMs = 2000;
    int idleMaxMs = 6000;
    int walkMinMs = 1500;
    int walkMaxMs = 4000;
    int restMinMs = 4000;
    int restMaxMs = 12000;
    int restChancePercent = 25;

    // 「接近鼠标」档位驱动。仅活跃模式生效。
    int approachCursorChancePercent = 12;

    // 「移动速度」档位驱动。
    int walkSpeedPxPerSec = 48;
    int returnSpeedPxPerSec = 90;

    int returnDelayMs = 1500;
    int cursorSafeDistancePx = 60;

    friend bool operator==(const BehaviorSettings &, const BehaviorSettings &) = default;
};

// 对话参数（第 14.4 节）。
struct DialogueSettings
{
    // 「说话频率」档位驱动的两个参数。第 14.4 节：「关闭」档由触发概率 `0%` 表达，
    // 不另存启用标志；概率为 `0` 时间隔值不生效。
    //
    // 两者都是 `1.1.0` 新增参数，`1.0.0` 候选里不存在——那一版的自主闲聊没有独立
    // 调度，气泡频率只区分「关闭」与「非关闭」。当前取值是原型值，待检查点 A 冻结。
    int chatterMinIntervalMs = 120000;
    int chatterChancePercent = 30;

    // 单击附带台词的唯一概率。第 14.4 节：不再按说话频率二选一。
    int clickTextChancePercent = 20;

    int singlePageAutoHideMs = 4000;

    // 第 4.1 节的 `28 ms` 由 2026-08-27 的 Qt 原型审核确定，第 14.4 节将其解冻为
    // 设置项，该值继续作为默认值。
    int typingMsPerChar = 28;

    friend bool operator==(const DialogueSettings &, const DialogueSettings &) = default;
};

// 外观参数（第 14.5 节）。
struct AppearanceSettings
{
    // 第 5.1 节：整数倍率，配最近邻采样。第 14.5 节明确不提供滑块、小数倍率
    // 或连续缩放。
    int scale = 2;

    // 「动画速度」档位对三个帧时长施加统一倍率。
    int idleFrameMs = 100;
    int runFrameMs = 80;
    int icecreamFrameMs = 100;

    friend bool operator==(const AppearanceSettings &, const AppearanceSettings &) = default;
};

// 窗口与桌面（第 14.6 节）。
struct WindowSettings
{
    // 第 3.4 节：默认始终置顶。
    bool alwaysOnTop = true;

    friend bool operator==(const WindowSettings &, const WindowSettings &) = default;
};

// 设置的四个领域。界面按它分区，控制器按它做「恢复本组默认值」。
enum class SettingsGroup
{
    Behavior,
    Dialogue,
    Appearance,
    Window,
};

struct Settings
{
    BehaviorSettings behavior;
    DialogueSettings dialogue;
    AppearanceSettings appearance;
    WindowSettings window;

    friend bool operator==(const Settings &, const Settings &) = default;
};

// 各参数的合法输入区间（第 14.3 至 14.5 节的「建议输入范围」）。
namespace ranges {

inline constexpr IntRange kIdleMs{500, 30000};
inline constexpr IntRange kWalkMs{500, 30000};
inline constexpr IntRange kRestMs{500, 60000};
inline constexpr IntRange kPercent{0, 100};
inline constexpr IntRange kSpeedPxPerSec{8, 400};
inline constexpr IntRange kReturnDelayMs{0, 30000};
inline constexpr IntRange kCursorSafeDistancePx{0, 400};
// 第 14.4 节：闲聊间隔下限必须严格大于 0。
inline constexpr IntRange kChatterIntervalMs{10000, 1800000};
inline constexpr IntRange kSinglePageAutoHideMs{1000, 60000};
inline constexpr IntRange kTypingMsPerChar{5, 200};
inline constexpr IntRange kFrameMs{20, 500};

} // namespace ranges

// 允许的整数倍率档位。
std::span<const int> allowedScales();
bool isAllowedScale(int scale);

// 配置文件里使用的稳定标识。
//
// 存字符串而不是枚举序号：枚举顺序将来若变化，序号会静默错位成另一档设置。
QString activityModeId(ActivityMode mode);
ActivityMode activityModeFromId(QStringView id, ActivityMode fallback);

// 把越界或损坏的取值拉回合法范围。
//
// 第 14.8 节：单值越界回到对应默认值；三组最小/最大时长成对校验，
// 关系非法时**整对**回到默认值——只回落其中一个会留下另一个用户没选过的组合。
// 配置文件是用户可编辑的普通文件，也可能被上一版程序写坏，任何非法值都不中断启动。
Settings sanitized(Settings settings);

} // namespace mub::core

// 设置要经由信号传递，必须能放进 QVariant。
Q_DECLARE_METATYPE(mub::core::Settings)
