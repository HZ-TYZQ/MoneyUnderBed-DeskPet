# 《床下有罐钱》非官方桌宠

**首个正式版本 [`v1.1.0`](https://github.com/HZ-TYZQ/MoneyUnderBed-DeskPet/releases/tag/v1.1.0)
已于 2026-08-28 发布。**

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
下通过综合检查。Windows 窗口能力已通过首轮实测（见
[`docs/WindowsFeasibilityResults.md`](docs/WindowsFeasibilityResults.md)）。

`v1.1.0` 的正式候选已于 2026-08-28 在两个平台完成人工验收，两边都没有出现核心
失败：KDE 侧完成连续三小时运行，Windows 侧覆盖 100%／125%／150%／200% 四档系统
缩放、Explorer 重启与锁屏／睡眠恢复。Windows 按
[`docs/Decisions.md`](docs/Decisions.md) 第 14.10 节不要求连续三小时。
逐项结果见 [`docs/ReleaseChecklist-1.1.md`](docs/ReleaseChecklist-1.1.md)。

验收同时如实记录了几处数据缺口：KDE 的三小时段只有首尾两次读数，Windows 侧未采集
线程数与启动时间。这些不改变发布结论——本版按第 14.10 节不设性能数值门槛——但也
不写成已完成。以上结果都不扩大上述支持范围。

Windows 免安装包第一版不进行代码签名，可能触发 SmartScreen，也可能被
Smart App Control 或企业策略阻止。

## 文档

| 文档 | 内容 |
| --- | --- |
| [`docs/Decisions.md`](docs/Decisions.md) | 唯一产品需求基线 |
| [`docs/Plans/DevelopmentPlan-1.1.md`](docs/Plans/DevelopmentPlan-1.1.md) | `1.1.0` 的分阶段执行计划 |
| [`docs/Plans/DevelopmentStatus-1.1.md`](docs/Plans/DevelopmentStatus-1.1.md) | `1.1.0` 各阶段实际状态 |
| [`docs/ReleaseChecklist-1.1.md`](docs/ReleaseChecklist-1.1.md) | `1.1.0` 正式候选验收清单 |
| [`docs/Plans/DevelopmentPlan.md`](docs/Plans/DevelopmentPlan.md) | `1.0` 的分阶段执行计划（历史） |
| [`docs/Plans/DevelopmentStatus.md`](docs/Plans/DevelopmentStatus.md) | `1.0` 各阶段实际状态（历史） |
| [`docs/FeasibilityResults.md`](docs/FeasibilityResults.md) | Linux 窗口可行性结果 |
| [`docs/WindowsFeasibilityResults.md`](docs/WindowsFeasibilityResults.md) | Windows 窗口可行性结果 |

## 构建

需要 CMake `3.21+`、Ninja 和 Qt `6.11`（CI 精确锁定 `6.11.2`）。

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```
