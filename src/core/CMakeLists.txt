# 纯逻辑库：可以在没有桌面会话的环境下构建和测试。
# 允许链接 Qt::Core 与 Qt::Gui。QImage、QRegion 等类型不需要 QGuiApplication 实例，
# 因此纯逻辑测试仍可无头运行。
# 加入 Qt::Widgets、Win32、XCB 或 D-Bus 都属于破坏该边界。

configure_file(Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/mub/Version.h" @ONLY)

add_library(money-under-bed-core STATIC
    AppMetadata.cpp
    AppMetadata.h
    ActivityMode.h
    AutonomousBehavior.cpp
    AutonomousBehavior.h
    BubbleFrequency.h
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
    Settings.cpp
    Settings.h
    SettingsStore.cpp
    SettingsStore.h
    TimeSource.cpp
    TimeSource.h
    "${CMAKE_CURRENT_BINARY_DIR}/generated/mub/Version.h"
)

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
