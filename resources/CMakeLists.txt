# 项目资源。第一版只有对话字体；角色素材与界面素材在后续阶段加入。
#
# 字体是单一来源：仓库内只有 third_party/ark-pixel-font/ 一份 TTF，
# 编译进 Qt 资源系统后发行目录不再单独放一份。
# 见 docs/Decisions.md 第 4.7 节与第 12.2 节。

set(MUB_ARK_PIXEL_TTF
    "${CMAKE_SOURCE_DIR}/third_party/ark-pixel-font/ark-pixel-12px-proportional-zh_cn.ttf")

# 该哈希由 docs/Decisions.md 第 4.7 节冻结。
# 配置期就核对，使字体被静默替换或升级时立即失败，而不是等到运行时。
set(MUB_ARK_PIXEL_SHA256
    "37f5c3ad3fb4f2f278580a41e03ca67c085b64db2460d533d84c24a52911be96")

if(NOT EXISTS "${MUB_ARK_PIXEL_TTF}")
    message(FATAL_ERROR "Ark Pixel font not found: ${MUB_ARK_PIXEL_TTF}")
endif()

file(SHA256 "${MUB_ARK_PIXEL_TTF}" MUB_ARK_PIXEL_ACTUAL_SHA256)
if(NOT MUB_ARK_PIXEL_ACTUAL_SHA256 STREQUAL MUB_ARK_PIXEL_SHA256)
    message(FATAL_ERROR
        "Ark Pixel font hash mismatch.\n"
        "  file    : ${MUB_ARK_PIXEL_TTF}\n"
        "  expected: ${MUB_ARK_PIXEL_SHA256}\n"
        "  actual  : ${MUB_ARK_PIXEL_ACTUAL_SHA256}\n"
        "The font version is frozen by docs/Decisions.md section 4.7. "
        "Upgrading requires the dual-platform probe, automated tests and the "
        "full glyph-coverage check first.")
endif()

qt_add_library(money-under-bed-resources STATIC)
add_library(MUB::Resources ALIAS money-under-bed-resources)

qt_add_resources(money-under-bed-resources "mub_fonts"
    PREFIX "/fonts"
    BASE "${CMAKE_SOURCE_DIR}/third_party/ark-pixel-font"
    FILES "${MUB_ARK_PIXEL_TTF}"
)

target_link_libraries(money-under-bed-resources
    PUBLIC Qt6::Core
    PRIVATE MUB::Warnings
)

# 资源路径的唯一来源，供加载方和测试共用。
target_compile_definitions(money-under-bed-resources PUBLIC
    MUB_ARK_PIXEL_RESOURCE=":/fonts/ark-pixel-12px-proportional-zh_cn.ttf"
    MUB_ARK_PIXEL_SHA256="${MUB_ARK_PIXEL_SHA256}"
)
