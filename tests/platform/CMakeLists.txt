# CharacterWindow 编在可执行目标里，不属于任何库，因此测试直接编译同一份源文件。
# 这样既不必为测试单独拆库，也保证测的是产品实际使用的代码。
mub_add_test(tst_characterwindow
    tst_characterwindow.cpp
    "${CMAKE_SOURCE_DIR}/src/ui/CharacterWindow.cpp"
)
target_include_directories(tst_characterwindow PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(tst_characterwindow PRIVATE
    MUB::Core
    MUB::Platform
    MUB::Resources
    Qt6::Widgets
)

# Presenter 是产品组装层，测试直接编译同一份 Presenter 与 CharacterWindow 源码。
# 重点覆盖纯逻辑单测无法看到的事件申请、替换和完成边界。
mub_add_test(tst_characterpresenter
    tst_characterpresenter.cpp
    "${CMAKE_SOURCE_DIR}/src/ui/CharacterPresenter.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/CharacterWindow.cpp"
)
target_include_directories(tst_characterpresenter PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(tst_characterpresenter PRIVATE
    MUB::Core
    MUB::Platform
    MUB::Resources
    Qt6::Widgets
)
