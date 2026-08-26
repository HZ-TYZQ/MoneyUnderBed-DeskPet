# character

动画定义、精灵表、移动与方向映射。

内容在阶段 4 加入，编入 `money-under-bed-core`。

边界：这里只有平台无关逻辑，不得引入 Qt Widgets、Win32、XCB 或 D-Bus。
运行时使用显式动画映射，不根据原始文件名推断方向（`docs/Decisions.md` 第 7 节）。
