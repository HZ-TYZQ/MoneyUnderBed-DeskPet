#include "app/SelfTest.h"

#include <QTest>

class TestSelfTest final : public QObject
{
    Q_OBJECT

private slots:
    void everyFailureClassHasADistinctNonzeroCode();
    void bundledProductPasses();
};

void TestSelfTest::everyFailureClassHasADistinctNonzeroCode()
{
    using namespace mub::app;
    QCOMPARE(selfTestExitCode(true, true, true, true, true, true), SelfTestSuccess);
    QCOMPARE(selfTestExitCode(false, true, true, true, true, true),
             SelfTestResourceFailure);
    QCOMPARE(selfTestExitCode(true, false, true, true, true, true), SelfTestFontFailure);
    QCOMPARE(selfTestExitCode(true, true, false, true, true, true),
             SelfTestDialogueFailure);
    QCOMPARE(selfTestExitCode(true, true, true, false, true, true),
             SelfTestConfigurationFailure);
    QCOMPARE(selfTestExitCode(true, true, true, true, false, true),
             SelfTestComponentFailure);
    QCOMPARE(selfTestExitCode(true, true, true, true, true, false),
             SelfTestPlatformFailure);
    QCOMPARE(selfTestExitCode(false, false, false, false, false, false), 63);
}

void TestSelfTest::bundledProductPasses()
{
    QCOMPARE(mub::app::runSelfTest(), mub::app::SelfTestSuccess);
}

QTEST_MAIN(TestSelfTest)
#include "tst_selftest.moc"
