#pragma once

#include "core/Settings.h"
#include "core/SettingsPresets.h"

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace mub::ui {

class ValueEditor;

// 设置窗口。
//
// docs/Decisions.md 第 14.2 节：
//
// - 分为行为、对话、外观、窗口与桌面四组。
// - 每组内分「普通」与「高级」两层。高级是**组内的展开区**，不是独立页面：
//   `idleMinMs` 这类原始数值脱离它所细化的档位就没有意义，与档位并列才可读，
//   「按组恢复默认」也才成立。
// - 普通层只提供命名档位，不出现毫秒、像素或百分比；高级层用面向用户的中文
//   名称与自然单位，不把内部变量名显示给用户。
// - **实际参数是唯一真相来源**，本窗口不保存第二份「当前预设」：档位由实际
//   参数反向匹配得出，匹配不上就显示「自定义」。
// - 「恢复默认设置」扩展为「按组恢复」与「全部恢复」两级，执行前都必须确认。
//
// 第 5.1 节：修改后立即生效并保存，不设额外「应用」阶段；只提供简体中文界面，
// 界面文本一律走 `tr()`。
//
// 本窗口不自己读写配置文件，也不直接操作角色：改动通过信号上报，由应用层的
// 设置控制器负责校验、套用和保存（第 14.2 节）。
class SettingsWindow final : public QDialog
{
    Q_OBJECT

public:
    // 确认框。产品用 QMessageBox；测试注入替身，避免模态对话框卡住。
    using Confirmer = std::function<bool(const QString &title, const QString &text)>;

    explicit SettingsWindow(QWidget *parent = nullptr);

    // 用给定取值刷新控件。刷新期间不发出任何信号，
    // 避免「套用 → 上报 → 再套用」的回环。
    void setSettings(const core::Settings &settings);
    core::Settings settings() const;

    // 应用菜单入口的当前状态。第 14.6 节把该操作定位在「窗口与桌面」组。
    // `offered` 为假时整行隐藏 —— 只有以 AppImage 运行时才谈得上这件事。
    void setDesktopEntryState(bool offered, bool installed);

    void setConfirmer(Confirmer confirmer);

signals:
    // 合法值发生变化，应当立即进入运行时配置。落盘由控制器去抖（第 14.8 节）。
    void settingsEdited(const core::Settings &settings);
    // 一次编辑完成（滑块释放、数字框完成编辑、下拉框或复选框变化），应当落盘。
    void settingsCommitted(const core::Settings &settings);

    // 用户已经确认过的重置命令。取消确认不会发出任何信号。
    void groupResetRequested(mub::core::SettingsGroup group);
    void resetAllRequested();

    void installDesktopEntryRequested();
    void removeDesktopEntryRequested();

private:
    using Field = std::function<int &(core::Settings &)>;

    QWidget *buildBehaviorGroup();
    QWidget *buildDialogueGroup();
    QWidget *buildAppearanceGroup();
    QWidget *buildWindowGroup();

    // 把一个数值行绑到设置里的一个字段，并登记到刷新列表。
    void bindEditor(ValueEditor *editor, const QString &name, Field field);
    // 组内的「高级」折叠区与「恢复本组默认值」按钮。
    void addAdvancedSection(QWidget *group, QWidget *advanced,
                            core::SettingsGroup which, const QString &groupName);

    void refreshAll();
    void refreshPresets();
    // 成对上下限不得在界面层形成非法组合：改动一端时把另一端顶开。
    void enforcePairs();

    void commitFromWidgets();
    void editFromWidgets();

    bool updating_ = false;
    Confirmer confirmer_;

    // 界面只编辑其中一部分字段，其余字段从这里原样带回。
    core::Settings current_;
    QList<QPair<ValueEditor *, Field>> editors_;

    QComboBox *mode_ = nullptr;
    QComboBox *tempo_ = nullptr;
    QComboBox *movement_ = nullptr;
    QComboBox *cursorAffinity_ = nullptr;
    QComboBox *speech_ = nullptr;
    QComboBox *clickText_ = nullptr;
    QComboBox *typingSpeed_ = nullptr;
    QComboBox *scale_ = nullptr;
    QComboBox *animationSpeed_ = nullptr;
    QCheckBox *alwaysOnTop_ = nullptr;

    bool desktopEntryInstalled_ = false;
    QLabel *desktopEntryLabel_ = nullptr;
    QPushButton *desktopEntryButton_ = nullptr;
};

} // namespace mub::ui
