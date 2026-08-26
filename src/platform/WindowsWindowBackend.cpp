#include "platform/WindowsWindowBackend.h"

#include <QByteArray>
#include <QLoggingCategory>
#include <QWindow>

#include <windows.h>

namespace mub::platform {

namespace {

Q_LOGGING_CATEGORY(lcWindowsBackend, "mub.platform.windows")

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

WindowsWindowBackend::WindowsWindowBackend()
    : strategy_(strategyFromEnvironment())
{
}

WindowsWindowBackend::PassthroughStrategy
WindowsWindowBackend::strategyFromEnvironment()
{
    const QByteArray value = qgetenv("MUB_WIN_PASSTHROUGH").toLower();
    if (value == "native") {
        return PassthroughStrategy::NativeExtendedStyle;
    }
    return PassthroughStrategy::QtWindowFlag;
}

BackendCapabilities WindowsWindowBackend::capabilities() const
{
    BackendCapabilities caps = QtWindowBackend::capabilities();
    caps.name = strategy_ == PassthroughStrategy::NativeExtendedStyle
        ? QStringLiteral("windows/native-passthrough")
        : QStringLiteral("windows/qt-passthrough");
    // Windows 没有公开稳定的「固定到所有虚拟桌面」接口，
    // 决策也明确不使用未公开接口模拟（docs/Decisions.md 第 3.4 节）。
    caps.workspacePinning = false;
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

void WindowsWindowBackend::setInputPassthrough(QWindow *window, const bool enabled)
{
    if (strategy_ == PassthroughStrategy::QtWindowFlag) {
        QtWindowBackend::setInputPassthrough(window, enabled);
        return;
    }

    HWND handle = nativeHandle(window);
    if (handle == nullptr) {
        return;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(handle, GWL_EXSTYLE);
    if (enabled) {
        // WS_EX_TRANSPARENT 需要 WS_EX_LAYERED 才对命中测试生效。
        // 半透明窗口通常已经是 layered，这里补齐缺失的情况。
        exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    }
    SetWindowLongPtrW(handle, GWL_EXSTYLE, exStyle);

    qCInfo(lcWindowsBackend)
        << "native passthrough" << (enabled ? "enabled" : "disabled");
}

WindowsWindowBackend::PassthroughStrategy
WindowsWindowBackend::passthroughStrategy() const
{
    return strategy_;
}

} // namespace mub::platform
