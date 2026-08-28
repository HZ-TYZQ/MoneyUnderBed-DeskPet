#pragma once

#include <QString>
#include <QStringView>

#include <span>

namespace mub::character {

enum class LoopMode
{
    Loop,      // 循环播放
    HoldLast,  // 播完停在最后一帧
};

// 帧时长归属的类别。
//
// docs/Decisions.md 第 14.5 节把三个帧时长开放为设置，「动画速度」档位对三者
// 施加统一倍率。类别在登记表里**显式写出**，不从素材文件名或标识前缀推断——
// 这与本文件既有的「运行时使用显式映射」约束一致。
enum class AnimationCategory
{
    Idle,
    Run,
    Icecream,
};

// 三个帧时长的当前取值。默认值即第 14.5 节的「正常」档。
struct AnimationTiming
{
    int idleFrameMs = 100;
    int runFrameMs = 80;
    int icecreamFrameMs = 100;

    friend bool operator==(const AnimationTiming &, const AnimationTiming &) = default;
};

// 一段动画的显式登记项。
//
// docs/Decisions.md 第 7 节与计划第 9.1 节要求运行时使用显式映射，
// 不根据原始文件名推断语义。`id` 是逻辑标识，与素材文件名分开维护；
// 改名素材必须同时改这张表和 assets/MANIFEST.md。
struct AnimationClip
{
    const char *id;
    const char *fileName;
    int frameCount;
    // 帧时长不再写死在这里：它由 `AnimationTiming` 提供，按类别取值。
    AnimationCategory category;
    LoopMode loop;
};

// 该片段在给定 timing 下的每帧毫秒数。
int frameDurationFor(const AnimationClip &clip, const AnimationTiming &timing);

std::span<const AnimationClip> registeredClips();

// 按逻辑标识查找。找不到时返回 nullptr。
const AnimationClip *findClip(QStringView id);

// 按逻辑标识取外部素材路径。找不到时返回空串。
QString clipAssetPath(QStringView id);

} // namespace mub::character
