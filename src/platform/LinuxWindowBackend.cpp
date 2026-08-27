#include "platform/LinuxWindowBackend.h"

namespace mub::platform {

BackendCapabilities LinuxWindowBackend::capabilities() const
{
    BackendCapabilities caps = QtWindowBackend::capabilities();
    caps.name = QStringLiteral("linux/xcb");
    return caps;
}

} // namespace mub::platform
