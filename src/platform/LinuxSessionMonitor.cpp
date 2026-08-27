#include "platform/LinuxSessionMonitor.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QLoggingCategory>
#include <QWindow>

namespace mub::platform {

namespace {

Q_LOGGING_CATEGORY(lcLinuxSession, "mub.platform.session.linux")

constexpr auto kService = "org.freedesktop.login1";
constexpr auto kManagerPath = "/org/freedesktop/login1";
constexpr auto kManagerInterface = "org.freedesktop.login1.Manager";
constexpr auto kSessionInterface = "org.freedesktop.login1.Session";
constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr auto kScreenSaverService = "org.freedesktop.ScreenSaver";
constexpr auto kScreenSaverPath = "/ScreenSaver";
constexpr auto kScreenSaverInterface = "org.freedesktop.ScreenSaver";
constexpr int kInitialQueryTimeoutMs = 1000;

} // namespace

bool LinuxSessionMonitor::start(QWindow *notificationWindow)
{
    Q_UNUSED(notificationWindow)

    QDBusConnection bus = QDBusConnection::systemBus();
    const bool systemBusAvailable = bus.isConnected();
    if (!systemBusAvailable) {
        qCWarning(lcLinuxSession) << "system D-Bus is unavailable";
    }

    const bool sleepConnected = systemBusAvailable && bus.connect(
        QLatin1String(kService), QLatin1String(kManagerPath),
        QLatin1String(kManagerInterface), QStringLiteral("PrepareForSleep"), this,
        SLOT(handlePrepareForSleep(bool)));
    if (systemBusAvailable && !sleepConnected) {
        qCWarning(lcLinuxSession) << "could not subscribe to PrepareForSleep";
    }

    sessionPath_ = systemBusAvailable ? resolveSessionPath() : QString();
    const bool sessionConnected = systemBusAvailable && connectSessionProperties();
    if (sessionConnected) {
        readInitialSessionState();
    }
    const bool screenSaverConnected = connectScreenSaver();
    if (screenSaverConnected) {
        readInitialScreenSaverState();
    }

    qCInfo(lcLinuxSession).noquote()
        << QStringLiteral("lifecycle monitor sleep=%1 session=%2 screensaver=%3 path=%4")
               .arg(sleepConnected)
               .arg(sessionConnected)
               .arg(screenSaverConnected)
               .arg(sessionPath_.isEmpty() ? QStringLiteral("unavailable")
                                           : sessionPath_);
    return sleepConnected || sessionConnected || screenSaverConnected;
}

void LinuxSessionMonitor::handlePrepareForSleep(const bool sleeping)
{
    setReason(core::SessionSuspendReason::Sleeping, sleeping);
}

void LinuxSessionMonitor::handleScreenSaverActive(const bool active)
{
    // ActiveChanged 表示桌面被屏保/锁屏界面遮蔽。把它作为独立原因，
    // 即使 logind 的 LockedHint 与它到达顺序不同也不会提前恢复。
    setReason(core::SessionSuspendReason::DisplayOff, active);
}

void LinuxSessionMonitor::handlePropertiesChanged(
    const QString &interfaceName, const QVariantMap &changed,
    const QStringList &invalidated)
{
    Q_UNUSED(invalidated)
    if (interfaceName != QLatin1String(kSessionInterface)) {
        return;
    }
    if (changed.contains(QStringLiteral("LockedHint"))) {
        setReason(core::SessionSuspendReason::Locked,
                  changed.value(QStringLiteral("LockedHint")).toBool());
    }
    if (changed.contains(QStringLiteral("Active"))) {
        setReason(core::SessionSuspendReason::Inactive,
                  !changed.value(QStringLiteral("Active")).toBool());
    }
}

QString LinuxSessionMonitor::resolveSessionPath() const
{
    QDBusInterface manager(QLatin1String(kService), QLatin1String(kManagerPath),
                           QLatin1String(kManagerInterface),
                           QDBusConnection::systemBus());
    if (!manager.isValid()) {
        return {};
    }
    manager.setTimeout(kInitialQueryTimeoutMs);

    const QString sessionId = qEnvironmentVariable("XDG_SESSION_ID");
    if (!sessionId.isEmpty()) {
        const QDBusReply<QDBusObjectPath> byId =
            manager.call(QStringLiteral("GetSession"), sessionId);
        if (byId.isValid()) {
            return byId.value().path();
        }
    }

    const QDBusReply<QDBusObjectPath> byPid = manager.call(
        QStringLiteral("GetSessionByPID"),
        static_cast<uint>(QCoreApplication::applicationPid()));
    return byPid.isValid() ? byPid.value().path() : QString();
}

bool LinuxSessionMonitor::connectSessionProperties()
{
    if (sessionPath_.isEmpty()) {
        qCWarning(lcLinuxSession) << "could not resolve the current logind session";
        return false;
    }
    return QDBusConnection::systemBus().connect(
        QLatin1String(kService), sessionPath_, QLatin1String(kPropertiesInterface),
        QStringLiteral("PropertiesChanged"), this,
        SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList)));
}

bool LinuxSessionMonitor::connectScreenSaver()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return false;
    }
    return bus.connect(QLatin1String(kScreenSaverService),
                       QLatin1String(kScreenSaverPath),
                       QLatin1String(kScreenSaverInterface),
                       QStringLiteral("ActiveChanged"), this,
                       SLOT(handleScreenSaverActive(bool)));
}

void LinuxSessionMonitor::readInitialSessionState()
{
    QDBusInterface session(QLatin1String(kService), sessionPath_,
                           QLatin1String(kSessionInterface),
                           QDBusConnection::systemBus());
    if (!session.isValid()) {
        return;
    }
    session.setTimeout(kInitialQueryTimeoutMs);
    setReason(core::SessionSuspendReason::Locked,
              session.property("LockedHint").toBool());
    setReason(core::SessionSuspendReason::Inactive,
              !session.property("Active").toBool());
}

void LinuxSessionMonitor::readInitialScreenSaverState()
{
    QDBusInterface screenSaver(QLatin1String(kScreenSaverService),
                               QLatin1String(kScreenSaverPath),
                               QLatin1String(kScreenSaverInterface),
                               QDBusConnection::sessionBus());
    if (!screenSaver.isValid()) {
        return;
    }
    screenSaver.setTimeout(kInitialQueryTimeoutMs);
    const QDBusReply<bool> active = screenSaver.call(QStringLiteral("GetActive"));
    if (active.isValid()) {
        handleScreenSaverActive(active.value());
    }
}

} // namespace mub::platform
