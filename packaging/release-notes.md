## 《床下有罐钱》非官方桌宠

女主会在桌面上走动、休息和说话，也可以被点击、拖动和投喂。设置中可以调整活动、
对话、动画、显示倍率与窗口行为。

## 下载

- **Windows 11 x86-64**：下载 ZIP，完整解压后运行
  `money-under-bed-deskpet.exe`。程序未签名，可能触发 SmartScreen、Smart App
  Control 或企业策略。
- **Linux x86-64**：下载 AppImage，添加执行权限后运行。正式支持 KDE Plasma 的
  XCB/XWayland 环境。
- 下载后可用同名 `.sha256` 文件核对产物。

## 支持与验收

- KDE Plasma + XCB/XWayland：发布前填写实际发行版、Plasma、缩放与连续运行结果。
- Windows 11 x86-64：发布前填写实际版本、缩放档位与检查结果。
- GNOME：实验性／未验证，等待社区测试。
- 多显示器、热插拔和混合 DPI：尚未实测，按 best-effort 处理。
- niri 与原生 Qt Wayland 后端：不支持。

发布前必须用实际验收结果替换上面的两条占位说明；验收使用本 Release 的原始文件，
不重新构建或替换二进制。

## 对应源码与许可

本版的对应源码在
[`@SOURCES_TAG@`](https://github.com/HZ-TYZQ/MoneyUnderBed-DeskPet/releases/tag/@SOURCES_TAG@)，
与本 Release 指向同一提交并产自同一次流水线。包内 `licenses/` 记录各组件的许可、
版本和运行库来源。

本项目是《床下有罐钱》的**非官方、非商业二次创作**，与原作开发者没有隶属关系，
也不由其发布或背书。角色素材来自作者 `_U5B_` 发布的
[二创素材包](https://www.bilibili.com/video/BV1XwhV6TEXQ/)，仅供二次创作：
**不可商用、禁止 R18、禁止用于 AI 训练**。

程序代码采用 GPL-3.0-or-later，Ark Pixel 字体采用 OFL-1.1，角色素材遵循作者的
独立条款。源码 Release 必须在二进制 Release 公开期间保持公开且内容不变。
