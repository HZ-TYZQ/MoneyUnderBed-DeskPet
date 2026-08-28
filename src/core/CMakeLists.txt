# 纯逻辑库：可以在没有桌面会话的环境下构建和测试。
# 允许链接 Qt::Core 与 Qt::Gui。QImage、QRegion 等类型不需要 QGuiApplication 实例，
# 因此纯逻辑测试仍可无头运行。
# 加入 Qt::Widgets、Win32、XCB 或 D-Bus 都属于破坏该边界。

configure_file(Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/mub/Version.h" @ONLY)

add_library(money-under-bed-core STATIC
    AppMetadata.cpp
    AppMetadata.h
    AssetPaths.cpp
    AssetPaths.h
    ActivityMode.h
    AutonomousBehavior.cpp
    AutonomousBehavior.h
    ClickFeedback.cpp
    ClickFeedback.h
    EventCoordinator.cpp
    EventCoordinator.h
    Feeding.cpp
    Feeding.h
    GestureRecognizer.cpp
    GestureRecognizer.h
    RandomSource.cpp
    RandomSource.h
    ScreenPlacement.cpp
    ScreenPlacement.h
    SessionSuspension.cpp
    SessionSuspension.h
    Settings.cpp
    Settings.h
    SettingsPresets.cpp
    SettingsPresets.h
    SettingsStore.cpp
    SettingsStore.h
    TimeSource.cpp
    TimeSource.h
    "${CMAKE_CURRENT_BINARY_DIR}/generated/mub/Version.h"
)

# 角色 PNG 不编入 GPL 可执行文件，而是作为独立数据文件放在 bin/assets。
# 任何链接 core 的产品或测试目标都会先得到同一份运行时目录结构。
add_dependencies(money-under-bed-core money-under-bed-assets)

# character/ 的内容编入 core，不单独成库。
target_sources(money-under-bed-core PRIVATE
    ../character/AnimationClip.cpp
    ../character/AnimationClip.h
    ../character/AnimationPlayer.cpp
    ../character/AnimationPlayer.h
    ../character/Direction.cpp
    ../character/Direction.h
    ../character/HitMask.cpp
    ../character/HitMask.h
    ../character/SpriteSheet.cpp
    ../character/SpriteSheet.h
)

# dialogue/ 的内容同样编入 core，不单独成库。
target_sources(money-under-bed-core PRIVATE
    ../dialogue/DialogueData.cpp
    ../dialogue/DialogueData.h
    ../dialogue/DialogueSession.cpp
    ../dialogue/DialogueSession.h
    ../dialogue/LineSelection.cpp
    ../dialogue/LineSelection.h
)
add_library(MUB::Core ALIAS money-under-bed-core)

target_include_directories(money-under-bed-core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(money-under-bed-core
    PUBLIC Qt6::Core Qt6::Gui
    PRIVATE MUB::Warnings
)

# 该基线之前被弃用的 Qt API 直接编译失败，避免无意中依赖旧接口。
target_compile_definitions(money-under-bed-core PUBLIC QT_DISABLE_DEPRECATED_UP_TO=0x061100)
