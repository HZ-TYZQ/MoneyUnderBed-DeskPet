# 开发期严格警告配置。
# docs/Plans/DevelopmentPlan.md 第 2.4 节要求编译警告按错误处理。
# GCC／Clang 与 MSVC 分别设置等价警告，不追求逐条一一对应，
# 而是覆盖同一类问题：隐式转换、未使用实体、遮蔽、未初始化、类型不匹配。

add_library(mub_warnings INTERFACE)
add_library(MUB::Warnings ALIAS mub_warnings)

option(MUB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mub_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(mub_warnings INTERFACE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()
    if(MUB_WARNINGS_AS_ERRORS)
        target_compile_options(mub_warnings INTERFACE -Werror)
    endif()
elseif(MSVC)
    target_compile_options(mub_warnings INTERFACE
        /W4
        /permissive-
        /utf-8
        /w14242  # 转换可能丢失数据
        /w14254  # 位域宽度转换丢失数据
        /w14263  # 成员函数没有覆盖任何基类虚函数
        /w14265  # 有虚函数但析构函数非虚
        /w14287  # 无符号／负常量不匹配
        /w14296  # 表达式恒为真或恒为假
        /w14545  # 逗号前的表达式没有效果
        /w14546  # 函数调用缺少参数列表
        /w14547  # 逗号前的运算符没有效果
        /w14549  # 运算符前的操作数没有效果
        /w14555  # 表达式没有效果
        /w14619  # 不存在的 pragma warning 编号
        /w14640  # 局部静态对象的线程安全构造
        /w14826  # 有符号／无符号扩展不一致
        /w14905  # 宽字符串字面量强制转换为 LPSTR
        /w14906  # 字符串字面量强制转换为 LPWSTR
        /w14928  # 非法的拷贝初始化
    )
    if(MUB_WARNINGS_AS_ERRORS)
        target_compile_options(mub_warnings INTERFACE /WX)
    endif()
endif()
