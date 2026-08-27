# platform

Linux XCB 与 Windows 的窄平台接口：置顶、输入命中与穿透、焦点策略、
任务栏与窗口列表表现，以及后续阶段的会话事件。

内容在阶段 3 加入，届时单独成目标以隔离平台实现。

边界：平台相关能力集中在这里，不把条件编译散落到动画、行为和角色逻辑中
（`docs/Decisions.md` 第 8.4 节）。

Windows 平台层的具体实现取决于阶段 1 的探针结果，见
`docs/WindowsFeasibilityResults.md`；结果出来前不实现。
