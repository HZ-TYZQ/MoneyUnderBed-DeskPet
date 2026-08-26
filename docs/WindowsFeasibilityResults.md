# Windows 窗口可行性结果

日期：待填（以项目所有者完成测试的日期为准）

对应计划：`docs/Plans/DevelopmentPlan.md` 第 6 节

对称文档：`docs/FeasibilityResults.md`（Linux 侧，已完成）

状态：**未实测**。本文只建立结构与检查项，尚未取得任何 Windows 实际结果。

本文记录测试观察和由观察直接支持的技术结论，不构成产品功能决策。
在全部核心能力有实际结果之前，不得据此进入阶段 2。

## 1. 测试对象

- 探针：`probes/window/`，可执行文件 `deskpet-probe.exe`。
- 构建：GitHub Actions `windows-2022` + MSVC 2022 + Ninja。
- Qt：`6.11.2`，`win64_msvc2022_64`。
- 素材：仓库内 `assets/character/idle-down-left.png`，`621 x 111`，按 `69 x 111` 裁切为 9 帧。
- 测试机：待填（Windows 版本号、CPU、显卡与驱动、显示器数量与缩放）。
- 待测试的 ZIP：Actions run 32995272648，commit `fc689995`，artifact `deskpet-probe-windows-x64-fc689995fbf9`，ZIP 大小 30 618 336 字节，SHA-256 `c335de582b6b06e8c64d94a94d4db3b084314a8484fed750293176bf26adb8f0`。
- 下载后先核对 SHA-256 再解压。GitHub 会把 artifact 再套一层 zip，上面的 SHA-256 指的是里面那个 `deskpet-probe-windows-x64-fc689995fbf9.zip`。

项目所有者测试的必须是 Actions 产出的同一份 ZIP，不使用本地构建替代。

## 2. CI 自动检查

已完成。结果来自 Actions run 32995272648，commit `fc689995`。

| 检查 | 结果 |
| --- | --- |
| MSVC 2022 配置与编译 | 通过，无 MSVC 警告 |
| 实际 Qt 版本等于 `6.11.2` | 通过，`qmake -query QT_VERSION` 报告 `6.11.2` |
| 参数校验退出码（未知用例、非法倍率、非法时长 → 2） | 通过，4 条断言 |
| 素材校验退出码（缺失、加载失败、尺寸非法 → 3） | 通过，2 条断言 |
| 全部用例的 offscreen 冒烟正常退出 | 通过，15 个用例全部退出 0 |
| `windeployqt` 后的可执行文件能加载随包素材 | 通过 |
| 包内含 app-local MSVC 运行库，且不含 `vc_redist.x64.exe` | 通过 |

实测工具链：

| 组件 | 版本 |
| --- | --- |
| Qt | `6.11.2`（`win64_msvc2022_64`） |
| CMake | `3.31.6` |
| Ninja | `1.13.2` |
| MSVC 工具集 | `14.44.35207` |
| Windows SDK | `10.0.26100.0` |
| runner | `windows-2022` |

offscreen 只证明代码路径和计时逻辑可执行，不能证明真实桌面中的透明、移动、输入和堆叠行为。

## 3. 屏幕与缩放观察

| 系统缩放 | Qt 报告的屏幕尺寸 | DPR | 逻辑 DPI | 刷新率 | 角色视觉大小 | 像素是否锐利 |
| --- | --- | --- | --- | --- | --- | --- |
| 100% | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 |
| 125% | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 |
| 150% | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 |
| 200% | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 | 未实测 |

倍率对照（同一档系统缩放下）：

| `--scale` | 期望像素尺寸 | 实际视觉尺寸 |
| --- | --- | --- |
| 1 | `69 x 111` | 未实测 |
| 2 | `138 x 222` | 未实测 |

## 4. 核心能力结果

“通过”必须来自实际观察。任何一项失败都要在第 6 节留下证据。

| 能力 | 用例 | 结果 | 观察 |
| --- | --- | --- | --- |
| 透明背景 | `render` | 未实测 |  |
| 整数倍最近邻像素 | `render` | 未实测 |  |
| 帧序与循环 | `animate` | 未实测 |  |
| 帧间隔稳定性 | `animate` | 未实测 |  |
| 自主移动 | `move` | 未实测 |  |
| 请求位置与实际位置一致 | `move` | 未实测 |  |
| 系统拖动 | `drag-system` | 未实测 |  |
| 手动拖动 | `drag-manual` | 未实测 |  |
| 可见像素接收点击 | `hittest` | 未实测 |  |
| 透明像素穿透到下层 | `hittest` | 未实测 |  |
| 整窗穿透（Qt 标志路径） | `passthrough-qt` | 未实测 |  |
| 整窗穿透（原生扩展样式路径） | `passthrough-native` | 未实测 |  |
| 置顶开启 | `topmost` | 未实测 |  |
| 置顶关闭 | `topmost` | 未实测 |  |
| 启动不抢焦点 | `focus` | 未实测 |  |
| 点击与拖动不抢焦点 | `focus` | 未实测 |  |
| 不出现在任务栏 | `windowlist` | 未实测 |  |
| 不进入 `Alt+Tab` | `windowlist` | 未实测 |  |
| 正常关闭与重复启动 | `lifecycle` | 未实测 |  |
| 锁屏、睡眠后窗口保持 | `lifecycle` | 未实测 |  |

拖动和穿透各拆成两个互不回退的用例。一条路径通过不能记作两条都通过。

## 5. 窗口重建观察

`passthrough-qt` 和 `topmost` 都会在切换前后打印原生窗口句柄。
句柄变化即窗口被重建，通常伴随闪烁、丢置顶或短暂出现在任务栏。

| 观察项 | 结果 |
| --- | --- |
| `passthrough-qt` 切换时 `rebuilt` | 未实测 |
| `passthrough-qt` 切换时是否闪烁 | 未实测 |
| `passthrough-qt` 切换后是否仍不在任务栏 | 未实测 |
| `passthrough-qt` 切换后是否仍然置顶 | 未实测 |
| `topmost` 切换时 `rebuilt` | 未实测 |
| `passthrough-native` 切换时 `rebuilt` | 未实测 |
| `passthrough-native` 是否生效并可恢复 | 未实测 |

结论走向：

- `passthrough-qt` 无重建、无闪烁：产品沿用纯 Qt 路径，不引入 Win32 穿透实现。
- `passthrough-qt` 复现重建问题，且 `passthrough-native` 正常：产品在 Windows 采用只改扩展样式的窄实现，并限制在 `src/platform/` 内。
- 两条路径都失败：停止阶段 2，回到原始失败重新形成假设。

## 6. 失败证据

每条失败记录：

- 完整的 `deskpet-probe-<用例>.log`。
- 日志中的 `probe.event=environment` 行（实际 Qt 版本与平台名）。
- 视觉问题的截图或短录屏。
- 该用例在其他缩放档位下是否同样失败。
- Windows 版本号与显卡驱动信息。

（尚无记录。）

## 7. 未测试范围

以下内容本轮不测，也不推测结果：

- 多显示器、显示器热插拔与混合 DPI。
- 独占全屏应用下的表现（第一版不实现自动隐藏，见 `docs/Decisions.md` 第 2.4 节）。
- 多用户会话切换。
- Windows 平板模式与触摸输入。
- 未签名可执行文件的 SmartScreen 与 Smart App Control 行为（属于阶段 9 发布说明范围）。

## 8. 可行性结论

待填。

在核心能力全部有实际结果之前，本节不得填写任何结论，
也不得以“预计 Win32 可行”通过阶段 1 的退出门。
