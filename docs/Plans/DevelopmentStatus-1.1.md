# MoneyUnderBed DeskPet 1.1.0 开发状态

需求基线：`docs/Decisions.md`（第 14 节为本版范围），冻结于提交 `0adb5b7`。

执行计划：`docs/Plans/DevelopmentPlan-1.1.md`，同一提交。

状态只使用 `未开始`、`进行中`、`阻塞`、`已通过`。工程阶段只有在容器本地门与该阶段
CI 都有实际结果后才写为 `已通过`；人工检查点在收到项目所有者结论前不得预填。

## 阶段总览

| 阶段 | 内容 | 状态 | 提交 | CI |
| --- | --- | --- | --- | --- |
| 1 | 应用生命周期根因修复 | 已通过 | `50bfc02` | CI-1 已通过 |
| 2 | 设置领域模型、schema 1 与应用控制器 | 已通过 | `d3640ac` | CI-2 已通过 |
| 3 | 运行时配置、生效快照与独立闲聊 | 已通过 | `18475a4` | CI-3 已通过 |
| 4 | 四组设置界面与端到端整合 | 已通过 | `d03d46d` | CI-4 已通过 |
| A | 人工检查点：KDE 集成功能与参数调优 | 已通过 | `d03d46d` | — |
| 5 | 调优收敛、完整回归与候选准备 | 未开始 |  | CI-5 未开始 |
| 6 | 正式标签、候选产物与发布验收 | 未开始 |  | CI-6 未开始 |
| B | 人工检查点：正式候选验收 | 未开始 |  | — |

## 1.0.0 候选历史

`1.0.0` 候选从未公开发布。按 `docs/Decisions.md` 第 14.1 节，不为它另建预发布标签；
候选历史由本节记录，标签与草稿 Release 在阶段 6 删除。

| 项目 | 值 |
| --- | --- |
| 标签 | `v1.0.0`（阶段 6 删除） |
| 标签指向的提交 | `8356ff2` |
| 草稿 Release | 存在，未发布，阶段 6 删除 |
| 候选包审计 run | `33096110820`（AppDir 55 条 ELF 记录，25 个 Ubuntu 源包） |

已完成的候选检查（`docs/ReleaseChecklist.md`，阶段 5 改名为 `ReleaseChecklist-1.0.md`）：

- 第 1 节产物身份与自动门：全部通过。
- 第 2 节 KDE Plasma + XCB/XWayland：除连续三小时运行外全部通过。
- 第 3 节 Windows 11：全部通过，含一次连续三小时运行。
- 第 4 节三小时性能记录：表格为空，没有留下数值。
- 第 5 节如实保留的边界：全部通过。
- 第 6 节发布确认：未做。

该批结果只作为历史基线。`1.1.0` 候选必须重新完成自动门、双平台检查表和 KDE 的
三小时运行（Windows 的三小时按第 14.10 节放宽）。

## 已知阻塞项

截至 `0adb5b7`，项目所有者确认阻止候选发布的问题只有一项：

- **关闭设置或关于窗口会连带退出桌宠。** 见下节根因证据。阶段 1 修复。

## 阶段 1 根因证据

日期：2026-08-28。环境：`dev-fedora` 容器，Qt 6.11.2，`QT_QPA_PLATFORM=offscreen`。

假设：`QApplication::quitOnLastWindowClosed` 默认为真，而角色窗口与气泡窗口按
`deskPetWindowFlags()` 带 `Qt::Tool`（`src/ui/CharacterWindow.cpp:55,261`），不计入
「最后一个窗口」；设置与关于窗口是 `QDialog`，计入。因此关闭辅助窗口时 Qt 认为
最后一个普通窗口已关闭，进而退出应用。全仓库没有任何 `setQuitOnLastWindowClosed`
调用，即从未关闭过该默认策略。

用一个独立探针分别验证两个方向，探针只用 Qt 公开 API，不引入产品代码：

| 条件 | `lastWindowClosed` | `aboutToQuit` | 角色窗口 |
| --- | --- | --- | --- |
| 默认策略 | — | 已触发，`exec()` 立即返回 | 进程已退出 |
| `setQuitOnLastWindowClosed(false)` | 已触发 | 未触发 | 仍可见 |

默认策略下，关闭 `QDialog` 后计划在 200 ms 后执行的检查根本没有机会运行，说明退出
发生在关闭的同一轮事件循环内。关闭策略后 `lastWindowClosed` 仍然触发，但应用不再
退出，角色窗口保持可见——这同时证明了 `Qt::Tool` 窗口确实不计入该规则。

结论：根因是启动层从未接管退出策略，不是设置窗口的关闭事件有问题。因此按计划
第 5 节修复启动层，不把关闭改写成隐藏。

## CI-1（阶段 1 退出门）

提交 `50bfc02`。两个必需工作流在同一提交上全绿：

| 工作流 | run | 结果 |
| --- | --- | --- |
| Build and test | `33138502597` | Linux (Ubuntu 22.04, GCC) 与 Windows (windows-2022, MSVC 2022) 均通过 |
| Package candidates | `33138502673` | metadata、Qt 对应源码、Linux AppImage、Windows 便携 ZIP 均通过；`Create draft GitHub Release` 按条件跳过（非标签推送） |

同一提交上的 `Windows window probe`（run `33138502624`）也通过。该工作流的去留按
计划第 10 节在阶段 5 决定，不属于任何阶段 CI 的必需门。

## CI-2（阶段 2 退出门）

提交 `d3640ac`。两个必需工作流在同一提交上全绿：

| 工作流 | run | 结果 |
| --- | --- | --- |
| Build and test | `33139854317` | Linux (Ubuntu 22.04, GCC) 与 Windows (windows-2022, MSVC 2022) 均通过 |
| Package candidates | `33139854258` | metadata、Qt 对应源码、Linux AppImage、Windows 便携 ZIP 均通过；`Create draft GitHub Release` 按条件跳过 |

同一提交上的 `Windows window probe`（run `33139854382`）也通过。

容器本地门：全量 CTest 34/34 通过（新增 `tst_settingspresets`、`tst_settingscontroller`），
`--self-test` 退出码 `0`，`MUB_WARNINGS_AS_ERRORS=ON` 下无警告。

### 阶段 2 结束时尚未接线的部分

按计划的阶段划分，以下内容属于阶段 3 与阶段 4，阶段 2 结束时**新参数还不产生实际效果**：

- `AutonomousBehaviorConfig`、打字速度、单页自动消失和三个帧时长尚未从设置接线到
  运行时（第 7.1、7.3 节）。
- `main.cpp` 仍在用自己的 lambda 套用并保存设置，`SettingsController` 尚未接入产品
  路径（第 8.3 节）。
- `CharacterPresenter` 的闲聊门是阶段 2 的临时实现，阶段 3 必须删除。**已于阶段 3 删除。**

容器本地门：全量 CTest 32/32 通过（新增 `tst_applifecycle`），
`--self-test` 退出码 `0`，`MUB_WARNINGS_AS_ERRORS=ON` 下无警告。

## 阶段 2 的边界调整

计划第 6.4 节记录了原因：新模型里没有气泡频率字段（`docs/Decisions.md` 第 14.4、
14.2 节），而 `settings.bubble` 的三个消费者分属阶段 3 和 4，替换 `core::Settings`
后无法编译。项目所有者于 2026-08-28 确认采用「阶段 2 顺带做完解耦」的方案：

- `ClickFeedbackSelector` 改为只收一个 `clickTextChancePercent`，`ClickFeedbackConfig`
  随之删除（原属第 7.3 节）。
- `CharacterPresenter` 的闲聊门改读 `chatterChancePercent > 0`，是**临时实现**，
  阶段 3 引入 `ChatterScheduler` 后必须连同 `AutonomousBehavior` 的闲聊职责一起删除。
- `SettingsWindow` 只做保持编译的机械改动：仍然只暴露四个既有设置项，其余参数
  以最近一次 `setSettings()` 的完整取值为底原样透传。
- `src/core/BubbleFrequency.h` 删除，其角色由 `SettingsPresets.h` 的 `SpeechFrequency`
  接替（四档，且「关闭」只由概率 `0%` 表达）。

## 检查点 A 冻结的档位取值

`src/core/SettingsPresets.cpp` 是低/中/高具体取值的唯一存放处。下列取值已由人工
检查点 A 实测后**原样冻结**，并写回 `docs/Decisions.md` 第 14.3 至 14.5 节，
冻结来源记于该文件第 14.11 节：

| 档位 | 低／慢／偶尔 | 中／正常 | 高／快／经常 |
| --- | --- | --- | --- |
| 活动节奏 `idleMin/Max` | `4000` / `12000` | `2000` / `6000` | `1200` / `3500` |
| 活动节奏 `walkMin/Max` | `1000` / `2500` | `1500` / `4000` | `2000` / `6000` |
| 活动节奏 `restMin/Max` | `6000` / `20000` | `4000` / `12000` | `3000` / `8000` |
| 活动节奏 `restChancePercent` | `40` | `25` | `15` |
| 移动速度 `walk/return` | `30` / `60` | `48` / `90` | `75` / `130` |
| 接近鼠标 | `0`（关）／`12`（偶尔） | — | `30` |
| 说话频率 间隔／概率 | `120000` / `30` | `90000` / `50` | `60000` / `70` |
| 单击台词概率 | `20`（第 14.4 节已定） | `45` | `70` |
| 打字速度 `typingMsPerChar` | `45` | `28`（第 4.1 节已定） | `16` |
| 动画速度 待机／跑动／冰淇淋 | `150` / `120` / `150` | `100` / `80` / `100` | `70` / `56` / `70` |

说话频率的默认档是「低」，与第 4 节「气泡默认低频」一致；最高档一次判定的期望
间隔仍在分钟量级，符合第 4 节「不做高频陪聊」。

## 阶段 3 的接线结果

阶段 2 遗留的两项临时实现都已清除：

- `AutonomousBehavior` 不再有 `consumeChatterRequest()` 与 `chatterRequested_`，
  `chooseNextFromIdle()` 里也没有任何闲聊概念。`tst_autonomousbehavior` 直接扫描
  头文件断言这两个名字不再出现，防止旧耦合被重新引回来。
- `CharacterPresenter` 的 `chatterChancePercent > 0` 临时门已由 `ChatterScheduler`
  取代。

新的生效边界：

| 参数 | 生效时机 | 实现方式 |
| --- | --- | --- |
| 待机／行走／休息时长 | 下一次进入对应状态 | 截止时间在进入状态时算好，不重算 |
| 移动与返回速度 | 下一次开始对应移动 | `activeSpeedPxPerSec_` 在进入移动状态时快照 |
| 休息／接近鼠标概率 | 下一次行为选择 | `chooseNextFromIdle()` 读当前配置 |
| 自主闲聊间隔与概率 | 下一轮调度 | 一轮开始时把间隔与概率一起定下来 |
| 单击台词概率 | 下一次单击 | 单击时读当前设置 |
| 打字速度、单页自动消失 | 下一次对话开始 | `DialogueSession::start()` 快照 |
| 动画帧时长 | 下一次启动对应动画 | `AnimationPlayer` 在 `restartFromFrame()` 快照 |
| 活动模式 | 当前行为结束后 | 状态机不因改模式而中断（切安静时停止接近鼠标除外） |
| 显示倍率、始终置顶 | 立即 | 窗口属性，没有「下一次行为」 |

`AnimationClip` 的 `frameDurationMs` 字段替换为显式的 `AnimationCategory`，帧时长
统一由 `AnimationTiming` 提供，符合本文件既有的「运行时使用显式映射，不从文件名
推断语义」约束。

`bottomTolerancePx` 与 `timeJumpThresholdMs` 由 `behaviorConfigFrom()` 固定取内部
默认值，不经过用户设置（第 14.7 节）。

`ChatterScheduler` 的第一轮从第一次 `update()` 起算，因此首次可能说话的时刻是
「启动 + 一整轮间隔」，不会一启动就说话。

## CI-3（阶段 3 退出门）

提交 `18475a4`。两个必需工作流在同一提交上全绿：

| 工作流 | run | 结果 |
| --- | --- | --- |
| Build and test | `33145713607` | Linux (Ubuntu 22.04, GCC) 与 Windows (windows-2022, MSVC 2022) 均通过 |
| Package candidates | `33145713538` | metadata、Qt 对应源码、Linux AppImage、Windows 便携 ZIP 均通过；`Create draft GitHub Release` 按条件跳过 |

同一提交上的 `Windows window probe`（run `33145713621`）也通过。

容器本地门：全量 CTest 35/35 通过（新增 `tst_chatterscheduler`），
`--self-test` 退出码 `0`，`MUB_WARNINGS_AS_ERRORS=ON` 下无警告。

### 阶段 3 结束时尚未接线的部分

- `main.cpp` 仍在用自己的 lambda 套用并保存设置，`SettingsController` 尚未接入
  产品路径（第 8.3 节）。因此设置窗口的改动目前仍不经过去抖落盘。
- 设置窗口仍只暴露四个既有设置项，四组结构、普通/高级分层、滑块与两级重置
  都在阶段 4。

## 阶段 4 的界面结构

设置窗口重写为四组：行为、对话、外观、窗口与桌面。每组内「高级」是一个折叠区，
不是独立页面；每组各带一个「恢复本组默认值」，窗口底部是「全部恢复默认值」。

控件形式按第 14.2 节区分：

| 参数类型 | 控件 | 例 |
| --- | --- | --- |
| 成对上下限 | **只有数字框** | 待机／行走／休息三组时长 |
| 单值有界 | 滑块 + 数字框 | 移动速度、返回延迟、帧时长、打字速度 |
| 百分比 | 滑块 + 数字框 | 休息、接近鼠标、闲聊触发、单击台词四个概率 |
| 离散档位 | 下拉框 | 活动模式、显示倍率、七组档位 |

秒级时长以秒显示（一位小数），保存与运行时转换为毫秒。数字框统一关闭
`keyboardTracking`，因此输入 `3000` 不会先产生 `3`、`30`、`300`——这些值既不进
运行时也不进配置文件。

主线程的落盘节奏：滑块拖动过程中每个中间值立即生效但不落盘，滑块释放或数字框
完成编辑时落盘一次。控制器的去抖窗口是 `400 ms`。

`main.cpp` 不再手工拼「套用并保存」的 lambda，也不再持有第二份运行时设置；
`--scale` 通过 `applyForThisRunOnly()` 只覆盖本次运行，不写回配置文件。
诊断信息新增一行设置摘要（活动模式、说话频率档位、显示倍率、置顶），
不含配置路径、用户名或对话历史。

## CI-4（阶段 4 退出门）

提交 `d03d46d`。两个必需工作流在同一提交上全绿：

| 工作流 | run | 结果 |
| --- | --- | --- |
| Build and test | `33146976111` | Linux (Ubuntu 22.04, GCC) 与 Windows (windows-2022, MSVC 2022) 均通过，各 36/36 |
| Package candidates | `33146976117` | metadata、Qt 对应源码、Linux AppImage、Windows 便携 ZIP 均通过；`Create draft GitHub Release` 按条件跳过 |

同一提交上的 `Windows window probe`（run `33146976066`）也通过。

容器本地门：全量 CTest 36/36 通过（新增 `tst_settingsintegration`，`tst_settingswindow`
重写为 21 个用例），`--self-test` 退出码 `0`，`MUB_WARNINGS_AS_ERRORS=ON` 下无警告。

检查点 A 使用的 Linux 产物是该 run 的
`MoneyUnderBed-DeskPet-linux-x86_64-dev-d03d46d513f9`。

### 阶段 4 结束时的未决项

- `src/core/SettingsPresets.cpp` 中除默认档位外的全部取值仍是原型值，须经检查点 A
  实测冻结后写回 `docs/Decisions.md` 第 14.3 至 14.5 节，见本文件第 131 行起的表。
- `probe-windows.yml` 的去留、`ReleaseChecklist-1.1.md` 的建立与旧清单改名、
  `DesktopChecklist.md` 复核、`build.yml` 的标签触发，都在阶段 5。

## 人工检查点 A（KDE 集成功能与参数调优）

结论：**已通过**。环境为 KDE Plasma，被测产物是提交 `d03d46d` 上
`Package candidates` run `33146976117` 的 Linux AppImage
（`MoneyUnderBed-DeskPet-linux-x86_64-dev-d03d46d513f9`）。

### 功能

项目所有者报告全部功能无问题。其中阶段 1 的根因修复在真实宿主上得到确认：
关闭设置窗口不再连带关闭桌宠。此前该修复只有独立 Qt 探针程序和
`tst_applifecycle` 的自动化证据，本次是第一份宿主证据。

### 资源占用

一次 5 分钟采样，间隔 `10 s`，共 30 个样本；采样期间穿插互动、设置修改、暂停与
恢复等操作。

| 指标 | 结果 |
| --- | --- |
| 常驻内存 RSS | `16.1` → `16.3 MB`，无增长趋势 |
| 线程数 | 恒为 `2` |
| 虚拟内存 VSZ | 恒为 `1025.4 MB` |

线程数在反复开关设置窗口与关于窗口的过程中始终为 `2`，说明 `AppLifecycle`
复用辅助窗口实例的做法在真机上成立，没有窗口相关的线程泄漏。

按 `docs/Decisions.md` 第 14.10 节，占用是否可接受由项目所有者判定：结论为
**通过**。

该次采样的两列数据不具备证据力，记录在此以免被后续引用时读过头：

- **CPU% 列几乎全为 `0.00`。** 采集用的是 `ps` 的 `%cpu`，它是进程存活期的累计
  平均而非采样区间的瞬时值，对一个跑定时器动画的进程会系统性偏低。要取瞬时
  CPU 需改用 `pidstat` 或对 `/proc/PID/stat` 求两次差值。
- **读／写列恒为 `0 KB`。** 该列通常取 `read_bytes`／`write_bytes`，只统计真正落到
  块设备的 I/O；配置写入停留在页缓存中等待回写，采样窗口内看不到。采样期间确有
  设置改动落盘，不能据此认为没有写入。
- 5 分钟只能说明启动后短期不增长，不能说明长期不增长。

### 结转到发布阶段的事项

更长时间的连续均值测量放到发布阶段执行，并按上述两条改用能取瞬时 CPU 的采集
方式。该项不改变第 11.3 节对 KDE 正式验证环境连续运行时长的要求，本次 5 分钟
采样也不替代它。
