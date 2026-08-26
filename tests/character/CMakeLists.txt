mub_add_test(tst_spritesheet tst_spritesheet.cpp)
target_link_libraries(tst_spritesheet PRIVATE MUB::Core MUB::Resources)

mub_add_test(tst_hitmask tst_hitmask.cpp)
target_link_libraries(tst_hitmask PRIVATE MUB::Core MUB::Resources)

mub_add_test(tst_direction tst_direction.cpp)
target_link_libraries(tst_direction PRIVATE MUB::Core)

mub_add_test(tst_animationplayer tst_animationplayer.cpp)
target_link_libraries(tst_animationplayer PRIVATE MUB::Core)
