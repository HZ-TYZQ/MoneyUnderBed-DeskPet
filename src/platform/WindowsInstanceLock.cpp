#include "platform/InstanceLock.h"

#include <QLoggingCategory>

#include <windows.h>

namespace mub::platform {

namespace {

Q_LOGGING_CATEGORY(lcInstanceLock, "mub.platform.instancelock")

// `Local\` 前缀把互斥量限定在当前登录会话内。
//
// 不用 `Global\`：那会跨会话，同一台机器上另一个用户登录后就起不来了。
// 桌宠是「每个桌面会话一个」，不是「每台机器一个」。
constexpr auto kSessionPrefix = L"Local\\";

// Windows 命名互斥量。
//
// 唯一性判据就是它：取得与否由内核原子决定，不存在「先连一次再监听」
// 那种竞态窗口；进程崩溃或被强杀时由操作系统释放，不会留下残留。
// 唤回仍然走 Qt 本地 IPC，与本类无关。
class WindowsInstanceLock final : public InstanceLock
{
public:
    ~WindowsInstanceLock() override { release(); }

    InstanceLockResult tryAcquire(const QString &name) override
    {
        const std::wstring full =
            std::wstring(kSessionPrefix) + name.toStdWString();

        // bInitialOwner 为真：创建成功即同时取得所有权。
        handle_ = CreateMutexW(nullptr, TRUE, full.c_str());
        const DWORD error = GetLastError();

        if (handle_ == nullptr) {
            qCWarning(lcInstanceLock).noquote()
                << QStringLiteral("could not create the instance mutex (error %1)")
                       .arg(error);
            // 拿不到锁本身不是「已有实例」的证据，交回调用方按端点判断。
            return InstanceLockResult::Unsupported;
        }

        if (error == ERROR_ALREADY_EXISTS) {
            // 互斥量已存在，说明另一个实例持有它。本进程没有所有权，
            // 但仍持有一个句柄，必须关掉。
            release();
            return InstanceLockResult::AlreadyHeld;
        }

        return InstanceLockResult::Acquired;
    }

    void release() override
    {
        if (handle_ == nullptr) {
            return;
        }
        ReleaseMutex(handle_);
        CloseHandle(handle_);
        handle_ = nullptr;
    }

private:
    HANDLE handle_ = nullptr;
};

} // namespace

std::unique_ptr<InstanceLock> createInstanceLock()
{
    return std::make_unique<WindowsInstanceLock>();
}

} // namespace mub::platform
