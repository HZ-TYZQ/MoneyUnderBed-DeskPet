#include "ui/FirstRunWindow.h"

#include "core/AppMetadata.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace mub::ui {

FirstRunWindow::FirstRunWindow(const bool offerDesktopEntry, QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("mub-first-run-window"));
    setWindowTitle(metadata::displayName());

    auto *layout = new QVBoxLayout(this);

    auto *text = new QLabel(
        tr("在角色身上<b>右键</b>打开菜单：投喂、活跃模式、暂停、设置、关于和退出都在那里。\n"
           "<b>按住左键拖动</b>可以移动角色，<b>单击</b>会有反应。\n"
           "退出请用右键菜单里的「退出」。"),
        this);
    text->setTextFormat(Qt::RichText);
    text->setWordWrap(true);
    layout->addWidget(text);

    if (offerDesktopEntry) {
        // 第 5.2 节：询问，不静默写入，也不依赖 AppImageLauncher。
        desktopEntry_ = new QCheckBox(tr("把本程序加入应用菜单"), this);
        desktopEntry_->setToolTip(
            tr("在应用菜单里再次启动本程序会唤回已经运行的角色，而不是开第二个。"
               "该入口随时可以在设置里移除。"));
        layout->addWidget(desktopEntry_);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

bool FirstRunWindow::wantsDesktopEntry() const
{
    return desktopEntry_ != nullptr && desktopEntry_->isChecked();
}

} // namespace mub::ui
