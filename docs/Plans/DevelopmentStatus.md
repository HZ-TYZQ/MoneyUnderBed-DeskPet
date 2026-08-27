# 第一版开发阶段状态

对应计划：`docs/Plans/DevelopmentPlan.md`

需求基线：`docs/Decisions.md`

本文只记录阶段的当前状态和已经取得的实际结果。

记录规则：

- 状态只能是 `未开始`、`进行中`、`阻塞`、`已通过`。
- `已通过` 必须在该阶段退出门的全部条件都有实际结果之后才填写，不预先填写。
- 需要项目所有者人工验收的阶段，未收到验收结论前不得填 `已通过`。
- 失败和未实测同样记录，不以“预计可行”替代结果。

## 阶段总表

| 阶段 | 名称 | 状态 | 最近更新 |
| --- | --- | --- | --- |
| 0 | 执行基线与历史计划整理 | 进行中 | 2026-08-27 |
| 1 | Windows 窗口探针与 Actions 产物验收 | 进行中 | 2026-08-27 |
| 2 | 正式工程骨架与双平台持续集成 | 已通过 | 2026-08-27 |
| 3 | 跨平台角色窗口最小纵切 | 进行中 | 2026-08-27 |
| 4 | 动画、方向映射与自主行为 | 进行中 | 2026-08-27 |
| 5 | 统一事件调度、单击、拖动与投喂 | 进行中 | 2026-08-27 |
| 6 | 气泡视觉原型与正式对话系统 | 进行中 | 2026-08-27 |
| 7 | 完整控制面、设置与桌面集成 | 未开始 | — |
| 8 | 会话生命周期、诊断与可靠性 | 未开始 | — |
| 9 | 打包、候选版验收与 1.0 发布 | 未开始 | — |

## 阶段 0

状态：进行中

已完成：

- `docs/Plans/WindowsAndCI.md` 已加历史提案说明。
- 本状态记录已建立。
- `.gitignore` 已确认排除 `Reference/`、`Temp/`，并补充了 `build/`、`build-*/`、`out/`、`CMakeUserPresets.json` 等构建与编辑器产物。`third_party/` 未被忽略，将在阶段 2 正式纳入版本控制。
- 素材基线校验通过：`assets/MANIFEST.md` 登记 22 个文件，磁盘上 22 个 PNG，路径一一对应，全部 SHA-256 与实际文件一致，无未登记文件、无失效条目。

未完成：

- 计划本身仍需项目所有者最终确认后才能把本阶段填为 `已通过`。

## 阶段 1

状态：进行中

已完成（工程侧）：

- `probes/window/` 已从 `Temp/qt-window-probe/` 抽取为受 Git 跟踪、与产品代码隔离的探针，含独立 `CMakeLists.txt`、`README.md` 和 `WindowsChecklist.md`。`Temp/qt-window-probe/` 保留为本地实验记录，未删除。
- 探针默认素材改为仓库内 `assets/character/idle-down-left.png`，构建后与安装后都会放在可执行文件旁的 `assets/character/`，不再依赖被忽略的 `Reference/`。
- 拖动拆成 `drag-system` 与 `drag-manual` 两个互不回退的用例。
- 穿透拆成 `passthrough-qt` 与 `passthrough-native` 两个独立用例。
- 新增 Windows 所需检查入口：`focus`、`windowlist`、`hittest`、`dpi`、`lifecycle`。
- 探针同时写日志文件，方便把完整输出附回结果文档。
- `.github/workflows/probe-windows.yml` 已建立：`windows-2022` + MSVC 2022 + Ninja + Qt `6.11.2`（`win64_msvc2022_64`），Qt 安装 Action 锁定完整提交 SHA `48d3ad6db93f3627c8ee7a0454bc6f3744f7e730`，只构建 `probes/window/`，跑参数校验与 offscreen 冒烟，`windeployqt` 打包并上传带 commit 与 SHA-256 的 artifact。
- `docs/WindowsFeasibilityResults.md` 已建立，结构与 Linux 结果文档对称，全部条目当前为“未实测”。

Linux 容器内的实际验证（`dev-fedora`，Qt 6.11.1，GCC 16.1.1）：

- 配置与编译成功，无编译警告。
- 15 个用例的 offscreen 冒烟全部以退出码 0 结束。
- 未知用例、非法倍率、非法时长返回 2；素材缺失、加载失败、尺寸非法返回 3。
- `hittest` 的 alpha 掩码在 2× 下生成 98 个矩形，包围盒 `38,44,72,178`，位于 `138x222` 窗口内。
- `animate` 的 100 ms 定时实测最小 99 ms、最大 101 ms、平均 100.00 ms。
- `cmake --install` 后的目录布局可直接运行，默认素材路径解析正确。

offscreen 只证明代码路径和计时逻辑可执行，不能代表真实桌面行为，也不能代表 Windows 行为。

首次 Actions 运行（run 32992591398）失败，已定位并修复：

- 现象：`Install Qt 6.11.2` 步骤报 `Failed to locate XML data for Qt version '6.11.2'`，请求的是 `qt6_6112/qt6_6112/Updates.xml`。
- 原因：Qt 官方存档在 `6.11.0` 改了目录结构。`6.10.1` 及更早是 `qt6_XXX/qt6_XXX/Updates.xml`，`6.11.0` 起改为按架构分目录的 `qt6_XXXX/qt6_XXXX_<arch>/Updates.xml`。已逐版本核对：`693`、`6100`、`6101` 只有旧结构，`6110`、`6111`、`6112` 只有新结构。
- 根因位置：`aqtinstall` 在 PyPI 上的最新发布版是 `3.3.0`（2025-06-02），只认旧结构。上游已在 master 合入 `version >= Version("6.11.0")` 的分支处理（issue #959 与 #1000），但 `3.4.0` 尚未发布。
- 修复：工作流改用 `aqtsource` 从 Git 安装 aqtinstall，锁定完整提交 SHA `16db45a70b5905ad596941b223469bc86a56901e`，不使用浮动 master。`3.4.0` 发布后应改回 `aqtversion` 固定版本号。
- 该问题与探针代码、CMake 和 MSVC 无关，属于 Qt 取包路径问题。

第二次失败（run 32993271199）与第三次成功（run 32993815259）：

- 第二次失败在「参数校验」步骤，但六条断言全部通过。原因是该步骤刻意让探针以非零码退出，最后一条断言留下的 `$LASTEXITCODE` 成了整个步骤的退出码。三个 PowerShell 步骤统一改为收集失败项并显式 `exit 0`／`exit 1`，同时关掉 PowerShell 7 默认把非零原生退出码当成错误的行为。
- 同时去掉了 `push` 与 `pull_request` 的 `paths` 过滤。新建分支的首次 push 不匹配 `paths`，前两次都没触发工作流；去掉后 push 直接触发成功。`push` 限定 `main`，避免同仓库 PR 分支跑两遍。

Windows CI 首次成功（run 32993815259，commit `acf10c40`，耗时 1 分 19 秒）：

| 检查 | 结果 |
| --- | --- |
| MSVC 2022 配置与编译 | 通过，无 MSVC 警告 |
| 实际 Qt 版本等于 `6.11.2` | 通过，`qmake -query` 报告 `6.11.2` |
| 参数校验退出码 | 通过，6 条断言全部符合预期 |
| 全部用例的 offscreen 冒烟 | 通过，15 个用例全部退出 0 |
| `windeployqt` 后的可执行文件能加载随包素材 | 通过 |

工具链实测版本：Qt `6.11.2`、CMake `3.31.6`、Ninja `1.13.2`、MSVC 工具集 `14.44.35207`、Windows SDK `10.0.26100.0`。

打包问题（已修并验证）：

- `windeployqt --compiler-runtime` 复制的是 25 MB 的 `vc_redist.x64.exe` 安装器，不是运行库本身，包内没有任何 `msvcp140.dll`／`vcruntime140*.dll`。没装过 VC++ 运行库的机器仍然起不来，测试者还得先手动跑安装器。
- 改为直接把运行库 DLL 放在可执行文件旁做 app-local 部署，并断言三个必需 DLL 存在。
- 只去掉 `--compiler-runtime` 不够：windeployqt 检测到 Visual Studio 环境时默认就带运行库，必须显式写 `--no-compiler-runtime`。已另加一条断言，包内不得出现 `vc_redist.x64.exe`。
- ZIP 从 56 018 347 字节降到 30 618 336 字节。

可交付给项目所有者测试的产物（run 32995272648，commit `fc689995`）：

- artifact：`deskpet-probe-windows-x64-fc689995fbf9`
- ZIP SHA-256：`c335de582b6b06e8c64d94a94d4db3b084314a8484fed750293176bf26adb8f0`
- 检查表：包内 `WindowsChecklist.md`，结果填入 `docs/WindowsFeasibilityResults.md`

### Windows 实测（2026-08-27）

项目所有者用 run 33001629207 的探针包（commit `017fd14e`，SHA-256 已核对）在 150% 缩放的单显示器上运行了 10 个用例。完整结果见 `docs/WindowsFeasibilityResults.md`。

通过的核心能力：帧间隔（596 采样，平均 100.00 ms）、自主移动（132/132 请求与实际位置一致）、透明背景、像素锐利、置顶、不进任务栏、启动不抢焦点、原生穿透。

复测后补齐了 `hittest`、`drag-system`、`drag-manual`、`lifecycle` 与 `passthrough-qt`，14 个用例全部核心能力通过，**没有任何失败项**：

- `hittest`：149 次角色点击**全部** `sprite_alpha=255`，另有 18 次从透明区穿到下层。掩码 98 个矩形，包围盒 `38,44,72,178` 明显小于 `138x222` 窗口。`docs/Decisions.md` 第 3.4 节在 Windows 上成立。
- `drag-manual`：11 702 次移动，请求与实际位置 **0 / 11 730** 不一致。
- `drag-system`：5 次请求全部 `accepted=true`。
- `lifecycle`：`window_close` → `exit`，干净退出。

**窗口重建问题没有复现。** 两条穿透路径与 `topmost` 的所有切换都是 `rebuilt=false`。计划 6.1 设想的「复现窗口重建后才引入 Win32」这个触发条件不成立。

**首轮对 `passthrough-qt` 的判断是错的。** 首轮只记到 1 次角色点击、0 次穿透，与 `passthrough-native` 的 43/49 形成 92 比 1，据此曾初步判断 Qt 路径无效。复测数据是 135/122 与 189/181，并且逐段核对开关与点击的对应关系，两条路径全程正确。首轮异常来自测试环境（下层点击靶很可能被 PowerShell 遮住，且那轮落在角色上的点击远少于复测轮）。教训：单看总数不足以判定，必须核对分段对应关系。

**因此 Windows 穿透采用纯 Qt 路径**，按 `docs/Decisions.md` 第 8.4 节「优先使用 Qt」。待办：删除 `WindowsWindowBackend` 的 `PassthroughStrategy` 与 `MUB_WIN_PASSTHROUGH` 环境变量，只保留继承自 `QtWindowBackend` 的实现；其中补齐 `WS_EX_TOOLWINDOW` 与 `WS_EX_NOACTIVATE` 的部分保留，与穿透路径无关。

**像素锐利的条件是乘积为整数。** 150% 缩放下 `--scale 2` 锐利，是因为 `2 x 1.50 = 3` 正好是整数物理倍率。实际生效的是 `项目倍率 x 系统 DPR`：125% 下没有任何整数倍率能得到整数物理倍率，150% 下只有偶数倍率可以。这是 `docs/Decisions.md` 第 13 节「完整的整数显示倍率集合」的直接输入，本轮不下结论。

### 退出门

计划第 6 节的三条退出门条件，按本轮数据均已满足：

| 条件 | 判定 |
| --- | --- |
| 透明、动画、自主移动、拖动、像素级输入、置顶、焦点和窗口列表表现均有实际结果 | 满足 |
| 核心失败已定位到具体路径并验证解决方案，不以“预计 Win32 可行”通过 | 满足，本轮无核心失败 |
| 任一核心能力仍失败则停止正式产品开发 | 不触发 |

状态仍填 `进行中` 而非 `已通过`：本记录的规则是「需要项目所有者人工验收的阶段，未收到验收结论前不得填 `已通过`」。数据已齐备，等项目所有者给出验收结论后改。

未完成：

- 100%／125%／200% 三档缩放、`Alt+Tab` 目视确认、置顶关闭态、点击与拖动时的焦点、动画帧序、锁屏与睡眠恢复。全部属于补档位或补单独确认，不涉及未知的能力风险。清单见 `docs/WindowsFeasibilityResults.md` 第 8 节。
- 穿透路径的代码清理（删除原生候选）尚未执行。

### 阶段顺序偏离（2026-08-27，项目所有者决定）

计划第 7 节写的是「阶段 2 前置条件：阶段 1 通过」，第 17 节写的是「不在 Windows 探针结果出来前并行编写正式角色状态机、气泡或设置界面」。
项目所有者以“去 Windows 测试比较麻烦”为由决定先继续后续阶段。

本记录据此调整，调整范围和理由如下：

- 阶段 1 **不填 `已通过`**，退出门维持未判定，`docs/WindowsFeasibilityResults.md` 第 3 至 8 节维持未实测。
- 阶段 2 解除阻塞。该阶段只建立构建体系、CI、许可与元数据，不含任何窗口行为，与探针结果无关。
- 阶段 3 的第 8.1 节「Windows 根据阶段 1 结果实现最小 Win32 边界」**继续挂起**。原阻断规则的真实目的是不让产品建立在未验证的平台假设上，该目的只落在这一条上。
- 阶段 3 的其余部分、阶段 4、5、6 及阶段 7 的多数内容可以推进：这些是纯逻辑或 Linux 侧可验证的内容，在容器内有确定性测试覆盖。
- 一旦取得 Windows 探针结果，先回填 `docs/WindowsFeasibilityResults.md` 并判定阶段 1 退出门，再实现 Windows 平台层。若届时核心能力失败，按计划第 6 节停止并重新形成假设，已完成的纯逻辑部分不因此免于返工。
- `docs/WindowsFeasibilityResults.md` 全部结果仍为未实测。
- 退出门未判定。阶段 2 不得开始。

一处需要项目所有者知情的判断：

- 计划 6.1 写的是“只有复现窗口重建问题后才引入 Win32 实现”。本轮已经把 `passthrough-native` 的 Win32 实现（只改 `WS_EX_TRANSPARENT`）一起放进探针，理由是它是独立用例，不参与 `passthrough-qt` 的纯 Qt 基线，而项目所有者在另一台机器上测试，每多一轮都要重新出包。若认为应当严格按原文先只测纯 Qt 路径，删掉该用例即可，其余部分不受影响。

## 阶段 2

状态：已通过

已完成并在 `dev-fedora` 内实测（Qt 6.11.1、GCC 16.1.1）：

- 根 `CMakeLists.txt`、`CMakePresets.json`（`dev`／`debug`／`release`／`ci` 四组预设）与计划第 4 节的目录结构。
- C++20、Qt API 基线 `6.11`、CMake、Ninja 的统一入口。`QT_DISABLE_DEPRECATED_UP_TO=0x061100` 使基线之前被弃用的 Qt API 直接编译失败。
- 严格警告配置 `cmake/ProjectWarnings.cmake`，GCC／Clang 与 MSVC 各自一套等价警告，默认按错误处理。GCC 下当前零警告通过。
- 两个正式目标：`money-under-bed-core`（纯逻辑静态库，只链接 `Qt6::Core`）与 `money-under-bed-deskpet`（GUI 可执行文件，Windows 用 GUI 子系统）。`ui/` 把源文件加到可执行目标上，保留目录职责边界但不额外拆库。
- 应用元数据集中在 `src/core/AppMetadata.*`，取值由 `src/core/Version.h.in` 在配置期注入，任何地方都不再硬编码。`tst_appmetadata` 锁住这些取值，改动必须同时改决策文档。
- 界面文本一律走 `QCoreApplication::translate`，不提供语言设置。
- 根 `LICENSE`（GPL-3.0 完整文本）、`packaging/LICENSES.md`（发行物许可清单模板）、根 `README.md`（含非官方声明与四类内容各自的许可）。
- Ark Pixel `2026.08.11` 作为单一来源编入 Qt 资源系统。配置期核对 SHA-256，不符直接 `FATAL_ERROR`；`tst_resources` 另外校验资源系统内那一份的哈希，并断言仓库中只存在一份 TTF。
- CTest 入口与两个测试目标，共 28 条断言：应用身份、字体哈希、许可文件存在性、11 个角色精灵表的 PNG 尺寸与帧数、11 个表情文件、以及 `assets/` 内每个 PNG 都登记在 `MANIFEST.md`。
- `.github/workflows/build.yml`：Linux（`ubuntu-22.04`，显式安装 XCB 构建依赖）与 Windows（`windows-2022`，MSVC 2022）两个 job，每次 push 与 pull request 配置、编译、跑 CTest 并执行 `--self-test`。

本地实测结果：`cmake --preset ci` 配置成功，编译零警告，`ctest --preset ci` 2 个测试全过，`--self-test` 退出码 0。

说明：

- `Network` 与 `DBus` 已在配置期 `find_package`，但尚未链接到任何目标。前者用于阶段 7 的本地 IPC，后者用于阶段 8 的 Linux 会话状态检测；在有使用者之前不链接，避免无谓依赖。
- Windows 的 `--self-test` 用 `Start-Process -Wait` 取退出码。GUI 子系统程序直接在 shell 里调用不会等待进程结束，拿到的退出码不可信。

双平台 CI 首次运行即通过（run 32996844709，commit `7ee7e81`）：

| 检查 | Linux（ubuntu-22.04，GCC） | Windows（windows-2022，MSVC 2022） |
| --- | --- | --- |
| 干净检出配置与编译 | 通过，1 分 58 秒 | 通过，1 分 25 秒 |
| 编译警告 | 零警告（`-Werror`） | 零警告（`/WX`） |
| 实际 Qt 版本 | `6.11.2` | `6.11.2` |
| CTest | 2 个测试全过 | 2 个测试全过 |
| `--self-test` 退出码 | 0 | 0 |

MSVC 的严格警告集合是首次在本项目验证，`/WX` 打开后一次通过，没有需要例外的条目。

退出门判定：

- 根工程在 `dev-fedora` 与 Windows Actions 均无警告构建。通过。
- CI 不依赖 `Reference/`、`Temp/` 或宿主已安装字体。通过：字体来自 `third_party/` 并编入 Qt 资源系统，素材来自 `assets/`。
- 后续阶段可以只向正式目标增量添加功能，不再改换构建体系。通过。

状态：已通过。

## 阶段 3

状态：进行中

### Windows 实现策略（2026-08-27，项目所有者决定）

原计划第 8.1 节「Windows 根据阶段 1 结果实现最小 Win32 边界」在上一条偏离记录里被挂起。
项目所有者认为分叉面小、改动容易，决定 Windows 部分跟着一起实现，只要 CI 能过即可，
成品出来后做一次综合校验。

据此调整：

- Windows 平台层不再挂起，与 Linux 同步实现。
- 判定标准降为「双平台编译通过、自动测试通过」，不再要求先有 Windows 探针结果。
- 阶段 3 的退出门仍不判定：退出门要求「正式产品窗口在 KDE 与 Windows 都达到探针已验证的核心表现」，那需要真实桌面人工验收。
- 前提条件仍然成立：平台相关能力必须集中在窄接口内（`docs/Decisions.md` 第 8.4 节）。分叉只允许出现在接口实现里，不得散入动画、行为和角色逻辑。
- 有不确定取舍的地方实现两条路径并可在运行时切换，使一次人工校验就能覆盖两种可能，而不是错了再出一次包。当前唯一这样的取舍是 Windows 的整窗穿透：`MUB_WIN_PASSTHROUGH=qt`（默认）走 Qt 窗口标志，`MUB_WIN_PASSTHROUGH=native` 只改 `WS_EX_TRANSPARENT`。默认选 Qt 路径符合第 8.4 节「优先使用 Qt」。
- `docs/WindowsFeasibilityResults.md` 继续如实标记未实测。凡是未经实测就写进产品的 Windows 行为，都记为假设而不是结论。

### 已完成

窄平台接口 `src/platform/`：

- `DeskPetWindowBackend` 是唯一允许出现平台分叉的地方，方法只接受 `QWindow`，不依赖 Qt Widgets。
- `BackendCapabilities` 让 UI 依据后端自述决定降级路径，而不是自己判断运行在哪个系统上。
- `QtWindowBackend` 是纯 Qt 默认实现；`WindowsWindowBackend` 继承它，只覆盖确实需要平台调用的部分。
- `BackendFactory.cpp` 是产品代码中唯一按操作系统分支的文件。
- 启动探测按平台分文件：`LinuxStartupProbe.cpp` 直接链接 libxcb，`GenericStartupProbe.cpp` 用于其他平台。

Linux 启动路径（`docs/Decisions.md` 第 8.2 节）：

- 把 `QT_QPA_PLATFORM` 设为单值 `xcb`，不使用候选列表。例外只有 `offscreen` 与 `minimal` 两个无头测试平台，它们不是桌面回退路径。
- 构造 `QApplication` 之前调用 `xcb_connect` 探测连接。
- 探测失败时不以 XCB 构造 `QApplication`；存在 Wayland 会话则用 Wayland 后端只弹一个说明原因的错误对话框，以退出码 3 结束。没有 Wayland 会话时只写 stderr 与日志，避免 Qt 在构造期 `qFatal` 连说明都留不下。
- 消息处理器在构造 `QApplication` 之前安装，保证平台插件加载失败时至少留下记录。

角色窗口 `src/ui/CharacterWindow`：

- 透明、无边框、不进任务栏、默认置顶、显示时不激活。
- 按帧 alpha 生成命中区域交给平台层，可见像素接收交互，透明区域穿透。
- 整数倍最近邻绘制，禁用平滑插值。
- 启动定位到鼠标所在屏幕可用区域底部，不恢复上次位置。
- 点击与拖动由 `GestureRecognizer` 区分，只发信号不做反馈。拖动用手动移动而非 `startSystemDrag`：由窗口管理器接管后收不到松开事件，就无法按第 3.1 节判断松手位置离底部多远。接口仍保留 `startSystemDrag`。

素材与自检：

- `assets/` 全部 22 个 PNG 编入 Qt 资源系统，保持原相对布局。
- `CharacterAssets.cpp` 是精灵表的显式登记表，逻辑标识与资源路径分开维护，不从文件名推断语义。
- `--self-test` 校验 11 张精灵表的帧高、帧宽倍数、帧数与透明通道，以及对话字体能否注册，结果以退出码为准。
- 早期日志 `DiagnosticLog` 写 stderr 与本地文件，两文件轮转。完整轮转策略与隐私检查在阶段 8。

本地实测（`dev-fedora`，Qt 6.11.1，GCC 16.1.1）：编译零警告，7 个测试目标共 121 条断言全过，`--self-test` 退出码 0。

新增测试：

| 目标 | 断言 | 覆盖 |
| --- | --- | --- |
| `tst_spritesheet` | 31 | 合法尺寸、6 类非法尺寸、越界取帧、11 张登记素材的实际帧数 |
| `tst_hitmask` | 12 | 全透明、全不透明、半透明、1×–4× 缩放、alpha 阈值、非法输入、真实素材角落穿透 |
| `tst_screenplacement` | 13 | 底部锚定、横向比例、偏移屏幕、边距、夹取、可用区小于窗口 |
| `tst_gesture` | 14 | 阈值边界、无中间移动事件的快速拖动、拖出再拖回、取消、对角线 |
| `tst_characterwindow` | 17 | 1×–4× 窗口尺寸、后端只配置一次、置顶转发、掩码与帧 alpha 一致、换帧重算掩码、最近邻放大逐像素校验、后端不支持掩码时不调用 |

双平台 CI 通过（run 32999324988，commit `179c6ef`）：

| 检查 | Linux（ubuntu-22.04，GCC） | Windows（windows-2022，MSVC 2022） |
| --- | --- | --- |
| 配置与编译 | 通过，1 分 17 秒 | 通过，1 分 26 秒 |
| 编译警告 | 零（`-Werror`） | 零（`/WX`） |
| 平台专用源文件 | `LinuxStartupProbe.cpp` 已编译 | `WindowsWindowBackend.cpp` 已编译 |
| CTest | 7/7 | 7/7 |
| `--self-test` | 通过，退出码 0 | 通过，退出码 0 |

`WindowsWindowBackend.cpp` 是本项目第一份 Win32 代码，`/WX` 打开后首次编译即无警告。

未完成：

- 人工检查（KDE Plasma + XCB、Windows 11）未做。Windows 侧的实际窗口行为仍然全部是假设，不是结论。
- 退出门不判定：要求「正式产品窗口在 KDE 与 Windows 都达到探针已验证的核心表现」，需要真实桌面人工验收。

## 阶段 4

状态：进行中

### 已完成

注入式时钟与随机（`src/core/`）：

- `TimeSource`：`MonotonicTimeSource` 用于产品，`ManualTimeSource` 用于测试。
- `RandomSource`：`SeededRandomSource` 同种子同序列，`ScriptedRandomSource` 用固定序列精确构造分支，不靠概率碰运气。`chance()` 在 0 与 100 两端不消耗随机数，使「必然发生」的测试不打乱后续序列。

动画（`src/character/`）：

- `AnimationClip.cpp` 是显式登记表，11 段动画各自登记资源路径、帧数、帧时长和循环方式。帧时长是待调优的内部参数。
- `AnimationPlayer` 只管帧范围、循环方式和时钟，不选择播放哪段动画。
- 暂停与恢复不补播；未显式暂停但时间跳变超过 2 秒时同样只前进一帧，覆盖进程被挂起的情况。

方向映射（`src/character/Direction`）：

- 四象限映射、停止保持最后朝向、纯水平回正面、纯垂直沿用左右朝向。
- 死区加切换滞后：维持当前朝向只需越过死区，改变朝向还要再越过滞后量。
- `spriteIdFor()` 由运动状态与朝向合成逻辑标识，八种组合都由测试确认在登记表中存在。

自主行为（`src/core/AutonomousBehavior`）：

- 状态：`Idle`、`Walking`、`Resting`、`ApproachingCursor`、`ReturningToBottom`、`HeldByUser`。没有饥饿、心情、亲密度或长期记忆。
- 休息不依赖新素材，只是停止移动并延长停留时间。
- 安静模式不接近鼠标、不请求闲聊；活跃模式才允许，且停在安全距离外。切到安静模式会立刻中止正在进行的接近动作。
- 运行期暂停冻结移动与行为，恢复时把状态截止时间一并后移，不会一恢复就立刻切换状态。
- 拖动冻结自主行为；松手靠近底部就留在原地，远离底部则先停留再返回。
- 拖到另一块屏幕后由外部更新活动区域，多显示器按 best-effort 实现，不伪造人工通过状态。

接线（`src/ui/CharacterPresenter`）：

- 约 60 Hz 节拍驱动行为与动画。移动平滑度由节拍决定，动画帧率由 `AnimationClip` 决定，两者互不影响。
- 精灵表按需加载并缓存；朝向变化时换表并重启动画。
- 本类只做接线，不自己判断行为，也不含平台分支。

### 本地实测

`dev-fedora`，Qt 6.11.1，GCC 16.1.1：编译零警告，10 个测试目标共 193 条断言全过。

新增测试：

| 目标 | 断言 | 覆盖 |
| --- | --- | --- |
| `tst_direction` | 31 | 四象限、死区边界、纯水平回正面、纯垂直沿用朝向、滞后带内不转身、八种组合都有素材 |
| `tst_animationplayer` | 23 | 帧推进与余量累积、循环计数、播完保持、暂停冻结、恢复不补播、时间跳变只前进一帧 |
| `tst_autonomousbehavior` | 18 | 5 分钟连续运行不越界、暂停冻结、安静模式 10 分钟内从不接近鼠标也不请求闲聊、接近鼠标停在安全距离外、拖动与返回路径、跨屏更新、时间跳变不补算、同种子序列可重复、不同种子必然分叉 |

`sameSeedProducesTheSameSequence` 与 `differentSeedsDiverge` 成对存在：前者证明可重复，后者证明相等不是因为角色根本没动。

### 双平台 CI

run 33000709190，commit `e9f3566`：Linux 2 分 13 秒、Windows 1 分 34 秒，均零警告，各 10/10。

`tst_autonomousbehavior` 在 MSVC 与 GCC 上都通过，说明固定种子的可重复性是跨平台成立的，不只是单一编译器的巧合。

### 未完成

- 退出门第二条「KDE 与 Windows 上连续运行基础自主行为无越界、抖动或窗口位置漂移」需要真实桌面观察，尚未判定。第一条「假时钟与固定随机种子下行为序列可重复」已由 `tst_autonomousbehavior` 覆盖。

## 阶段 5

状态：进行中

### 已完成

事件协调器 `src/core/EventCoordinator`：

- `EventKind` 的枚举值本身就是优先级，顺序由 `docs/Decisions.md` 第 4.2 节冻结：退出／隐藏 > 投喂 > 连续对话 > 单击反馈 > 自主闲聊。改顺序等于改产品行为。
- 裁决只有三种：接受、替换、抑制。**没有队列** —— 被抑制或替换的事件不会在稍后补播。
- 同类事件默认抑制，只有单击反馈允许从头重来：用户连续点击应当得到新反馈，而不是被自己上一次点击挡住；投喂则按第 3.2 节明确忽略，且没有冷却。
- `finish(kind)` 只清除正在进行的那一类，避免迟到的结束通知误清掉已经换上来的更高优先级事件。
- 对外暴露 `current()`、`lastDecision()`、`lastReplaced()`、`suppressedCount()`、`replacedCount()`，供日志与测试观察。

选择逻辑：

- `FeedingSelector` 只决定正常吃还是掉落。「投喂进行中忽略新请求」交给协调器统一裁决，不在两处各判一次。
- `ClickFeedbackSelector` 保证 `hasReaction` 恒为真；安静模式或气泡关闭时只是 `hasText` 为假，动作反馈仍在（第 3.1 节）。

接线：

- 所有行为请求都经过 `CharacterPresenter::requestEvent()`，并打日志记录请求、裁决、当前事件和被替换者。自主闲聊同样走协调器，不绕开优先级。
- 投喂期间事件独占角色：自主行为与方向映射都不参与，动画播完自动结束事件。掉落结束后立即请求连续对话，其优先级高于自主闲聊，因此闲聊会被协调器自动抑制。
- 用户暂停与事件冻结分开记账，事件结束不会覆盖用户的暂停设置。
- 角色右键菜单加了投喂的临时测试入口。同时加了退出项：无边框窗口没有关闭按钮，否则程序无法正常结束。完整菜单在阶段 7。

### 本地实测

`dev-fedora`，Qt 6.11.1，GCC 16.1.1：编译零警告，12 个测试目标共 274 条断言全过，`--self-test` 退出码 0。

| 目标 | 断言 | 覆盖 |
| --- | --- | --- |
| `tst_eventcoordinator` | 47 | 全部 25 种两两组合的裁决（期望值逐条写出，不由规则推算）、决策文档逐条规则、抑制与替换都不排队、迟到的结束通知不误清、连击重来、重复投喂被忽略且无冷却、事件风暴收敛到最高优先级 |
| `tst_feedback` | 30 | 掉落概率的边界与强制分支、安静模式与气泡关闭下仍有动作反馈但无文字、低频与正常两档确实不同 |

### 未完成

- 双平台 CI 结果待确认。
- 计划第 10.1 节「隐藏清除当前对话页面」的菜单项属于阶段 7；协调器侧已具备。
- 退出门第二条「快速连续点击、投喂和自主触发不会产生重叠动画、滞后台词或不可恢复状态」需要真实桌面观察，尚未判定。

## 阶段 6

状态：进行中

本阶段有一道必须等待项目所有者的检查点：计划第 11.1 节要求「项目所有者审核并冻结具体渲染参数及是否采用 `1.5×`；结果写回 `docs/Decisions.md` 后才能实现正式气泡」。
因此先做与视觉参数无关的部分。

### 已完成

台词数据 `src/dialogue/DialogueData`：

- 26 段对话、**29 条来源台词、36 个显示页面**，与 `docs/Decisions.md` 第 4.5 节声明的数字一致。
- 内容由决策文档第 4.4 与 4.5 节的表格**程序化抽取**，不是手工转写，避免抄错。
- `tst_dialoguedata` 把每一页文本和每一条触发场景**回查决策文档**，任何一侧改动而另一侧没跟上都会失败。决策文档因此是台词的唯一来源。
- 逐条标记来源类别，实测原作 8 条、新增 21 条。
- 常规表情池 10 个，**不含 `shadow`**；另有测试断言表情池与实际用到的表情完全相等，多一个少一个都失败。

字体：

- 通过 Qt 应用字体接口从 Qt 资源系统加载，不依赖系统字体。
- **逐字覆盖检查通过**：36 个页面共 137 个不同字符，与第 4.7 节声明一致，`QRawFont::supportsCharacter` 全部返回真。

对话状态机 `src/dialogue/DialogueSession`：

- 打字速度取 25 ms／字符，落在第 4.1 节的 `20–30 ms` 区间内，最终值待原型审核。
- 打字中点击先补全、再次点击翻页、最后一页再次点击关闭。
- 翻页时同时切换该页指定的表情。
- 连续对话不自动翻页，20 秒无操作后整体结束，重新触发从第一页开始。
- 单页气泡在文字完成后自动消失；多页对话不受该时长影响。
- 纯逻辑，时间由注入的 `TimeSource` 提供。

### 本地实测

`dev-fedora`，Qt 6.11.1，GCC 16.1.1：编译零警告，14 个测试目标共 **479 条断言**全过，`--self-test` 退出码 0。

| 目标 | 断言 | 覆盖 |
| --- | --- | --- |
| `tst_dialoguedata` | 约 150 | 条数与页数、来源分组、每页文本与触发场景回查决策文档、表情素材存在、`shadow` 不在池内、137 字符数、字体逐字覆盖 |
| `tst_dialoguesession` | 约 55 | 逐字显示与不足一字符不变化、点击补全不翻页、翻页换表情、最后一页关闭、不自动翻页、20 秒超时与交互重置、单页自动消失且多页不受影响、重新触发回到第一页、26 段对话逐段完整走通 |

### 未完成

- 双平台 CI 结果待确认。
- 计划第 11.1 节的正式 Qt 交互原型（边缘避让、拖动跟随、长短台词切换、`1.5×` 复核）未做。
- **等待项目所有者冻结渲染参数**，在那之前不实现正式气泡的视觉部分。
- 第 11.3 节中与布局相关的条目（边缘避让、面板跟随、点击区域）未做。

## Linux 侧已有结果

`docs/FeasibilityResults.md` 记录的 Linux 结论在本计划中继续有效，不重新测试：

- KDE Plasma + XCB/XWayland：透明、动画、自主移动、拖动、穿透、置顶全部通过。
- KDE Plasma + Wayland：自主移动与置顶失败，拖动有残影。
- niri：Wayland 与 XCB/xwayland-satellite 均不支持普通顶层窗口自主定位。
