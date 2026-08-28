// 四组设置界面。
//
// docs/Decisions.md 第 14.2 节：四组分区、组内普通/高级两层、实际参数是唯一
// 真相来源、按组与全部两级恢复默认值且都必须确认。
// 第 14.8 节：接收与落盘分开——拖动过程中的值立即生效但不落盘。
//
// 这些断言取代不了真实桌面验收（第 11.1 节），offscreen 只承担自动测试。

#include "core/Settings.h"
#include "core/SettingsPresets.h"
#include "ui/AboutWindow.h"
#include "ui/SettingsWindow.h"
#include "ui/ValueEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>

using mub::core::ActivityMode;
using mub::core::Settings;
using mub::core::SettingsGroup;
using mub::ui::diagnosticsText;
using mub::ui::SettingsWindow;
using mub::ui::ValueEditor;

namespace {

QComboBox *comboNamed(const SettingsWindow &window, const char *name)
{
    return window.findChild<QComboBox *>(QLatin1StringView(name));
}

ValueEditor *editorNamed(const SettingsWindow &window, const char *name)
{
    return window.findChild<ValueEditor *>(QLatin1StringView(name));
}

// 点击文字里含有 `text` 的按钮。返回是否点到了。
bool clickButton(const SettingsWindow &window, const QString &text)
{
    for (QPushButton *button : window.findChildren<QPushButton *>()) {
        if (button->text().contains(text)) {
            button->click();
            return true;
        }
    }
    return false;
}

} // namespace

class TestSettingsWindow final : public QObject
{
    Q_OBJECT

private slots:
    void workspaceControlIsNotPartOfThisRelease();
    void allFourGroupsArePresent();
    void everyAdvancedParameterHasAnEditor();
    void pairedBoundsHaveNoSlider();
    void singleValuesAndPercentagesHaveBothSliderAndSpinBox();
    void loadingValuesDoesNotReportAChange();
    void parametersTheWindowDoesNotShowArePassedThrough();
    void choosingAPresetWritesTheActualParameters();
    void editingAnAdvancedValueShowsCustom();
    void sliderAndSpinBoxStayInSync();
    void draggingReportsLiveValuesWithoutCommitting();
    void pairedBoundsCannotBecomeInvalid();
    void groupResetAsksAndReportsTheGroup();
    void cancellingAGroupResetChangesNothing();
    void resetAllAsksAndReports();
    void cancellingResetAllChangesNothing();
    void windowNeverTouchesTheSettingsStore();
    void diagnosticsCoverTheRequiredFields();
    void diagnosticsCarryNoUnrelatedPrivateData();
};

void TestSettingsWindow::workspaceControlIsNotPartOfThisRelease()
{
    // 第 14.7 节：工作区／虚拟桌面设置继续不开放。
    SettingsWindow window;
    for (QComboBox *box : window.findChildren<QComboBox *>()) {
        QVERIFY(!box->toolTip().contains(QStringLiteral("工作区")));
    }
}

void TestSettingsWindow::allFourGroupsArePresent()
{
    SettingsWindow window;
    QStringList titles;
    for (const QGroupBox *group : window.findChildren<QGroupBox *>()) {
        titles.append(group->title());
    }
    QCOMPARE(titles.size(), 4);
    for (const QString &expected : {QStringLiteral("行为"), QStringLiteral("对话"),
                                    QStringLiteral("外观"),
                                    QStringLiteral("窗口与桌面")}) {
        QVERIFY2(titles.contains(expected), qPrintable(expected));
    }
}

// 第 14.3 至 14.6 节列出的每个高级参数都必须有对应的输入行。
void TestSettingsWindow::everyAdvancedParameterHasAnEditor()
{
    SettingsWindow window;
    for (const char *name :
         {"behavior-idle-min", "behavior-idle-max", "behavior-walk-min",
          "behavior-walk-max", "behavior-rest-min", "behavior-rest-max",
          "behavior-rest-chance", "behavior-approach-chance", "behavior-walk-speed",
          "behavior-return-speed", "behavior-return-delay", "behavior-cursor-distance",
          "dialogue-chatter-interval", "dialogue-chatter-chance",
          "dialogue-click-chance", "dialogue-auto-hide", "dialogue-typing-ms",
          "appearance-idle-frame", "appearance-run-frame", "appearance-icecream-frame"}) {
        QVERIFY2(editorNamed(window, name) != nullptr, name);
    }
    // 普通层的档位控件。
    for (const char *name :
         {"behavior-mode", "behavior-tempo", "behavior-movement", "behavior-cursor",
          "dialogue-speech", "dialogue-click-text", "dialogue-typing",
          "appearance-scale", "appearance-animation"}) {
        QVERIFY2(comboNamed(window, name) != nullptr, name);
    }
    QVERIFY(window.findChild<QCheckBox *>(QStringLiteral("window-always-on-top"))
            != nullptr);
}

// 第 14.2 节：成对上下限只提供数字框，不给滑块。
void TestSettingsWindow::pairedBoundsHaveNoSlider()
{
    SettingsWindow window;
    for (const char *name :
         {"behavior-idle-min", "behavior-idle-max", "behavior-walk-min",
          "behavior-walk-max", "behavior-rest-min", "behavior-rest-max"}) {
        QVERIFY2(!editorNamed(window, name)->hasSlider(), name);
    }
}

// 第 14.2 节：单值与百分比配滑块与数字框，且滑块永远与数字框成对出现。
void TestSettingsWindow::singleValuesAndPercentagesHaveBothSliderAndSpinBox()
{
    SettingsWindow window;
    for (const char *name :
         {"behavior-rest-chance", "behavior-walk-speed", "behavior-return-delay",
          "dialogue-chatter-chance", "dialogue-typing-ms", "appearance-run-frame"}) {
        ValueEditor *editor = editorNamed(window, name);
        QVERIFY2(editor->hasSlider(), name);
        QVERIFY2(editor->findChild<QDoubleSpinBox *>() != nullptr, name);
    }
    // 反过来：界面上不存在没有数字框陪着的滑块。
    for (const QSlider *slider : window.findChildren<QSlider *>()) {
        const QObject *row = slider->parent();
        QVERIFY(row->findChild<QDoubleSpinBox *>() != nullptr);
    }
}

// 刷新控件不能反过来再上报一次，否则「套用 → 上报 → 再套用」会形成回环。
void TestSettingsWindow::loadingValuesDoesNotReportAChange()
{
    SettingsWindow window;
    const QSignalSpy edited(&window, &SettingsWindow::settingsEdited);
    const QSignalSpy committed(&window, &SettingsWindow::settingsCommitted);

    Settings settings;
    settings.behavior.mode = ActivityMode::Active;
    settings.behavior.walkSpeedPxPerSec = 61;
    settings.appearance.scale = 1;
    window.setSettings(settings);

    QCOMPARE(edited.count(), 0);
    QCOMPARE(committed.count(), 0);
    QCOMPARE(window.settings(), settings);
}

void TestSettingsWindow::parametersTheWindowDoesNotShowArePassedThrough()
{
    SettingsWindow window;
    Settings settings;
    settings.behavior.cursorSafeDistancePx = 91;
    settings.dialogue.singlePageAutoHideMs = 7000;
    window.setSettings(settings);

    QCheckBox *alwaysOnTop =
        window.findChild<QCheckBox *>(QStringLiteral("window-always-on-top"));
    alwaysOnTop->setChecked(!alwaysOnTop->isChecked());

    QCOMPARE(window.settings().behavior.cursorSafeDistancePx, 91);
    QCOMPARE(window.settings().dialogue.singlePageAutoHideMs, 7000);
}

// 第 14.2 节：选择档位时把该档对应的参数写入实际参数。
void TestSettingsWindow::choosingAPresetWritesTheActualParameters()
{
    SettingsWindow window;
    window.setSettings(Settings{});
    const QSignalSpy committed(&window, &SettingsWindow::settingsCommitted);

    QComboBox *tempo = comboNamed(window, "behavior-tempo");
    tempo->setCurrentIndex(
        tempo->findData(static_cast<int>(mub::core::ActivityTempo::High)));

    Settings expected;
    mub::core::applyActivityTempo(expected.behavior, mub::core::ActivityTempo::High);
    QCOMPARE(window.settings().behavior, expected.behavior);
    QCOMPARE(committed.count(), 1);

    // 高级层的输入行同步显示新数值。
    QCOMPARE(editorNamed(window, "behavior-idle-min")->value(),
             expected.behavior.idleMinMs);
    QCOMPARE(editorNamed(window, "behavior-rest-chance")->value(),
             expected.behavior.restChancePercent);
}

// 第 14.2 节：高级层改过之后反向匹配不上，档位显示「自定义」。
void TestSettingsWindow::editingAnAdvancedValueShowsCustom()
{
    SettingsWindow window;
    window.setSettings(Settings{});

    QComboBox *tempo = comboNamed(window, "behavior-tempo");
    QCOMPARE(tempo->currentText(), QStringLiteral("中"));

    ValueEditor *restChance = editorNamed(window, "behavior-rest-chance");
    restChance->findChild<QDoubleSpinBox *>()->setValue(
        Settings{}.behavior.restChancePercent + 1);

    QCOMPARE(tempo->currentText(), QStringLiteral("自定义"));
    // 只影响它所属的那一个档位。
    QCOMPARE(comboNamed(window, "behavior-movement")->currentText(),
             QStringLiteral("正常"));
}

void TestSettingsWindow::sliderAndSpinBoxStayInSync()
{
    SettingsWindow window;
    window.setSettings(Settings{});

    ValueEditor *speed = editorNamed(window, "behavior-walk-speed");
    QSlider *slider = speed->findChild<QSlider *>();
    QDoubleSpinBox *spin = speed->findChild<QDoubleSpinBox *>();

    slider->setValue(123);
    QCOMPARE(spin->value(), 123.0);
    QCOMPARE(window.settings().behavior.walkSpeedPxPerSec, 123);

    spin->setValue(77.0);
    QCOMPARE(slider->value(), 77);
    QCOMPARE(window.settings().behavior.walkSpeedPxPerSec, 77);
}

// 第 14.8 节：拖动过程中的值立即生效但不落盘，滑块释放才落盘。
void TestSettingsWindow::draggingReportsLiveValuesWithoutCommitting()
{
    SettingsWindow window;
    window.setSettings(Settings{});
    const QSignalSpy edited(&window, &SettingsWindow::settingsEdited);
    const QSignalSpy committed(&window, &SettingsWindow::settingsCommitted);

    QSlider *slider = editorNamed(window, "behavior-walk-speed")->findChild<QSlider *>();
    for (const int value : {50, 55, 60, 65}) {
        slider->setValue(value);
    }

    QCOMPARE(edited.count(), 4);
    QCOMPARE(committed.count(), 0);

    emit slider->sliderReleased();
    QCOMPARE(committed.count(), 1);
    QCOMPARE(window.settings().behavior.walkSpeedPxPerSec, 65);
}

// 第 8.2 节：成对值在界面层就不能形成非法运行时配置。
void TestSettingsWindow::pairedBoundsCannotBecomeInvalid()
{
    SettingsWindow window;
    window.setSettings(Settings{});

    ValueEditor *minimum = editorNamed(window, "behavior-idle-min");
    ValueEditor *maximum = editorNamed(window, "behavior-idle-max");

    // 把最小值顶到最大值之上：另一端被顶开，而不是整对退回默认值。
    minimum->findChild<QDoubleSpinBox *>()->setValue(20.0);

    const Settings reported = window.settings();
    QCOMPARE(reported.behavior.idleMinMs, 20000);
    QVERIFY(reported.behavior.idleMaxMs >= reported.behavior.idleMinMs);
    QCOMPARE(maximum->value(), reported.behavior.idleMaxMs);
    // 界面报出来的值本身就是合法的，不需要控制器再兜底。
    QCOMPARE(mub::core::sanitized(reported), reported);
}

void TestSettingsWindow::groupResetAsksAndReportsTheGroup()
{
    SettingsWindow window;
    QString askedText;
    window.setConfirmer([&askedText](const QString &, const QString &text) {
        askedText = text;
        return true;
    });
    QSignalSpy resets(&window, &SettingsWindow::groupResetRequested);

    QVERIFY(clickButton(window, QStringLiteral("恢复本组默认值")));

    QCOMPARE(resets.count(), 1);
    QCOMPARE(resets.constFirst().constFirst().value<SettingsGroup>(),
             SettingsGroup::Behavior);
    // 确认框必须明确显示组名。
    QVERIFY2(askedText.contains(QStringLiteral("行为")), qPrintable(askedText));
}

void TestSettingsWindow::cancellingAGroupResetChangesNothing()
{
    SettingsWindow window;
    Settings settings;
    settings.behavior.walkSpeedPxPerSec = 61;
    window.setSettings(settings);
    window.setConfirmer([](const QString &, const QString &) { return false; });

    const QSignalSpy resets(&window, &SettingsWindow::groupResetRequested);
    const QSignalSpy committed(&window, &SettingsWindow::settingsCommitted);
    QVERIFY(clickButton(window, QStringLiteral("恢复本组默认值")));

    QCOMPARE(resets.count(), 0);
    QCOMPARE(committed.count(), 0);
    QCOMPARE(window.settings(), settings);
}

void TestSettingsWindow::resetAllAsksAndReports()
{
    SettingsWindow window;
    QString askedText;
    window.setConfirmer([&askedText](const QString &, const QString &text) {
        askedText = text;
        return true;
    });
    const QSignalSpy resets(&window, &SettingsWindow::resetAllRequested);

    QVERIFY(clickButton(window, QStringLiteral("全部恢复默认值")));

    QCOMPARE(resets.count(), 1);
    QVERIFY2(askedText.contains(QStringLiteral("全部")), qPrintable(askedText));
}

void TestSettingsWindow::cancellingResetAllChangesNothing()
{
    SettingsWindow window;
    Settings settings;
    settings.dialogue.typingMsPerChar = 40;
    window.setSettings(settings);
    window.setConfirmer([](const QString &, const QString &) { return false; });

    const QSignalSpy resets(&window, &SettingsWindow::resetAllRequested);
    QVERIFY(clickButton(window, QStringLiteral("全部恢复默认值")));

    QCOMPARE(resets.count(), 0);
    QCOMPARE(window.settings(), settings);
}

// 第 14.2 节：设置界面不直接读写 QSettings，也不构造持久化后端。
void TestSettingsWindow::windowNeverTouchesTheSettingsStore()
{
    for (const QString &name : {QStringLiteral("SettingsWindow.cpp"),
                                QStringLiteral("SettingsWindow.h"),
                                QStringLiteral("ValueEditor.cpp"),
                                QStringLiteral("ValueEditor.h")}) {
        const QString path = QStringLiteral(MUB_SOURCE_ROOT "/src/ui/") + name;
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(path));
        const QString text = QString::fromUtf8(file.readAll());
        QVERIFY2(!text.contains(QStringLiteral("QSettings")), qPrintable(name));
        QVERIFY2(!text.contains(QStringLiteral("SettingsStore")), qPrintable(name));
    }
}

// 第 5.3 节列出的字段必须都在。
void TestSettingsWindow::diagnosticsCoverTheRequiredFields()
{
    const QString text = diagnosticsText(QStringLiteral("fake-backend"), true, Settings{});
    for (const QString &field : {QStringLiteral("version:"), QStringLiteral("qt:"),
                                 QStringLiteral("system:"), QStringLiteral("backend:"),
                                 QStringLiteral("tray:"), QStringLiteral("screen[0]:")}) {
        QVERIFY2(text.contains(field), qPrintable(field));
    }
    QVERIFY(text.contains(QStringLiteral("fake-backend")));
    QVERIFY(text.contains(QStringLiteral("tray: available")));
    // 第 8.3 节的设置摘要。
    QVERIFY(text.contains(QStringLiteral("settings: mode=")));
    QVERIFY(diagnosticsText(QStringLiteral("x"), false, Settings{})
                .contains(QStringLiteral("tray: unavailable")));
}

// 第 5.3 节：诊断信息不得包含完整环境变量、用户文件内容或无关隐私。
void TestSettingsWindow::diagnosticsCarryNoUnrelatedPrivateData()
{
    const QString text = diagnosticsText(QStringLiteral("fake-backend"), false, Settings{});

    QVERIFY(!text.contains(QDir::homePath()));
    QVERIFY(!text.contains(QStringLiteral("PATH=")));

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    for (const QString &name : {QStringLiteral("USER"), QStringLiteral("USERNAME")}) {
        const QString value = environment.value(name);
        if (!value.isEmpty()) {
            QVERIFY2(!text.contains(value), qPrintable(name));
        }
    }
}

QTEST_MAIN(TestSettingsWindow)
#include "tst_settingswindow.moc"
