#include "platform/StartupProbe.h"

#include <QByteArray>

namespace mub::platform {

// Windows 与其他平台使用 Qt 的默认平台插件选择，没有需要预先探测的连接。
// docs/Decisions.md 第 8.2 节的报错通道只针对 Linux 的 XCB 要求。
StartupProbeResult probeWindowBackend()
{
    StartupProbeResult result;
    const QByteArray requested = qgetenv("QT_QPA_PLATFORM").trimmed();
    result.selectedPlatform = requested.isEmpty()
        ? QStringLiteral("<platform default>")
        : QString::fromLatin1(requested);
    result.detail = QStringLiteral("no pre-construction probe is required on this platform");
    return result;
}

} // namespace mub::platform
