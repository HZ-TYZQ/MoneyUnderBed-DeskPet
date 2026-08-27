#pragma once

#include <QString>

#include <memory>

namespace mub::platform {

// 唯一实例锁的取得结果。
enum class InstanceLockResult
{
    // 本进程取得了锁，是唯一实例。
    Acquired,
    // 已有实例持有锁。
    AlreadyHeld,
    // 本平台不提供独立的锁，由调用方改用端点占用判断唯一性。
    Unsupported,
};

// 唯一实例锁。
//
// docs/Decisions.md 第 3.3 节要求程序必须是单实例。判断「是否已有实例」和
// 「怎么把唤回消息送过去」是**两件事**：
//
// - Windows 用命名互斥量判断唯一性。它是内核对象，取得与否是原子的，
//   进程结束时由操作系统释放，不会留下需要清理的残留。
// - Linux 不单独提供锁：Unix socket 端点被占用本身就是判据，
//   遗留端点由 `QLocalServer::removeServer()` 处理。
//
// 两个平台的**唤回**都走 Qt 本地 IPC，与本接口无关。
//
// 第 8.4 节要求平台分叉集中在窄接口里，因此本接口放在 platform 而不是 app。
class InstanceLock
{
public:
    InstanceLock() = default;
    virtual ~InstanceLock();

    InstanceLock(const InstanceLock &) = delete;
    InstanceLock &operator=(const InstanceLock &) = delete;
    InstanceLock(InstanceLock &&) = delete;
    InstanceLock &operator=(InstanceLock &&) = delete;

    // 尝试取得名为 `name` 的锁。只应调用一次。
    virtual InstanceLockResult tryAcquire(const QString &name) = 0;

    // 释放。未取得时是空操作。析构时自动调用。
    virtual void release() = 0;
};

std::unique_ptr<InstanceLock> createInstanceLock();

} // namespace mub::platform
