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
    // 每帧毫秒数。属于 docs/Decisions.md 第 2.1 节所说的内部参数，
    // 在原型阶段调优，第一版不在设置界面暴露。
    int frameDurationMs;
    LoopMode loop;
};

std::span<const AnimationClip> registeredClips();

// 按逻辑标识查找。找不到时返回 nullptr。
const AnimationClip *findClip(QStringView id);

// 按逻辑标识取外部素材路径。找不到时返回空串。
QString clipAssetPath(QStringView id);

} // namespace mub::character
