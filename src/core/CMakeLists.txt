# 纯逻辑库：可以在没有桌面会话的环境下构建和测试。
# 只允许链接 Qt::Core。加入 Gui、Widgets、DBus 或任何平台 API 都属于破坏该边界。

configure_file(Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/mub/Version.h" @ONLY)

add_library(money-under-bed-core STATIC
    AppMetadata.cpp
    AppMetadata.h
    "${CMAKE_CURRENT_BINARY_DIR}/generated/mub/Version.h"
)
add_library(MUB::Core ALIAS money-under-bed-core)

target_include_directories(money-under-bed-core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(money-under-bed-core
    PUBLIC Qt6::Core
    PRIVATE MUB::Warnings
)

# 该基线之前被弃用的 Qt API 直接编译失败，避免无意中依赖旧接口。
target_compile_definitions(money-under-bed-core PUBLIC QT_DISABLE_DEPRECATED_UP_TO=0x061100)
