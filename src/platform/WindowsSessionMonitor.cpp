#include "platform/WindowsSessionMonitor.h"

#include <QCoreApplication>
#include <QEvent>
#include <QLoggingCategory>
#include <QPlatformSurfaceEvent>
#include <QWindow>

#include <windows.h>
#include <wtsapi32.h>

#include <cstring>

namespace mub::platform {

namespace {

Q_LOGGING_CATEGORY(lcWindowsSession, "mub.platform.session.windows")

// GUID_CONSOLE_DISPLAY_STATE，见 Microsoft Power Setting GUIDs。
constexpr GUID kConsoleDisplayState{
    0x6fe69556,
    0x704a,
    0x47a0,
    {0x8f, 0x24, 0xc2, 0x8d, 0x93, 0x6f, 0xda, 0x47},
};

} // namespace

WindowsSessionMonitor::~WindowsSessionMonitor()
{
    unregisterNotifications();
}

bool WindowsSessionMonitor::start(QWindow *notificationWindow)
{
    unregisterNotifications();
    if (notificationWindow == nullptr || notificationWindow->winId() == 0
        || QCoreApplication::instance() == nullptr) {
        qCWarning(lcWindowsSession) << "a native notification window is unavailable";
        return false;
    }

    notificationWindow_ = notificationWindow;
    notificationWindow_->installEventFilter(this);
    return registerNotifications();
}

bool WindowsSessionMonitor::registerNotifications()
{
    if (notificationWindow_ == nullptr || notificationWindow_->winId() == 0) {
        return false;
    }

    unregisterWindowNotifications();

    windowId_ = static_cast<quintptr>(notificationWindow_->winId());
    const HWND handle = reinterpret_cast<HWND>(windowId_);

    sessionNotificationsRegistered_ =
        WTSRegisterSessionNotification(handle, NOTIFY_FOR_THIS_SESSION) != FALSE;
    if (!sessionNotificationsRegistered_) {
        qCWarning(lcWindowsSession)
            << "WTSRegisterSessionNotification failed error=" << GetLastError();
    }

    displayNotification_ = RegisterPowerSettingNotification(
        handle, &kConsoleDisplayState, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (displayNotification_ == nullptr) {
        qCWarning(lcWindowsSession)
            << "RegisterPowerSettingNotification failed error=" << GetLastError();
    }

    if (!eventFilterInstalled_) {
        QCoreApplication::instance()->installNativeEventFilter(this);
        eventFilterInstalled_ = true;
    }
    qCInfo(lcWindowsSession)
        << "native monitor session=" << sessionNotificationsRegistered_
        << "display=" << (displayNotification_ != nullptr);
    return true;
}

bool WindowsSessionMonitor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != notificationWindow_ || event->type() != QEvent::PlatformSurface) {
        return SessionMonitor::eventFilter(watched, event);
    }

    const auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
    if (surfaceEvent->surfaceEventType()
        == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        unregisterWindowNotifications();
    } else if (surfaceEvent->surfaceEventType()
               == QPlatformSurfaceEvent::SurfaceCreated) {
        qCInfo(lcWindowsSession)
            << "native surface recreated; registering session notifications again";
        registerNotifications();
    }
    return false;
}

bool WindowsSessionMonitor::nativeEventFilter(const QByteArray &eventType,
                                              void *message, qintptr *result)
{
    Q_UNUSED(result)
    if (eventType != QByteArrayLiteral("windows_generic_MSG") || message == nullptr) {
        return false;
    }

    const auto *nativeMessage = static_cast<const MSG *>(message);
    if (reinterpret_cast<quintptr>(nativeMessage->hwnd) != windowId_) {
        return false;
    }

    if (nativeMessage->message == WM_WTSSESSION_CHANGE) {
        switch (nativeMessage->wParam) {
        case WTS_SESSION_LOCK:
            setReason(core::SessionSuspendReason::Locked, true);
            break;
        case WTS_SESSION_UNLOCK:
            setReason(core::SessionSuspendReason::Locked, false);
            break;
        case WTS_CONSOLE_DISCONNECT:
        case WTS_REMOTE_DISCONNECT:
            setReason(core::SessionSuspendReason::Inactive, true);
            break;
        case WTS_CONSOLE_CONNECT:
        case WTS_REMOTE_CONNECT:
        case WTS_SESSION_LOGON:
        case WTS_SESSION_DESKTOP_READY:
            setReason(core::SessionSuspendReason::Inactive, false);
            break;
        default:
            break;
        }
        return false;
    }

    if (nativeMessage->message != WM_POWERBROADCAST) {
        return false;
    }
    switch (nativeMessage->wParam) {
    case PBT_APMSUSPEND:
        setReason(core::SessionSuspendReason::Sleeping, true);
        break;
    case PBT_APMRESUMEAUTOMATIC:
    case PBT_APMRESUMESUSPEND:
        setReason(core::SessionSuspendReason::Sleeping, false);
        break;
    case PBT_POWERSETTINGCHANGE: {
        const auto *setting = reinterpret_cast<const POWERBROADCAST_SETTING *>(
            nativeMessage->lParam);
        if (setting != nullptr && IsEqualGUID(setting->PowerSetting,
                                              kConsoleDisplayState)
            && setting->DataLength >= sizeof(DWORD)) {
            DWORD displayState = 0;
            std::memcpy(&displayState, setting->Data, sizeof(displayState));
            // 0=关闭，1=开启，2=变暗。变暗仍然可见，不冻结。
            setReason(core::SessionSuspendReason::DisplayOff, displayState == 0);
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void WindowsSessionMonitor::unregisterWindowNotifications()
{
    const HWND handle = reinterpret_cast<HWND>(windowId_);
    if (sessionNotificationsRegistered_ && handle != nullptr) {
        WTSUnRegisterSessionNotification(handle);
    }
    if (displayNotification_ != nullptr) {
        UnregisterPowerSettingNotification(
            reinterpret_cast<HPOWERNOTIFY>(displayNotification_));
    }
    sessionNotificationsRegistered_ = false;
    displayNotification_ = nullptr;
    windowId_ = 0;
}

void WindowsSessionMonitor::unregisterNotifications()
{
    unregisterWindowNotifications();
    if (notificationWindow_ != nullptr) {
        notificationWindow_->removeEventFilter(this);
    }
    if (eventFilterInstalled_ && QCoreApplication::instance() != nullptr) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
    eventFilterInstalled_ = false;
    notificationWindow_ = nullptr;
}

} // namespace mub::platform
