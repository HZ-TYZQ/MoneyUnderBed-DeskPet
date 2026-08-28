#include "core/Settings.h"
#include "core/SettingsStore.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using mub::core::ActivityMode;
using mub::core::allowedScales;
using mub::core::isAllowedScale;
using mub::core::sanitized;
using mub::core::Settings;
using mub::core::SettingsStore;

namespace {

// 每个用例一个独立的临时 ini，互不影响，也不碰用户真实配置。
class TemporaryStore
{
public:
    TemporaryStore()
        : backend_(dir_.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat)
        , store_(backend_)
    {
    }

    SettingsStore &store() { return store_; }
    QSettings &backend() { return backend_; }

private:
    QTemporaryDir dir_;
    QSettings backend_;
    SettingsStore store_;
};

// 一份四组都被改过的设置，用来证明往返不丢字段。
Settings distinctiveSettings()
{
    Settings settings;
    settings.behavior.mode = ActivityMode::Active;
    settings.behavior.idleMinMs = 1000;
    settings.behavior.idleMaxMs = 5000;
    settings.behavior.walkMinMs = 900;
    settings.behavior.walkMaxMs = 3000;
    settings.behavior.restMinMs = 3000;
    settings.behavior.restMaxMs = 9000;
    settings.behavior.restChancePercent = 33;
    settings.behavior.approachCursorChancePercent = 7;
    settings.behavior.walkSpeedPxPerSec = 60;
    settings.behavior.returnSpeedPxPerSec = 120;
    settings.behavior.returnDelayMs = 800;
    settings.behavior.cursorSafeDistancePx = 90;
    settings.dialogue.chatterMinIntervalMs = 75000;
    settings.dialogue.chatterChancePercent = 44;
    settings.dialogue.clickTextChancePercent = 55;
    settings.dialogue.singlePageAutoHideMs = 5000;
    settings.dialogue.typingMsPerChar = 33;
    settings.appearance.scale = 1;
    settings.appearance.idleFrameMs = 120;
    settings.appearance.runFrameMs = 90;
    settings.appearance.icecreamFrameMs = 110;
    settings.window.alwaysOnTop = false;
    return settings;
}

} // namespace

class TestSettings final : public QObject
{
    Q_OBJECT

private slots:
    void defaultsFollowTheDecisionRecord();
    void allowedScalesAreIntegersAndCoverTheAcceptedOnes();
    void sanitizeRejectsScalesOutsideTheAllowedSet();
    void identifiersRoundTrip();
    void unknownIdentifiersFallBack();
    void singleValuesOutsideTheirRangeFallBack_data();
    void singleValuesOutsideTheirRangeFallBack();
    void invalidDurationPairsFallBackTogether_data();
    void invalidDurationPairsFallBackTogether();
    void validValuesSurviveSanitizing();
    void emptyStoreYieldsDefaults();
    void savedSettingsSurviveAReload();
    void savingWritesTheSchemaVersion();
    void configurationWithoutSchemaIsRebuiltFromDefaults();
    void futureSchemaFallsBackWithoutTouchingTheFile();
    void corruptedValuesFallBackWithoutLosingTheRest();
    void malformedConfigurationFileDoesNotCrash();
    void restoreDefaultsClearsOnlyOurGroup();
    void firstRunNoticeFlagSurvivesRestoreDefaults();
};

// 默认值直接对应决策文档，改这里等于改产品首次启动行为。
void TestSettings::defaultsFollowTheDecisionRecord()
{
    const Settings defaults;

    // 第 2.2 节：首次启动默认安静模式。
    QCOMPARE(defaults.behavior.mode, ActivityMode::Quiet);
    // 第 14.3 节的「中」档取值。
    QCOMPARE(defaults.behavior.idleMinMs, 2000);
    QCOMPARE(defaults.behavior.idleMaxMs, 6000);
    QCOMPARE(defaults.behavior.restChancePercent, 25);
    QCOMPARE(defaults.behavior.approachCursorChancePercent, 12);
    QCOMPARE(defaults.behavior.walkSpeedPxPerSec, 48);
    // 第 14.4 节：单击台词低档 20%，打字速度沿用原型审核的 28 ms。
    QCOMPARE(defaults.dialogue.clickTextChancePercent, 20);
    QCOMPARE(defaults.dialogue.typingMsPerChar, 28);
    QCOMPARE(defaults.dialogue.singlePageAutoHideMs, 4000);
    // 第 14.5 节的三个帧时长。
    QCOMPARE(defaults.appearance.idleFrameMs, 100);
    QCOMPARE(defaults.appearance.runFrameMs, 80);
    QCOMPARE(defaults.appearance.icecreamFrameMs, 100);
    // 第 5.1 节与第 3.4 节。
    QCOMPARE(defaults.appearance.scale, 2);
    QVERIFY(defaults.window.alwaysOnTop);
}

void TestSettings::allowedScalesAreIntegersAndCoverTheAcceptedOnes()
{
    // 第 5.1 节只冻结「必须是整数」；第一版至少验收 1× 与 2×。
    QVERIFY(isAllowedScale(1));
    QVERIFY(isAllowedScale(2));
    QVERIFY(!allowedScales().empty());
}

void TestSettings::sanitizeRejectsScalesOutsideTheAllowedSet()
{
    Settings settings;
    settings.appearance.scale = 7;
    QCOMPARE(sanitized(settings).appearance.scale, Settings{}.appearance.scale);

    // 第 14.5 节：不接受小数或连续缩放，`0` 同样非法。
    settings.appearance.scale = 0;
    QCOMPARE(sanitized(settings).appearance.scale, Settings{}.appearance.scale);
}

void TestSettings::identifiersRoundTrip()
{
    using mub::core::activityModeFromId;
    using mub::core::activityModeId;

    for (const ActivityMode mode : {ActivityMode::Quiet, ActivityMode::Active}) {
        QCOMPARE(activityModeFromId(activityModeId(mode), ActivityMode::Active), mode);
    }
}

void TestSettings::unknownIdentifiersFallBack()
{
    using mub::core::activityModeFromId;

    // 存的是字符串而不是枚举序号，读到不认识的值时回落而不是错位。
    QCOMPARE(activityModeFromId(QStringLiteral("berserk"), ActivityMode::Quiet),
             ActivityMode::Quiet);
}

void TestSettings::singleValuesOutsideTheirRangeFallBack_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<int>("value");

    QTest::newRow("restChance below") << QStringLiteral("restChancePercent") << -1;
    QTest::newRow("restChance above") << QStringLiteral("restChancePercent") << 101;
    QTest::newRow("approach above") << QStringLiteral("approachCursorChancePercent") << 200;
    QTest::newRow("walk speed below") << QStringLiteral("walkSpeedPxPerSec") << 0;
    QTest::newRow("walk speed above") << QStringLiteral("walkSpeedPxPerSec") << 4000;
    QTest::newRow("return speed below") << QStringLiteral("returnSpeedPxPerSec") << -5;
    QTest::newRow("return delay above") << QStringLiteral("returnDelayMs") << 999999;
    QTest::newRow("cursor distance above") << QStringLiteral("cursorSafeDistancePx") << 10000;
    // 第 14.4 节：闲聊间隔下限必须严格大于 0。
    QTest::newRow("chatter interval zero") << QStringLiteral("chatterMinIntervalMs") << 0;
    QTest::newRow("chatter interval negative") << QStringLiteral("chatterMinIntervalMs") << -1;
    QTest::newRow("chatter chance above") << QStringLiteral("chatterChancePercent") << 101;
    QTest::newRow("click chance below") << QStringLiteral("clickTextChancePercent") << -1;
    QTest::newRow("auto hide below") << QStringLiteral("singlePageAutoHideMs") << 10;
    QTest::newRow("typing below") << QStringLiteral("typingMsPerChar") << 0;
    QTest::newRow("typing above") << QStringLiteral("typingMsPerChar") << 5000;
    QTest::newRow("idle frame below") << QStringLiteral("idleFrameMs") << 1;
    QTest::newRow("run frame above") << QStringLiteral("runFrameMs") << 5000;
    QTest::newRow("icecream frame below") << QStringLiteral("icecreamFrameMs") << 0;
}

void TestSettings::singleValuesOutsideTheirRangeFallBack()
{
    QFETCH(QString, field);
    QFETCH(int, value);

    const Settings defaults;
    Settings settings;
    int *target = nullptr;

    if (field == QStringLiteral("restChancePercent")) {
        target = &settings.behavior.restChancePercent;
    } else if (field == QStringLiteral("approachCursorChancePercent")) {
        target = &settings.behavior.approachCursorChancePercent;
    } else if (field == QStringLiteral("walkSpeedPxPerSec")) {
        target = &settings.behavior.walkSpeedPxPerSec;
    } else if (field == QStringLiteral("returnSpeedPxPerSec")) {
        target = &settings.behavior.returnSpeedPxPerSec;
    } else if (field == QStringLiteral("returnDelayMs")) {
        target = &settings.behavior.returnDelayMs;
    } else if (field == QStringLiteral("cursorSafeDistancePx")) {
        target = &settings.behavior.cursorSafeDistancePx;
    } else if (field == QStringLiteral("chatterMinIntervalMs")) {
        target = &settings.dialogue.chatterMinIntervalMs;
    } else if (field == QStringLiteral("chatterChancePercent")) {
        target = &settings.dialogue.chatterChancePercent;
    } else if (field == QStringLiteral("clickTextChancePercent")) {
        target = &settings.dialogue.clickTextChancePercent;
    } else if (field == QStringLiteral("singlePageAutoHideMs")) {
        target = &settings.dialogue.singlePageAutoHideMs;
    } else if (field == QStringLiteral("typingMsPerChar")) {
        target = &settings.dialogue.typingMsPerChar;
    } else if (field == QStringLiteral("idleFrameMs")) {
        target = &settings.appearance.idleFrameMs;
    } else if (field == QStringLiteral("runFrameMs")) {
        target = &settings.appearance.runFrameMs;
    } else if (field == QStringLiteral("icecreamFrameMs")) {
        target = &settings.appearance.icecreamFrameMs;
    }
    QVERIFY(target != nullptr);

    *target = value;
    // 越界值回到默认值，其余字段不受影响——因此整体又等于默认设置。
    QCOMPARE(sanitized(settings), defaults);
}

void TestSettings::invalidDurationPairsFallBackTogether_data()
{
    QTest::addColumn<QString>("pair");
    QTest::addColumn<int>("minValue");
    QTest::addColumn<int>("maxValue");

    // 第 14.8 节：`sanitized()` 必须能处理「最小值大于最大值」这类关系错误。
    QTest::newRow("idle inverted") << QStringLiteral("idle") << 9000 << 1000;
    QTest::newRow("idle min out of range") << QStringLiteral("idle") << 10 << 6000;
    QTest::newRow("walk inverted") << QStringLiteral("walk") << 8000 << 2000;
    QTest::newRow("walk max out of range") << QStringLiteral("walk") << 1500 << 90000;
    QTest::newRow("rest inverted") << QStringLiteral("rest") << 30000 << 5000;
    QTest::newRow("rest both out of range") << QStringLiteral("rest") << 0 << 999999;
}

void TestSettings::invalidDurationPairsFallBackTogether()
{
    QFETCH(QString, pair);
    QFETCH(int, minValue);
    QFETCH(int, maxValue);

    const Settings defaults;
    Settings settings;
    if (pair == QStringLiteral("idle")) {
        settings.behavior.idleMinMs = minValue;
        settings.behavior.idleMaxMs = maxValue;
    } else if (pair == QStringLiteral("walk")) {
        settings.behavior.walkMinMs = minValue;
        settings.behavior.walkMaxMs = maxValue;
    } else {
        settings.behavior.restMinMs = minValue;
        settings.behavior.restMaxMs = maxValue;
    }

    // 整对回落：只修其中一个会留下一个用户从未选过的组合。
    QCOMPARE(sanitized(settings), defaults);
}

void TestSettings::validValuesSurviveSanitizing()
{
    const Settings settings = distinctiveSettings();
    QCOMPARE(sanitized(settings), settings);
}

void TestSettings::emptyStoreYieldsDefaults()
{
    TemporaryStore temporary;
    QCOMPARE(temporary.store().load(), Settings{});
}

void TestSettings::savedSettingsSurviveAReload()
{
    TemporaryStore temporary;

    // `1.0.0` 候选写下的扁平键没有迁移价值，保存时应当被清掉。
    temporary.backend().setValue(QStringLiteral("settings/workspace"),
                                 QStringLiteral("all-workspaces"));
    temporary.backend().setValue(QStringLiteral("settings/bubble"),
                                 QStringLiteral("normal"));

    const Settings settings = distinctiveSettings();
    temporary.store().save(settings);

    QCOMPARE(temporary.store().load(), settings);
    QVERIFY(!temporary.backend().contains(QStringLiteral("settings/workspace")));
    QVERIFY(!temporary.backend().contains(QStringLiteral("settings/bubble")));
}

void TestSettings::savingWritesTheSchemaVersion()
{
    TemporaryStore temporary;
    temporary.store().save(Settings{});

    QCOMPARE(temporary.backend().value(QStringLiteral("settings/schemaVersion")).toInt(),
             SettingsStore::kSchemaVersion);
    QCOMPARE(SettingsStore::kSchemaVersion, 1);
}

void TestSettings::configurationWithoutSchemaIsRebuiltFromDefaults()
{
    TemporaryStore temporary;

    // `1.0.0` 候选的配置：有值，但没有 schema。第 14.8 节明确不迁移旧字段。
    temporary.backend().setValue(QStringLiteral("settings/mode"),
                                 QStringLiteral("active"));
    temporary.backend().setValue(QStringLiteral("settings/scale"), 1);
    temporary.backend().sync();

    QCOMPARE(temporary.store().load(), Settings{});
}

void TestSettings::futureSchemaFallsBackWithoutTouchingTheFile()
{
    TemporaryStore temporary;

    temporary.backend().setValue(QStringLiteral("settings/schemaVersion"),
                                 SettingsStore::kSchemaVersion + 1);
    temporary.backend().setValue(QStringLiteral("settings/behavior/idleMinMs"), 4321);
    temporary.backend().setValue(QStringLiteral("settings/somethingNew"),
                                 QStringLiteral("from a later version"));
    temporary.backend().sync();

    // 降级运行：用安全默认值启动。
    QCOMPARE(temporary.store().load(), Settings{});
    QVERIFY(temporary.store().isFutureSchema());

    // 本次运行不得覆盖未来版本的配置。
    temporary.store().save(distinctiveSettings());
    temporary.store().restoreDefaults();
    temporary.backend().sync();

    QCOMPARE(temporary.backend().value(QStringLiteral("settings/schemaVersion")).toInt(),
             SettingsStore::kSchemaVersion + 1);
    QCOMPARE(temporary.backend().value(QStringLiteral("settings/behavior/idleMinMs")).toInt(),
             4321);
    QCOMPARE(temporary.backend().value(QStringLiteral("settings/somethingNew")).toString(),
             QStringLiteral("from a later version"));
}

void TestSettings::corruptedValuesFallBackWithoutLosingTheRest()
{
    TemporaryStore temporary;

    const Settings settings = distinctiveSettings();
    temporary.store().save(settings);

    // 配置文件是用户可编辑的普通文件。写坏一项不应该把其余设置一起丢掉。
    temporary.backend().setValue(QStringLiteral("settings/appearance/scale"), 7);
    temporary.backend().setValue(QStringLiteral("settings/behavior/mode"),
                                 QStringLiteral("???"));
    temporary.backend().setValue(QStringLiteral("settings/dialogue/typingMsPerChar"),
                                 QStringLiteral("很快"));
    temporary.backend().sync();

    const Settings loaded = temporary.store().load();
    const Settings defaults;
    QCOMPARE(loaded.appearance.scale, defaults.appearance.scale);
    QCOMPARE(loaded.behavior.mode, defaults.behavior.mode);
    QCOMPARE(loaded.dialogue.typingMsPerChar, defaults.dialogue.typingMsPerChar);
    // 没被写坏的项照常保留。
    QCOMPARE(loaded.dialogue.clickTextChancePercent,
             settings.dialogue.clickTextChancePercent);
    QCOMPARE(loaded.behavior.walkSpeedPxPerSec, settings.behavior.walkSpeedPxPerSec);
}

void TestSettings::malformedConfigurationFileDoesNotCrash()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("broken.ini"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("[settings\nschemaVersion=not-a-number\nscale=???\n") > 0);
    file.close();

    QSettings backend(path, QSettings::IniFormat);
    SettingsStore store(backend);
    QCOMPARE(store.load(), Settings{});
}

void TestSettings::restoreDefaultsClearsOnlyOurGroup()
{
    TemporaryStore temporary;

    temporary.store().save(distinctiveSettings());

    // 同一个配置文件里可能有别的分组，恢复默认不应该连它们一起清掉。
    temporary.backend().setValue(QStringLiteral("window/geometry"),
                                 QStringLiteral("keep me"));
    temporary.backend().sync();

    temporary.store().restoreDefaults();

    QCOMPARE(temporary.store().load(), Settings{});
    QCOMPARE(temporary.backend().value(QStringLiteral("window/geometry")).toString(),
             QStringLiteral("keep me"));
}

void TestSettings::firstRunNoticeFlagSurvivesRestoreDefaults()
{
    TemporaryStore temporary;

    temporary.store().markFirstRunNoticeShown();
    QVERIFY(temporary.store().firstRunNoticeShown());

    // 恢复默认是把设置调回出厂值，不是把程序变回从没运行过。
    temporary.store().restoreDefaults();
    QVERIFY(temporary.store().firstRunNoticeShown());
}

QTEST_MAIN(TestSettings)
#include "tst_settings.moc"
