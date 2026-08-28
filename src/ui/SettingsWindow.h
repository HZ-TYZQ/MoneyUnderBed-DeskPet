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
// 不设额外「应用」阶段；提供「恢复默认设置」。
//
// 本窗口不自己读写配置文件，也不直接操作角色：改动通过 settingsChanged()
// 上报，由上层负责套用和保存。这样设置界面可以脱离产品单独测试。
//
// **阶段 2 的过渡状态。** 第 14.2 节要求的四组结构、普通/高级分层、滑块与
// 按组重置都属于阶段 4。本窗口目前只暴露 `1.0.0` 候选就有的四个设置项，
// 其余新参数原样透传：`settings()` 以最近一次 `setSettings()` 的完整取值为底，
// 只覆盖自己编辑的那几个字段，绝不把没显示的参数重置成默认值。
class SettingsWindow final : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);

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

    bool updating_ = false;

    // 界面只编辑其中几个字段，其余字段从这里原样带回（见类注释）。
    core::Settings current_;

    QComboBox *mode_ = nullptr;
    QComboBox *speech_ = nullptr;
    QCheckBox *alwaysOnTop_ = nullptr;
    QComboBox *scale_ = nullptr;
    QPushButton *restoreDefaults_ = nullptr;

    bool desktopEntryInstalled_ = false;
    QLabel *desktopEntryLabel_ = nullptr;
    QPushButton *desktopEntryButton_ = nullptr;
};

} // namespace mub::ui
