#pragma once

#include <QString>

class QImage;

namespace mub::app {

// Linux 应用菜单入口（`.desktop` 文件）。
//
// docs/Decisions.md 第 5.2 节：AppImage 首次启动时**询问**用户是否安装入口，
// 不静默写入，也不依赖 AppImageLauncher；程序必须提供移除自己创建的入口；
// 用户移动 AppImage 后重新运行该文件可以更新集成。
// 第 3.3 节还规定，原生 GNOME 没有托盘时，应用菜单的二次启动是正式唤回通道。
//
// 本类只做文件读写，不弹任何界面，也不判断当前是不是 AppImage —— 那是调用方
// 的判断。这样安装、更新和移除的逻辑可以在临时目录里完整测试。
class DesktopEntry
{
public:
    // 目录由调用方给出，测试传临时目录，产品传 XDG 数据目录。
    DesktopEntry(QString applicationsDir, QString iconsDir);

    QString entryFilePath() const;
    QString iconFilePath() const;

    bool isInstalled() const;

    // 已安装入口记录的可执行文件路径。未安装或读不出时返回空串。
    QString installedExecutable() const;

    // 写入或覆盖入口。`icon` 为空时只写 `.desktop`，不写图标。
    // 覆盖已存在的入口就是「更新」：AppImage 被移动后重新运行走的正是这条路。
    bool install(const QString &executablePath, const QImage &icon);

    // 移除本程序创建的入口与图标。未安装时视为成功。
    bool remove();

private:
    QString applicationsDir_;
    QString iconsDir_;
};

// `.desktop` 文件内容。单独暴露以便逐字段核对。
QString desktopEntryContents(const QString &executablePath, const QString &iconName);

} // namespace mub::app
