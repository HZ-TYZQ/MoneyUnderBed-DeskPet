#include "platform/InstanceLock.h"

namespace mub::platform {

namespace {

// Linux 与其他类 Unix 平台不单独提供锁。
//
// docs/Decisions.md 第 3.3 节的要求是「必须是单实例」，没有规定用什么判据。
// Unix socket 的端点文件被占用本身就是判据，再叠一个 POSIX 信号量或锁文件
// 只会多出一个需要清理的残留物 —— 而遗留端点已经由
// `QLocalServer::removeServer()` 处理。
class GenericInstanceLock final : public InstanceLock
{
public:
    InstanceLockResult tryAcquire(const QString &name) override
    {
        Q_UNUSED(name)
        return InstanceLockResult::Unsupported;
    }

    void release() override {}
};

} // namespace

std::unique_ptr<InstanceLock> createInstanceLock()
{
    return std::make_unique<GenericInstanceLock>();
}

} // namespace mub::platform
