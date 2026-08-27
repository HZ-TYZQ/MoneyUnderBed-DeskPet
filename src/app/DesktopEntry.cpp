#include "app/DesktopEntry.h"

#include "core/AppMetadata.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLoggingCategory>
#include <QSettings>
#include <QTextStream>

#include <utility>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcDesktop, "mub.app.desktop")

QString entryFileName()
{
    return metadata::applicationId() + QStringLiteral(".desktop");
}

} // namespace

QString desktopEntryContents(const QString &executablePath, const QString &iconName)
{
    // Exec 里的路径要加引号：AppImage 常被放在带空格的目录下。
    QString contents;
    QTextStream out(&contents);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Version=1.0\n";
    out << "Name=" << metadata::displayName() << '\n';
    out << "Comment=" << metadata::unofficialNotice() << '\n';
    out << "Exec=\"" << executablePath << "\"\n";
    out << "Icon=" << iconName << '\n';
    out << "Categories=Utility;\n";
    // 桌宠没有主窗口图标可供任务栏关联，也不需要出现在通知区域配置里。
    out << "Terminal=false\n";
    out << "StartupNotify=false\n";
    // 二次启动即唤回，不是新开一个实例（第 3.3 节）。
    out << "SingleMainWindow=true\n";
    return contents;
}

DesktopEntry::DesktopEntry(QString applicationsDir, QString iconsDir)
    : applicationsDir_(std::move(applicationsDir))
    , iconsDir_(std::move(iconsDir))
{
}

QString DesktopEntry::entryFilePath() const
{
    return QDir(applicationsDir_).filePath(entryFileName());
}

QString DesktopEntry::iconFilePath() const
{
    return QDir(iconsDir_).filePath(metadata::applicationId() + QStringLiteral(".png"));
}

bool DesktopEntry::isInstalled() const
{
    return QFileInfo::exists(entryFilePath());
}

QString DesktopEntry::installedExecutable() const
{
    if (!isInstalled()) {
        return {};
    }
    QSettings entry(entryFilePath(), QSettings::IniFormat);
    entry.beginGroup(QStringLiteral("Desktop Entry"));
    QString exec = entry.value(QStringLiteral("Exec")).toString();
    entry.endGroup();

    // 写入时加了引号，读回来要去掉。
    if (exec.startsWith(QLatin1Char('"')) && exec.endsWith(QLatin1Char('"'))
        && exec.size() >= 2) {
        exec = exec.mid(1, exec.size() - 2);
    }
    return exec;
}

bool DesktopEntry::install(const QString &executablePath, const QImage &icon)
{
    if (!QDir().mkpath(applicationsDir_)) {
        qCWarning(lcDesktop).noquote()
            << QStringLiteral("could not create %1").arg(applicationsDir_);
        return false;
    }

    QString iconName = metadata::applicationId();
    if (!icon.isNull()) {
        if (QDir().mkpath(iconsDir_) && icon.save(iconFilePath(), "PNG")) {
            // 图标装好了才按图标主题名引用，否则退回可执行文件同名，
            // 让桌面环境自己找，而不是引用一个不存在的图标。
        } else {
            qCWarning(lcDesktop).noquote()
                << QStringLiteral("could not write %1").arg(iconFilePath());
            iconName = metadata::executableName();
        }
    } else {
        iconName = metadata::executableName();
    }

    QFile file(entryFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qCWarning(lcDesktop).noquote()
            << QStringLiteral("could not write %1").arg(entryFilePath());
        return false;
    }
    file.write(desktopEntryContents(executablePath, iconName).toUtf8());
    file.close();
    // 桌面环境要求 .desktop 可执行位才认为它可信。
    file.setPermissions(file.permissions() | QFileDevice::ExeOwner);

    qCInfo(lcDesktop).noquote()
        << QStringLiteral("installed %1 -> %2").arg(entryFilePath(), executablePath);
    return true;
}

bool DesktopEntry::remove()
{
    bool ok = true;
    if (QFileInfo::exists(entryFilePath()) && !QFile::remove(entryFilePath())) {
        qCWarning(lcDesktop).noquote()
            << QStringLiteral("could not remove %1").arg(entryFilePath());
        ok = false;
    }
    // 图标是本程序自己写进去的，随入口一起移除。
    if (QFileInfo::exists(iconFilePath()) && !QFile::remove(iconFilePath())) {
        qCWarning(lcDesktop).noquote()
            << QStringLiteral("could not remove %1").arg(iconFilePath());
        ok = false;
    }
    return ok;
}

} // namespace mub::app
