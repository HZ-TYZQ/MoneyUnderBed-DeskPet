#include "app/SingleInstance.h"

#include "core/AppMetadata.h"

#include <QCryptographicHash>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <utility>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcInstance, "mub.app.instance")

// 唤回消息只有一个动作，不需要协议。内容仅用于确认对端确实是本程序。
constexpr auto kRecallMessage = "recall";
constexpr int kConnectTimeoutMs = 500;

} // namespace

QString singleInstanceName()
{
    // 用配置目录路径派生用户区分后缀：它对每个用户唯一，且不必读环境变量，
    // 也不会把用户名原样写进套接字名。
    const QByteArray seed =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
            .toUtf8();
    const QString suffix = QString::fromLatin1(
        QCryptographicHash::hash(seed, QCryptographicHash::Sha256).toHex().left(12));
    return QStringLiteral("%1-%2").arg(metadata::applicationId(), suffix);
}

SingleInstance::SingleInstance(QString name, QObject *parent)
    : QObject(parent)
    , name_(std::move(name))
{
}

SingleInstance::~SingleInstance() = default;

SingleInstance::Role SingleInstance::role() const
{
    return role_;
}

SingleInstance::Role SingleInstance::acquire()
{
    const auto notifyExisting = [this] {
        QLocalSocket socket;
        socket.connectToServer(name_);
        if (!socket.waitForConnected(kConnectTimeoutMs)) {
            return false;
        }
        socket.write(kRecallMessage);
        socket.waitForBytesWritten(kConnectTimeoutMs);
        socket.disconnectFromServer();
        return true;
    };

    if (notifyExisting()) {
        qCInfo(lcInstance) << "another instance is running; sent a recall request";
        role_ = Role::Secondary;
        return role_;
    }

    // 连不上有两种情况：确实没有实例在跑，或上次异常退出留下了遗留端点。
    // removeServer() 只在连接失败之后调用，因此不会踢掉一个活着的实例。
    QLocalServer::removeServer(name_);

    server_ = new QLocalServer(this);
    // 只允许当前用户连接，避免同机其他用户向本实例发唤回。
    server_->setSocketOptions(QLocalServer::UserAccessOption);
    connect(server_, &QLocalServer::newConnection, this,
            &SingleInstance::handleConnection);

    if (!server_->listen(name_)) {
        // 罕见竞态：两个进程几乎同时启动，本进程刚才连不上，
        // 对方在这中间抢先监听成功。再连一次，连上就退为副实例。
        if (notifyExisting()) {
            qCInfo(lcInstance) << "lost the startup race; sent a recall request";
            role_ = Role::Secondary;
            return role_;
        }
        qCCritical(lcInstance).noquote()
            << QStringLiteral("could not listen on %1: %2")
                   .arg(name_, server_->errorString());
        // 监听失败又连不上时按主实例继续运行。宁可失去唤回通道，
        // 也不要因为一个辅助功能拒绝启动。
        role_ = Role::Primary;
        return role_;
    }

    qCInfo(lcInstance).noquote()
        << QStringLiteral("primary instance listening on %1").arg(name_);
    role_ = Role::Primary;
    return role_;
}

void SingleInstance::handleConnection()
{
    while (QLocalSocket *socket = server_->nextPendingConnection()) {
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
        socket->waitForReadyRead(kConnectTimeoutMs);
        const QByteArray message = socket->readAll();
        socket->disconnectFromServer();

        if (message != QByteArray(kRecallMessage)) {
            qCWarning(lcInstance) << "ignoring an unexpected message on the local socket";
            continue;
        }
        qCInfo(lcInstance) << "recall requested by another instance";
        emit recallRequested();
    }
}

} // namespace mub::app
