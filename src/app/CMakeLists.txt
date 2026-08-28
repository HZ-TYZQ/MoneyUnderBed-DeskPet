# QApplication 前后的启动、组装与命令行入口。
qt_add_executable(money-under-bed-deskpet
    AppLifecycle.cpp
    AppLifecycle.h
    DesktopEntry.cpp
    DesktopEntry.h
    DesktopEnvironment.cpp
    DesktopEnvironment.h
    DiagnosticLog.cpp
    DiagnosticLog.h
    DiagnosticPrivacy.cpp
    DiagnosticPrivacy.h
    RotatingLogWriter.cpp
    RotatingLogWriter.h
    SelfTest.cpp
    SelfTest.h
    SettingsController.cpp
    SettingsController.h
    SingleInstance.cpp
    SingleInstance.h
    StartupFailureReport.cpp
    StartupFailureReport.h
    main.cpp
)

target_include_directories(money-under-bed-deskpet PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(money-under-bed-deskpet PRIVATE
    MUB::Core
    MUB::Platform
    MUB::Resources
    MUB::Warnings
    # 单实例与唤回用 Qt 的本地套接字（Linux 是 Unix socket，Windows 是命名管道）。
    Qt6::Network
    Qt6::Widgets
)

set_target_properties(money-under-bed-deskpet PROPERTIES
    OUTPUT_NAME "money-under-bed-deskpet"
    # Windows 使用 GUI 子系统，不弹控制台窗口；诊断信息写本地日志。
    # 见 docs/Decisions.md 第 8.3 节。
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)
