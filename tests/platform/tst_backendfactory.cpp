#include "platform/BackendFactory.h"
#include "platform/DeskPetWindowBackend.h"

#include <QTest>
#include <QWindow>

#include <memory>

using mub::platform::BackendCapabilities;
using mub::platform::createWindowBackend;
using mub::platform::DeskPetWindowBackend;

class TestBackendFactory final : public QObject
{
    Q_OBJECT

private slots:
    void alwaysProducesABackend();
    void capabilitiesNameTheActualPlatform();
    void deskPetCapabilitiesAreAvailableOnEveryPlatform();
};

void TestBackendFactory::alwaysProducesABackend()
{
    // 工厂是产品代码里唯一按操作系统分支的地方（第 8.4 节）。
    // 任何平台上都必须给出一个可用实现，不允许返回空。
    const std::unique_ptr<DeskPetWindowBackend> backend = createWindowBackend();
    QVERIFY(backend != nullptr);
}

void TestBackendFactory::capabilitiesNameTheActualPlatform()
{
    const std::unique_ptr<DeskPetWindowBackend> backend = createWindowBackend();
    const BackendCapabilities caps = backend->capabilities();

    // 后端名会进诊断信息，空名字等于诊断里少一条关键线索。
    QVERIFY(!caps.name.isEmpty());
#if defined(Q_OS_WIN)
    QVERIFY2(caps.name.startsWith(QStringLiteral("windows/")), qPrintable(caps.name));
#elif defined(Q_OS_LINUX)
    QVERIFY2(caps.name.startsWith(QStringLiteral("linux/")), qPrintable(caps.name));
#endif
}

// 这些是第 3.4 节要求的桌宠基本能力，两个平台都已实测通过，
// 因此任何平台的后端都不得自述为不支持。
void TestBackendFactory::deskPetCapabilitiesAreAvailableOnEveryPlatform()
{
    const std::unique_ptr<DeskPetWindowBackend> backend = createWindowBackend();
    const BackendCapabilities caps = backend->capabilities();

    QVERIFY(caps.alwaysOnTop);
    QVERIFY(caps.pixelHitMask);
    QVERIFY(caps.inputPassthrough);
    QVERIFY(caps.excludeFromWindowList);
}

QTEST_MAIN(TestBackendFactory)
#include "tst_backendfactory.moc"
