// 唯一运行时设置的持有者。
//
// docs/Decisions.md 第 14.2 节：设置界面只产生变更，不直接读写 QSettings；
// 第 14.8 节：接收、作用与落盘分开——一次拖动只在停下来之后写一次。

#include "app/SettingsController.h"

#include "core/Settings.h"
#include "core/SettingsPresets.h"
#include "core/SettingsStore.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using mub::app::SettingsController;
using mub::core::Settings;
using mub::core::SettingsStore;

namespace {

// 每个用例一个独立的临时 ini。
class Fixture
{
public:
    Fixture()
        : backend_(dir_.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat)
        , store_(backend_)
        , controller_(store_)
    {
        // 去抖窗口在测试里压到最短，用等待信号而不是等待挂钟。
        controller_.setPersistDelayMs(1);
    }

    SettingsController &controller() { return controller_; }
    SettingsStore &store() { return store_; }
    QSettings &backend() { return backend_; }

private:
    QTemporaryDir dir_;
    QSettings backend_;
    SettingsStore store_;
    SettingsController controller_;
};

} // namespace

class TestSettingsController final : public QObject
{
    Q_OBJECT

private slots:
    void startsFromTheStoredConfiguration();
    void applyPublishesOnlyTheChangedDomains();
    void unchangedValuesProduceNothing();
    void invalidValuesAreSanitizedBeforePublishing();
    void aDragCollapsesIntoOneWrite();
    void finishingAnEditWritesImmediately();
    void finishingAnEditFlushesAPendingDrag();
    void flushWritesAPendingChange();
    void resetGroupOnlyAffectsThatGroup();
    void resetAllRestoresEveryGroup();
    void resetAllKeepsTheFirstRunFlag();
    void persistenceStaysOutOfTheUiAndDomainLayers();
};

void TestSettingsController::startsFromTheStoredConfiguration()
{
    QTemporaryDir dir;
    QSettings backend(dir.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
    SettingsStore store(backend);

    Settings stored;
    stored.behavior.walkSpeedPxPerSec = 61;
    stored.dialogue.typingMsPerChar = 40;
    store.save(stored);

    const SettingsController controller(store);
    QCOMPARE(controller.settings(), stored);
}

void TestSettingsController::applyPublishesOnlyTheChangedDomains()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();

    const QSignalSpy behavior(&controller, &SettingsController::behaviorChanged);
    const QSignalSpy dialogue(&controller, &SettingsController::dialogueChanged);
    const QSignalSpy appearance(&controller, &SettingsController::appearanceChanged);
    const QSignalSpy window(&controller, &SettingsController::windowChanged);
    const QSignalSpy all(&controller, &SettingsController::settingsChanged);

    Settings next = controller.settings();
    next.behavior.walkSpeedPxPerSec = 61;
    controller.applyAndPersist(next);

    // 运行时模块只订阅自己需要的领域，不该被无关改动叫醒。
    QCOMPARE(behavior.count(), 1);
    QCOMPARE(dialogue.count(), 0);
    QCOMPARE(appearance.count(), 0);
    QCOMPARE(window.count(), 0);
    QCOMPARE(all.count(), 1);
    QCOMPARE(controller.settings().behavior.walkSpeedPxPerSec, 61);
}

void TestSettingsController::unchangedValuesProduceNothing()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();

    const QSignalSpy all(&controller, &SettingsController::settingsChanged);
    const QSignalSpy persisted(&controller, &SettingsController::persisted);

    // 滑块拖出去又拖回原处：不是一次修改。
    controller.apply(controller.settings());

    QCOMPARE(all.count(), 0);
    QCOMPARE(persisted.count(), 0);
    QVERIFY(!controller.hasPendingWrite());
}

void TestSettingsController::invalidValuesAreSanitizedBeforePublishing()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();

    Settings next = controller.settings();
    next.behavior.idleMinMs = 9000;
    next.behavior.idleMaxMs = 1000;
    next.appearance.scale = 7;
    controller.applyAndPersist(next);

    // 领域模块拿到的永远是校验过的值：成对错误整对回落，倍率回到默认。
    const Settings defaults;
    QCOMPARE(controller.settings().behavior.idleMinMs, defaults.behavior.idleMinMs);
    QCOMPARE(controller.settings().behavior.idleMaxMs, defaults.behavior.idleMaxMs);
    QCOMPARE(controller.settings().appearance.scale, defaults.appearance.scale);
}

void TestSettingsController::aDragCollapsesIntoOneWrite()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();

    const QSignalSpy runtime(&controller, &SettingsController::behaviorChanged);
    QSignalSpy persisted(&controller, &SettingsController::persisted);

    // 一次拖动经过的中间值：每个都立即生效，但只应写一次盘。
    for (const int speed : {50, 52, 55, 58, 61}) {
        Settings next = controller.settings();
        next.behavior.walkSpeedPxPerSec = speed;
        controller.apply(next);
    }

    QCOMPARE(runtime.count(), 5);
    QCOMPARE(persisted.count(), 0);
    QVERIFY(controller.hasPendingWrite());

    QVERIFY(persisted.wait(2000));
    QCOMPARE(persisted.count(), 1);
    QVERIFY(!controller.hasPendingWrite());
    QCOMPARE(fixture.store().load().behavior.walkSpeedPxPerSec, 61);
}

void TestSettingsController::finishingAnEditWritesImmediately()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();
    const QSignalSpy persisted(&controller, &SettingsController::persisted);

    Settings next = controller.settings();
    next.dialogue.typingMsPerChar = 40;
    controller.applyAndPersist(next);

    // 不等去抖：编辑完成就落盘。
    QCOMPARE(persisted.count(), 1);
    QVERIFY(!controller.hasPendingWrite());
    QCOMPARE(fixture.store().load().dialogue.typingMsPerChar, 40);
}

void TestSettingsController::finishingAnEditFlushesAPendingDrag()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();
    const QSignalSpy persisted(&controller, &SettingsController::persisted);

    Settings dragged = controller.settings();
    dragged.behavior.walkSpeedPxPerSec = 61;
    controller.apply(dragged);
    QVERIFY(controller.hasPendingWrite());

    // 松手时报告的值与最后一个中间值相同，但仍然要把待写的写掉。
    controller.applyAndPersist(dragged);

    QCOMPARE(persisted.count(), 1);
    QVERIFY(!controller.hasPendingWrite());
    QCOMPARE(fixture.store().load().behavior.walkSpeedPxPerSec, 61);
}

void TestSettingsController::flushWritesAPendingChange()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();
    const QSignalSpy persisted(&controller, &SettingsController::persisted);

    Settings next = controller.settings();
    next.appearance.scale = 1;
    controller.apply(next);

    // 退出前调用：最后一次修改不会因为进程结束而丢掉。
    controller.flush();
    QCOMPARE(persisted.count(), 1);
    QCOMPARE(fixture.store().load().appearance.scale, 1);

    // 没有待写内容时 flush 不产生多余的写入。
    controller.flush();
    QCOMPARE(persisted.count(), 1);
}

void TestSettingsController::resetGroupOnlyAffectsThatGroup()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();

    Settings changed = controller.settings();
    changed.behavior.walkSpeedPxPerSec = 61;
    changed.dialogue.typingMsPerChar = 40;
    changed.appearance.scale = 1;
    changed.window.alwaysOnTop = false;
    controller.applyAndPersist(changed);

    controller.resetGroup(mub::core::SettingsGroup::Behavior);

    const Settings defaults;
    const Settings after = controller.settings();
    QCOMPARE(after.behavior, defaults.behavior);
    // 其他三组保持用户的取值。
    QCOMPARE(after.dialogue.typingMsPerChar, 40);
    QCOMPARE(after.appearance.scale, 1);
    QCOMPARE(after.window.alwaysOnTop, false);
    QCOMPARE(fixture.store().load(), after);
}

void TestSettingsController::resetAllRestoresEveryGroup()
{
    Fixture fixture;
    SettingsController &controller = fixture.controller();

    Settings changed = controller.settings();
    changed.behavior.walkSpeedPxPerSec = 61;
    changed.dialogue.typingMsPerChar = 40;
    changed.appearance.scale = 1;
    changed.window.alwaysOnTop = false;
    controller.applyAndPersist(changed);

    const QSignalSpy persisted(&controller, &SettingsController::persisted);
    controller.resetAll();

    QCOMPARE(controller.settings(), Settings{});
    QCOMPARE(fixture.store().load(), Settings{});
    QCOMPARE(persisted.count(), 1);
}

void TestSettingsController::resetAllKeepsTheFirstRunFlag()
{
    Fixture fixture;
    fixture.store().markFirstRunNoticeShown();

    fixture.controller().resetAll();

    // 恢复默认是把设置调回出厂值，不是把程序变回从没运行过（第 5.2 节）。
    QVERIFY(fixture.store().firstRunNoticeShown());
}

// 第 14.2 节：设置界面不直接读写 QSettings，各行为与对话模块也不接触持久化后端。
// 这条边界靠源码扫描守住——加一句 `#include <QSettings>` 就会被立刻发现。
void TestSettingsController::persistenceStaysOutOfTheUiAndDomainLayers()
{
    const QString root = QStringLiteral(MUB_SOURCE_ROOT);
    const QStringList layers{QStringLiteral("src/ui"), QStringLiteral("src/dialogue"),
                             QStringLiteral("src/character"), QStringLiteral("src/core")};

    QStringList offenders;
    for (const QString &layer : layers) {
        QDirIterator it(root + QLatin1Char('/') + layer,
                        {QStringLiteral("*.cpp"), QStringLiteral("*.h")}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            // 持久化后端本身当然要用 QSettings。
            if (path.contains(QStringLiteral("SettingsStore"))) {
                continue;
            }
            QFile file(path);
            QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(path));
            const QString text = QString::fromUtf8(file.readAll());
            if (text.contains(QStringLiteral("QSettings"))
                || text.contains(QStringLiteral("SettingsStore"))) {
                offenders.append(QDir(root).relativeFilePath(path));
            }
        }
    }

    QVERIFY2(offenders.isEmpty(), qPrintable(offenders.join(QStringLiteral(", "))));
}

QTEST_MAIN(TestSettingsController)
#include "tst_settingscontroller.moc"
