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
// 主实例收到并认可后回一个字节。
//
// 这个应答不是为了传信息，而是为了**让客户端在服务端读完之前不要断开**。
// 没有它时，客户端写完就断开，Windows 命名管道上服务端可能读到空数据，
// 唤回就会静默丢失；Unix socket 上因为缓冲区语义不同而看不出问题。
constexpr auto kAcknowledgement = "k";
constexpr int kConnectTimeoutMs = 500;
constexpr int kHandshakeTimeoutMs = 2000;

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
    : SingleInstance(std::move(name), platform::createInstanceLock(), parent)
{
}

SingleInstance::SingleInstance(QString name,
                               std::unique_ptr<platform::InstanceLock> lock,
                               QObject *parent)
    : QObject(parent)
    , name_(std::move(name))
    , lock_(std::move(lock))
{
}

SingleInstance::~SingleInstance()
{
    // 服务端先停，再放锁：反过来的话，锁已经放开而端点还在，
    // 新进程会拿到锁却监听不上。
    delete server_;
    server_ = nullptr;
    if (lock_ != nullptr) {
        lock_->release();
    }
}

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
        if (!socket.waitForBytesWritten(kHandshakeTimeoutMs)) {
            qCWarning(lcInstance) << "could not send the recall request";
            return false;
        }
        // 等主实例的应答再断开。连上了就说明确实有实例在运行，
        // 因此即使应答超时也照样退让 —— 不能因为握手慢就开第二个实例。
        if (!socket.waitForReadyRead(kHandshakeTimeoutMs)) {
            qCWarning(lcInstance)
                << "the running instance did not acknowledge the recall request";
        }
        socket.disconnectFromServer();
        return true;
    };

    // 唯一性判据。Windows 走命名互斥量，Linux 返回 Unsupported，
    // 由下面的端点占用判断接手。
    const platform::InstanceLockResult lockResult = lock_->tryAcquire(name_);

    if (lockResult == platform::InstanceLockResult::AlreadyHeld) {
        // 锁已被占用就是确定的「已有实例」。唤回消息即使没送到也照样退让 ——
        // 送不到只说明对方忙或正在退出，不说明可以再开一个。
        if (!notifyExisting()) {
            qCWarning(lcInstance)
                << "another instance holds the lock but did not accept the recall request";
        } else {
            qCInfo(lcInstance) << "another instance is running; sent a recall request";
        }
        role_ = Role::Secondary;
        return role_;
    }

    if (lockResult == platform::InstanceLockResult::Unsupported && notifyExisting()) {
        // 本平台没有独立的锁，端点被占用即已有实例在跑。
        qCInfo(lcInstance) << "another instance is running; sent a recall request";
        role_ = Role::Secondary;
        return role_;
    }

    // 走到这里：已取得锁，或本平台没有锁且端点也连不上。
    // 端点连不上有两种情况：确实没有实例在跑，或上次异常退出留下了遗留端点。
    // removeServer() 只在连接失败或已经拿到锁之后调用，
    // 因此不会踢掉一个活着的实例。
    QLocalServer::removeServer(name_);

    server_ = new QLocalServer(this);
    // 只允许当前用户连接，避免同机其他用户向本实例发唤回。
    server_->setSocketOptions(QLocalServer::UserAccessOption);
    connect(server_, &QLocalServer::newConnection, this,
            &SingleInstance::handleConnection);

    if (!server_->listen(name_)) {
        // 没有锁的平台上，这里还剩一个罕见竞态：两个进程几乎同时启动，
        // 本进程刚才连不上，对方在这中间抢先监听成功。再连一次，
        // 连上就退为副实例。取得了锁的平台不存在这个窗口，因此不必重试。
        if (lockResult == platform::InstanceLockResult::Unsupported
            && notifyExisting()) {
            qCInfo(lcInstance) << "lost the startup race; sent a recall request";
            role_ = Role::Secondary;
            return role_;
        }
        qCCritical(lcInstance).noquote()
            << QStringLiteral("could not listen on %1: %2")
                   .arg(name_, server_->errorString());
        // 监听失败时按主实例继续运行。宁可失去唤回通道，
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
    const QByteArray expected(kRecallMessage);

    while (QLocalSocket *socket = server_->nextPendingConnection()) {
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);

        // 一次 readyRead 不保证收全。按期望长度读满为止，超时就放弃这条连接。
        QByteArray message;
        while (message.size() < expected.size()
               && socket->waitForReadyRead(kHandshakeTimeoutMs)) {
            message.append(socket->readAll());
        }

        if (message != expected) {
            qCWarning(lcInstance) << "ignoring an unexpected message on the local socket";
            socket->disconnectFromServer();
            continue;
        }

        // 先应答再断开：客户端在收到应答前不会关闭连接。
        socket->write(kAcknowledgement);
        socket->waitForBytesWritten(kHandshakeTimeoutMs);
        socket->disconnectFromServer();

        qCInfo(lcInstance) << "recall requested by another instance";
        emit recallRequested();
    }
}

} // namespace mub::app
