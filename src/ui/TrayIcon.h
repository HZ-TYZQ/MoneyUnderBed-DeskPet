#pragma once

#include "core/ActivityMode.h"

#include <QObject>

#include <memory>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace mub::ui {

// 系统托盘图标。
//
// docs/Decisions.md 第 3.3 节：托盘**只作为备用入口**，必要功能不能只存在于
// 托盘中；托盘可用时提供显示角色、模式切换和退出等常用控制。
// 因此本类提供的每一项在角色右键菜单里都另有入口，唯一的例外是「显示角色」
// —— 角色被隐藏时右键菜单也点不到，这正是托盘存在的理由；
// 没有托盘的环境（原生 GNOME）由再次启动程序完成唤回。
//
// 托盘不可用时本类保持惰性：不创建图标，isActive() 为假，不影响其余功能。
class TrayIcon final : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

    // 当前桌面环境是否提供托盘。
    static bool isAvailable();

    // 图标是否真的建立起来了。
    bool isActive() const;

    void setMode(core::ActivityMode mode);
    // 角色隐藏时「显示角色」才有意义。
    void setCharacterVisible(bool visible);

signals:
    void showCharacterRequested();
    void modeChangeRequested(core::ActivityMode mode);
    void quitRequested();

private:
    // QMenu 是 QWidget，不能挂在 QSystemTrayIcon（QObject）名下，
    // 因此由本类直接持有。
    std::unique_ptr<QMenu> menu_;
    QSystemTrayIcon *tray_ = nullptr;
    QAction *show_ = nullptr;
    QAction *active_ = nullptr;
};

} // namespace mub::ui
