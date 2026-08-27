# 第一版可行性探测结果

日期：2026-08-26

对应计划：`docs/Plans/FeasibilityProbe.md`

状态：测试完成，结论为条件可行

本文记录测试观察和由观察直接支持的技术结论，不构成产品功能或正式平台支持决策。

## 1. 测试对象

- 语言与框架：C++、Qt 6.11.1。
- 探针：`Temp/qt-window-probe/deskpet-probe`。
- 素材：`Reference/Hero/hero-idel-l-Sheet01.png`。
- 精灵表尺寸：`621 x 111`，按 `69 x 111` 裁切为 9 帧。
- 测试用例：构建、屏幕信息、透明渲染、动画、自主移动、拖动、鼠标穿透和置顶。

## 2. 容器自动检查

依赖查询、CMake 配置、编译和自动检查均在 Distrobox `dev-fedora` 内执行，没有在宿主机安装项目依赖。

结果：

- CMake 与 Ninja 配置成功。
- GCC 16.1.1 编译和链接成功，无编译警告。
- `build`、`screen`、`render`、`animate`、`move`、`drag`、`input`、`topmost` 的 offscreen 冒烟路径均正常退出。
- 非法测试用例返回退出码 2。
- 非法精灵尺寸返回退出码 3。
- 精灵表成功识别为 9 帧。
- 动画 100 ms 定时测试中，采样间隔最小 99 ms、最大 101 ms、平均 100.06 ms。
- 鼠标穿透的定时开启与恢复代码路径正常执行。
- Qt 的 Wayland 与 XCB 平台插件均能加载。

offscreen 测试只证明代码路径和计时逻辑可执行，不能证明真实合成器中的透明、移动、输入与堆叠行为。

## 3. 屏幕与缩放观察

### niri 会话

| Qt 后端 | Qt 报告的屏幕尺寸 | DPR | 刷新率 |
| --- | --- | --- | --- |
| Wayland | `1707 x 1067` | `2.0` | `240 Hz` |
| XCB/XWayland | `2560 x 1600` | `1.0` | `239.94 Hz` |

在 niri 下，XCB 角色窗口视觉上略小于 Wayland。该现象与两个后端报告的坐标空间和 DPR 不同一致。

### KDE Plasma 会话

在 KDE Plasma 下，XCB 与 Wayland 角色窗口的视觉大小一致，没有复现 niri 下的尺寸差异。

因此，尺寸差异是桌面环境、XWayland 实现与缩放报告共同造成的环境行为，不是固定的 Qt XCB 行为。

## 4. 自主移动结果

| 桌面环境 | Qt 后端 | 结果 |
| --- | --- | --- |
| niri | Wayland | 失败，窗口不移动 |
| niri | XCB/xwayland-satellite | 失败，窗口不移动 |
| KDE Plasma | Wayland | 失败，窗口不移动 |
| KDE Plasma | XCB/XWayland | 通过，窗口能够自主移动 |

该矩阵表明：

- 探针的 Qt 窗口移动实现有效，因为相同二进制在 Plasma + XCB 下成功。
- 原生 Wayland 普通顶层窗口不能依赖客户端自由设置全局位置。
- niri 使用的 xwayland-satellite 不保留普通 X11 顶层窗口的绝对定位语义，因此强制 XCB 也不能恢复自主移动。
- KDE Plasma 的 XWayland 路径能够满足本探针的 XCB 窗口自主定位需求。

## 5. 其他人工测试结果

### KDE Plasma + Wayland

- 自主移动：失败。
- 鼠标拖动：可能出现闪烁的像素残影。
- 始终置顶：不生效。

拖动残影只在 Wayland 路径报告，而 XCB 路径正常，因此当前证据更支持 Qt Wayland、KWin 合成或透明窗口损伤更新路径的问题，不支持将其归因于精灵表或通用 QPainter 绘制逻辑。

置顶不生效与 Wayland 中窗口层级由合成器控制的模型一致。当前测试没有使用 layer-shell 等专用协议。

### KDE Plasma + XCB/XWayland

以下能力均由人工测试确认正常：

- 透明窗口与像素渲染。
- 精灵动画。
- 自主窗口移动。
- 鼠标拖动。
- 鼠标穿透与恢复。
- 始终置顶。
- 窗口视觉大小与 Wayland 一致。

## 6. 假设检验

原核心假设为：原生 Wayland 可能禁止自主定位，而 XCB/XWayland 应当能够自主移动。

结果为部分成立：

- 原生 Wayland 在 niri 和 Plasma 下均不能自主定位。
- XCB 是否能自主定位取决于 XWayland 集成方式。
- Plasma + XCB 成功。
- niri + xwayland-satellite 失败。

因此，不能把“强制 XCB”视为所有 Wayland 合成器上的通用解决方案。

## 7. 可行性结论

### 条件可行

C++/Qt 可以实现本次探测范围内的桌宠基础能力，证据是 KDE Plasma + XCB/XWayland 下所有测试能力均正常。

当前后端限制：

- KDE Plasma + XCB/XWayland：本轮完整通过。
- KDE Plasma + Wayland：不满足自主移动和置顶要求，并存在拖动残影。
- niri + Wayland：不支持普通顶层窗口自主定位。
- niri + XCB/xwayland-satellite：同样不支持该定位方式。

上述结论只证明技术可行性。是否将 XCB 作为 Linux 运行要求、是否支持 niri、是否采用 compositor IPC 或 layer-shell，不在本次测试中决定，也不应自动写入 `docs/Decisions.md`。

## 8. 后续若继续探测

只有在需要扩大 Linux Wayland 支持范围时，才需要另立测试计划验证：

- Wayland layer-shell 是否适合透明、置顶且频繁移动的桌宠表面。
- niri IPC 能否以可接受的频率移动浮动窗口。
- 透明窗口拖动残影是否能在最小 Qt/KWin 复现程序中稳定重现。
- 多显示器、混合缩放和不同刷新率下的窗口尺寸与边界行为。

在尚未决定平台支持范围前，不围绕这些路径加入产品代码。
