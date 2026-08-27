#include "app/DesktopEntry.h"
#include "core/AppMetadata.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

using mub::app::DesktopEntry;
using mub::app::desktopEntryContents;

namespace {

QImage smallIcon()
{
    QImage image(8, 8, QImage::Format_ARGB32);
    image.fill(Qt::red);
    return image;
}

QString readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

} // namespace

class TestDesktopEntry final : public QObject
{
    Q_OBJECT

private slots:
    void contentsCarryTheRequiredFields();
    void executablePathIsQuoted();
    void nothingIsInstalledInAFreshDirectory();
    void installWritesTheEntryAndTheIcon();
    void installedExecutableReadsBackTheQuotedPath();
    void reinstallingUpdatesThePathInPlace();
    void removeDeletesBothFilesAndIsIdempotent();
    void removeLeavesOtherEntriesAlone();
};

void TestDesktopEntry::contentsCarryTheRequiredFields()
{
    const QString text = desktopEntryContents(QStringLiteral("/opt/pet.AppImage"),
                                              QStringLiteral("pet-icon"));
    QVERIFY(text.startsWith(QStringLiteral("[Desktop Entry]")));
    QVERIFY(text.contains(QStringLiteral("Type=Application")));
    QVERIFY(text.contains(QStringLiteral("Name=") + mub::metadata::displayName()));
    QVERIFY(text.contains(QStringLiteral("Icon=pet-icon")));
    QVERIFY(text.contains(QStringLiteral("Terminal=false")));
    // 第 1.2 节：非官方声明要出现在发行相关的展示位。
    QVERIFY(text.contains(mub::metadata::unofficialNotice()));
}

// AppImage 常被放在带空格的目录下，Exec 不加引号会被拆成两个参数。
void TestDesktopEntry::executablePathIsQuoted()
{
    const QString text = desktopEntryContents(
        QStringLiteral("/home/someone/My Apps/pet.AppImage"), QStringLiteral("icon"));
    QVERIFY(text.contains(QStringLiteral("Exec=\"/home/someone/My Apps/pet.AppImage\"")));
}

void TestDesktopEntry::nothingIsInstalledInAFreshDirectory()
{
    QTemporaryDir dir;
    DesktopEntry entry(dir.filePath(QStringLiteral("applications")),
                       dir.filePath(QStringLiteral("icons")));
    QVERIFY(!entry.isInstalled());
    QVERIFY(entry.installedExecutable().isEmpty());
}

void TestDesktopEntry::installWritesTheEntryAndTheIcon()
{
    QTemporaryDir dir;
    DesktopEntry entry(dir.filePath(QStringLiteral("applications")),
                       dir.filePath(QStringLiteral("icons")));

    // 目录还不存在，install() 必须自己建出来。
    QVERIFY(entry.install(QStringLiteral("/opt/pet.AppImage"), smallIcon()));

    QVERIFY(entry.isInstalled());
    QVERIFY(QFile::exists(entry.entryFilePath()));
    QVERIFY(QFile::exists(entry.iconFilePath()));
    QVERIFY(readAll(entry.entryFilePath()).contains(QStringLiteral("/opt/pet.AppImage")));
    // 桌面环境要求 .desktop 带可执行位才认为它可信。
    QVERIFY(QFile::permissions(entry.entryFilePath()).testFlag(QFileDevice::ExeOwner));
}

void TestDesktopEntry::installedExecutableReadsBackTheQuotedPath()
{
    QTemporaryDir dir;
    DesktopEntry entry(dir.filePath(QStringLiteral("applications")),
                       dir.filePath(QStringLiteral("icons")));

    const QString path = QStringLiteral("/home/someone/My Apps/pet.AppImage");
    QVERIFY(entry.install(path, smallIcon()));
    QCOMPARE(entry.installedExecutable(), path);
}

// 第 5.2 节：用户移动 AppImage 后重新运行该文件可以更新集成。
void TestDesktopEntry::reinstallingUpdatesThePathInPlace()
{
    QTemporaryDir dir;
    DesktopEntry entry(dir.filePath(QStringLiteral("applications")),
                       dir.filePath(QStringLiteral("icons")));

    QVERIFY(entry.install(QStringLiteral("/old/pet.AppImage"), smallIcon()));
    QVERIFY(entry.install(QStringLiteral("/new/pet.AppImage"), smallIcon()));

    QCOMPARE(entry.installedExecutable(), QStringLiteral("/new/pet.AppImage"));
    // 更新是覆盖，不是再写一个入口。
    QCOMPARE(QDir(dir.filePath(QStringLiteral("applications")))
                 .entryList(QDir::Files)
                 .size(),
             1);
    QVERIFY(!readAll(entry.entryFilePath()).contains(QStringLiteral("/old/")));
}

void TestDesktopEntry::removeDeletesBothFilesAndIsIdempotent()
{
    QTemporaryDir dir;
    DesktopEntry entry(dir.filePath(QStringLiteral("applications")),
                       dir.filePath(QStringLiteral("icons")));

    QVERIFY(entry.install(QStringLiteral("/opt/pet.AppImage"), smallIcon()));
    QVERIFY(entry.remove());

    QVERIFY(!entry.isInstalled());
    QVERIFY(!QFile::exists(entry.entryFilePath()));
    QVERIFY(!QFile::exists(entry.iconFilePath()));
    // 没装过也要能安全调用，否则设置界面上重复点击会报错。
    QVERIFY(entry.remove());
}

// 只移除本程序创建的入口，不碰应用目录里别人的文件。
void TestDesktopEntry::removeLeavesOtherEntriesAlone()
{
    QTemporaryDir dir;
    const QString applications = dir.filePath(QStringLiteral("applications"));
    DesktopEntry entry(applications, dir.filePath(QStringLiteral("icons")));
    QVERIFY(entry.install(QStringLiteral("/opt/pet.AppImage"), smallIcon()));

    const QString other = QDir(applications).filePath(QStringLiteral("other.desktop"));
    QFile file(other);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[Desktop Entry]\n");
    file.close();

    QVERIFY(entry.remove());
    QVERIFY(QFile::exists(other));
}

QTEST_MAIN(TestDesktopEntry)
#include "tst_desktopentry.moc"
