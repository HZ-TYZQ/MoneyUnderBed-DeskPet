#include "core/AppMetadata.h"

#include <QCoreApplication>
#include <QTest>

// 应用身份的取值由 docs/Decisions.md 第 1.2 节冻结。
// 本测试的作用是让任何一处改动都必须同时改决策文档。
class TestAppMetadata final : public QObject
{
    Q_OBJECT

private slots:
    void frozenIdentityValues();
    void versionStringIsNotEmpty();
    void applySetsQtGlobals();
    void userFacingTextIsTranslatable();
};

void TestAppMetadata::frozenIdentityValues()
{
    QCOMPARE(mub::metadata::applicationName(),
             QStringLiteral("MoneyUnderBed DeskPet"));
    QCOMPARE(mub::metadata::executableName(),
             QStringLiteral("money-under-bed-deskpet"));
    QCOMPARE(mub::metadata::applicationId(),
             QStringLiteral("io.github.hz_tyzq.MoneyUnderBedDeskPet"));
    QCOMPARE(mub::metadata::organizationName(), QStringLiteral("HZ-TYZQ"));
    QCOMPARE(mub::metadata::organizationDomain(),
             QStringLiteral("hz-tyzq.github.io"));

    // 应用 ID 的域名段必须用下划线，以兼容 Linux desktop ID 与 D-Bus 名称。
    QVERIFY(!mub::metadata::applicationId().contains(QLatin1Char('-')));
}

void TestAppMetadata::versionStringIsNotEmpty()
{
    const QString version = mub::metadata::versionString();
    QVERIFY(!version.isEmpty());
    QVERIFY(version.startsWith(QStringLiteral("0.")));
}

void TestAppMetadata::applySetsQtGlobals()
{
    mub::metadata::apply();

    QCOMPARE(QCoreApplication::applicationName(),
             mub::metadata::applicationName());
    QCOMPARE(QCoreApplication::applicationVersion(),
             mub::metadata::versionString());
    QCOMPARE(QCoreApplication::organizationName(),
             mub::metadata::organizationName());
    QCOMPARE(QCoreApplication::organizationDomain(),
             mub::metadata::organizationDomain());
}

void TestAppMetadata::userFacingTextIsTranslatable()
{
    // 第一版只提供简体中文，但文本必须走翻译接口而不是硬编码返回。
    QVERIFY(!mub::metadata::displayName().isEmpty());
    QVERIFY(!mub::metadata::unofficialNotice().isEmpty());

    // 非官方声明必须实际说明“非官方”，否则该声明形同虚设。
    QVERIFY(mub::metadata::unofficialNotice().contains(
        QStringLiteral("非官方")));
}

QTEST_MAIN(TestAppMetadata)
#include "tst_appmetadata.moc"
