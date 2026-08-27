# Qt 6.11.2 使用与对应源码

本程序动态链接 Qt 6.11.2 的 Qt Base 模块（Core、Gui、Widgets、Network，
Linux 另使用 DBus），并选择 `GPL-3.0-only` 开源许可路径。

正式候选构建同时提供以下对应源码归档，不要求接收者另行从 Qt 安装器取得：

- 文件：`qtbase-everywhere-src-6.11.2.tar.xz`
- Qt 官方下载：<https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/qtbase-everywhere-src-6.11.2.tar.xz>
- SHA-256：`5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22`

该源码归档及其校验文件与每次 GitHub Actions 候选产物一起保存；正式标签
构建还会把同一归档附到草稿 Release。发行包内的
`licenses/qtbase-6.11.2.spdx` 来自构建所用 Qt 安装，记录 Qt Base 与其
第三方组件的许可证和版权信息；`licenses/qt-gpl-3.0-only.txt` 是 Qt
安装随附的 GPLv3 完整文本。
