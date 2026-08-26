mub_add_test(tst_spritesheet tst_spritesheet.cpp)
target_link_libraries(tst_spritesheet PRIVATE MUB::Core MUB::Resources)

mub_add_test(tst_hitmask tst_hitmask.cpp)
target_link_libraries(tst_hitmask PRIVATE MUB::Core MUB::Resources)
