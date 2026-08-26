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
| 2 | 正式工程骨架与双平台持续集成 | 未开始 | — |
| 3 | 跨平台角色窗口最小纵切 | 未开始 | — |
| 4 | 动画、方向映射与自主行为 | 未开始 | — |
| 5 | 统一事件调度、单击、拖动与投喂 | 未开始 | — |
| 6 | 气泡视觉原型与正式对话系统 | 未开始 | — |
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

打包问题（已修，待下一轮验证）：

- `windeployqt --compiler-runtime` 复制的是 25 MB 的 `vc_redist.x64.exe` 安装器，不是运行库本身，包内没有任何 `msvcp140.dll`／`vcruntime140*.dll`。没装过 VC++ 运行库的机器仍然起不来。已改为直接把运行库 DLL 放在可执行文件旁（app-local 部署），并断言三个必需 DLL 存在。ZIP 体积从 55 MB 降到约 30 MB。

未完成：

- 上述打包修复的运行结果待确认。
- 项目所有者尚未在 Windows 11 上运行探针 ZIP。
- `docs/WindowsFeasibilityResults.md` 全部结果仍为未实测。
- 退出门未判定。阶段 2 不得开始。

一处需要项目所有者知情的判断：

- 计划 6.1 写的是“只有复现窗口重建问题后才引入 Win32 实现”。本轮已经把 `passthrough-native` 的 Win32 实现（只改 `WS_EX_TRANSPARENT`）一起放进探针，理由是它是独立用例，不参与 `passthrough-qt` 的纯 Qt 基线，而项目所有者在另一台机器上测试，每多一轮都要重新出包。若认为应当严格按原文先只测纯 Qt 路径，删掉该用例即可，其余部分不受影响。

## Linux 侧已有结果

`docs/FeasibilityResults.md` 记录的 Linux 结论在本计划中继续有效，不重新测试：

- KDE Plasma + XCB/XWayland：透明、动画、自主移动、拖动、穿透、置顶全部通过。
- KDE Plasma + Wayland：自主移动与置顶失败，拖动有残影。
- niri：Wayland 与 XCB/xwayland-satellite 均不支持普通顶层窗口自主定位。
