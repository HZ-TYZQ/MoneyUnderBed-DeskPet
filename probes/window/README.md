# 跨平台窗口探针

一次性可行性代码，对应 `docs/Plans/DevelopmentPlan.md` 的阶段 1。

约束：

- 本目录不链接、不包含任何正式产品代码。探针和产品各自独立实现，避免其中一方的缺陷被另一方遮蔽。
- 探针使用 C++17；正式工程使用 C++20。该差异是有意的，只影响探针自身。
- Qt 版本必须与正式工程一致（`6.11` 系列），否则结论不能代表产品。
- 默认素材是仓库内的 `assets/character/idle-down-left.png`，不依赖被 Git 忽略的 `Reference/`。
- Windows 上探针保留控制台子系统，输出即结果；正式产品按决策使用 GUI 子系统。

Linux 侧的结论已记录在 `docs/FeasibilityResults.md`，本轮不重测。
本轮的目标是 Windows 结果，写入 `docs/WindowsFeasibilityResults.md`。

## 用例

每个用例只测一件事。拖动和穿透刻意拆成互不回退的独立用例：
一条路径成功不能记作两条都成功。

| 用例 | 测什么 |
| --- | --- |
| `build` | 构造 `QApplication` 并立即退出 |
| `screen` | 后端名、屏幕几何、DPR、DPI、刷新率 |
| `dpi` | 缩放信息，并跟踪屏幕切换 |
| `render` | 透明背景与整数倍最近邻绘制 |
| `animate` | 帧序、循环与实测帧间隔 |
| `move` | 自主移动，请求位置与实际位置对照 |
| `drag-system` | 只用 `startSystemMove()`，不接受即记失败 |
| `drag-manual` | 只用手动 `move()`，不调用 `startSystemMove()` |
| `hittest` | 由 alpha 生成窗口掩码：可见像素接收点击，透明像素穿透 |
| `passthrough-qt` | 整窗穿透的 Qt 路径：`Qt::WindowTransparentForInput` |
| `passthrough-native` | 整窗穿透的候选路径：只改原生扩展样式 |
| `topmost` | 置顶开关每 4 秒切换一次 |
| `focus` | 显示、点击、拖动是否抢走当前应用焦点 |
| `windowlist` | 任务栏与 `Alt+Tab` 表现 |
| `lifecycle` | 关闭、退出与重复启动 |

`hittest`、`passthrough-qt`、`passthrough-native` 会同时打开一个位于下方的点击靶窗口，
用来区分“角色吃掉了点击”和“点击穿到了下层”。

`passthrough-native` 目前只在 Windows 上有实现。Linux 的 Qt 标志路径已在
`docs/FeasibilityResults.md` 实测通过，因此不引入 XCB input shape 候选实现；
在 Linux 上运行该用例会记录 `not_implemented` 并继续。

## 选项

```text
--case <name>       用例名，默认 screen
--list-cases        列出全部用例名并退出
--sprite <path>     精灵表路径，默认使用随包的 assets/character/idle-down-left.png
--scale <1..8>      整数倍率，默认 2
--duration <s>      自动退出秒数，0 表示一直运行到关闭
--log <path>        日志文件路径，默认 ./deskpet-probe-<case>.log
```

退出码：

| 码 | 含义 |
| --- | --- |
| 0 | 正常结束 |
| 2 | 参数错误（未知用例、非法倍率、非法时长） |
| 3 | 素材缺失、加载失败或尺寸非法 |
| 4 | 没有主屏幕 |

## Linux 构建与自动检查

依赖、配置、编译和自动检查都在 Distrobox `dev-fedora` 内执行，不向宿主安装开发依赖。

```bash
distrobox enter dev-fedora -- bash -lc '
  cd /home/tyzq/Projects/MoneyUnderBed_DeskPet
  cmake -S probes/window -B probes/window/build -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
  cmake --build probes/window/build
'
```

offscreen 冒烟检查：

```bash
distrobox enter dev-fedora -- bash -lc '
  cd /home/tyzq/Projects/MoneyUnderBed_DeskPet/probes/window/build
  export QT_QPA_PLATFORM=offscreen
  for c in $(./deskpet-probe --list-cases); do
    ./deskpet-probe --case "$c" --duration 1 >/dev/null 2>&1 \
      && echo "$c ok" || echo "$c exit=$?"
  done
'
```

offscreen 只证明代码路径和计时逻辑可执行，不能证明真实合成器中的透明、移动、输入和堆叠行为。

## 交互式运行

```bash
distrobox enter dev-fedora
cd /home/tyzq/Projects/MoneyUnderBed_DeskPet/probes/window/build

QT_QPA_PLATFORM=xcb ./deskpet-probe --case hittest --duration 0
QT_QPA_PLATFORM=xcb ./deskpet-probe --case drag-system --duration 0
```

比较两个后端时不得在两次运行之间修改探针，且必须使用相同的二进制、素材、倍率和用例。

## Windows

由 GitHub Actions 构建并打包。项目所有者下载同一份 ZIP，按
`WindowsChecklist.md` 执行，不使用本地临时构建替代。
