#pragma once

#include "core/Settings.h"

#include <optional>
#include <span>

namespace mub::core {

// 普通层的命名档位，以及档位与实际参数之间的双向映射。
//
// docs/Decisions.md 第 14.2 节：由档位驱动的设置，**实际参数是唯一真相来源**，
// 配置文件不另存「当前预设」。选择档位时把该档对应的参数写入实际参数；界面按
// 实际参数反向匹配，完全匹配时显示该档位，否则显示「自定义」。因此这里只有纯
// 函数，没有任何状态，也不出现在 `Settings` 里。
//
// **本文件是低/中/高具体取值的唯一存放处**（计划第 6.1 节）。这些取值已由人工
// 检查点 A 实测冻结，并写回 `docs/Decisions.md` 第 14.3 至 14.5 节，冻结来源见
// 该文件第 14.11 节。两处不一致时以决策文档为准。改动任一数字都要同时改决策
// 文档；不要把这些数字复制到第三处。

enum class ActivityTempo
{
    Low,
    Normal,
    High,
};

enum class MovementSpeed
{
    Slow,
    Normal,
    Fast,
};

enum class CursorAffinity
{
    Off,
    Occasional,
    Frequent,
};

enum class SpeechFrequency
{
    Off,
    Low,
    Normal,
    High,
};

enum class ClickTextFrequency
{
    Low,
    Normal,
    High,
};

enum class TypingSpeed
{
    Slow,
    Normal,
    Fast,
};

enum class AnimationSpeed
{
    Slow,
    Normal,
    Fast,
};

// 全部档位，按界面顺序。
std::span<const ActivityTempo> activityTempos();
std::span<const MovementSpeed> movementSpeeds();
std::span<const CursorAffinity> cursorAffinities();
std::span<const SpeechFrequency> speechFrequencies();
std::span<const ClickTextFrequency> clickTextFrequencies();
std::span<const TypingSpeed> typingSpeeds();
std::span<const AnimationSpeed> animationSpeeds();

// 正向：把档位对应的参数写入实际参数。只写该档位驱动的字段，不碰其他字段。
void applyActivityTempo(BehaviorSettings &behavior, ActivityTempo tempo);
void applyMovementSpeed(BehaviorSettings &behavior, MovementSpeed speed);
void applyCursorAffinity(BehaviorSettings &behavior, CursorAffinity affinity);
void applySpeechFrequency(DialogueSettings &dialogue, SpeechFrequency frequency);
void applyClickTextFrequency(DialogueSettings &dialogue, ClickTextFrequency frequency);
void applyTypingSpeed(DialogueSettings &dialogue, TypingSpeed speed);
void applyAnimationSpeed(AppearanceSettings &appearance, AnimationSpeed speed);

// 反向：整组参数完全相等才算命中，否则返回空——界面据此显示「自定义」。
std::optional<ActivityTempo> matchActivityTempo(const BehaviorSettings &behavior);
std::optional<MovementSpeed> matchMovementSpeed(const BehaviorSettings &behavior);
std::optional<CursorAffinity> matchCursorAffinity(const BehaviorSettings &behavior);
std::optional<SpeechFrequency> matchSpeechFrequency(const DialogueSettings &dialogue);
std::optional<ClickTextFrequency> matchClickTextFrequency(const DialogueSettings &dialogue);
std::optional<TypingSpeed> matchTypingSpeed(const DialogueSettings &dialogue);
std::optional<AnimationSpeed> matchAnimationSpeed(const AppearanceSettings &appearance);

} // namespace mub::core
