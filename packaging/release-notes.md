## 候选产物

本 Release 的二进制由标签对应的 GitHub Actions 自动构建。候选验收期间保持为草稿；请核对随附的 `.sha256` 文件后再测试，验收通过时发布同一份产物，不重新构建。

## 授权与身份

- 本项目是《床下有罐钱》的非官方、非商业二次创作，与原作开发者没有隶属关系，也不由其发布或背书。
- 角色素材来自作者 `_U5B_` 发布的[二创素材包](https://www.bilibili.com/video/BV1XwhV6TEXQ/)，仅供二次创作、不可商用、禁止 R18、禁止用于 AI 训练。
- 程序代码采用 GPL-3.0-or-later；Ark Pixel 字体采用 OFL-1.1。各自完整文本和素材清单均包含在发行物中。
- Qt 6.11.2 采用 GPLv3 动态链接路径；Qt Base／Qt SVG 对应源码归档及校验文件随本 Release 提供。
- Windows 包未签名，可能触发 SmartScreen，也可能被 Smart App Control 或企业策略阻止。

## 支持与验证边界

- 正式支持目标：Windows 11 x86-64；KDE Plasma 上的 Linux XCB/XWayland。
- GNOME 在获得合格社区候选测试前标为实验性／未验证。
- niri 不支持；原生 Qt Wayland 后端不属于第一版支持范围。
- 多显示器、热插拔和混合 DPI 多显示器尚未实测，按 best-effort 处理。

## 候选验收记录（发布前更新）

- KDE Plasma + XCB/XWayland：待填写候选 AppImage 的精确环境、检查表与三小时结果。
- Windows 11 x86-64：待填写候选 ZIP 的系统缩放档位、检查表与三小时结果。
- GNOME：没有合格社区结果时保留“实验性／未验证”，不得改写为已支持。

发布草稿前必须把前两项“待填写”替换为实际结果；只编辑 Release 说明，不重新构建或替换已经验收的二进制。
