#include "app/DesktopEnvironment.h"

#include <QTest>

class TestDesktopEnvironment final : public QObject
{
    Q_OBJECT

private slots:
    void recognizesWholeDesktopTokens();
    void rejectsUnrelatedSubstrings();
};

void TestDesktopEnvironment::recognizesWholeDesktopTokens()
{
    QVERIFY(mub::app::desktopListContainsNiri(QStringLiteral("niri")));
    QVERIFY(mub::app::desktopListContainsNiri(QStringLiteral("GNOME:NIRI")));
    QVERIFY(mub::app::desktopListContainsNiri(QStringLiteral(" KDE : niri ")));
}

void TestDesktopEnvironment::rejectsUnrelatedSubstrings()
{
    QVERIFY(!mub::app::desktopListContainsNiri(QString()));
    QVERIFY(!mub::app::desktopListContainsNiri(QStringLiteral("KDE")));
    QVERIFY(!mub::app::desktopListContainsNiri(QStringLiteral("not-niri")));
}

QTEST_APPLESS_MAIN(TestDesktopEnvironment)
#include "tst_desktopenvironment.moc"
