# 测试目录结构与生产职责对应。
# 纯逻辑测试不得调用桌面 API；需要 QGuiApplication 的测试在 CI 中以
# offscreen 平台运行，offscreen 只承担自动测试，不冒充真实桌面验收。

# 仓库根路径，供需要核对源文件的测试使用。
set(MUB_SOURCE_ROOT "${CMAKE_SOURCE_DIR}")

function(mub_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE MUB::Warnings Qt6::Test)
    target_compile_definitions(${name} PRIVATE
        MUB_SOURCE_ROOT="${MUB_SOURCE_ROOT}")
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE OFF)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_subdirectory(character)
add_subdirectory(core)
add_subdirectory(dialogue)
add_subdirectory(platform)
add_subdirectory(resources)
