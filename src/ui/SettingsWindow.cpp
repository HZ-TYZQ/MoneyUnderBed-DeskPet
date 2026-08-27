#include "ui/SettingsWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

namespace mub::ui {

namespace {

int indexOfData(const QComboBox *box, const QVariant &value)
{
    const int index = box->findData(value);
    return index >= 0 ? index : 0;
}

} // namespace

SettingsWindow::SettingsWindow(const bool workspaceSupported, QWidget *parent)
    : QDialog(parent)
    , workspaceSupported_(workspaceSupported)
{
    setObjectName(QStringLiteral("mub-settings-window"));
    setWindowTitle(tr("设置"));

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    layout->addLayout(form);

    mode_ = new QComboBox(this);
    mode_->addItem(tr("安静"), QVariant::fromValue(static_cast<int>(core::ActivityMode::Quiet)));
    mode_->addItem(tr("活跃"), QVariant::fromValue(static_cast<int>(core::ActivityMode::Active)));
    mode_->setToolTip(tr("安静模式仍播放待机动画，但不主动接近鼠标，也不主动显示气泡。"));
    form->addRow(tr("活动模式"), mode_);

    bubble_ = new QComboBox(this);
    bubble_->addItem(tr("关闭"), static_cast<int>(core::BubbleFrequency::Off));
    bubble_->addItem(tr("低频"), static_cast<int>(core::BubbleFrequency::Low));
    bubble_->addItem(tr("正常"), static_cast<int>(core::BubbleFrequency::Normal));
    bubble_->setToolTip(tr("安静模式完全抑制气泡，与本设置无关。"));
    form->addRow(tr("气泡"), bubble_);

    alwaysOnTop_ = new QCheckBox(tr("始终置顶"), this);
    form->addRow(QString(), alwaysOnTop_);

    scale_ = new QComboBox(this);
    for (const int scale : core::allowedScales()) {
        scale_->addItem(tr("%1×").arg(scale), scale);
    }
    scale_->setToolTip(
        tr("只提供整数倍率。系统 DPI 缩放会再叠加一层，只有两者的乘积为整数时像素才完全清晰。"));
    form->addRow(tr("显示倍率"), scale_);

    workspace_ = new QComboBox(this);
    workspace_->addItem(tr("所有工作区"),
                        static_cast<int>(core::WorkspaceVisibility::AllWorkspaces));
    workspace_->addItem(tr("当前工作区"),
                        static_cast<int>(core::WorkspaceVisibility::CurrentWorkspace));
    // 平台不适用的设置项直接隐藏，不置灰。
    if (workspaceSupported_) {
        form->addRow(tr("Linux 工作区"), workspace_);
    } else {
        workspace_->hide();
    }

    auto *buttons = new QDialogButtonBox(this);
    restoreDefaults_ =
        buttons->addButton(tr("恢复默认设置"), QDialogButtonBox::ResetRole);
    buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(restoreDefaults_, &QPushButton::clicked, this,
            &SettingsWindow::restoreDefaultsRequested);

    // 第 5.1 节：修改后立即生效并保存，不设「应用」阶段。
    connect(mode_, &QComboBox::currentIndexChanged, this,
            &SettingsWindow::emitIfNotUpdating);
    connect(bubble_, &QComboBox::currentIndexChanged, this,
            &SettingsWindow::emitIfNotUpdating);
    connect(scale_, &QComboBox::currentIndexChanged, this,
            &SettingsWindow::emitIfNotUpdating);
    connect(workspace_, &QComboBox::currentIndexChanged, this,
            &SettingsWindow::emitIfNotUpdating);
    connect(alwaysOnTop_, &QCheckBox::toggled, this, &SettingsWindow::emitIfNotUpdating);
}

void SettingsWindow::setSettings(const core::Settings &settings)
{
    const core::Settings valid = core::sanitized(settings);

    updating_ = true;
    mode_->setCurrentIndex(indexOfData(mode_, static_cast<int>(valid.mode)));
    bubble_->setCurrentIndex(indexOfData(bubble_, static_cast<int>(valid.bubble)));
    alwaysOnTop_->setChecked(valid.alwaysOnTop);
    scale_->setCurrentIndex(indexOfData(scale_, valid.scale));
    workspace_->setCurrentIndex(indexOfData(workspace_, static_cast<int>(valid.workspace)));
    updating_ = false;
}

core::Settings SettingsWindow::settings() const
{
    core::Settings settings;
    settings.mode = static_cast<core::ActivityMode>(mode_->currentData().toInt());
    settings.bubble =
        static_cast<core::BubbleFrequency>(bubble_->currentData().toInt());
    settings.alwaysOnTop = alwaysOnTop_->isChecked();
    settings.scale = scale_->currentData().toInt();
    // 平台不支持时该项不显示，也就不参与设置；保持默认值。
    settings.workspace = workspaceSupported_
        ? static_cast<core::WorkspaceVisibility>(workspace_->currentData().toInt())
        : core::Settings{}.workspace;
    return core::sanitized(settings);
}

void SettingsWindow::emitIfNotUpdating()
{
    if (updating_) {
        return;
    }
    emit settingsChanged(settings());
}

} // namespace mub::ui
