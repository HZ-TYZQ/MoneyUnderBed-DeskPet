#include "platform/InstanceLock.h"

#include <QTest>
#include <QUuid>

#include <memory>

using mub::platform::createInstanceLock;
using mub::platform::InstanceLock;
using mub::platform::InstanceLockResult;

namespace {

QString uniqueName()
{
    return QStringLiteral("mub-lock-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128).left(12));
}

} // namespace

class TestInstanceLock final : public QObject
{
    Q_OBJECT

private slots:
    void factoryAlwaysProducesALock();
    void platformsWithoutALockSaySoExplicitly();
    void aSecondLockOnTheSameNameIsRefused();
    void releasingMakesTheNameAvailableAgain();
    void differentNamesDoNotCollide();
};

void TestInstanceLock::factoryAlwaysProducesALock()
{
    // 没有锁的平台也要返回一个明确回答 Unsupported 的实现，不能返回空。
    QVERIFY(createInstanceLock() != nullptr);
}

// Linux 不单独加锁：Unix socket 端点被占用本身就是唯一性判据，
// 再叠一个锁文件只会多出一个需要清理的残留物。
void TestInstanceLock::platformsWithoutALockSaySoExplicitly()
{
#ifdef Q_OS_WIN
    QSKIP("Windows 提供命名互斥量。");
#else
    const std::unique_ptr<InstanceLock> lock = createInstanceLock();
    QCOMPARE(lock->tryAcquire(uniqueName()), InstanceLockResult::Unsupported);
    // release() 在未取得时必须是安全的空操作。
    lock->release();
    lock->release();
#endif
}

// 唯一性的核心断言：同一个名字第二次取不到。
void TestInstanceLock::aSecondLockOnTheSameNameIsRefused()
{
#ifndef Q_OS_WIN
    QSKIP("本平台不提供独立的锁，唯一性由端点占用判断，见 tst_singleinstance。");
#else
    const QString name = uniqueName();

    const std::unique_ptr<InstanceLock> first = createInstanceLock();
    QCOMPARE(first->tryAcquire(name), InstanceLockResult::Acquired);

    const std::unique_ptr<InstanceLock> second = createInstanceLock();
    QCOMPARE(second->tryAcquire(name), InstanceLockResult::AlreadyHeld);
#endif
}

// 主实例退出后，下一次启动必须能重新拿到锁。
void TestInstanceLock::releasingMakesTheNameAvailableAgain()
{
#ifndef Q_OS_WIN
    QSKIP("本平台不提供独立的锁。");
#else
    const QString name = uniqueName();
    {
        const std::unique_ptr<InstanceLock> first = createInstanceLock();
        QCOMPARE(first->tryAcquire(name), InstanceLockResult::Acquired);
    }

    const std::unique_ptr<InstanceLock> second = createInstanceLock();
    QCOMPARE(second->tryAcquire(name), InstanceLockResult::Acquired);
#endif
}

void TestInstanceLock::differentNamesDoNotCollide()
{
#ifndef Q_OS_WIN
    QSKIP("本平台不提供独立的锁。");
#else
    const std::unique_ptr<InstanceLock> first = createInstanceLock();
    const std::unique_ptr<InstanceLock> second = createInstanceLock();
    QCOMPARE(first->tryAcquire(uniqueName()), InstanceLockResult::Acquired);
    // 端点名带用户区分后缀，同机不同用户必须互不影响。
    QCOMPARE(second->tryAcquire(uniqueName()), InstanceLockResult::Acquired);
#endif
}

QTEST_MAIN(TestInstanceLock)
#include "tst_instancelock.moc"
