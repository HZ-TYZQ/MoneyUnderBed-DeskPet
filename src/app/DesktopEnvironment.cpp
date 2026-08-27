#include "app/DesktopEnvironment.h"

#include <QList>
#include <QString>

namespace mub::app {

bool desktopListContainsNiri(const QStringView value)
{
    for (const QStringView part : value.split(u':')) {
        if (part.trimmed().compare(QStringLiteral("niri"), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool isNiriDesktop()
{
#if defined(Q_OS_LINUX) || defined(__linux__)
    return desktopListContainsNiri(qEnvironmentVariable("XDG_CURRENT_DESKTOP"))
        || desktopListContainsNiri(qEnvironmentVariable("XDG_SESSION_DESKTOP"))
        || desktopListContainsNiri(qEnvironmentVariable("DESKTOP_SESSION"));
#else
    return false;
#endif
}

} // namespace mub::app
