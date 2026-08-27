#include "core/Settings.h"
#include "core/SettingsStore.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using mub::core::ActivityMode;
using mub::core::allowedScales;
using mub::core::BubbleFrequency;
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
    void emptyStoreYieldsDefaults();
    void savedSettingsSurviveAReload();
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
    QCOMPARE(defaults.mode, ActivityMode::Quiet);
    // 第 4 节：气泡默认低频。
    QCOMPARE(defaults.bubble, BubbleFrequency::Low);
    // 第 3.4 节：默认始终置顶。
    QVERIFY(defaults.alwaysOnTop);
    QVERIFY(isAllowedScale(defaults.scale));
}

void TestSettings::allowedScalesAreIntegersAndCoverTheAcceptedOnes()
{
    // 第 5.1 节：只用整数倍率；第一版至少验收 1x 与 2x。
    QVERIFY(isAllowedScale(1));
    QVERIFY(isAllowedScale(2));
    for (const int scale : allowedScales()) {
        QVERIFY2(scale >= 1, qPrintable(QString::number(scale)));
    }
}

void TestSettings::sanitizeRejectsScalesOutsideTheAllowedSet()
{
    const int fallback = Settings{}.scale;
    for (const int bad : {0, -1, 4, 99}) {
        Settings settings;
        settings.scale = bad;
        QCOMPARE(sanitized(settings).scale, fallback);
    }

    // 合法值原样保留，不被「校正」掉。
    for (const int good : allowedScales()) {
        Settings settings;
        settings.scale = good;
        QCOMPARE(sanitized(settings).scale, good);
    }
}

void TestSettings::identifiersRoundTrip()
{
    using mub::core::activityModeFromId;
    using mub::core::activityModeId;
    using mub::core::bubbleFrequencyFromId;
    using mub::core::bubbleFrequencyId;

    for (const ActivityMode mode : {ActivityMode::Quiet, ActivityMode::Active}) {
        QCOMPARE(activityModeFromId(activityModeId(mode), ActivityMode::Active), mode);
    }
    for (const BubbleFrequency bubble :
         {BubbleFrequency::Off, BubbleFrequency::Low, BubbleFrequency::Normal}) {
        QCOMPARE(bubbleFrequencyFromId(bubbleFrequencyId(bubble), BubbleFrequency::Off),
                 bubble);
    }
}

void TestSettings::unknownIdentifiersFallBack()
{
    using mub::core::activityModeFromId;
    using mub::core::bubbleFrequencyFromId;

    QCOMPARE(activityModeFromId(QStringLiteral("sleepy"), ActivityMode::Quiet),
             ActivityMode::Quiet);
    QCOMPARE(activityModeFromId(QString(), ActivityMode::Active), ActivityMode::Active);
    QCOMPARE(bubbleFrequencyFromId(QStringLiteral("chatty"), BubbleFrequency::Low),
             BubbleFrequency::Low);
}

void TestSettings::emptyStoreYieldsDefaults()
{
    TemporaryStore temporary;
    QCOMPARE(temporary.store().load(), Settings{});
}

void TestSettings::savedSettingsSurviveAReload()
{
    TemporaryStore temporary;

    // 旧开发构建写过该键；功能撤回后下一次保存应完成清理。
    temporary.backend().setValue(QStringLiteral("settings/workspace"),
                                 QStringLiteral("all-workspaces"));

    Settings settings;
    settings.mode = ActivityMode::Active;
    settings.bubble = BubbleFrequency::Normal;
    settings.alwaysOnTop = false;
    settings.scale = 1;
    temporary.store().save(settings);

    QCOMPARE(temporary.store().load(), settings);
    QVERIFY(!temporary.backend().contains(QStringLiteral("settings/workspace")));
}

void TestSettings::corruptedValuesFallBackWithoutLosingTheRest()
{
    TemporaryStore temporary;

    Settings settings;
    settings.mode = ActivityMode::Active;
    settings.bubble = BubbleFrequency::Normal;
    temporary.store().save(settings);

    // 配置文件是用户可编辑的普通文件。写坏一项不应该把其余设置一起丢掉。
    temporary.backend().setValue(QStringLiteral("settings/scale"), 7);
    temporary.backend().setValue(QStringLiteral("settings/mode"), QStringLiteral("???"));
    temporary.backend().sync();

    const Settings loaded = temporary.store().load();
    QCOMPARE(loaded.scale, Settings{}.scale);
    QCOMPARE(loaded.mode, Settings{}.mode);
    // 没被写坏的项照常保留。
    QCOMPARE(loaded.bubble, BubbleFrequency::Normal);
}

void TestSettings::malformedConfigurationFileDoesNotCrash()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("broken.ini"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("[settings\nscale=not-a-number\nmode=???\n") > 0);
    file.close();

    QSettings backend(path, QSettings::IniFormat);
    SettingsStore store(backend);
    const Settings loaded = store.load();
    QCOMPARE(loaded.scale, Settings{}.scale);
    QCOMPARE(loaded.mode, Settings{}.mode);
}

void TestSettings::restoreDefaultsClearsOnlyOurGroup()
{
    TemporaryStore temporary;

    Settings settings;
    settings.mode = ActivityMode::Active;
    settings.scale = 1;
    temporary.store().save(settings);

    // 同一个配置文件里可能有别的分组，恢复默认不应该连它们一起清掉。
    temporary.backend().setValue(QStringLiteral("window/geometry"),
                                 QStringLiteral("keep-me"));
    temporary.backend().sync();

    temporary.store().restoreDefaults();

    QCOMPARE(temporary.store().load(), Settings{});
    QCOMPARE(temporary.backend().value(QStringLiteral("window/geometry")).toString(),
             QStringLiteral("keep-me"));
}

// 恢复默认是把设置调回出厂值，不是把程序变回从没运行过。
void TestSettings::firstRunNoticeFlagSurvivesRestoreDefaults()
{
    TemporaryStore temporary;
    QVERIFY(!temporary.store().firstRunNoticeShown());

    temporary.store().markFirstRunNoticeShown();
    QVERIFY(temporary.store().firstRunNoticeShown());

    temporary.store().restoreDefaults();
    QVERIFY(temporary.store().firstRunNoticeShown());
    QCOMPARE(temporary.store().load(), Settings{});
}

QTEST_MAIN(TestSettings)
#include "tst_settings.moc"
