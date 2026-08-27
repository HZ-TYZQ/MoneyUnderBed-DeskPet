#pragma once

#include <QDialog>

class QCheckBox;

namespace mub::ui {

// 首次启动提示。
//
// docs/Decisions.md 第 5.2 节：首次启动显示**简短提示**，说明右键菜单、拖动和
// 退出方式，**不做多页欢迎向导**。因此这里只有一页、一段文字和至多一个勾选项。
//
// AppImage 的应用菜单入口在同一页里问：第 5.2 节要求询问而不是静默写入，
// 但没有要求为它单开一步。
class FirstRunWindow final : public QDialog
{
    Q_OBJECT

public:
    // `offerDesktopEntry` 为真时显示应用菜单入口的勾选项。
    // 只有确实以 AppImage 运行时才应为真。
    explicit FirstRunWindow(bool offerDesktopEntry, QWidget *parent = nullptr);

    // 用户是否勾选了「加入应用菜单」。未提供该项时恒为假。
    bool wantsDesktopEntry() const;

private:
    QCheckBox *desktopEntry_ = nullptr;
};

} // namespace mub::ui
