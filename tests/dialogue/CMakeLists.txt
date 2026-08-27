mub_add_test(tst_dialoguedata tst_dialoguedata.cpp)
target_link_libraries(tst_dialoguedata PRIVATE MUB::Core MUB::Resources Qt6::Gui)

mub_add_test(tst_dialoguesession tst_dialoguesession.cpp)
target_link_libraries(tst_dialoguesession PRIVATE MUB::Core MUB::Resources)
