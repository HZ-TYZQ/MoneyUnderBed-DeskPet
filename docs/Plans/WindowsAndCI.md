# Windows 支持与 GitHub Actions 构建提案（历史资料）

日期：2026-08-26

状态：历史提案，不再作为可执行计划。已确认结论以 `docs/Decisions.md` 为准，新的执行顺序见 `docs/Plans/DevelopmentPlan.md`。本文保留早期技术分析；其中开机自启、独占全屏自动隐藏、普通 `QLocalServer` 作为 Windows 唯一实例锁等内容已经被最终决策否决或替代，不得据此实现。

本文规划 Windows 平台的实现路径，以及 Linux/Windows 双平台的 GitHub Actions 构建方案。

## 1. 结论摘要

- Windows 不存在 Linux 上的合成器限制。自主移动、置顶、透明和穿透在 Win32 上都是常规能力，不需要像 Linux 那样先做后端选型。
- Windows 的难点不在"能不能"，而在"不打扰"的细节：焦点抢占、任务栏与 Alt-Tab 露出、独占全屏被降层、运行时切换穿透导致的窗口重建。
- 现有探针 `Temp/qt-window-probe/` 是纯 Qt 代码，没有任何平台条件编译或 X11 调用，可以原样编译到 Windows 运行同一套用例，取得与 Linux 对称的结论。建议以此作为 Windows 第一步，而不是直接写产品代码。
- 双平台构建适合用一个 GitHub Actions 工作流加两个 job，共用 CMake 预设与 offscreen 冒烟测试。
- 存在一个阻塞项：角色 PNG 是否随仓库分发尚未决定，该问题直接决定 CI 能否产出可运行的发行包。

## 2. Windows 技术前提

对照 `docs/FeasibilityResults.md` 中 Linux 的受限点：

| 能力 | Linux 结论 | Windows 预期实现 |
| --- | --- | --- |
| 透明窗口 | XCB 正常 | 分层窗口 `WS_EX_LAYERED`，Qt 由 `WA_TranslucentBackground` 启用 |
| 自主移动 | 仅 XCB 可用 | 常规能力，无合成器否决 |
| 始终置顶 | 仅 XCB 可用 | `HWND_TOPMOST`，但独占全屏下会被降层 |
| 鼠标穿透 | XCB 正常 | `WS_EX_TRANSPARENT` |
| 不进任务栏 | 依赖 `Qt::Tool` | `WS_EX_TOOLWINDOW`，由 `Qt::Tool` 映射 |
| 不抢焦点 | 未专门测试 | 需显式加 `WS_EX_NOACTIVATE` |
| 高 DPI | 后端间有差异 | Per-Monitor V2，Qt 6 默认启用 |

Windows 侧不需要在两个窗口后端之间做选型，因此平台风险显著低于 Linux。

## 3. 建议的第二轮探测：Windows 复用现有探针

现有探针没有任何平台特定代码，`CMakeLists.txt` 也只依赖 `Qt6::Widgets`。因此：

- 不新写 Windows 探针，直接用 MSVC 编译现有源码。
- 运行与 Linux 相同的 `build`、`screen`、`render`、`animate`、`move`、`drag`、`input`、`topmost` 用例。
- 产出与 `docs/FeasibilityResults.md` 结构一致的 Windows 结果表，便于横向对照。

Windows 侧需要额外记录、Linux 未覆盖的观察项：

1. 独占全屏应用运行时，角色是否被降到全屏窗口之下。
2. 切换穿透时窗口是否发生重建，是否出现闪烁、置顶丢失或任务栏瞬时露出。
3. `startSystemMove()` 进入系统移动循环期间，动画定时器是否被拖慢或饿死。
4. 窗口是否会抢走焦点，尤其是启动瞬间和被点击时。
5. 混合 DPI 多显示器之间拖动时，缩放倍率与像素清晰度的变化。
6. 窗口是否出现在 Alt-Tab 与任务栏中。

## 4. 已知需要平台分支的两处代码

现有探针在 Windows 上语义与 Linux 不同，产品实现不应直接照搬：

### 4.1 运行时切换穿透

`ProbeWindow.cpp:249` 使用 `setWindowFlag(Qt::WindowTransparentForInput, enabled)`。

在 Windows 上修改窗口标志会触发原生窗口重建。对一个分层、置顶、无边框的窗口，重建可能带来闪烁、置顶状态丢失或短暂在任务栏出现。

建议实现：Windows 分支直接用 `SetWindowLongPtrW` 增删 `WS_EX_TRANSPARENT`，句柄取自 `winId()`，不改动 Qt 窗口标志，避免重建。

### 4.2 拖动

`ProbeWindow.cpp:146` 优先使用 `startSystemMove()`。

Windows 的系统移动是一个模态循环，可能影响拖动期间的定时器精度。探针已有手动拖动回退分支，Windows 上应实测两条路径后再决定默认用哪条，而不是沿用 Linux 的结论。

### 4.3 建议的抽象边界

不要在行为逻辑里散落条件编译。建议定义一个窄接口，例如：

- `setClickThrough(bool)`
- `setTopmost(bool)`
- `setAcceptsFocus(bool)`
- `setAutoStart(bool)`
- `isExclusiveFullscreenActive()`

接口在 `platform/` 下按平台各实现一份，角色状态机、动画与漫游逻辑保持平台无关。这样 Windows 的加入不污染现有 Linux 路径。

## 5. 平台差异对照

| 项目 | Linux | Windows |
| --- | --- | --- |
| 开机自启 | `~/.config/autostart/` 下的 desktop 文件 | 注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` |
| 托盘 | KDE 正常；GNOME 需要用户安装扩展 | `QSystemTrayIcon` 原生可用 |
| 配置路径 | `QStandardPaths` | `QStandardPaths` |
| 单实例 | `QLocalServer` | `QLocalServer` |
| 可用区域 | 排除面板 | 排除任务栏 |
| 置顶上限 | 全屏窗口行为待记录 | 独占全屏会强制降层 |

### 5.1 需要注意的 Linux 侧连带问题

GNOME 已移除传统托盘，`QSystemTrayIcon` 在未装扩展的 GNOME 上可能不显示。而托盘若被当作唯一控制入口，显示隐藏、模式切换和退出都会随之失效。`docs/Decisions.md` 第 3.3 节已据此定为：角色右键菜单是主要控制入口，托盘只作为备用入口，必要功能不能只存在于托盘中。

这意味着在 GNOME 上，用户可能失去唯一的控制入口。建议第一版不把托盘作为唯一入口，至少提供一个不依赖托盘的兜底交互，例如角色右键菜单或全局快捷键。

该问题由本次平台梳理发现，属于对现有功能规划的修正建议，需要确认后才更新功能提案。

### 5.2 独占全屏的行为选择

Windows 上桌宠浮在游戏或全屏视频之上通常是负体验。建议检测到独占全屏或演示状态时自动隐藏角色，退出后恢复。可用 `SHQueryUserNotificationState` 判断。

这是产品行为决策，不是技术限制，需要确认。

## 6. GitHub Actions 方案

### 6.1 工作流结构

单个工作流，两个并行 job，触发条件为 push、pull request 和 tag。

共用部分：

- CMake + Ninja。
- 统一的 `CMakePresets.json`，避免两边命令行分叉。
- 通过 `jurplel/install-qt-action` 安装同一个固定 Qt 版本，保证两平台版本一致。不使用发行版仓库的 Qt，因为 Ubuntu 与 Fedora 的版本差距会造成两边行为不一致。
- 冒烟测试统一使用 `QT_QPA_PLATFORM=offscreen`，这一路径在 Linux 已验证可用，Windows 同样支持。

### 6.2 Linux job

- 运行器建议 `ubuntu-22.04` 而不是最新版，以获得更宽的 glibc 兼容范围。
- 需要安装 XCB 相关运行时依赖，Qt 6.5 及以上通常还需要 `libxcb-cursor0`。
- 打包建议 AppImage，适合 GitHub Releases 分发且不依赖发行版包管理。

### 6.3 Windows job

- 运行器 `windows-2022`，编译器 MSVC 2022。不建议 MinGW，Qt 官方二进制与调试工具链对 MSVC 支持更完整。
- 使用 `windeployqt` 收集运行时依赖。
- 产物为免安装 zip。安装包可延后，桌宠类应用免安装分发通常足够。

### 6.4 发布

打 tag 时由两个 job 的产物合并创建 GitHub Release。发行说明必须包含角色素材的作者署名与非商业限制，与 `docs/Decisions.md` 第 4 节一致。

### 6.5 代码签名

Windows 未签名可执行文件会触发 SmartScreen 警告。购买签名证书属于商业支出，与项目非商业定位冲突。

建议：不签名，在 README 与 Release 说明中提示该警告及其原因。此项需要确认。
决定: 接受不签名

## 7. CI 会强制提前的决策

以下条目目前在 `docs/Decisions.md` 第 5 节属于未确定，但建立 CI 必须先给出答案：

1. C++ 标准版本。建议 C++20，MSVC 2022 与 GCC 12 以上均完整支持。探针使用的 C++17 只对探针有效。
2. 固定的 Qt 版本与最低 Qt 版本。
3. 构建系统与预设结构。
4. 使用的 Qt 模块清单，直接影响 `windeployqt` 与 AppImage 的体积。

## 8. 素材分发方式（已确认）

角色 PNG 随公开仓库分发，见 `docs/Decisions.md` 第 5 节。

对 CI 的影响：

- 打包步骤直接使用仓库内的 `assets/`，不需要外部下载步骤，也不需要首次运行引导。
- 发行包内必须一并包含 `assets/LICENSE.md` 与 `assets/MANIFEST.md`。
- Release 说明需附作者署名、原视频链接与非商业限制。
- 因素材非商业，发行产物不得用于任何收费或含广告的分发渠道。

`Reference/` 保持不入仓，CI 不依赖该目录。

## 9. 建议顺序

1. 确认第 7 节的 C++ 标准与 Qt 版本。
2. 在 Windows 上编译现有探针，跑完整用例矩阵，补写 Windows 可行性结果。
3. 根据探测结果确定 Windows 的穿透与拖动实现方式。
4. 建立最小 CI，先只做两平台编译与 offscreen 冒烟测试。
5. 再加打包与发布。
6. 最后处理独占全屏隐藏等平台行为细节。

## 10. 待确认问题

1. C++ 标准与固定 Qt 版本。
2. Windows 是否在独占全屏时自动隐藏。
3. 是否接受不签名与 SmartScreen 警告。
4. Windows 最低支持版本，建议 Windows 10 及以上。
5. 是否需要 Windows 安装包，还是免安装 zip 即可。

托盘兜底入口的问题由右键菜单功能规划一并处理，不在本文追踪。
