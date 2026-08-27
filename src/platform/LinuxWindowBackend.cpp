#include "platform/LinuxWindowBackend.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QWindow>
#include <QtGui/qguiapplication_platform.h>

#include <xcb/xcb.h>

#include <cstring>

namespace mub::platform {

namespace {

Q_LOGGING_CATEGORY(lcLinuxBackend, "mub.platform.linux")

// EWMH：`_NET_WM_DESKTOP` 取 0xFFFFFFFF 表示出现在全部工作区。
constexpr uint32_t kAllDesktops = 0xFFFFFFFFu;

xcb_connection_t *x11Connection()
{
    auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    return x11 != nullptr ? x11->connection() : nullptr;
}

xcb_atom_t internAtom(xcb_connection_t *connection, const char *name)
{
    const auto length = static_cast<uint16_t>(std::strlen(name));
    const xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(connection, 0, length, name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    if (reply == nullptr) {
        return XCB_ATOM_NONE;
    }
    const xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

xcb_screen_t *firstScreen(xcb_connection_t *connection)
{
    const xcb_setup_t *setup = xcb_get_setup(connection);
    if (setup == nullptr) {
        return nullptr;
    }
    xcb_screen_iterator_t iterator = xcb_setup_roots_iterator(setup);
    return iterator.rem > 0 ? iterator.data : nullptr;
}

// 当前工作区序号。读不到时回退为 0，即第一个工作区。
uint32_t currentDesktop(xcb_connection_t *connection, const xcb_window_t root)
{
    const xcb_atom_t atom = internAtom(connection, "_NET_CURRENT_DESKTOP");
    if (atom == XCB_ATOM_NONE) {
        return 0;
    }
    const xcb_get_property_cookie_t cookie =
        xcb_get_property(connection, 0, root, atom, XCB_ATOM_CARDINAL, 0, 1);
    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(connection, cookie, nullptr);
    if (reply == nullptr) {
        return 0;
    }
    uint32_t desktop = 0;
    if (xcb_get_property_value_length(reply) >= static_cast<int>(sizeof(uint32_t))) {
        desktop = *static_cast<uint32_t *>(xcb_get_property_value(reply));
    }
    free(reply);
    return desktop;
}

} // namespace

BackendCapabilities LinuxWindowBackend::capabilities() const
{
    BackendCapabilities caps = QtWindowBackend::capabilities();
    caps.name = QStringLiteral("linux/xcb");
    // 只有真的拿得到 X11 连接才自述支持。Qt 在别的平台插件上运行时
    // 这里必须是假，否则设置界面会显示一个点了没反应的选项。
    caps.workspacePinning = x11Connection() != nullptr;
    return caps;
}

void LinuxWindowBackend::setWorkspaceVisibility(QWindow *window,
                                                const bool allWorkspaces)
{
    if (window == nullptr) {
        return;
    }
    xcb_connection_t *connection = x11Connection();
    if (connection == nullptr) {
        return;
    }
    xcb_screen_t *screen = firstScreen(connection);
    if (screen == nullptr) {
        return;
    }
    const xcb_atom_t desktopAtom = internAtom(connection, "_NET_WM_DESKTOP");
    if (desktopAtom == XCB_ATOM_NONE) {
        qCWarning(lcLinuxBackend) << "_NET_WM_DESKTOP is unavailable";
        return;
    }

    const auto target = static_cast<xcb_window_t>(window->winId());
    const uint32_t desktop =
        allWorkspaces ? kAllDesktops : currentDesktop(connection, screen->root);

    // 两条路都走：属性用于窗口尚未映射的情况，客户端消息用于已映射的窗口。
    // EWMH 规定已映射窗口要由窗口管理器代改，只写属性不一定生效。
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, target, desktopAtom,
                        XCB_ATOM_CARDINAL, 32, 1, &desktop);

    xcb_client_message_event_t event{};
    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = target;
    event.type = desktopAtom;
    event.data.data32[0] = desktop;
    // EWMH 的来源标识：2 表示直接来自应用的用户操作。
    event.data.data32[1] = 2;
    xcb_send_event(connection, 0, screen->root,
                   XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                       | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                   reinterpret_cast<const char *>(&event));
    xcb_flush(connection);

    qCInfo(lcLinuxBackend).noquote()
        << QStringLiteral("workspace visibility set to %1")
               .arg(allWorkspaces ? QStringLiteral("all") : QString::number(desktop));
}

} // namespace mub::platform
