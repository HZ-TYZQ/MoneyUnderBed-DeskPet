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
    void workspacePinningIsNeverClaimedWithoutSupport();
    void nullWindowsAreIgnoredInsteadOfCrashing();
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

// 第 3.4 节：Windows 不支持固定到全部虚拟桌面，且决策明确不使用未公开接口
// 模拟。Linux 只有真的拿得到 X11 连接时才支持 —— 否则设置界面会显示一个
// 点了没反应的选项。测试在 offscreen 下运行，因此这里必然为假。
void TestBackendFactory::workspacePinningIsNeverClaimedWithoutSupport()
{
    const std::unique_ptr<DeskPetWindowBackend> backend = createWindowBackend();
    QVERIFY(!backend->capabilities().workspacePinning);
}

// 设置界面按能力自述决定是否调用，但接口本身也不能因为拿到空窗口就崩。
void TestBackendFactory::nullWindowsAreIgnoredInsteadOfCrashing()
{
    const std::unique_ptr<DeskPetWindowBackend> backend = createWindowBackend();
    backend->setWorkspaceVisibility(nullptr, true);
    backend->setWorkspaceVisibility(nullptr, false);
    QVERIFY(true);
}

QTEST_MAIN(TestBackendFactory)
#include "tst_backendfactory.moc"
