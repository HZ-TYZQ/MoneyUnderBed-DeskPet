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
