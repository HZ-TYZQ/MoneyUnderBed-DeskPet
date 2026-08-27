#pragma once

#include "platform/SessionMonitor.h"

#include <QAbstractNativeEventFilter>
#include <QPointer>
#include <QtGlobal>

namespace mub::platform {

class WindowsSessionMonitor final : public SessionMonitor,
                                    public QAbstractNativeEventFilter
{
public:
    using SessionMonitor::SessionMonitor;
    ~WindowsSessionMonitor() override;

    bool start(QWindow *notificationWindow) override;
    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           qintptr *result) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool registerNotifications();
    void unregisterWindowNotifications();
    void unregisterNotifications();

    QPointer<QWindow> notificationWindow_;
    quintptr windowId_ = 0;
    void *displayNotification_ = nullptr;
    bool sessionNotificationsRegistered_ = false;
    bool eventFilterInstalled_ = false;
};

} // namespace mub::platform
