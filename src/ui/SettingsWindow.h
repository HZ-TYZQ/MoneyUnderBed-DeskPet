#pragma once

#include "core/Settings.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace mub::ui {

// 设置窗口。
//
// docs/Decisions.md 第 5.1 节：小型 Qt Widgets 界面；修改后立即生效并保存，
// 不设额外「应用」阶段；提供「恢复默认设置」；平台不适用的项**直接隐藏**，
// 不以置灰形式保留。
//
// 本窗口不自己读写配置文件，也不直接操作角色：改动通过 settingsChanged()
// 上报，由上层负责套用和保存。这样设置界面可以脱离产品单独测试。
class SettingsWindow final : public QDialog
{
    Q_OBJECT

public:
    // `workspaceSupported` 来自平台能力自述，不是「当前是不是 Linux」。
    // 为假时整项隐藏（Windows 没有公开稳定的固定到全部虚拟桌面的接口）。
    explicit SettingsWindow(bool workspaceSupported, QWidget *parent = nullptr);

    // 用给定取值刷新控件。刷新期间不发出 settingsChanged()，
    // 避免「套用 → 上报 → 再套用」的回环。
    void setSettings(const core::Settings &settings);
    core::Settings settings() const;

    // 应用菜单入口的当前状态。
    //
    // 第 5.2 节要求程序提供移除自己创建的入口，并把该操作放在设置或关于界面。
    // `offered` 为假时整行隐藏 —— 只有以 AppImage 运行时才谈得上这件事。
    void setDesktopEntryState(bool offered, bool installed);

signals:
    void settingsChanged(const core::Settings &settings);
    void restoreDefaultsRequested();
    void installDesktopEntryRequested();
    void removeDesktopEntryRequested();

private:
    void emitIfNotUpdating();

    bool workspaceSupported_ = false;
    bool updating_ = false;

    QComboBox *mode_ = nullptr;
    QComboBox *bubble_ = nullptr;
    QCheckBox *alwaysOnTop_ = nullptr;
    QComboBox *scale_ = nullptr;
    QComboBox *workspace_ = nullptr;
    QPushButton *restoreDefaults_ = nullptr;

    bool desktopEntryInstalled_ = false;
    QLabel *desktopEntryLabel_ = nullptr;
    QPushButton *desktopEntryButton_ = nullptr;
};

} // namespace mub::ui
