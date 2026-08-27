# 角色窗口、气泡、菜单、设置、关于与首次提示。
# 这些源文件直接编入可执行目标，不单独成库。
target_sources(money-under-bed-deskpet PRIVATE
    AboutWindow.cpp
    AboutWindow.h
    BubbleHost.cpp
    BubbleHost.h
    BubbleMetrics.h
    BubbleRenderer.cpp
    BubbleRenderer.h
    BubbleWindow.cpp
    BubbleWindow.h
    CharacterPresenter.cpp
    CharacterPresenter.h
    CharacterWindow.cpp
    CharacterWindow.h
    DialogueController.cpp
    DialogueController.h
    DialogueFont.cpp
    DialogueFont.h
    SettingsWindow.cpp
    SettingsWindow.h
)

target_include_directories(money-under-bed-deskpet PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
