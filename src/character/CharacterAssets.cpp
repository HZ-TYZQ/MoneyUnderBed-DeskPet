#include "character/CharacterAssets.h"

#include <array>

namespace mub::character {

namespace {

// 帧数来自 assets/MANIFEST.md，并由 tests/resources 对源文件独立校验。
constexpr std::array<SpriteSheetEntry, 11> kSheets{{
    {"idle-up-left", ":/assets/character/idle-up-left.png", 9},
    {"idle-down-left", ":/assets/character/idle-down-left.png", 9},
    {"idle-up-right", ":/assets/character/idle-up-right.png", 9},
    {"idle-down-right", ":/assets/character/idle-down-right.png", 9},
    {"run-up-left", ":/assets/character/run-up-left.png", 8},
    {"run-down-left", ":/assets/character/run-down-left.png", 8},
    {"run-up-right", ":/assets/character/run-up-right.png", 8},
    {"run-down-right", ":/assets/character/run-down-right.png", 8},
    {"icecream-drop", ":/assets/character/icecream-drop.png", 17},
    {"icecream-drop-still", ":/assets/character/icecream-drop-still.png", 1},
    {"icecream-eat", ":/assets/character/icecream-eat.png", 20},
}};

} // namespace

std::span<const SpriteSheetEntry> registeredSpriteSheets()
{
    return {kSheets.data(), kSheets.size()};
}

QString spriteSheetPath(const QString &id)
{
    for (const SpriteSheetEntry &entry : kSheets) {
        if (id == QLatin1String(entry.id)) {
            return QString::fromLatin1(entry.resourcePath);
        }
    }
    return {};
}

} // namespace mub::character
