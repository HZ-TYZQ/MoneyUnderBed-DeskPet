#include "platform/QtWindowBackend.h"

#include <QGuiApplication>
#include <QWindow>

namespace mub::platform {

Qt::WindowFlags QtWindowBackend::deskPetFlags()
{
    // Qt::Tool 让窗口不出现在任务栏，并尽量不进入普通窗口切换列表
    // （docs/Decisions.md 第 3.4 节）。
    // Qt::WindowDoesNotAcceptFocus 保证点击角色不抢走当前应用的焦点。
    return Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus
        | Qt::NoDropShadowWindowHint;
}

BackendCapabilities QtWindowBackend::capabilities() const
{
    BackendCapabilities caps;
    caps.name = QStringLiteral("qt/%1").arg(QGuiApplication::platformName());
    caps.alwaysOnTop = true;
    caps.inputPassthrough = true;
    caps.pixelHitMask = true;
    caps.systemDrag = true;
    caps.excludeFromWindowList = true;
    return caps;
}

void QtWindowBackend::configureAsDeskPet(QWindow *window)
{
    if (window == nullptr) {
        return;
    }
    window->setFlags(deskPetFlags() | (window->flags() & Qt::WindowStaysOnTopHint));
}

void QtWindowBackend::setAlwaysOnTop(QWindow *window, const bool enabled)
{
    if (window == nullptr) {
        return;
    }
    Qt::WindowFlags flags = window->flags();
    flags.setFlag(Qt::WindowStaysOnTopHint, enabled);
    window->setFlags(flags);
}

void QtWindowBackend::setInputPassthrough(QWindow *window, const bool enabled)
{
    if (window == nullptr) {
        return;
    }
    Qt::WindowFlags flags = window->flags();
    flags.setFlag(Qt::WindowTransparentForInput, enabled);
    window->setFlags(flags);
}

void QtWindowBackend::setHitMask(QWindow *window, const QRegion &region)
{
    if (window == nullptr) {
        return;
    }
    window->setMask(region);
}

bool QtWindowBackend::startSystemDrag(QWindow *window)
{
    if (window == nullptr) {
        return false;
    }
    return window->startSystemMove();
}

} // namespace mub::platform
