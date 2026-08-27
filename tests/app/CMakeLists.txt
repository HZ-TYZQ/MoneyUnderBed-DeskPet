# 启动与组装层。单实例直接编译产品源码，不复制一份实现。
mub_add_test(tst_singleinstance
    tst_singleinstance.cpp
    "${CMAKE_SOURCE_DIR}/src/app/SingleInstance.cpp"
)
target_include_directories(tst_singleinstance PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(tst_singleinstance PRIVATE
    MUB::Core
    Qt6::Network
)
