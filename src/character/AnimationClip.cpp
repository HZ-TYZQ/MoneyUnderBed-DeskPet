#include "character/AnimationClip.h"

#include "core/AssetPaths.h"

#include <QLatin1String>

#include <array>

namespace mub::character {

namespace {

// 帧数来自 assets/MANIFEST.md，并由 tests/ 对实际素材独立校验。
// 帧时长是待调优的内部参数，当前取值来自可行性探针的观察，尚未定稿。
constexpr int kIdleFrameMs = 100;
constexpr int kRunFrameMs = 80;
constexpr int kIcecreamFrameMs = 100;

constexpr std::array<AnimationClip, 11> kClips{{
    {"idle-up-left", "idle-up-left.png", 9, kIdleFrameMs, LoopMode::Loop},
    {"idle-down-left", "idle-down-left.png", 9, kIdleFrameMs, LoopMode::Loop},
    {"idle-up-right", "idle-up-right.png", 9, kIdleFrameMs, LoopMode::Loop},
    {"idle-down-right", "idle-down-right.png", 9, kIdleFrameMs, LoopMode::Loop},
    {"run-up-left", "run-up-left.png", 8, kRunFrameMs, LoopMode::Loop},
    {"run-down-left", "run-down-left.png", 8, kRunFrameMs, LoopMode::Loop},
    {"run-up-right", "run-up-right.png", 8, kRunFrameMs, LoopMode::Loop},
    {"run-down-right", "run-down-right.png", 8, kRunFrameMs, LoopMode::Loop},
    // 投喂相关动画是一次性事件，播完停在最后一帧，由阶段 5 的事件协调器接管。
    {"icecream-drop", "icecream-drop.png", 17, kIcecreamFrameMs, LoopMode::HoldLast},
    {"icecream-drop-still", "icecream-drop-still.png", 1, kIcecreamFrameMs, LoopMode::HoldLast},
    {"icecream-eat", "icecream-eat.png", 20, kIcecreamFrameMs, LoopMode::HoldLast},
}};

} // namespace

std::span<const AnimationClip> registeredClips()
{
    return {kClips.data(), kClips.size()};
}

const AnimationClip *findClip(const QStringView id)
{
    for (const AnimationClip &clip : kClips) {
        if (id == QLatin1String(clip.id)) {
            return &clip;
        }
    }
    return nullptr;
}

QString clipAssetPath(const QStringView id)
{
    const AnimationClip *clip = findClip(id);
    if (clip == nullptr) {
        return {};
    }
    return core::assetFilePath(
        QStringLiteral("character/") + QString::fromLatin1(clip->fileName));
}

} // namespace mub::character
