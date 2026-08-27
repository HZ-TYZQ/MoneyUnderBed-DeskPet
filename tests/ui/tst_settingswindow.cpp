#include "core/Settings.h"
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
using mub::core::BubbleFrequency;
using mub::core::Settings;
using mub::core::WorkspaceVisibility;
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
    void workspaceRowIsHiddenWhenThePlatformCannotPin();
    void workspaceRowIsShownWhenThePlatformCanPin();
    void loadingValuesDoesNotReportAChange();
    void changingAControlReportsTheWholeSettings();
    void restoreDefaultsIsReportedSeparately();
    void diagnosticsCoverTheRequiredFields();
    void diagnosticsCarryNoUnrelatedPrivateData();
};

// 第 5.1 节：平台不适用的设置项直接隐藏，不以置灰形式保留。
void TestSettingsWindow::workspaceRowIsHiddenWhenThePlatformCannotPin()
{
    SettingsWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QComboBox *workspace = comboFor(window, QStringLiteral("工作区"));
    QVERIFY(workspace == nullptr || !workspace->isVisible());

    // 隐藏时该项不参与设置，保持默认值。
    QCOMPARE(window.settings().workspace, Settings{}.workspace);
}

void TestSettingsWindow::workspaceRowIsShownWhenThePlatformCanPin()
{
    SettingsWindow window(true);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    bool found = false;
    for (const QComboBox *box : window.findChildren<QComboBox *>()) {
        if (box->count() == 2 && box->itemData(0).toInt()
                == static_cast<int>(WorkspaceVisibility::AllWorkspaces)
            && box->isVisible()) {
            found = true;
        }
    }
    QVERIFY(found);
}

// 刷新控件不能反过来再上报一次，否则「套用 → 上报 → 再套用」会形成回环。
void TestSettingsWindow::loadingValuesDoesNotReportAChange()
{
    SettingsWindow window(true);
    QSignalSpy changes(&window, &SettingsWindow::settingsChanged);

    Settings settings;
    settings.mode = ActivityMode::Active;
    settings.bubble = BubbleFrequency::Normal;
    settings.alwaysOnTop = false;
    settings.scale = 1;
    window.setSettings(settings);

    QCOMPARE(changes.count(), 0);
    QCOMPARE(window.settings(), settings);
}

// 第 5.1 节：修改后立即生效，不设额外「应用」阶段。
void TestSettingsWindow::changingAControlReportsTheWholeSettings()
{
    SettingsWindow window(true);
    window.setSettings(Settings{});
    QSignalSpy changes(&window, &SettingsWindow::settingsChanged);

    QCheckBox *alwaysOnTop = window.findChild<QCheckBox *>();
    QVERIFY(alwaysOnTop != nullptr);
    const bool before = alwaysOnTop->isChecked();
    alwaysOnTop->setChecked(!before);

    QCOMPARE(changes.count(), 1);
    QCOMPARE(changes.constFirst().constFirst().value<Settings>().alwaysOnTop, !before);
}

void TestSettingsWindow::restoreDefaultsIsReportedSeparately()
{
    SettingsWindow window(true);
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
