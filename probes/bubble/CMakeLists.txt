# 气泡原型 A 的交互审核工具。
#
# 与 probes/window/ 不同，本原型必须使用**真实**的台词数据、表情素材和字体，
# 因此链接 MUB::Core 与 MUB::Resources，而不是自带一份副本。
# 复制一份台词只会引入漂移，正是本原型要避免的。
#
# 这不是产品代码。审核结束、参数写回 docs/Decisions.md 之后，
# 正式气泡按冻结值在 src/ui/ 中实现，本目录随即可以删除。

qt_add_executable(mub-bubble-prototype
    BubbleParameters.h
    BubbleRenderer.cpp
    BubbleRenderer.h
    ControlWindow.cpp
    ControlWindow.h
    Preview.cpp
    Preview.h
    main.cpp
)

target_include_directories(mub-bubble-prototype PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(mub-bubble-prototype PRIVATE
    MUB::Core
    MUB::Resources
    MUB::Warnings
    Qt6::Widgets
)

set_target_properties(mub-bubble-prototype PROPERTIES WIN32_EXECUTABLE OFF)
