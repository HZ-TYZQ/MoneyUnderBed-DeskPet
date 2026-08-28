#include "ui/SettingsWindow.h"

#include "ui/ValueEditor.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QToolButton>
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

void selectPreset(QComboBox *box, const int data)
{
    const QSignalBlocker blocker(box);
    box->setCurrentIndex(indexOfData(box, data));
}

int currentPreset(const QComboBox *box)
{
    return box->currentData().toInt();
}

// 每个档位下拉框末尾都带一个「自定义」：高级层改过之后反向匹配不上时显示它。
void appendCustom(QComboBox *box)
{
    box->addItem(QCoreApplication::translate("SettingsWindow", "自定义"),
                 kCustomPreset);
}

} // namespace

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("mub-settings-window"));
    setWindowTitle(tr("设置"));

    confirmer_ = [this](const QString &title, const QString &text) {
        return QMessageBox::question(this, title, text,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)
            == QMessageBox::Yes;
    };

    auto *layout = new QVBoxLayout(this);

    // 四组都在同一页里纵向排列，高级层折叠后整窗不会过高。
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->addWidget(buildBehaviorGroup());
    contentLayout->addWidget(buildDialogueGroup());
    contentLayout->addWidget(buildAppearanceGroup());
    contentLayout->addWidget(buildWindowGroup());
    contentLayout->addStretch(1);
    scroll->setWidget(content);
    layout->addWidget(scroll, 1);

    auto *buttons = new QDialogButtonBox(this);
    QPushButton *resetAll =
        buttons->addButton(tr("全部恢复默认值"), QDialogButtonBox::ResetRole);
    buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(resetAll, &QPushButton::clicked, this, [this] {
        if (!confirmer_(tr("全部恢复默认值"),
                        tr("这会把行为、对话、外观、窗口与桌面**全部**设置恢复成"
                           "默认值。确定继续吗？"))) {
            return;
        }
        emit resetAllRequested();
    });

    refreshAll();
}

void SettingsWindow::addAdvancedSection(QWidget *group, QWidget *advanced,
                                        const core::SettingsGroup which,
                                        const QString &groupName)
{
    auto *groupLayout = qobject_cast<QVBoxLayout *>(group->layout());
    Q_ASSERT(groupLayout != nullptr);

    // 高级是组内的展开区，不是独立页面（第 14.2 节）。
    auto *toggle = new QToolButton(group);
    toggle->setText(tr("高级"));
    toggle->setCheckable(true);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    groupLayout->addWidget(toggle, 0, Qt::AlignLeft);

    advanced->setVisible(false);
    groupLayout->addWidget(advanced);
    connect(toggle, &QToolButton::toggled, this, [toggle, advanced](const bool on) {
        toggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
        advanced->setVisible(on);
    });

    auto *reset = new QPushButton(tr("恢复本组默认值"), group);
    groupLayout->addWidget(reset, 0, Qt::AlignLeft);
    connect(reset, &QPushButton::clicked, this, [this, which, groupName] {
        // 确认框必须明确显示组名（第 14.2 节）。取消不改变任何值。
        if (!confirmer_(tr("恢复本组默认值"),
                        tr("这会把「%1」这一组的全部设置恢复成默认值，"
                           "其他组不受影响。确定继续吗？")
                            .arg(groupName))) {
            return;
        }
        emit groupResetRequested(which);
    });
}

void SettingsWindow::bindEditor(ValueEditor *editor, const QString &name, Field field)
{
    // 对象名给测试和排障用，不显示给用户。
    editor->setObjectName(name);
    editors_.append({editor, field});
    connect(editor, &ValueEditor::valueEdited, this, [this, field](const int value) {
        if (updating_) {
            return;
        }
        field(current_) = value;
        editFromWidgets();
    });
    connect(editor, &ValueEditor::editingCommitted, this,
            [this, field](const int value) {
                if (updating_) {
                    return;
                }
                field(current_) = value;
                commitFromWidgets();
            });
}

QWidget *SettingsWindow::buildBehaviorGroup()
{
    auto *group = new QGroupBox(tr("行为"), this);
    auto *outer = new QVBoxLayout(group);
    auto *form = new QFormLayout;
    outer->addLayout(form);

    mode_ = new QComboBox(group);
    mode_->setObjectName(QStringLiteral("behavior-mode"));
    mode_->addItem(tr("安静"), static_cast<int>(core::ActivityMode::Quiet));
    mode_->addItem(tr("活跃"), static_cast<int>(core::ActivityMode::Active));
    mode_->setToolTip(tr("安静模式仍播放待机动画，但不主动接近鼠标，也不主动显示气泡。"));
    form->addRow(tr("活动模式"), mode_);
    connect(mode_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_) {
            return;
        }
        current_.behavior.mode =
            static_cast<core::ActivityMode>(currentPreset(mode_));
        commitFromWidgets();
    });

    tempo_ = new QComboBox(group);
    tempo_->setObjectName(QStringLiteral("behavior-tempo"));
    tempo_->addItem(tr("低"), static_cast<int>(core::ActivityTempo::Low));
    tempo_->addItem(tr("中"), static_cast<int>(core::ActivityTempo::Normal));
    tempo_->addItem(tr("高"), static_cast<int>(core::ActivityTempo::High));
    appendCustom(tempo_);
    tempo_->setToolTip(tr("同时影响待机、行走和休息的时长与比例。"));
    form->addRow(tr("活动节奏"), tempo_);
    connect(tempo_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(tempo_) == kCustomPreset) {
            return;
        }
        core::applyActivityTempo(
            current_.behavior,
            static_cast<core::ActivityTempo>(currentPreset(tempo_)));
        commitFromWidgets();
    });

    movement_ = new QComboBox(group);
    movement_->setObjectName(QStringLiteral("behavior-movement"));
    movement_->addItem(tr("慢"), static_cast<int>(core::MovementSpeed::Slow));
    movement_->addItem(tr("正常"), static_cast<int>(core::MovementSpeed::Normal));
    movement_->addItem(tr("快"), static_cast<int>(core::MovementSpeed::Fast));
    appendCustom(movement_);
    form->addRow(tr("移动速度"), movement_);
    connect(movement_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(movement_) == kCustomPreset) {
            return;
        }
        core::applyMovementSpeed(
            current_.behavior,
            static_cast<core::MovementSpeed>(currentPreset(movement_)));
        commitFromWidgets();
    });

    cursorAffinity_ = new QComboBox(group);
    cursorAffinity_->setObjectName(QStringLiteral("behavior-cursor"));
    cursorAffinity_->addItem(tr("关"), static_cast<int>(core::CursorAffinity::Off));
    cursorAffinity_->addItem(tr("偶尔"),
                             static_cast<int>(core::CursorAffinity::Occasional));
    cursorAffinity_->addItem(tr("经常"),
                             static_cast<int>(core::CursorAffinity::Frequent));
    appendCustom(cursorAffinity_);
    cursorAffinity_->setToolTip(tr("只在活跃模式下生效。"));
    form->addRow(tr("接近鼠标"), cursorAffinity_);
    connect(cursorAffinity_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(cursorAffinity_) == kCustomPreset) {
            return;
        }
        core::applyCursorAffinity(
            current_.behavior,
            static_cast<core::CursorAffinity>(currentPreset(cursorAffinity_)));
        commitFromWidgets();
    });

    auto *advanced = new QWidget(group);
    auto *advancedForm = new QFormLayout(advanced);

    // 成对上下限只给数字框（第 14.2 节）。秒级时长以秒显示。
    const auto addPair = [&](const QString &name, const QString &label,
                             const core::IntRange range, const Field minField,
                             const Field maxField) {
        auto *minimum = new ValueEditor(range.min, range.max, 1000, tr(" 秒"), false,
                                        advanced);
        auto *maximum = new ValueEditor(range.min, range.max, 1000, tr(" 秒"), false,
                                        advanced);
        bindEditor(minimum, name + QStringLiteral("-min"), minField);
        bindEditor(maximum, name + QStringLiteral("-max"), maxField);
        advancedForm->addRow(tr("%1（最短）").arg(label), minimum);
        advancedForm->addRow(tr("%1（最长）").arg(label), maximum);
    };

    addPair(QStringLiteral("behavior-idle"), tr("待机时长"), core::ranges::kIdleMs,
            [](core::Settings &s) -> int & { return s.behavior.idleMinMs; },
            [](core::Settings &s) -> int & { return s.behavior.idleMaxMs; });
    addPair(QStringLiteral("behavior-walk"), tr("行走时长"), core::ranges::kWalkMs,
            [](core::Settings &s) -> int & { return s.behavior.walkMinMs; },
            [](core::Settings &s) -> int & { return s.behavior.walkMaxMs; });
    addPair(QStringLiteral("behavior-rest"), tr("休息时长"), core::ranges::kRestMs,
            [](core::Settings &s) -> int & { return s.behavior.restMinMs; },
            [](core::Settings &s) -> int & { return s.behavior.restMaxMs; });

    const auto addValue = [&](const QString &name, const QString &label,
                              const core::IntRange range, const int divisor,
                              const QString &suffix, const Field field) {
        auto *editor = new ValueEditor(range.min, range.max, divisor, suffix, true,
                                       advanced);
        bindEditor(editor, name, field);
        advancedForm->addRow(label, editor);
    };

    addValue(QStringLiteral("behavior-rest-chance"), tr("进入休息的概率"), core::ranges::kPercent, 1, tr(" %"),
             [](core::Settings &s) -> int & { return s.behavior.restChancePercent; });
    addValue(QStringLiteral("behavior-approach-chance"), tr("接近鼠标的概率"), core::ranges::kPercent, 1, tr(" %"),
             [](core::Settings &s) -> int & {
                 return s.behavior.approachCursorChancePercent;
             });
    addValue(QStringLiteral("behavior-walk-speed"), tr("行走速度"), core::ranges::kSpeedPxPerSec, 1, tr(" 像素/秒"),
             [](core::Settings &s) -> int & { return s.behavior.walkSpeedPxPerSec; });
    addValue(QStringLiteral("behavior-return-speed"), tr("返回底部的速度"), core::ranges::kSpeedPxPerSec, 1, tr(" 像素/秒"),
             [](core::Settings &s) -> int & { return s.behavior.returnSpeedPxPerSec; });
    addValue(QStringLiteral("behavior-return-delay"), tr("放开后返回的延迟"), core::ranges::kReturnDelayMs, 1000, tr(" 秒"),
             [](core::Settings &s) -> int & { return s.behavior.returnDelayMs; });
    addValue(QStringLiteral("behavior-cursor-distance"), tr("与鼠标保持的距离"), core::ranges::kCursorSafeDistancePx, 1,
             tr(" 像素"),
             [](core::Settings &s) -> int & { return s.behavior.cursorSafeDistancePx; });

    addAdvancedSection(group, advanced, core::SettingsGroup::Behavior, tr("行为"));
    return group;
}

QWidget *SettingsWindow::buildDialogueGroup()
{
    auto *group = new QGroupBox(tr("对话"), this);
    auto *outer = new QVBoxLayout(group);
    auto *form = new QFormLayout;
    outer->addLayout(form);

    speech_ = new QComboBox(group);
    speech_->setObjectName(QStringLiteral("dialogue-speech"));
    speech_->addItem(tr("关闭"), static_cast<int>(core::SpeechFrequency::Off));
    speech_->addItem(tr("低"), static_cast<int>(core::SpeechFrequency::Low));
    speech_->addItem(tr("中"), static_cast<int>(core::SpeechFrequency::Normal));
    speech_->addItem(tr("高"), static_cast<int>(core::SpeechFrequency::High));
    appendCustom(speech_);
    speech_->setToolTip(tr("角色主动说话的频率。安静模式完全抑制气泡，与本设置无关。"));
    form->addRow(tr("说话频率"), speech_);
    connect(speech_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(speech_) == kCustomPreset) {
            return;
        }
        core::applySpeechFrequency(
            current_.dialogue,
            static_cast<core::SpeechFrequency>(currentPreset(speech_)));
        commitFromWidgets();
    });

    clickText_ = new QComboBox(group);
    clickText_->setObjectName(QStringLiteral("dialogue-click-text"));
    clickText_->addItem(tr("低"), static_cast<int>(core::ClickTextFrequency::Low));
    clickText_->addItem(tr("中"), static_cast<int>(core::ClickTextFrequency::Normal));
    clickText_->addItem(tr("高"), static_cast<int>(core::ClickTextFrequency::High));
    appendCustom(clickText_);
    clickText_->setToolTip(tr("单击角色时附带台词的概率，与说话频率无关。"));
    form->addRow(tr("单击台词概率"), clickText_);
    connect(clickText_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(clickText_) == kCustomPreset) {
            return;
        }
        core::applyClickTextFrequency(
            current_.dialogue,
            static_cast<core::ClickTextFrequency>(currentPreset(clickText_)));
        commitFromWidgets();
    });

    typingSpeed_ = new QComboBox(group);
    typingSpeed_->setObjectName(QStringLiteral("dialogue-typing"));
    typingSpeed_->addItem(tr("慢"), static_cast<int>(core::TypingSpeed::Slow));
    typingSpeed_->addItem(tr("正常"), static_cast<int>(core::TypingSpeed::Normal));
    typingSpeed_->addItem(tr("快"), static_cast<int>(core::TypingSpeed::Fast));
    appendCustom(typingSpeed_);
    form->addRow(tr("打字速度"), typingSpeed_);
    connect(typingSpeed_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(typingSpeed_) == kCustomPreset) {
            return;
        }
        core::applyTypingSpeed(
            current_.dialogue,
            static_cast<core::TypingSpeed>(currentPreset(typingSpeed_)));
        commitFromWidgets();
    });

    auto *advanced = new QWidget(group);
    auto *advancedForm = new QFormLayout(advanced);
    const auto addValue = [&](const QString &name, const QString &label,
                              const core::IntRange range, const int divisor,
                              const QString &suffix, const Field field) {
        auto *editor = new ValueEditor(range.min, range.max, divisor, suffix, true,
                                       advanced);
        bindEditor(editor, name, field);
        advancedForm->addRow(label, editor);
    };

    addValue(QStringLiteral("dialogue-chatter-interval"), tr("主动说话的最小间隔"), core::ranges::kChatterIntervalMs, 1000,
             tr(" 秒"),
             [](core::Settings &s) -> int & { return s.dialogue.chatterMinIntervalMs; });
    addValue(QStringLiteral("dialogue-chatter-chance"), tr("主动说话的触发概率"), core::ranges::kPercent, 1, tr(" %"),
             [](core::Settings &s) -> int & { return s.dialogue.chatterChancePercent; });
    addValue(QStringLiteral("dialogue-click-chance"), tr("单击附带台词的概率"), core::ranges::kPercent, 1, tr(" %"),
             [](core::Settings &s) -> int & {
                 return s.dialogue.clickTextChancePercent;
             });
    addValue(QStringLiteral("dialogue-auto-hide"), tr("单句气泡的停留时间"), core::ranges::kSinglePageAutoHideMs, 1000,
             tr(" 秒"),
             [](core::Settings &s) -> int & { return s.dialogue.singlePageAutoHideMs; });
    addValue(QStringLiteral("dialogue-typing-ms"), tr("每个字的打字时间"), core::ranges::kTypingMsPerChar, 1, tr(" 毫秒"),
             [](core::Settings &s) -> int & { return s.dialogue.typingMsPerChar; });

    addAdvancedSection(group, advanced, core::SettingsGroup::Dialogue, tr("对话"));
    return group;
}

QWidget *SettingsWindow::buildAppearanceGroup()
{
    auto *group = new QGroupBox(tr("外观"), this);
    auto *outer = new QVBoxLayout(group);
    auto *form = new QFormLayout;
    outer->addLayout(form);

    scale_ = new QComboBox(group);
    scale_->setObjectName(QStringLiteral("appearance-scale"));
    for (const int scale : core::allowedScales()) {
        scale_->addItem(tr("%1×").arg(scale), scale);
    }
    // 第 14.5 节：显示倍率保持离散整数档，不提供滑块、小数倍率或连续缩放。
    scale_->setToolTip(
        tr("只提供整数倍率。系统 DPI 缩放会再叠加一层，只有两者的乘积为整数时像素才完全清晰。"));
    form->addRow(tr("角色显示倍率"), scale_);
    connect(scale_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_) {
            return;
        }
        current_.appearance.scale = scale_->currentData().toInt();
        commitFromWidgets();
    });

    animationSpeed_ = new QComboBox(group);
    animationSpeed_->setObjectName(QStringLiteral("appearance-animation"));
    animationSpeed_->addItem(tr("慢"), static_cast<int>(core::AnimationSpeed::Slow));
    animationSpeed_->addItem(tr("正常"), static_cast<int>(core::AnimationSpeed::Normal));
    animationSpeed_->addItem(tr("快"), static_cast<int>(core::AnimationSpeed::Fast));
    appendCustom(animationSpeed_);
    form->addRow(tr("动画速度"), animationSpeed_);
    connect(animationSpeed_, &QComboBox::currentIndexChanged, this, [this] {
        if (updating_ || currentPreset(animationSpeed_) == kCustomPreset) {
            return;
        }
        core::applyAnimationSpeed(
            current_.appearance,
            static_cast<core::AnimationSpeed>(currentPreset(animationSpeed_)));
        commitFromWidgets();
    });

    auto *advanced = new QWidget(group);
    auto *advancedForm = new QFormLayout(advanced);
    const auto addValue = [&](const QString &name, const QString &label,
                              const Field field) {
        auto *editor = new ValueEditor(core::ranges::kFrameMs.min,
                                       core::ranges::kFrameMs.max, 1, tr(" 毫秒"), true,
                                       advanced);
        bindEditor(editor, name, field);
        advancedForm->addRow(label, editor);
    };
    addValue(QStringLiteral("appearance-idle-frame"), tr("待机动画每帧"),
             [](core::Settings &s) -> int & { return s.appearance.idleFrameMs; });
    addValue(QStringLiteral("appearance-run-frame"), tr("跑动动画每帧"),
             [](core::Settings &s) -> int & { return s.appearance.runFrameMs; });
    addValue(QStringLiteral("appearance-icecream-frame"), tr("冰淇淋动画每帧"),
             [](core::Settings &s) -> int & { return s.appearance.icecreamFrameMs; });

    addAdvancedSection(group, advanced, core::SettingsGroup::Appearance, tr("外观"));
    return group;
}

QWidget *SettingsWindow::buildWindowGroup()
{
    auto *group = new QGroupBox(tr("窗口与桌面"), this);
    auto *outer = new QVBoxLayout(group);
    auto *form = new QFormLayout;
    outer->addLayout(form);

    alwaysOnTop_ = new QCheckBox(tr("始终置顶"), group);
    alwaysOnTop_->setObjectName(QStringLiteral("window-always-on-top"));
    form->addRow(QString(), alwaysOnTop_);
    connect(alwaysOnTop_, &QCheckBox::toggled, this, [this](const bool on) {
        if (updating_) {
            return;
        }
        current_.window.alwaysOnTop = on;
        commitFromWidgets();
    });

    // 第 5.2 节要求提供移除自己创建的入口；第 14.6 节把它定位在本组。
    // 默认隐藏，只有以 AppImage 运行时由调用方打开。
    desktopEntryLabel_ = new QLabel(tr("应用菜单"), group);
    desktopEntryButton_ = new QPushButton(group);
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

    // 本组没有高级层，但仍然要能按组恢复默认值。
    auto *reset = new QPushButton(tr("恢复本组默认值"), group);
    outer->addWidget(reset, 0, Qt::AlignLeft);
    connect(reset, &QPushButton::clicked, this, [this] {
        if (!confirmer_(tr("恢复本组默认值"),
                        tr("这会把「%1」这一组的全部设置恢复成默认值，"
                           "其他组不受影响。确定继续吗？")
                            .arg(tr("窗口与桌面")))) {
            return;
        }
        emit groupResetRequested(core::SettingsGroup::Window);
    });
    return group;
}

void SettingsWindow::setConfirmer(Confirmer confirmer)
{
    confirmer_ = std::move(confirmer);
}

void SettingsWindow::setSettings(const core::Settings &settings)
{
    current_ = core::sanitized(settings);
    refreshAll();
}

core::Settings SettingsWindow::settings() const
{
    return core::sanitized(current_);
}

void SettingsWindow::refreshAll()
{
    updating_ = true;
    for (const auto &entry : editors_) {
        entry.first->setValue(entry.second(current_));
    }
    selectPreset(mode_, static_cast<int>(current_.behavior.mode));
    selectPreset(scale_, current_.appearance.scale);
    {
        const QSignalBlocker blocker(alwaysOnTop_);
        alwaysOnTop_->setChecked(current_.window.alwaysOnTop);
    }
    updating_ = false;
    refreshPresets();
}

void SettingsWindow::refreshPresets()
{
    // 第 14.2 节：档位由实际参数反向匹配得出，界面不保存第二份预设状态。
    const bool wasUpdating = updating_;
    updating_ = true;

    const auto select = [](QComboBox *box, const auto &matched) {
        selectPreset(box, matched ? static_cast<int>(*matched) : kCustomPreset);
    };
    select(tempo_, core::matchActivityTempo(current_.behavior));
    select(movement_, core::matchMovementSpeed(current_.behavior));
    select(cursorAffinity_, core::matchCursorAffinity(current_.behavior));
    select(speech_, core::matchSpeechFrequency(current_.dialogue));
    select(clickText_, core::matchClickTextFrequency(current_.dialogue));
    select(typingSpeed_, core::matchTypingSpeed(current_.dialogue));
    select(animationSpeed_, core::matchAnimationSpeed(current_.appearance));

    updating_ = wasUpdating;
}

void SettingsWindow::enforcePairs()
{
    // 第 8.2 节：成对值在界面层就不能形成非法运行时配置。改动一端时顶开另一端，
    // 而不是交给 sanitized() 把整对退回默认值——那会在用户输入途中丢掉他刚填的值。
    const auto fix = [](int &minimum, int &maximum) {
        if (minimum > maximum) {
            maximum = minimum;
        }
    };
    fix(current_.behavior.idleMinMs, current_.behavior.idleMaxMs);
    fix(current_.behavior.walkMinMs, current_.behavior.walkMaxMs);
    fix(current_.behavior.restMinMs, current_.behavior.restMaxMs);
}

void SettingsWindow::editFromWidgets()
{
    enforcePairs();
    refreshAll();
    emit settingsEdited(settings());
}

void SettingsWindow::commitFromWidgets()
{
    enforcePairs();
    refreshAll();
    emit settingsCommitted(settings());
}

void SettingsWindow::setDesktopEntryState(const bool offered, const bool installed)
{
    desktopEntryInstalled_ = installed;
    desktopEntryLabel_->setVisible(offered);
    desktopEntryButton_->setVisible(offered);
    desktopEntryButton_->setText(installed ? tr("移除应用菜单入口")
                                           : tr("加入应用菜单"));
}

} // namespace mub::ui
