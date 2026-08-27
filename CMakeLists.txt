cmake_minimum_required(VERSION 3.21)

project(MoneyUnderBedDeskPet
    VERSION 0.0.0
    DESCRIPTION "Unofficial desktop pet for the game MoneyUnderBed"
    HOMEPAGE_URL "https://github.com/HZ-TYZQ/MoneyUnderBed-DeskPet"
    LANGUAGES CXX)

# 版本编号方案尚未冻结，见 docs/Decisions.md 第 13 节。
# 在冻结前使用 0.0.0，并附加构建标识用于日志和诊断。
set(PROJECT_VERSION_SUFFIX "dev" CACHE STRING "Version suffix for pre-release builds")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# 不启用 AUTOUIC：第一版界面由代码构建，不使用 .ui 文件。
set(CMAKE_AUTOUIC OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# API 基线为 Qt 6.11（docs/Decisions.md 第 9.1 节）。
# CI 另行断言实际版本为 6.11.2；此处不写 EXACT，否则本地 6.11.1 无法配置。
set(MUB_QT_API_BASELINE 6.11)
find_package(Qt6 ${MUB_QT_API_BASELINE} REQUIRED COMPONENTS Core Gui Widgets Network)
if(LINUX)
    find_package(Qt6 ${MUB_QT_API_BASELINE} REQUIRED COMPONENTS DBus)
endif()

qt_standard_project_setup(REQUIRES ${MUB_QT_API_BASELINE})

include(ProjectWarnings)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 ${MUB_QT_API_BASELINE} REQUIRED COMPONENTS Test)
endif()

add_subdirectory(resources)
add_subdirectory(src)

# 审核用原型。不是产品代码，默认只在开发构建中启用。
# 参数冻结并写回 docs/Decisions.md 后可以整个删除。
option(MUB_BUILD_PROTOTYPES "Build the review prototypes under probes/" ON)
if(MUB_BUILD_PROTOTYPES)
    add_subdirectory(probes/bubble)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

message(STATUS "MoneyUnderBed DeskPet ${PROJECT_VERSION}-${PROJECT_VERSION_SUFFIX}")
message(STATUS "  Qt              : ${Qt6_VERSION} (API baseline ${MUB_QT_API_BASELINE})")
message(STATUS "  C++ standard    : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Testing         : ${BUILD_TESTING}")
