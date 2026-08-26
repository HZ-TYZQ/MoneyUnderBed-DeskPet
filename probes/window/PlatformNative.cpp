#include "PlatformNative.h"

#include <QStringList>
#include <QWidget>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace platform_native {

#if defined(Q_OS_WIN)

namespace {

HWND nativeHandle(const QWidget *widget)
{
    if (widget == nullptr) {
        return nullptr;
    }
    // winId() 在这里只读取已经存在的原生句柄；窗口显示后必然已创建。
    return reinterpret_cast<HWND>(const_cast<QWidget *>(widget)->winId());
}

QString exStyleText(const LONG_PTR exStyle)
{
    struct Bit
    {
        LONG_PTR mask;
        const char *name;
    };
    static const Bit bits[] = {
        {WS_EX_TOPMOST, "topmost"},
        {WS_EX_TOOLWINDOW, "toolwindow"},
        {WS_EX_APPWINDOW, "appwindow"},
        {WS_EX_LAYERED, "layered"},
        {WS_EX_TRANSPARENT, "transparent"},
        {WS_EX_NOACTIVATE, "noactivate"},
    };

    QStringList present;
    for (const Bit &bit : bits) {
        if ((exStyle & bit.mask) != 0) {
            present.append(QString::fromLatin1(bit.name));
        }
    }
    if (present.isEmpty()) {
        present.append(QStringLiteral("<none>"));
    }
    return present.join(QLatin1Char('+'));
}

} // namespace

bool isSupported()
{
    return true;
}

QString platformName()
{
    return QStringLiteral("win32");
}

QString describeWindow(const QWidget *widget)
{
    HWND handle = nativeHandle(widget);
    if (handle == nullptr) {
        return QStringLiteral("hwnd=<null>");
    }

    const LONG_PTR exStyle = GetWindowLongPtrW(handle, GWL_EXSTYLE);
    const LONG_PTR style = GetWindowLongPtrW(handle, GWL_STYLE);
    return QStringLiteral("hwnd=0x%1 ex_style=0x%2 ex_bits=%3 style=0x%4")
        .arg(reinterpret_cast<quintptr>(handle), 0, 16)
        .arg(static_cast<qulonglong>(exStyle), 0, 16)
        .arg(exStyleText(exStyle))
        .arg(static_cast<qulonglong>(style), 0, 16);
}

QString foregroundWindowTitle()
{
    HWND handle = GetForegroundWindow();
    if (handle == nullptr) {
        return QStringLiteral("<none>");
    }

    wchar_t buffer[256] = {};
    const int length = GetWindowTextW(handle, buffer,
                                      static_cast<int>(std::size(buffer)));
    if (length <= 0) {
        return QStringLiteral("<untitled>");
    }
    return QString::fromWCharArray(buffer, length);
}

bool setNativeInputTransparent(QWidget *widget, const bool enabled)
{
    HWND handle = nativeHandle(widget);
    if (handle == nullptr) {
        return false;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(handle, GWL_EXSTYLE);
    // WS_EX_TRANSPARENT 需要 WS_EX_LAYERED 才对命中测试生效。
    // 半透明窗口通常已经是 layered，这里只补齐缺失的情况。
    if (enabled) {
        exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    }
    SetLastError(0);
    const LONG_PTR previous = SetWindowLongPtrW(handle, GWL_EXSTYLE, exStyle);
    if (previous == 0 && GetLastError() != 0) {
        return false;
    }
    return true;
}

#else

bool isSupported()
{
    return false;
}

QString platformName()
{
    return QStringLiteral("non-win32");
}

QString describeWindow(const QWidget *widget)
{
    Q_UNUSED(widget)
    return QStringLiteral("<native inspection not implemented on this platform>");
}

QString foregroundWindowTitle()
{
    return QStringLiteral("<foreground query not implemented on this platform>");
}

bool setNativeInputTransparent(QWidget *widget, const bool enabled)
{
    Q_UNUSED(widget)
    Q_UNUSED(enabled)
    // Linux 侧的 Qt 标志路径已经在 docs/FeasibilityResults.md 中实测通过，
    // 因此这里不引入 XCB input shape 候选实现。
    return false;
}

#endif

} // namespace platform_native
