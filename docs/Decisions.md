# MoneyUnderBed DeskPet 决策记录

最后更新：2026-10-02

本文只记录已经由项目所有者明确确认的现行决策。

## 1. 文档状态

- 最初版本的决策记录（截至 `1.1.0`）已移到 [`docs/legacy/Decisions.md`](legacy/Decisions.md)。它已经过期，只作为历史记录保留，不再修改。
- 本文是现行决策。旧文档中没有被本文取代的条款，描述的是 `1.1.0` 时的实现与约束，可以作为参考；两者冲突时以本文为准。
- 代码注释和测试引用旧文档时写明 `docs/legacy/Decisions.md`；注释里不带路径的「第 x 节」同样指旧文档。
- 已经确认、但评估后影响不大而暂不修复的问题记录在 [`docs/known_issue.md`](known_issue.md)。

## 2. 界面语言

取代旧文档第 5.1 节「第一版只提供简体中文界面」一条，以及第 14.2 节对它的重申。

- 界面文本的源文本是简体中文，一律走 `tr()` 或 `QCoreApplication::translate()`，不硬编码。
- 系统首选界面语言的第一项是中文时显示简体中文；其他任何语言，包括 C locale 和未设置语言，一律显示英文。
- 不提供语言设置项。
- 角色台词不属于界面文本，始终显示中文原文，不翻译。
- 英文译文保存在 `resources/translations/money-under-bed-deskpet_en.ts`，构建时编成 `.qm` 嵌入程序资源。界面文本改动后运行 `update_translations` 构建目标提取新文本，再补齐译文；自动测试拒绝任何未翻译的条目。
- 起因：上架 AppImageHub（第 3 节）要求程序在英文或未设置语言的环境下显示英文。

## 3. 上架 AppImageHub

- 接受 AppImageHub 自动发现后提交的收录 PR（[AppImage/appimage.github.io#6093](https://github.com/AppImage/appimage.github.io/pull/6093)）。
- 该收录测试读取最新 Release 里的 AppImage，在 C locale 下启动并截图，截图文字必须以英文为主（第 2 节）。因此本文的改动要随新版本发布之后才能重新测试。

## 4. 发布物

- AppImage 文件名改为 `MoneyUnderBed-DeskPet-<版本>-x86_64.AppImage`，不再含 `linux`，以通过 AppImageHub 的命名检查。Windows ZIP 文件名不变：`MoneyUnderBed-DeskPet-windows-x86_64-<版本>.zip`。
- AppImage 附带 AppStream 元数据 `usr/share/metainfo/io.github.hz_tyzq.MoneyUnderBedDeskPet.appdata.xml`，源文件在 `packaging/linux/`。用 `.appdata.xml` 而不是 `.metainfo.xml`，因为 AppImageHub 的检查脚本只识别前者。
- 元数据的许可表达式为 `GPL-3.0-or-later AND OFL-1.1 AND LicenseRef-proprietary=<素材条款链接>`：角色素材不属于 GPL，按其单独条款发行（旧文档第 12 节），因此单独列出。
- 只用于审计的产物清单（`.contents.txt`、`.elf-needed.txt`、`.dll-dependencies.txt`）不再生成，也不再随 Release 发布。包内的许可文件、`licenses/linux-runtime.tsv` 与对应源码 Release 保持不变。

## 5. 持续集成

取代 `1.1.0` 时 `Build and test` 与 `Package candidates` 两条工作流并存的安排。

- 只保留一个工作流 `.github/workflows/ci.yml`，顺序为：测试 → 打包并对实际产物自检 → 发布。
- 每次 push 和 pull request 都在 Linux 与 Windows 上构建 Release 配置、运行全部自动测试、打出候选包并对实际产物执行 `--self-test`。不再为了跑测试另外构建一份 RelWithDebInfo。
- 版本标签额外发布对应源码 Release 和二进制草稿 Release，流程与 `1.1.0` 相同。
- 打包步骤写在 `packaging/` 下的脚本里（`PackageAppImage.sh`、`PackageWindows.ps1`、`ArchiveCorrespondingSource.sh`）；打包工具与对应源码的版本锁定集中在 `packaging/pins.env`。Qt 与 aqtinstall 的版本锁定留在工作流里，因为安装 Qt 的 Action 要直接读取它们。
- 合规产物保持不变：对应源码 Release 的内容与发布流程、包内许可文件、运行库来源核对与 Ubuntu 源码包归档都与 `1.1.0` 相同。
