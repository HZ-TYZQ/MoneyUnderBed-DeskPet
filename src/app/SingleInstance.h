#pragma once

#include "platform/InstanceLock.h"

#include <QObject>
#include <QString>

#include <memory>

class QLocalServer;

namespace mub::app {

// 单实例与唤回。
//
// docs/Decisions.md 第 3.3 节：程序必须是单实例；已经运行时再次启动应唤回
// 并显示现有角色，不创建第二个实例。第 3.3 节还规定，Linux 原生 GNOME 可能
// 没有托盘，因此「再次启动」是正式唤回通道，不是可有可无的便利功能。
//
// **判断唯一性和传递唤回是两件事，分别用不同机制：**
//
// - 唯一性：Windows 用命名互斥量（`platform::InstanceLock`），取得与否由内核
//   原子决定，进程结束时必然释放。Linux 不单独加锁，由 Unix socket 端点占用
//   判断，遗留端点用 `QLocalServer::removeServer()` 清理。
// - 唤回：两个平台一律走 Qt 本地 IPC。
//
// 不让本地套接字兼任唯一性锁：那要靠「先连一次、连不上再监听」推断，中间有
// 竞态窗口，还要自己处理残留端点。锁只在 Windows 上真正需要，因此平台分叉
// 收在 platform 的窄接口里（第 8.4 节）。
class SingleInstance final : public QObject
{
    Q_OBJECT

public:
    enum class Role
    {
        // 本进程是唯一实例，应当正常启动。
        Primary,
        // 已有实例在运行，唤回消息已发出，本进程应当立即退出。
        Secondary,
    };

    // `lock` 为空时使用当前平台的默认实现。测试可以注入替身。
    explicit SingleInstance(QString name, QObject *parent = nullptr);
    SingleInstance(QString name, std::unique_ptr<platform::InstanceLock> lock,
                   QObject *parent = nullptr);
    ~SingleInstance() override;

    // 尝试成为主实例。只应调用一次。
    Role acquire();
    Role role() const;

signals:
    // 另一个进程请求唤回。主实例据此显示并前置角色。
    void recallRequested();

private:
    void handleConnection();

    QString name_;
    std::unique_ptr<platform::InstanceLock> lock_;
    Role role_ = Role::Secondary;
    QLocalServer *server_ = nullptr;
};

// 本机当前用户专用的端点名。
//
// 必须带用户区分：Linux 的套接字文件默认落在共享的临时目录下，
// 只用应用 ID 会让同一台机器上的另一个用户被误判成「已有实例」。
QString singleInstanceName();

} // namespace mub::app
