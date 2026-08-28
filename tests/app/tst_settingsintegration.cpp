// 设置的端到端装配：界面 → 控制器 → 存储 → 再回到界面。
//
// docs/Decisions.md 第 14.2 节要求设置界面不直接读写配置、由应用层统一处理；
// 第 14.6 节要求关闭设置窗口不退出桌宠、可反复打开且只有一个实例；
// 第 14.8 节要求接收与落盘分开、重启后设置保持。
//
// offscreen 只承担自动测试，不冒充真实桌面验收（第 11.1 节）。

#include "app/AppLifecycle.h"
#include "app/SettingsController.h"

#include "core/Settings.h"
#include "core/SettingsPresets.h"
#include "core/SettingsStore.h"
#include "ui/SettingsWindow.h"
#include "ui/ValueEditor.h"

#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSettings>
#include <QSignalSpy>
#include <QSlider>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

using mub::app::AppLifecycle;
using mub::app::SettingsController;
using mub::core::Settings;
using mub::core::SettingsStore;
using mub::ui::SettingsWindow;
using mub::ui::ValueEditor;

namespace {

// 按产品的方式把三者接在一起，测试断言的是这条完整链路。
class Fixture
{
public:
    Fixture()
        : backend_(dir_.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat)
        , store_(backend_)
        , controller_(store_)
    {
        controller_.setPersistDelayMs(1);
        lifecycle_.takeOverQuitPolicy();
        lifecycle_.setAuxiliaryWindow(AppLifecycle::AuxiliaryWindow::Settings, &window_);

        window_.setConfirmer([this](const QString &, const QString &) {
            return confirmResets_;
        });

        QObject::connect(&window_, &SettingsWindow::settingsEdited, &controller_,
                         &SettingsController::apply);
        QObject::connect(&window_, &SettingsWindow::settingsCommitted, &controller_,
                         &SettingsController::applyAndPersist);
        QObject::connect(&window_, &SettingsWindow::groupResetRequested, &controller_,
                         &SettingsController::resetGroup);
        QObject::connect(&window_, &SettingsWindow::resetAllRequested, &controller_,
                         &SettingsController::resetAll);
        QObject::connect(&controller_, &SettingsController::settingsChanged, &window_,
                         &SettingsWindow::setSettings);

        window_.setSettings(controller_.settings());
    }

    SettingsController &controller() { return controller_; }
    SettingsWindow &window() { return window_; }
    AppLifecycle &lifecycle() { return lifecycle_; }
    SettingsStore &store() { return store_; }
    void setConfirmResets(const bool confirm) { confirmResets_ = confirm; }

    QString configPath() const { return dir_.filePath(QStringLiteral("settings.ini")); }

private:
    QTemporaryDir dir_;
    QSettings backend_;
    SettingsStore store_;
    SettingsController controller_;
    SettingsWindow window_;
    AppLifecycle lifecycle_;
    bool confirmResets_ = true;
};

} // namespace

class TestSettingsIntegration final : public QObject
{
    Q_OBJECT

private slots:
    void editingInTheWindowReachesTheStore();
    void closingTheSettingsWindowDoesNotQuit();
    void reopeningShowsTheSameValues();
    void valuesSurviveARestart();
    void aDragWritesOnceAndKeepsTheFinalValue();
    void groupResetFlowsThroughTheController();
    void cancellingAResetChangesNothingAnywhere();
    void runtimeChangesRefreshTheOpenWindow();
};

void TestSettingsIntegration::editingInTheWindowReachesTheStore()
{
    Fixture fixture;
    QComboBox *speech =
        fixture.window().findChild<QComboBox *>(QStringLiteral("dialogue-speech"));
    QVERIFY(speech != nullptr);

    speech->setCurrentIndex(
        speech->findData(static_cast<int>(mub::core::SpeechFrequency::High)));

    Settings expected;
    mub::core::applySpeechFrequency(expected.dialogue,
                                    mub::core::SpeechFrequency::High);
    QCOMPARE(fixture.controller().settings().dialogue, expected.dialogue);
    // 下拉框选择是一次完成的编辑，立即落盘。
    QCOMPARE(fixture.store().load().dialogue, expected.dialogue);
}

// 第 14.6 节：关闭辅助窗口只关闭它自己。
void TestSettingsIntegration::closingTheSettingsWindowDoesNotQuit()
{
    Fixture fixture;
    const QSignalSpy quitting(&fixture.lifecycle(), &AppLifecycle::quitting);

    QWidget character;
    character.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    character.show();

    fixture.lifecycle().showAuxiliaryWindow(AppLifecycle::AuxiliaryWindow::Settings);
    QVERIFY(fixture.window().isVisible());
    fixture.window().close();
    QCoreApplication::processEvents();

    QVERIFY(!fixture.window().isVisible());
    QVERIFY(character.isVisible());
    QVERIFY(!fixture.lifecycle().quitRequested());
    QCOMPARE(quitting.count(), 0);

    // 明确退出只走一次。
    fixture.lifecycle().requestQuit();
    fixture.lifecycle().requestQuit();
    QCOMPARE(quitting.count(), 1);
}

void TestSettingsIntegration::reopeningShowsTheSameValues()
{
    Fixture fixture;
    ValueEditor *speed = fixture.window().findChild<ValueEditor *>(
        QStringLiteral("behavior-walk-speed"));
    QVERIFY(speed != nullptr);

    speed->findChild<QDoubleSpinBox *>()->setValue(123.0);
    fixture.lifecycle().showAuxiliaryWindow(AppLifecycle::AuxiliaryWindow::Settings);
    fixture.window().close();
    QCoreApplication::processEvents();

    // 关闭只是隐藏，窗口对象与其中的取值都还在。
    fixture.lifecycle().showAuxiliaryWindow(AppLifecycle::AuxiliaryWindow::Settings);
    QCOMPARE(fixture.window().settings().behavior.walkSpeedPxPerSec, 123);
    QCOMPARE(speed->value(), 123);
    // 同一种辅助窗口始终只有一个实例。
    QCOMPARE(fixture.lifecycle().auxiliaryWindow(
                 AppLifecycle::AuxiliaryWindow::Settings),
             &fixture.window());
}

// 第 14.8 节：schema 1 写入后，下一次启动读回同样的取值。
void TestSettingsIntegration::valuesSurviveARestart()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("settings.ini"));

    Settings expected;
    {
        QSettings backend(path, QSettings::IniFormat);
        SettingsStore store(backend);
        SettingsController controller(store);
        SettingsWindow window;
        QObject::connect(&window, &SettingsWindow::settingsCommitted, &controller,
                         &SettingsController::applyAndPersist);
        window.setSettings(controller.settings());

        QComboBox *scale =
            window.findChild<QComboBox *>(QStringLiteral("appearance-scale"));
        scale->setCurrentIndex(scale->findData(1));
        window.findChild<ValueEditor *>(QStringLiteral("dialogue-typing-ms"))
            ->findChild<QDoubleSpinBox *>()
            ->setValue(40.0);
        expected = controller.settings();
    }

    // 新进程读同一个配置文件。
    QSettings backend(path, QSettings::IniFormat);
    SettingsStore store(backend);
    const SettingsController restarted(store);
    QCOMPARE(restarted.settings(), expected);
    QCOMPARE(restarted.settings().appearance.scale, 1);
    QCOMPARE(restarted.settings().dialogue.typingMsPerChar, 40);
}

// 第 14.8 节：一次拖动立即生效但只写一次盘。
void TestSettingsIntegration::aDragWritesOnceAndKeepsTheFinalValue()
{
    Fixture fixture;
    QSignalSpy persisted(&fixture.controller(), &SettingsController::persisted);

    QSlider *slider = fixture.window()
                          .findChild<ValueEditor *>(QStringLiteral("behavior-walk-speed"))
                          ->findChild<QSlider *>();
    QVERIFY(slider != nullptr);
    for (const int value : {50, 55, 60, 65}) {
        slider->setValue(value);
    }

    // 中间值都已经进入运行时配置。
    QCOMPARE(fixture.controller().settings().behavior.walkSpeedPxPerSec, 65);
    QCOMPARE(persisted.count(), 0);

    QVERIFY(persisted.wait(2000));
    QCOMPARE(persisted.count(), 1);
    QCOMPARE(fixture.store().load().behavior.walkSpeedPxPerSec, 65);
}

void TestSettingsIntegration::groupResetFlowsThroughTheController()
{
    Fixture fixture;
    Settings changed = fixture.controller().settings();
    changed.behavior.walkSpeedPxPerSec = 61;
    changed.dialogue.typingMsPerChar = 40;
    fixture.controller().applyAndPersist(changed);

    for (QPushButton *button : fixture.window().findChildren<QPushButton *>()) {
        if (button->text().contains(QStringLiteral("恢复本组默认值"))) {
            button->click();
            break;
        }
    }

    const Settings defaults;
    // 只有行为组回到默认值。
    QCOMPARE(fixture.controller().settings().behavior, defaults.behavior);
    QCOMPARE(fixture.controller().settings().dialogue.typingMsPerChar, 40);
    QCOMPARE(fixture.store().load(), fixture.controller().settings());
    // 界面同步刷新。
    QCOMPARE(fixture.window()
                 .findChild<ValueEditor *>(QStringLiteral("behavior-walk-speed"))
                 ->value(),
             defaults.behavior.walkSpeedPxPerSec);
}

void TestSettingsIntegration::cancellingAResetChangesNothingAnywhere()
{
    Fixture fixture;
    Settings changed = fixture.controller().settings();
    changed.behavior.walkSpeedPxPerSec = 61;
    fixture.controller().applyAndPersist(changed);
    fixture.setConfirmResets(false);

    for (QPushButton *button : fixture.window().findChildren<QPushButton *>()) {
        if (button->text().contains(QStringLiteral("恢复"))) {
            button->click();
        }
    }

    QCOMPARE(fixture.controller().settings(), changed);
    QCOMPARE(fixture.store().load(), changed);
}

// 第 8.3 节：右键菜单与托盘的活动模式切换也经过同一个控制器，
// 设置窗口打开时显示的是唯一的运行时真相。
void TestSettingsIntegration::runtimeChangesRefreshTheOpenWindow()
{
    Fixture fixture;
    fixture.lifecycle().showAuxiliaryWindow(AppLifecycle::AuxiliaryWindow::Settings);

    Settings next = fixture.controller().settings();
    next.behavior.mode = mub::core::ActivityMode::Active;
    fixture.controller().applyAndPersist(next);

    QComboBox *mode =
        fixture.window().findChild<QComboBox *>(QStringLiteral("behavior-mode"));
    QCOMPARE(mode->currentData().toInt(),
             static_cast<int>(mub::core::ActivityMode::Active));
    QCOMPARE(fixture.window().settings().behavior.mode,
             mub::core::ActivityMode::Active);
}

QTEST_MAIN(TestSettingsIntegration)
#include "tst_settingsintegration.moc"
