#pragma once

#include "platform/SessionMonitor.h"

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace mub::platform {

class LinuxSessionMonitor final : public SessionMonitor
{
    Q_OBJECT

public:
    using SessionMonitor::SessionMonitor;

    bool start(QWindow *notificationWindow) override;

private slots:
    void handlePrepareForSleep(bool sleeping);
    void handleScreenSaverActive(bool active);
    void handlePropertiesChanged(const QString &interfaceName,
                                 const QVariantMap &changed,
                                 const QStringList &invalidated);

private:
    bool connectSessionProperties();
    bool connectScreenSaver();
    QString resolveSessionPath() const;
    void readInitialSessionState();
    void readInitialScreenSaverState();

    QString sessionPath_;
};

} // namespace mub::platform
