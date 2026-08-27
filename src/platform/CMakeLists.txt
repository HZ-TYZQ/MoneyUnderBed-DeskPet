# 窄平台接口与各平台实现。
#
# 这是产品代码中唯一允许出现按操作系统分支的地方
# （docs/Decisions.md 第 8.4 节）。各平台实现分文件，由 CMake 按平台取舍，
# 而不是在一个文件里堆条件编译。

add_library(money-under-bed-platform STATIC
    BackendFactory.cpp
    BackendFactory.h
    DeskPetWindowBackend.cpp
    DeskPetWindowBackend.h
    QtWindowBackend.cpp
    QtWindowBackend.h
    StartupProbe.h
)
add_library(MUB::Platform ALIAS money-under-bed-platform)

target_include_directories(money-under-bed-platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(money-under-bed-platform
    PUBLIC Qt6::Core Qt6::Gui
    PRIVATE MUB::Warnings
)

target_compile_definitions(money-under-bed-platform PUBLIC QT_DISABLE_DEPRECATED_UP_TO=0x061100)

# 启动探测按平台取舍。Linux 需要在构造 QApplication 之前用原生 xcb_connect
# 探测连接，因此必须直接链接 libxcb（docs/Decisions.md 第 8.2 节）。
if(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
    target_sources(money-under-bed-platform PRIVATE
        LinuxStartupProbe.cpp
        LinuxWindowBackend.cpp
        LinuxWindowBackend.h
    )
    target_link_libraries(money-under-bed-platform PRIVATE PkgConfig::XCB)
else()
    target_sources(money-under-bed-platform PRIVATE GenericStartupProbe.cpp)
endif()

if(WIN32)
    target_sources(money-under-bed-platform PRIVATE
        WindowsWindowBackend.cpp
        WindowsWindowBackend.h
    )
    target_link_libraries(money-under-bed-platform PRIVATE user32)
    # 减少 windows.h 引入的宏污染，避免与 Qt 的名字冲突。
    target_compile_definitions(money-under-bed-platform PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
endif()
