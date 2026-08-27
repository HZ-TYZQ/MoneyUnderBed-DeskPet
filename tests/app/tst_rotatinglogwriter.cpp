#include "app/DiagnosticPrivacy.h"
#include "app/RotatingLogWriter.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class TestRotatingLogWriter final : public QObject
{
    Q_OBJECT

private slots:
    void staysBoundedWhileTheProcessIsRunning();
    void anOversizedMessageIsTruncated();
    void privateDiagnosticFieldsAreRedacted();
    void oversizedLegacyLogIsBoundedDuringStartup();
};

void TestRotatingLogWriter::staysBoundedWhileTheProcessIsRunning()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("deskpet.log"));
    mub::app::RotatingLogWriter writer(path, 80);

    for (int i = 0; i < 20; ++i) {
        QVERIFY(writer.writeLine(QByteArray("diagnostic-line-") + QByteArray::number(i)));
    }

    QVERIFY(QFileInfo(path).size() <= 80);
    QVERIFY(QFileInfo(path + QStringLiteral(".1")).size() <= 80);
    QVERIFY(!QFile::exists(path + QStringLiteral(".2")));
}

void TestRotatingLogWriter::privateDiagnosticFieldsAreRedacted()
{
    const QString input = QDir::homePath()
        + QStringLiteral("/private/file PATH=/bin:/private HOME=/secret USER=alice");
    const QString redacted = mub::app::redactDiagnosticText(input);

    QVERIFY(!redacted.contains(QDir::homePath()));
    QVERIFY(!redacted.contains(QStringLiteral("/bin:/private")));
    QVERIFY(!redacted.contains(QStringLiteral("/secret")));
    QVERIFY(!redacted.contains(QStringLiteral("alice")));
    QVERIFY(redacted.contains(QStringLiteral("<home>")));
    QVERIFY(redacted.contains(QStringLiteral("PATH=<redacted>")));
}

void TestRotatingLogWriter::oversizedLegacyLogIsBoundedDuringStartup()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("deskpet.log"));
    QFile legacy(path);
    QVERIFY(legacy.open(QIODevice::WriteOnly));
    QCOMPARE(legacy.write(QByteArray(1000, 'x')), 1000);
    legacy.close();

    mub::app::RotatingLogWriter writer(path, 64);
    QVERIFY(writer.open());
    QCOMPARE(QFileInfo(path).size(), 0);
    QCOMPARE(QFileInfo(path + QStringLiteral(".1")).size(), 64);
}

void TestRotatingLogWriter::anOversizedMessageIsTruncated()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("deskpet.log"));
    mub::app::RotatingLogWriter writer(path, 64);

    QVERIFY(writer.writeLine(QByteArray(1000, 'x')));
    QCOMPARE(QFileInfo(path).size(), 64);
}

QTEST_APPLESS_MAIN(TestRotatingLogWriter)
#include "tst_rotatinglogwriter.moc"
