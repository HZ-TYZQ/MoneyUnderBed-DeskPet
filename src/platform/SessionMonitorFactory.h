#pragma once

#include <memory>

namespace mub::platform {

class SessionMonitor;

std::unique_ptr<SessionMonitor> createSessionMonitor();

} // namespace mub::platform
