cmake_minimum_required(VERSION 3.21)

# 独立工程。GitHub Actions 直接以本目录为源码根构建。
# 探针不链接、不包含任何正式产品代码，避免探针与产品实现互相遮蔽问题。
project(MoneyUnderBedWindowProbe LANGUAGES CXX)

# 该标准只属于探针，不决定正式工程的 C++ 标准（正式工程为 C++20）。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# Qt 版本必须与正式工程一致，否则探针结论不能代表产品。
find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets)
message(STATUS "Probe Qt version: ${Qt6_VERSION}")

add_executable(deskpet-probe
    main.cpp
    PlatformNative.cpp
    PlatformNative.h
    ProbeWindow.cpp
    ProbeWindow.h
)

target_link_libraries(deskpet-probe PRIVATE Qt6::Widgets)

# Windows 上刻意保留控制台子系统：探针的输出就是测试结果。
# 正式产品按 docs/Decisions.md 使用 GUI 子系统，两者不一致是有意的。
set_target_properties(deskpet-probe PROPERTIES WIN32_EXECUTABLE OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(deskpet-probe PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
elseif(MSVC)
    target_compile_options(deskpet-probe PRIVATE /W4 /permissive- /utf-8)
endif()

# 例外说明，范围仅限本探针目标：
# 探针是一次性可行性代码，本轮唯一目的是取得 Windows 实测结果。
# MSVC 的警告基线尚未在本项目验证过，此处不打开 /WX，
# 以免一条未知的编译器警告阻断项目所有者的测试轮次。
# 正式工程（阶段 2）按计划第 2.4 节把警告按错误处理，不沿用本例外。
option(PROBE_WARNINGS_AS_ERRORS "Treat probe warnings as errors" OFF)
if(PROBE_WARNINGS_AS_ERRORS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(deskpet-probe PRIVATE -Werror)
    elseif(MSVC)
        target_compile_options(deskpet-probe PRIVATE /WX)
    endif()
endif()

# 测试素材来自仓库内 assets/，不依赖被 Git 忽略的 Reference/。
set(PROBE_SPRITE "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/character/idle-down-left.png")
if(NOT EXISTS "${PROBE_SPRITE}")
    message(FATAL_ERROR "Probe sprite not found: ${PROBE_SPRITE}")
endif()

# 复制到可执行文件旁的 assets/character/，与打包后的布局一致，
# 使同一条默认查找路径在构建目录和分发 ZIP 中都成立。
add_custom_command(TARGET deskpet-probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:deskpet-probe>/assets/character"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROBE_SPRITE}"
            "$<TARGET_FILE_DIR:deskpet-probe>/assets/character/idle-down-left.png"
    VERBATIM)

include(GNUInstallDirs)
install(TARGETS deskpet-probe RUNTIME DESTINATION .)
install(FILES "${PROBE_SPRITE}" DESTINATION assets/character)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/WindowsChecklist.md"
    DESTINATION .)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/LICENSE.md"
    DESTINATION assets)
