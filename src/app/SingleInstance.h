#pragma once

#include <QObject>
#include <QString>

class QLocalServer;

namespace mub::app {

// 单实例与唤回。
//
// docs/Decisions.md 第 3.3 节：程序必须是单实例；已经运行时再次启动应唤回
// 并显示现有角色，不创建第二个实例。第 3.3 节还规定，Linux 原生 GNOME 可能
// 没有托盘，因此「再次启动」是正式唤回通道，不是可有可无的便利功能。
//
// 两个平台都用 Qt 的本地套接字（Linux 是 Unix socket，Windows 是命名管道），
// 不引入平台特有的互斥量：第 8.4 节要求优先使用 Qt，能力不足时才下沉到平台层，
// 而这里 Qt 已经足够 —— 端点名被占用本身就是「已有实例」的判据。
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

    explicit SingleInstance(QString name, QObject *parent = nullptr);
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
    Role role_ = Role::Secondary;
    QLocalServer *server_ = nullptr;
};

// 本机当前用户专用的端点名。
//
// 必须带用户区分：Linux 的套接字文件默认落在共享的临时目录下，
// 只用应用 ID 会让同一台机器上的另一个用户被误判成「已有实例」。
QString singleInstanceName();

} // namespace mub::app
