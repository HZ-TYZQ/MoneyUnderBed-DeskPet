# 《床下有罐钱》非官方桌宠

把《床下有罐钱》的女主放到桌面上：她会自己走动、休息、说几句话，也可以被点击、
拖动和投喂。

**[下载最新版本](https://github.com/HZ-TYZQ/MoneyUnderBed-DeskPet/releases/latest)**

## 功能

- 四方向移动与像素动画。
- 点击、拖动和冰淇淋投喂互动。
- 符合原作人设的短句和连续对话。
- 可调整活动、对话、动画、显示倍率与窗口行为。
- 托盘隐藏、再次启动唤回、锁屏与睡眠恢复。

## 下载与运行

### Windows 11

下载 `windows-x86_64` ZIP，完整解压后运行 `money-under-bed-deskpet.exe`。

程序暂未签名，Windows 可能显示 SmartScreen 提醒，也可能被 Smart App Control 或
企业策略阻止。

### Linux

下载 `linux-x86_64` AppImage，添加执行权限后直接运行：

```bash
chmod +x MoneyUnderBed-DeskPet-*.AppImage
./MoneyUnderBed-DeskPet-*.AppImage
```

正式支持 KDE Plasma 的 XCB/XWayland 环境。程序使用 Qt XCB 后端，不支持 niri 和
原生 Qt Wayland 后端。

## 支持情况

| 环境 | 状态 | 实际验证 |
| --- | --- | --- |
| Windows 11 x86-64 | 正式支持 | 100%／125%／150%／200% 缩放、Explorer 重启、锁屏与睡眠恢复 |
| KDE Plasma + XCB/XWayland | 正式支持 | Fedora 44、Plasma 6.7.4、125% 缩放；候选包连续运行三小时 |
| GNOME | 实验性／未验证 | 等待社区测试 |
| 多显示器、热插拔、混合 DPI | best-effort | 尚未实测 |
| niri、原生 Qt Wayland | 不支持 | — |

完整验收记录见 [1.1.0 检查表](docs/ReleaseChecklist-1.1.md)。

## 非官方声明与许可

本项目是《床下有罐钱》的**非官方、非商业二次创作**，与原作开发者没有隶属关系，
也不由其发布或背书。

角色素材来自作者 `_U5B_` 公开发布的
[二创素材包](https://www.bilibili.com/video/BV1XwhV6TEXQ/)，仅供二次创作：
**不可商用、禁止 R18、禁止用于 AI 训练**。角色素材不属于 GPL，详细条款见
[assets/LICENSE.md](assets/LICENSE.md)。

| 内容 | 许可 |
| --- | --- |
| 程序代码与项目文档 | [GPL-3.0-or-later](LICENSE) |
| Ark Pixel 对话字体 | [OFL-1.1](third_party/ark-pixel-font/OFL.txt) |
| 角色素材 | [作者二创条款](assets/LICENSE.md) |

仓库内容并非全部采用同一许可证，完整说明见
[packaging/LICENSES.md](packaging/LICENSES.md)。

## 构建

需要 CMake `3.21+`、Ninja 和 Qt `6.11`。CI 使用 Qt `6.11.2`。

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

产品决策、开发记录和发布验收分别见 [Decisions](docs/Decisions.md)、
[DevelopmentStatus-1.1](docs/Plans/DevelopmentStatus-1.1.md) 与
[ReleaseChecklist-1.1](docs/ReleaseChecklist-1.1.md)。
