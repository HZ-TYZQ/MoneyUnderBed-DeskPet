#include "ui/SettingsWindow.h"

#include "core/SettingsPresets.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

namespace mub::ui {

namespace {

// 「自定义」不是档位，用一个不会与枚举值相撞的标识占位。
constexpr int kCustomPreset = -1;

int indexOfData(const QComboBox *box, const QVariant &value)
{
    const int index = box->findData(value);
    return index >= 0 ? index : 0;
}

} // namespace

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
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

    speech_ = new QComboBox(this);
    speech_->addItem(tr("关闭"), static_cast<int>(core::SpeechFrequency::Off));
    speech_->addItem(tr("低"), static_cast<int>(core::SpeechFrequency::Low));
    speech_->addItem(tr("中"), static_cast<int>(core::SpeechFrequency::Normal));
    speech_->addItem(tr("高"), static_cast<int>(core::SpeechFrequency::High));
    // 高级层改过间隔或概率后不再匹配任何档位。第 14.2 节：此时显示「自定义」，
    // 选中它不写回任何档位，保持用户自己填的取值。
    speech_->addItem(tr("自定义"), kCustomPreset);
    speech_->setToolTip(tr("安静模式完全抑制气泡，与本设置无关。"));
    form->addRow(tr("说话频率"), speech_);

    alwaysOnTop_ = new QCheckBox(tr("始终置顶"), this);
    form->addRow(QString(), alwaysOnTop_);

    scale_ = new QComboBox(this);
    for (const int scale : core::allowedScales()) {
        scale_->addItem(tr("%1×").arg(scale), scale);
    }
    scale_->setToolTip(
        tr("只提供整数倍率。系统 DPI 缩放会再叠加一层，只有两者的乘积为整数时像素才完全清晰。"));
    form->addRow(tr("显示倍率"), scale_);

    // 应用菜单入口。默认隐藏，只有以 AppImage 运行时由调用方打开。
    desktopEntryLabel_ = new QLabel(tr("应用菜单"), this);
    desktopEntryButton_ = new QPushButton(this);
    form->addRow(desktopEntryLabel_, desktopEntryButton_);
    desktopEntryLabel_->hide();
    desktopEntryButton_->hide();
    connect(desktopEntryButton_, &QPushButton::clicked, this, [this] {
        if (desktopEntryInstalled_) {
            emit removeDesktopEntryRequested();
        } else {
            emit installDesktopEntryRequested();
        }
    });

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
    connect(speech_, &QComboBox::currentIndexChanged, this,
            &SettingsWindow::emitIfNotUpdating);
    connect(scale_, &QComboBox::currentIndexChanged, this,
            &SettingsWindow::emitIfNotUpdating);
    connect(alwaysOnTop_, &QCheckBox::toggled, this, &SettingsWindow::emitIfNotUpdating);
}

void SettingsWindow::setSettings(const core::Settings &settings)
{
    current_ = core::sanitized(settings);

    const std::optional<core::SpeechFrequency> speech =
        core::matchSpeechFrequency(current_.dialogue);

    updating_ = true;
    mode_->setCurrentIndex(
        indexOfData(mode_, static_cast<int>(current_.behavior.mode)));
    speech_->setCurrentIndex(indexOfData(
        speech_, speech ? static_cast<int>(*speech) : kCustomPreset));
    alwaysOnTop_->setChecked(current_.window.alwaysOnTop);
    scale_->setCurrentIndex(indexOfData(scale_, current_.appearance.scale));
    updating_ = false;
}

core::Settings SettingsWindow::settings() const
{
    // 以最近一次收到的完整取值为底，只覆盖本窗口编辑的字段。
    core::Settings settings = current_;
    settings.behavior.mode =
        static_cast<core::ActivityMode>(mode_->currentData().toInt());
    settings.window.alwaysOnTop = alwaysOnTop_->isChecked();
    settings.appearance.scale = scale_->currentData().toInt();

    const int speech = speech_->currentData().toInt();
    if (speech != kCustomPreset) {
        core::applySpeechFrequency(settings.dialogue,
                                   static_cast<core::SpeechFrequency>(speech));
    }
    return core::sanitized(settings);
}

void SettingsWindow::setDesktopEntryState(const bool offered, const bool installed)
{
    desktopEntryInstalled_ = installed;
    desktopEntryLabel_->setVisible(offered);
    desktopEntryButton_->setVisible(offered);
    desktopEntryButton_->setText(installed ? tr("移除应用菜单入口")
                                           : tr("加入应用菜单"));
}

void SettingsWindow::emitIfNotUpdating()
{
    if (updating_) {
        return;
    }
    emit settingsChanged(settings());
}

} // namespace mub::ui
