# 启动与组装层。单实例直接编译产品源码，不复制一份实现。
mub_add_test(tst_singleinstance
    tst_singleinstance.cpp
    "${CMAKE_SOURCE_DIR}/src/app/SingleInstance.cpp"
)
target_include_directories(tst_singleinstance PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_singleinstance PRIVATE
    MUB::Core
    # 唯一实例锁是平台窄接口的一部分（Windows 命名互斥量）。
    MUB::Platform
    Qt6::Network
)

# 应用菜单入口是纯文件逻辑，全部在临时目录里测试，不碰真实的 XDG 数据目录。
mub_add_test(tst_desktopentry
    tst_desktopentry.cpp
    "${CMAKE_SOURCE_DIR}/src/app/DesktopEntry.cpp"
)
target_include_directories(tst_desktopentry PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_desktopentry PRIVATE MUB::Core Qt6::Gui)

mub_add_test(tst_desktopenvironment
    tst_desktopenvironment.cpp
    "${CMAKE_SOURCE_DIR}/src/app/DesktopEnvironment.cpp"
)
target_include_directories(tst_desktopenvironment PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_desktopenvironment PRIVATE Qt6::Core)

mub_add_test(tst_rotatinglogwriter
    tst_rotatinglogwriter.cpp
    "${CMAKE_SOURCE_DIR}/src/app/DiagnosticPrivacy.cpp"
    "${CMAKE_SOURCE_DIR}/src/app/RotatingLogWriter.cpp"
)
target_include_directories(tst_rotatinglogwriter PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_rotatinglogwriter PRIVATE Qt6::Core)

mub_add_test(tst_selftest
    tst_selftest.cpp
    "${CMAKE_SOURCE_DIR}/src/app/SelfTest.cpp"
)
target_include_directories(tst_selftest PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_selftest PRIVATE
    MUB::Core
    MUB::Resources
    Qt6::Widgets
)

# 生命周期是纯应用层策略：辅助窗口所有权与唯一退出路径。
# 需要 QApplication，因此在 offscreen 下运行。
mub_add_test(tst_applifecycle
    tst_applifecycle.cpp
    "${CMAKE_SOURCE_DIR}/src/app/AppLifecycle.cpp"
)
target_include_directories(tst_applifecycle PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_applifecycle PRIVATE Qt6::Widgets)

# 设置控制器：唯一运行时设置、领域通知，以及「接收」与「落盘」的分离。
mub_add_test(tst_settingscontroller
    tst_settingscontroller.cpp
    "${CMAKE_SOURCE_DIR}/src/app/SettingsController.cpp"
)
target_include_directories(tst_settingscontroller PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_settingscontroller PRIVATE MUB::Core)

# 设置的端到端装配：界面、控制器、存储与生命周期接在一起。
mub_add_test(tst_settingsintegration
    tst_settingsintegration.cpp
    "${CMAKE_SOURCE_DIR}/src/app/AppLifecycle.cpp"
    "${CMAKE_SOURCE_DIR}/src/app/SettingsController.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/SettingsWindow.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/ValueEditor.cpp"
)
target_include_directories(tst_settingsintegration PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_settingsintegration PRIVATE MUB::Core Qt6::Widgets)
