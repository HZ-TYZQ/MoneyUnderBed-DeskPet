# Qt 6.11.2 使用与对应源码

本程序动态链接 Qt 6.11.2 的 Qt Base 模块（Core、Gui、Widgets、Network，
Linux 另使用 DBus），选择 `GPL-3.0-only` 开源许可路径。两平台打包均明确排除
未使用的 SVG 图标与图像插件，最终成品清单也确认不含 Qt SVG。

正式候选构建同时提供以下对应源码归档，不要求接收者另行从 Qt 安装器取得：

- `qtbase-everywhere-src-6.11.2.tar.xz`：<https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/qtbase-everywhere-src-6.11.2.tar.xz>，SHA-256 `5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22`

同一份源码 artifact 还包含 Qt 官方 Linux 包依赖的 ICU 73.2，以及固定
AppImage runtime 和生成 AppRun/hook 所用工具的源码；Linux runner 复制的系统库
则放在独立的 `Linux-system-source-*` artifact 中。每份文件都带同目录校验清单。

该源码归档及其校验文件与每次 GitHub Actions 候选产物一起保存；正式标签
构建还会把同一归档附到草稿 Release。发行包内的
`licenses/qt-modules-6.11.2.spdx` 记录本发行物使用的 Qt Base、
选择的 GPLv3 路径以及对应源码的固定哈希。Actions 另附实际发行目录
的逐文件哈希和动态依赖清单，两者合起来用于核对成品内容。
