mub_add_test(tst_appmetadata tst_appmetadata.cpp)
target_link_libraries(tst_appmetadata PRIVATE MUB::Core)

mub_add_test(tst_screenplacement tst_screenplacement.cpp)
target_link_libraries(tst_screenplacement PRIVATE MUB::Core)

mub_add_test(tst_gesture tst_gesture.cpp)
target_link_libraries(tst_gesture PRIVATE MUB::Core)
