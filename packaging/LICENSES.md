# 发行物许可清单

每个发行包（Linux AppImage 与 Windows 免安装 ZIP）都必须携带本清单所列的
全部文件，并在 Release 说明中重复其要点。本文件以
`licenses/README.md` 路径随包分发。

对应 `docs/Decisions.md` 第 10.1 节与第 12 节。

> 阶段 9 审计状态：Qt、ICU、AppImage runtime、MSVC CRT 与 Linux 随包系统库
> 都已建立固定来源、许可材料和对应源码收集路径。只有新的 Actions 候选实际
> 产出并通过清单校验后，才可把审计标为完成；在此之前不得公开发行。

## 仓库全部内容不是同一个许可证

不能对外笼统宣称仓库全部内容均为 GPL。四类内容各自独立：

| 内容 | 许可 | 说明 |
| --- | --- | --- |
| 项目程序代码 | `GPL-3.0-or-later` | `Copyright (C) 2026 HZ-TYZQ` |
| 项目自行编写的文档 | `GPL-3.0-or-later` | 不另立文档许可证 |
| 第三方对话字体 | `OFL-1.1` | Ark Pixel `2026.08.11`，保持原许可，不重新授权 |
| 角色素材 | 作者二创条款 | 非商业、禁 R18、禁 AI 训练，不属于 GPL |

GPL 代码本身允许商业使用，但商业使用者必须移除或替换非商业的角色素材。

## 每个发行包必须包含的文件

| 发行包内路径 | 来源 | 内容 |
| --- | --- | --- |
| `LICENSE` | 仓库根 `LICENSE` | GPL-3.0 完整文本 |
| `licenses/OFL.txt` | `third_party/ark-pixel-font/OFL.txt` | OFL-1.1 完整文本 |
| `licenses/ark-pixel-font.md` | `third_party/ark-pixel-font/README.md` | 字体来源、版本、SHA-256 与版权声明 |
| `licenses/assets.md` | `assets/LICENSE.md` | 角色素材授权条款 |
| `licenses/assets-manifest.md` | `assets/MANIFEST.md` | 素材清单与哈希 |
| `licenses/qt-source.md` | `packaging/licenses/qt-source.md` | Qt GPL 路径、对应源码地址与固定哈希 |
| `licenses/qt-gpl-3.0-only.txt` | 仓库根 `LICENSE` 的 GPLv3 完整正文 | 本发行物选择的 Qt GPLv3 路径 |
| `licenses/qt-modules-6.11.2.spdx` | `packaging/licenses/qt-modules-6.11.2.spdx` | Qt Base、许可路径与对应源码哈希 |
| `licenses/appimage-runtime.txt`（仅 Linux） | `packaging/licenses/appimage-runtime.txt` | AppImage runtime 及其静态依赖声明 |
| `licenses/appimage-tooling.txt`（仅 Linux） | `packaging/licenses/appimage-tooling.txt` | AppRun 与 Qt 环境 hook 生成工具的 MIT 声明 |
| `licenses/icu.txt`（仅 Linux，构建时固定下载） | ICU 73.2 `icu4c/LICENSE` | Qt 官方 Linux 包携带的 ICU 完整声明 |
| `licenses/appimage-runtime/*`（仅 Linux，构建时固定下载） | 各 runtime 依赖上游 | libfuse、squashfuse、musl、zstd、zlib 完整许可文本 |
| `licenses/linux-runtime.tsv` 与 `licenses/linux-runtime/*.copyright`（仅 Linux） | Ubuntu 22.04 实际二进制包 | 随包系统库的精确版本、源包和 Debian copyright |
| `licenses/msvc-runtime.md`（仅 Windows） | `packaging/licenses/msvc-runtime.md` | app-local MSVC CRT 来源和微软再分发条款 |

对话字体的 TTF 编译进 Qt 资源系统，发行目录不再单独放一份，
以免出现同一字体的两个副本（`docs/Decisions.md` 第 4.7 节）。
但 OFL 要求的版权声明和完整许可文本仍必须随包分发。

角色 PNG 不编入 Qt 资源系统或 GPL 可执行文件。它们以原始只读数据文件
放在可执行文件旁的 `assets/character/` 与 `assets/face/`，并继续遵循
`licenses/assets.md` 中的单独授权；程序缺少这些文件时自检会失败。

Qt Base、ICU 73.2、固定 AppImage runtime、libfuse、squashfuse、
linuxdeploy 与 linuxdeploy-plugin-qt 的源码归档与每批候选一起保存。Linux 候选
还从实际 AppDir 反查 Ubuntu 二进制包，下载精确版本的 Debian 源码包作为独立
artifact。正式标签构建把这些材料全部附到同一草稿 Release 并核对 SHA-256。
项目使用动态链接，不禁止用户替换 Qt 动态库。

Windows 包不携带系统 DLL。app-local MSVC CRT 只从 runner 的
`%VCToolsRedistDir%` 复制未修改 release 文件；调试版与 `debug_nonredist` 均不进入
发行物。DirectX shader compiler、D3D compiler 与软件 OpenGL 不是本项目的运行
需求，已经从部署命令中排除，不为无用软依赖扩大再分发面。

## Release 说明必须包含的声明

- 非官方、非商业二次创作项目，与《床下有罐钱》的开发者没有隶属关系，也不由其发布或背书。
- 角色素材来自作者 `_U5B_` 发布的二创素材包，附原视频链接与素材条款要点。
- 使用角色素材的发行版本必须保持非商业。
- 对话字体为 Ark Pixel，采用 OFL-1.1，附版权声明。
- Windows 包未签名，可能触发 SmartScreen 警告，也可能被 Smart App Control 或企业策略直接阻止。
- 支持目标、实际验证配置、实验性环境和未测试范围分别列出，不笼统宣称已全面验证。

## 打包时的检查项

- [ ] 上表各平台适用的文件全部存在且非空。
- [ ] 包内不存在第二份 Ark Pixel TTF。
- [ ] 包内素材与 `assets/MANIFEST.md` 一致。
- [ ] Qt GPL 文本与构建所用 Qt Base SBOM 存在，Qt 对应源码归档哈希通过。
- [ ] Linux 每个随包非 Qt 库均映射到版本化 Ubuntu 源包、copyright 和同批源码 artifact。
- [ ] AppImage runtime 的完整静态依赖许可与 libfuse 对应源码存在。
- [ ] Windows CRT 只来自 `VC\Redist`，且 DirectX／软件 OpenGL 等未用组件不存在。
- [ ] Release 说明包含上一节全部声明。

每个 Actions 候选 artifact 还附带实际成品的逐文件 SHA-256 清单；Linux
另附全部 ELF 的 `DT_NEEDED` 清单，Windows 另附 EXE／DLL 导入清单。
平台运行库许可审计必须以这些成品清单为输入，不能只查看 runner 已安装包。
