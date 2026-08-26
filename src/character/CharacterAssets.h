#pragma once

#include <QString>

#include <span>

namespace mub::character {

// 精灵表的显式登记表。
//
// docs/Decisions.md 第 7 节与计划第 9.1 节要求运行时使用显式映射，
// 不根据原始文件名推断语义。因此这里的 `id` 是逻辑标识，
// 与资源路径分开维护；改名素材必须同时改这张表和素材清单。
//
// 阶段 3 只用到帧数校验。播放语义（循环方式、帧率、朝向映射）在阶段 4 加入。
struct SpriteSheetEntry
{
    const char *id;
    const char *resourcePath;
    int expectedFrameCount;
};

std::span<const SpriteSheetEntry> registeredSpriteSheets();

// 按逻辑标识取资源路径。找不到时返回空串。
QString spriteSheetPath(const QString &id);

} // namespace mub::character
