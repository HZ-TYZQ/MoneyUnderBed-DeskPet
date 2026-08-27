#include "app/SingleInstance.h"
#include "core/AppMetadata.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

using mub::app::SingleInstance;
using mub::app::singleInstanceName;

namespace {

// 每个用例一个独立端点名，避免用例之间互相干扰，也不碰真实运行中的程序。
QString uniqueName()
{
    return QStringLiteral("mub-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128).left(12));
}

} // namespace

class TestSingleInstance final : public QObject
{
    Q_OBJECT

private slots:
    void endpointNameIsPerUserAndCarriesTheApplicationId();
    void firstInstanceBecomesPrimary();
    void secondInstanceRecallsTheFirstAndStepsAside();
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

    // 第 3.3 节：已经运行时再次启动应唤回现有角色，不创建第二个实例。
    SingleInstance second(name);
    QCOMPARE(second.acquire(), SingleInstance::Role::Secondary);

    QTRY_COMPARE_WITH_TIMEOUT(recalls.count(), 1, 2000);
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
