# 发行物许可清单模板

本文是模板。每个发行包（Linux AppImage 与 Windows 免安装 ZIP）都必须携带
本清单所列的全部文件，并在 Release 说明中重复其要点。

对应 `docs/Decisions.md` 第 10.1 节与第 12 节。

打包脚本在阶段 9 加入。在那之前，本文的作用是让许可要求先于打包实现固定下来。

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

对话字体的 TTF 编译进 Qt 资源系统，发行目录不再单独放一份，
以免出现同一字体的两个副本（`docs/Decisions.md` 第 4.7 节）。
但 OFL 要求的版权声明和完整许可文本仍必须随包分发。

## Release 说明必须包含的声明

- 非官方、非商业二次创作项目，与《床下有罐钱》的开发者没有隶属关系，也不由其发布或背书。
- 角色素材来自作者 `_U5B_` 发布的二创素材包，附原视频链接与素材条款要点。
- 使用角色素材的发行版本必须保持非商业。
- 对话字体为 Ark Pixel，采用 OFL-1.1，附版权声明。
- Windows 包未签名，可能触发 SmartScreen 警告，也可能被 Smart App Control 或企业策略直接阻止。
- 支持目标、实际验证配置、实验性环境和未测试范围分别列出，不笼统宣称已全面验证。

## 打包时的检查项

- [ ] 上表五个文件全部存在且非空。
- [ ] 包内不存在第二份 Ark Pixel TTF。
- [ ] 包内素材与 `assets/MANIFEST.md` 一致。
- [ ] Release 说明包含上一节全部声明。
