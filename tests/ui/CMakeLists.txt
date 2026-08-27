# ui/ 的源文件直接编入可执行目标，不单独成库，因此测试也直接编译同一份源码。

# 布局与冻结参数。不需要真实窗口，但需要字体资源来折行。
mub_add_test(tst_bubblelayout
    tst_bubblelayout.cpp
    "${CMAKE_SOURCE_DIR}/src/ui/BubbleRenderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/DialogueFont.cpp"
)
target_include_directories(tst_bubblelayout PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_bubblelayout PRIVATE
    MUB::Core
    MUB::Resources
    Qt6::Widgets
)

# 对话系统与事件协调器的集成。用假后端，不接触真实桌面 API。
mub_add_test(tst_dialoguecontroller
    tst_dialoguecontroller.cpp
    "${CMAKE_SOURCE_DIR}/src/ui/BubbleHost.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/BubbleRenderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/BubbleWindow.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/CharacterPresenter.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/CharacterWindow.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/DialogueController.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/DialogueFont.cpp"
)
target_include_directories(tst_dialoguecontroller PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests/platform"
)
target_link_libraries(tst_dialoguecontroller PRIVATE
    MUB::Core
    MUB::Platform
    MUB::Resources
    Qt6::Widgets
)
