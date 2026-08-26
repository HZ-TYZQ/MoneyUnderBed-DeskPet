# 依赖方向（docs/Plans/DevelopmentPlan.md 第 4 节）：
#   app → ui → character/dialogue/core
#   app → platform
#   ui  → platform 的抽象接口
#   core 不依赖 Widgets、Win32、XCB 或 D-Bus
#
# 不为每个小类创建独立库。当前只有两个目标：
#   money-under-bed-core     可脱离桌面运行的纯逻辑库
#   money-under-bed-deskpet  Qt Widgets GUI 可执行文件
# ui/ 把自己的源文件加到可执行目标上，保留目录职责边界但不额外拆库。
#   money-under-bed-platform 窄平台接口与各平台实现
# character/ 的内容编入 core；dialogue/ 随阶段 6 加入 core。

add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(app)
add_subdirectory(ui)
