# QApplication 前后的启动、组装与命令行入口。
qt_add_executable(money-under-bed-deskpet
    main.cpp
)

target_link_libraries(money-under-bed-deskpet PRIVATE
    MUB::Core
    MUB::Resources
    MUB::Warnings
    Qt6::Widgets
)

set_target_properties(money-under-bed-deskpet PROPERTIES
    OUTPUT_NAME "money-under-bed-deskpet"
    # Windows 使用 GUI 子系统，不弹控制台窗口；诊断信息写本地日志。
    # 见 docs/Decisions.md 第 8.3 节。
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)
