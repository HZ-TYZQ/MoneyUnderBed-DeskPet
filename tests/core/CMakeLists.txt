mub_add_test(tst_appmetadata tst_appmetadata.cpp)
target_link_libraries(tst_appmetadata PRIVATE MUB::Core)

mub_add_test(tst_screenplacement tst_screenplacement.cpp)
target_link_libraries(tst_screenplacement PRIVATE MUB::Core)

mub_add_test(tst_gesture tst_gesture.cpp)
target_link_libraries(tst_gesture PRIVATE MUB::Core)

mub_add_test(tst_autonomousbehavior tst_autonomousbehavior.cpp)
target_link_libraries(tst_autonomousbehavior PRIVATE MUB::Core)

mub_add_test(tst_eventcoordinator tst_eventcoordinator.cpp)
target_link_libraries(tst_eventcoordinator PRIVATE MUB::Core)

mub_add_test(tst_feedback tst_feedback.cpp)
target_link_libraries(tst_feedback PRIVATE MUB::Core)

mub_add_test(tst_settings tst_settings.cpp)
target_link_libraries(tst_settings PRIVATE MUB::Core)

# 档位映射是纯函数，不需要 Qt 事件循环。
mub_add_test(tst_settingspresets tst_settingspresets.cpp)
target_link_libraries(tst_settingspresets PRIVATE MUB::Core)

mub_add_test(tst_sessionsuspension tst_sessionsuspension.cpp)
target_link_libraries(tst_sessionsuspension PRIVATE MUB::Core)
