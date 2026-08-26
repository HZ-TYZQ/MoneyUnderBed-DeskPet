# dialogue

台词数据、分页、表情映射与打字状态。

内容在阶段 6 加入，编入 `money-under-bed-core`。

边界：这里只有平台无关逻辑，不得引入 Qt Widgets、Win32、XCB 或 D-Bus。
分页和表情映射是内容数据的一部分，不在运行时按字数、标点或文件名自动生成
（`docs/Decisions.md` 第 4 节）。
