# 《床下有罐钱》非官方桌宠

**开发中，尚未发布正式版本。当前正在生成并验收首批双平台候选包。**

一个把《床下有罐钱》的女主放在桌面上陪伴用户的桌宠程序。

## 非官方声明

本项目是**非官方、非商业**的二次创作，与《床下有罐钱》的开发者没有隶属关系，
也不由其发布或背书。请不要将本项目误认为官方产品。

## 角色素材

角色素材来自《床下有罐钱》作者 `_U5B_` 公开发布的二创素材包，
不属于本项目代码的 GPL 范围，条款见 [`assets/LICENSE.md`](assets/LICENSE.md)，
文件清单与哈希见 [`assets/MANIFEST.md`](assets/MANIFEST.md)。
发行包把这些 PNG 作为可执行文件旁的独立数据文件提供，不把它们编入 GPL
程序；素材与程序仍分别遵循各自条款。

要点：仅供二次创作、不可商用、不可制作 R18 内容、禁止用于 AI 训练。
任何包含这些素材的发行版本都必须保持非商业。

## 许可

| 内容 | 许可 |
| --- | --- |
| 程序代码与项目文档 | [`GPL-3.0-or-later`](LICENSE)，`Copyright (C) 2026 HZ-TYZQ` |
| 对话字体 [Ark Pixel](https://github.com/TakWolf/ark-pixel-font) | [`OFL-1.1`](third_party/ark-pixel-font/OFL.txt) |
| 角色素材 | [作者二创条款](assets/LICENSE.md)，非 GPL |

仓库全部内容**不是**同一个许可证。详见
[`packaging/LICENSES.md`](packaging/LICENSES.md)。

## 支持与验证边界

- 正式支持目标：Windows 11 x86-64；KDE Plasma 的 Linux XCB/XWayland。
- GNOME 在取得合格社区候选测试前属于实验性／未验证。
- niri 与原生 Qt Wayland 后端不在第一版支持范围内。
- 多显示器、热插拔和混合 DPI 多显示器尚未实测，按 best-effort 处理。

截至 2026-08-27，正式产品的 KDE 开发构建已在 Fedora 44、KDE Plasma
6.7.4、Wayland 会话与 XWayland、单屏 2560×1600@240 Hz、系统缩放 125%
下通过综合检查。Windows 探针已通过首轮实测；正式 Windows ZIP、Actions
AppImage 和双平台三小时运行仍待候选产物验收。这些结果不扩大上述支持范围。

Windows 免安装包第一版不进行代码签名，可能触发 SmartScreen，也可能被
Smart App Control 或企业策略阻止。

## 文档

| 文档 | 内容 |
| --- | --- |
| [`docs/Decisions.md`](docs/Decisions.md) | 唯一产品需求基线 |
| [`docs/Plans/DevelopmentPlan.md`](docs/Plans/DevelopmentPlan.md) | 分阶段执行计划 |
| [`docs/Plans/DevelopmentStatus.md`](docs/Plans/DevelopmentStatus.md) | 各阶段实际状态 |
| [`docs/FeasibilityResults.md`](docs/FeasibilityResults.md) | Linux 窗口可行性结果 |
| [`docs/WindowsFeasibilityResults.md`](docs/WindowsFeasibilityResults.md) | Windows 窗口可行性结果 |

## 构建

需要 CMake `3.21+`、Ninja 和 Qt `6.11`（CI 精确锁定 `6.11.2`）。

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```
