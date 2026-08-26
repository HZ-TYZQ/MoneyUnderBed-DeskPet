#include "platform/StartupProbe.h"

#include <QByteArray>
#include <QCoreApplication>

#include <xcb/xcb.h>

namespace mub::platform {

namespace {

QString translate(const char *text)
{
    return QCoreApplication::translate("mub::platform", text);
}

// 只有无头测试平台可以保留原值。任何其他取值都会被改成单值 xcb。
bool isHeadlessTestPlatform(const QByteArray &value)
{
    return value == "offscreen" || value == "minimal";
}

} // namespace

StartupProbeResult probeWindowBackend()
{
    StartupProbeResult result;

    const QByteArray requested = qgetenv("QT_QPA_PLATFORM").trimmed();
    if (isHeadlessTestPlatform(requested)) {
        result.selectedPlatform = QString::fromLatin1(requested);
        result.detail = QStringLiteral(
            "headless test platform requested; skipping the XCB probe");
        return result;
    }

    // 单值，不使用候选列表。
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
    result.selectedPlatform = QStringLiteral("xcb");

    xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
    const int error = connection != nullptr
        ? xcb_connection_has_error(connection)
        : -1;
    if (connection != nullptr) {
        xcb_disconnect(connection);
    }

    if (error == 0) {
        result.detail = QStringLiteral("xcb_connect succeeded");
        return result;
    }

    result.ok = false;
    result.reason = translate(
        "无法连接到 X11 显示服务。\n\n"
        "本程序在 Linux 上使用 Qt 的 XCB 后端运行，在 Wayland 桌面上通过 "
        "XWayland 工作。当前环境没有可用的 XWayland 或 X11 连接。\n\n"
        "请确认桌面环境已启用 XWayland，或在 X11 会话中运行。");
    result.detail = QStringLiteral("xcb_connect failed, error=%1, DISPLAY=%2")
                        .arg(error)
                        .arg(qEnvironmentVariable("DISPLAY",
                                                  QStringLiteral("<unset>")));
    return result;
}

} // namespace mub::platform
