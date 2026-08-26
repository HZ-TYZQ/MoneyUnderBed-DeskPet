#pragma once

#include <QString>

namespace mub {

// 应用身份的唯一来源。取值见 docs/Decisions.md 第 1.2 节。
// 任何地方都不得再硬编码这些字符串。
namespace metadata {

// 技术名称，用于进程名、日志和配置路径。不翻译。
QString applicationName();

// 可执行文件名，不带平台后缀。
QString executableName();

// Linux desktop ID 与 D-Bus 相关名称使用的应用 ID。
QString applicationId();

QString organizationName();
QString organizationDomain();
QString homepageUrl();

// 完整版本串，形如 `0.0.0-dev`。用于关于窗口、日志和诊断信息。
QString versionString();

// 面向用户的显示名称。走翻译，第一版只提供简体中文。
QString displayName();

// 非官方声明。README、关于窗口和发行说明都必须展示，
// 见 docs/Decisions.md 第 1.2 节。
QString unofficialNotice();

// 一次性把上述身份写入 Qt 全局状态。
// 这些都是静态设置，可以在构造 QApplication 之前调用，
// 使早期日志也能拿到正确的 QStandardPaths 路径。
void apply();

} // namespace metadata

} // namespace mub
