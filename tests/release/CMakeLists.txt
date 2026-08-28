# 发布接线的断言：标签触发、已退役工作流和当前清单的一致性。
# 只读仓库内的文本文件，不需要 Qt 事件循环，也不接触桌面 API。
mub_add_test(tst_releaseworkflows tst_releaseworkflows.cpp)
