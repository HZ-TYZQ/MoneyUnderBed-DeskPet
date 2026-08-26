# 《床下有罐钱》非官方桌宠

**开发中，尚未发布任何版本。**

一个把《床下有罐钱》的女主放在桌面上陪伴用户的桌宠程序。

## 非官方声明

本项目是**非官方、非商业**的二次创作，与《床下有罐钱》的开发者没有隶属关系，
也不由其发布或背书。请不要将本项目误认为官方产品。

## 角色素材

角色素材来自《床下有罐钱》作者 `_U5B_` 公开发布的二创素材包，
不属于本项目代码的 GPL 范围，条款见 [`assets/LICENSE.md`](assets/LICENSE.md)，
文件清单与哈希见 [`assets/MANIFEST.md`](assets/MANIFEST.md)。

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

## 当前状态

支持目标为 Linux 与 Windows 双平台。目前处于工程搭建阶段，
尚未产生可供使用的版本，也没有完成任何平台的正式验收。
实际验证配置、实验性环境和未测试范围会在首个发行版的说明中分别列出。

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
