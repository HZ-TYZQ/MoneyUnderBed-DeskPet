#include "app/SingleInstance.h"
#include "core/AppMetadata.h"
#include "platform/InstanceLock.h"

#include <QDir>
#include <QFile>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTest>
#include <QThread>
#include <QUuid>

#include <memory>

using mub::app::SingleInstance;
using mub::app::singleInstanceName;

namespace {

// 每个用例一个独立端点名，避免用例之间互相干扰，也不碰真实运行中的程序。
QString uniqueName()
{
    return QStringLiteral("mub-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128).left(12));
}

// 可控的锁替身。产品在 Windows 上用命名互斥量，Linux 上没有锁，
// 用替身才能在任何平台上都覆盖到两条分支。
class ScriptedLock final : public mub::platform::InstanceLock
{
public:
    explicit ScriptedLock(const mub::platform::InstanceLockResult result)
        : result_(result)
    {
    }

    mub::platform::InstanceLockResult tryAcquire(const QString &name) override
    {
        Q_UNUSED(name)
        ++acquireCount;
        return result_;
    }

    void release() override { ++releaseCount; }

    int acquireCount = 0;
    int releaseCount = 0;

private:
    mub::platform::InstanceLockResult result_;
};

} // namespace

class TestSingleInstance final : public QObject
{
    Q_OBJECT

private slots:
    void endpointNameIsPerUserAndCarriesTheApplicationId();
    void firstInstanceBecomesPrimary();
    void secondInstanceRecallsTheFirstAndStepsAside();
    void aHeldLockAloneMakesTheInstanceStepAside();
    void anAcquiredLockStillOpensTheRecallChannel();
    void staleEndpointDoesNotBlockStartup();
    void endpointIsReusableAfterThePrimaryGoesAway();
};

void TestSingleInstance::endpointNameIsPerUserAndCarriesTheApplicationId()
{
    const QString name = singleInstanceName();
    QVERIFY(!name.isEmpty());
    QVERIFY(name.startsWith(mub::metadata::applicationId()));
    // 必须带用户区分后缀：Linux 的套接字文件默认落在共享临时目录下。
    QVERIFY(name.size() > mub::metadata::applicationId().size() + 1);
    // 同一用户下必须稳定，否则每次启动都会认不出已有实例。
    QCOMPARE(singleInstanceName(), name);
}

void TestSingleInstance::firstInstanceBecomesPrimary()
{
    SingleInstance instance(uniqueName());
    QCOMPARE(instance.acquire(), SingleInstance::Role::Primary);
    QCOMPARE(instance.role(), SingleInstance::Role::Primary);
}

void TestSingleInstance::secondInstanceRecallsTheFirstAndStepsAside()
{
    const QString name = uniqueName();

    SingleInstance first(name);
    QCOMPARE(first.acquire(), SingleInstance::Role::Primary);
    QSignalSpy recalls(&first, &SingleInstance::recallRequested);

    // 副实例必须跑在另一个线程里。产品里它是另一个**进程**，主实例的事件循环
    // 一直在转；同线程直接调用 acquire() 会把主实例的事件循环一起堵住，
    // 服务端收不到连接，测的就不是真实时序了。
    SingleInstance::Role role = SingleInstance::Role::Primary;
    QThread *worker = QThread::create([&name, &role] {
        SingleInstance second(name);
        role = second.acquire();
    });
    worker->start();

    // 第 3.3 节：已经运行时再次启动应唤回现有角色，不创建第二个实例。
    QTRY_COMPARE_WITH_TIMEOUT(recalls.count(), 1, 5000);
    QVERIFY(worker->wait(5000));
    QCOMPARE(role, SingleInstance::Role::Secondary);
    delete worker;
}

// 锁被占用就是确定的「已有实例」，即使唤回消息送不到也必须退让 ——
// 送不到只说明对方忙或正在退出，不说明可以再开一个。
void TestSingleInstance::aHeldLockAloneMakesTheInstanceStepAside()
{
    auto lock = std::make_unique<ScriptedLock>(
        mub::platform::InstanceLockResult::AlreadyHeld);
    ScriptedLock *observed = lock.get();

    // 端点上没有任何服务端在监听，唤回必然失败。
    SingleInstance instance(uniqueName(), std::move(lock));
    QCOMPARE(instance.acquire(), SingleInstance::Role::Secondary);
    QCOMPARE(observed->acquireCount, 1);
}

// 取得了锁就是主实例。锁与唤回通道是两件事：即使端点此刻空着，
// 拿到锁的一方也必须建立起服务端，否则隐藏后就再也唤不回来。
void TestSingleInstance::anAcquiredLockStillOpensTheRecallChannel()
{
    const QString name = uniqueName();

    SingleInstance instance(
        name,
        std::make_unique<ScriptedLock>(mub::platform::InstanceLockResult::Acquired));
    QCOMPARE(instance.acquire(), SingleInstance::Role::Primary);

    // 端点确实在监听：另一个进程连得上，才谈得上唤回。
    QLocalSocket probe;
    probe.connectToServer(name);
    QVERIFY(probe.waitForConnected(2000));
    probe.abort();
}

// 上次异常退出留下的遗留端点不能让程序再也起不来。
void TestSingleInstance::staleEndpointDoesNotBlockStartup()
{
#ifdef Q_OS_WIN
    QSKIP("Windows 使用命名管道，进程结束后不会留下端点文件。");
#else
    const QString name = uniqueName();
    const QString path = QDir::tempPath() + QLatin1Char('/') + name;

    // 一个同名的普通文件就足以模拟遗留套接字：连不上，但占着名字。
    QFile stale(path);
    QVERIFY(stale.open(QIODevice::WriteOnly));
    stale.close();
    QVERIFY(QFile::exists(path));

    SingleInstance instance(name);
    QCOMPARE(instance.acquire(), SingleInstance::Role::Primary);
#endif
}

void TestSingleInstance::endpointIsReusableAfterThePrimaryGoesAway()
{
    const QString name = uniqueName();
    {
        SingleInstance first(name);
        QCOMPARE(first.acquire(), SingleInstance::Role::Primary);
    }

    // 前一个实例退出后，同一个端点名必须能重新拿下，
    // 否则一次正常退出就会让下一次启动误判成「已有实例」。
    SingleInstance second(name);
    QCOMPARE(second.acquire(), SingleInstance::Role::Primary);
}

QTEST_MAIN(TestSingleInstance)
#include "tst_singleinstance.moc"
