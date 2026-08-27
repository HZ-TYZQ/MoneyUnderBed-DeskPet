#include "platform/InstanceLock.h"

namespace mub::platform {

// 关键函数就地定义，把 vtable 固定在这个翻译单元里。
InstanceLock::~InstanceLock() = default;

} // namespace mub::platform
