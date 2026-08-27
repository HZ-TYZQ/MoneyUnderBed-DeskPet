#include "platform/WindowsWindowBackend.h"

#include <QWindow>

#include <windows.h>

namespace mub::platform {

namespace {

HWND nativeHandle(QWindow *window)
{
    if (window == nullptr) {
        return nullptr;
    }
    const WId id = window->winId();
    if (id == 0) {
        return nullptr;
    }
    return reinterpret_cast<HWND>(id);
}

} // namespace

BackendCapabilities WindowsWindowBackend::capabilities() const
{
    BackendCapabilities caps = QtWindowBackend::capabilities();
    caps.name = QStringLiteral("windows/qt");
    return caps;
}

void WindowsWindowBackend::configureAsDeskPet(QWindow *window)
{
    QtWindowBackend::configureAsDeskPet(window);

    HWND handle = nativeHandle(window);
    if (handle == nullptr) {
        return;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(handle, GWL_EXSTYLE);
    // WS_EX_TOOLWINDOW 使窗口不出现在任务栏与 Alt+Tab；
    // WS_EX_NOACTIVATE 使点击窗口不抢走前台焦点。
    // Qt::Tool 与 Qt::WindowDoesNotAcceptFocus 通常已经设置这两位，
    // 这里补齐是为了不依赖 Qt 内部实现细节。
    exStyle |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    exStyle &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW);
    SetWindowLongPtrW(handle, GWL_EXSTYLE, exStyle);
}

} // namespace mub::platform
