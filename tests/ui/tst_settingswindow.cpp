#include "core/Settings.h"
#include "core/SettingsPresets.h"
#include "ui/AboutWindow.h"
#include "ui/SettingsWindow.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

using mub::core::ActivityMode;
using mub::core::Settings;
using mub::ui::diagnosticsText;
using mub::ui::SettingsWindow;

namespace {

QComboBox *comboFor(const SettingsWindow &window, const QString &tip)
{
    for (QComboBox *box : window.findChildren<QComboBox *>()) {
        if (box->toolTip().contains(tip)) {
            return box;
        }
    }
    return nullptr;
}

} // namespace

class TestSettingsWindow final : public QObject
{
    Q_OBJECT

private slots:
    void workspaceControlIsNotPartOfTheFirstRelease();
    void loadingValuesDoesNotReportAChange();
    void parametersTheWindowDoesNotShowArePassedThrough();
    void changingAControlReportsTheWholeSettings();
    void restoreDefaultsIsReportedSeparately();
    void diagnosticsCoverTheRequiredFields();
    void diagnosticsCarryNoUnrelatedPrivateData();
};

void TestSettingsWindow::workspaceControlIsNotPartOfTheFirstRelease()
{
    SettingsWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QComboBox *workspace = comboFor(window, QStringLiteral("工作区"));
    QVERIFY(workspace == nullptr);
}

// 刷新控件不能反过来再上报一次，否则「套用 → 上报 → 再套用」会形成回环。
void TestSettingsWindow::loadingValuesDoesNotReportAChange()
{
    SettingsWindow window;
    QSignalSpy changes(&window, &SettingsWindow::settingsChanged);

    Settings settings;
    settings.behavior.mode = ActivityMode::Active;
    mub::core::applySpeechFrequency(settings.dialogue,
                                    mub::core::SpeechFrequency::Normal);
    settings.window.alwaysOnTop = false;
    settings.appearance.scale = 1;
    window.setSettings(settings);

    QCOMPARE(changes.count(), 0);
    QCOMPARE(window.settings(), settings);
}

// 阶段 2 的设置窗口只显示四个设置项，但新模型有十几个参数。
// 界面必须把没显示的参数原样带回，否则改一次活动模式就会把高级参数清成默认值。
void TestSettingsWindow::parametersTheWindowDoesNotShowArePassedThrough()
{
    SettingsWindow window;

    Settings settings;
    settings.behavior.walkSpeedPxPerSec = 61;
    settings.behavior.restChancePercent = 37;
    settings.dialogue.clickTextChancePercent = 44;
    settings.dialogue.typingMsPerChar = 33;
    settings.appearance.idleFrameMs = 123;
    window.setSettings(settings);

    QCheckBox *alwaysOnTop = window.findChild<QCheckBox *>();
    QVERIFY(alwaysOnTop != nullptr);
    alwaysOnTop->setChecked(!alwaysOnTop->isChecked());

    const Settings reported = window.settings();
    QCOMPARE(reported.behavior.walkSpeedPxPerSec, 61);
    QCOMPARE(reported.behavior.restChancePercent, 37);
    QCOMPARE(reported.dialogue.clickTextChancePercent, 44);
    QCOMPARE(reported.dialogue.typingMsPerChar, 33);
    QCOMPARE(reported.appearance.idleFrameMs, 123);
}

// 第 5.1 节：修改后立即生效，不设额外「应用」阶段。
void TestSettingsWindow::changingAControlReportsTheWholeSettings()
{
    SettingsWindow window;
    window.setSettings(Settings{});
    QSignalSpy changes(&window, &SettingsWindow::settingsChanged);

    QCheckBox *alwaysOnTop = window.findChild<QCheckBox *>();
    QVERIFY(alwaysOnTop != nullptr);
    const bool before = alwaysOnTop->isChecked();
    alwaysOnTop->setChecked(!before);

    QCOMPARE(changes.count(), 1);
    QCOMPARE(changes.constFirst().constFirst().value<Settings>().window.alwaysOnTop,
             !before);
}

void TestSettingsWindow::restoreDefaultsIsReportedSeparately()
{
    SettingsWindow window;
    QSignalSpy resets(&window, &SettingsWindow::restoreDefaultsRequested);

    for (QPushButton *button : window.findChildren<QPushButton *>()) {
        if (button->text().contains(QStringLiteral("恢复默认"))) {
            button->click();
        }
    }
    QCOMPARE(resets.count(), 1);
}

// 第 5.3 节列出的字段必须都在。
void TestSettingsWindow::diagnosticsCoverTheRequiredFields()
{
    const QString text = diagnosticsText(QStringLiteral("fake-backend"), true);
    for (const QString &field : {QStringLiteral("version:"), QStringLiteral("qt:"),
                                 QStringLiteral("system:"), QStringLiteral("backend:"),
                                 QStringLiteral("tray:"), QStringLiteral("screen[0]:")}) {
        QVERIFY2(text.contains(field), qPrintable(field));
    }
    QVERIFY(text.contains(QStringLiteral("fake-backend")));
    QVERIFY(text.contains(QStringLiteral("tray: available")));
    QVERIFY(diagnosticsText(QStringLiteral("x"), false)
                .contains(QStringLiteral("tray: unavailable")));
}

// 第 5.3 节：诊断信息不得包含完整环境变量、用户文件内容或无关隐私。
void TestSettingsWindow::diagnosticsCarryNoUnrelatedPrivateData()
{
    const QString text = diagnosticsText(QStringLiteral("fake-backend"), false);

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
