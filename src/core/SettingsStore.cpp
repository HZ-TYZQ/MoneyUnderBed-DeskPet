#include "core/SettingsStore.h"

#include <QLoggingCategory>
#include <QSettings>
#include <QString>

namespace mub::core {

namespace {

Q_LOGGING_CATEGORY(lcSettingsStore, "mub.core.settingsstore")

// 键名是用户配置文件的一部分。`1.1.0` 是首个带 schema 的结构，
// 之后改名等于丢掉用户已保存的设置。
constexpr auto kGroup = "settings";
constexpr auto kSchemaVersionKey = "schemaVersion";

constexpr auto kBehaviorGroup = "behavior";
constexpr auto kMode = "mode";
constexpr auto kIdleMinMs = "idleMinMs";
constexpr auto kIdleMaxMs = "idleMaxMs";
constexpr auto kWalkMinMs = "walkMinMs";
constexpr auto kWalkMaxMs = "walkMaxMs";
constexpr auto kRestMinMs = "restMinMs";
constexpr auto kRestMaxMs = "restMaxMs";
constexpr auto kRestChancePercent = "restChancePercent";
constexpr auto kApproachCursorChancePercent = "approachCursorChancePercent";
constexpr auto kWalkSpeedPxPerSec = "walkSpeedPxPerSec";
constexpr auto kReturnSpeedPxPerSec = "returnSpeedPxPerSec";
constexpr auto kReturnDelayMs = "returnDelayMs";
constexpr auto kCursorSafeDistancePx = "cursorSafeDistancePx";

constexpr auto kDialogueGroup = "dialogue";
constexpr auto kChatterMinIntervalMs = "chatterMinIntervalMs";
constexpr auto kChatterChancePercent = "chatterChancePercent";
constexpr auto kClickTextChancePercent = "clickTextChancePercent";
constexpr auto kSinglePageAutoHideMs = "singlePageAutoHideMs";
constexpr auto kTypingMsPerChar = "typingMsPerChar";

constexpr auto kAppearanceGroup = "appearance";
constexpr auto kScale = "scale";
constexpr auto kIdleFrameMs = "idleFrameMs";
constexpr auto kRunFrameMs = "runFrameMs";
constexpr auto kIcecreamFrameMs = "icecreamFrameMs";

constexpr auto kWindowGroup = "window";
constexpr auto kAlwaysOnTop = "alwaysOnTop";

// 首次启动标记放在设置分组之外，因此不会被「恢复默认设置」或 schema 重建清掉。
constexpr auto kFirstRunNoticeShown = "state/firstRunNoticeShown";

int readInt(const QSettings &backend, const char *key, const int fallback)
{
    // 类型错误（例如手工把毫秒写成了 `"很快"`）在这里就退回默认值，
    // 之后 sanitized() 再做区间与成对校验。
    bool ok = false;
    const int value = backend.value(QLatin1StringView(key), fallback).toInt(&ok);
    return ok ? value : fallback;
}

} // namespace

SettingsStore::SettingsStore(QSettings &backend)
    : backend_(&backend)
{
}

Settings SettingsStore::load() const
{
    const Settings defaults;

    backend_->beginGroup(QLatin1StringView(kGroup));
    const int schema = readInt(*backend_, kSchemaVersionKey, 0);

    if (schema == 0) {
        // 没有 schema：`1.0.0` 候选留下的开发期数据。第 14.8 节明确不迁移旧字段。
        backend_->endGroup();
        qCInfo(lcSettingsStore)
            << "no schema in the stored configuration; rebuilding from defaults";
        return defaults;
    }
    if (schema > kSchemaVersion) {
        // 降级运行。用默认值启动，并且本次运行不再写盘。
        backend_->endGroup();
        futureSchema_ = true;
        qCWarning(lcSettingsStore)
            << "the stored configuration uses schema" << schema
            << "which is newer than the supported" << kSchemaVersion
            << "; starting from defaults and leaving it untouched";
        return defaults;
    }

    Settings settings;

    backend_->beginGroup(QLatin1StringView(kBehaviorGroup));
    BehaviorSettings &behavior = settings.behavior;
    behavior.mode = activityModeFromId(
        backend_->value(QLatin1StringView(kMode)).toString(), defaults.behavior.mode);
    behavior.idleMinMs = readInt(*backend_, kIdleMinMs, defaults.behavior.idleMinMs);
    behavior.idleMaxMs = readInt(*backend_, kIdleMaxMs, defaults.behavior.idleMaxMs);
    behavior.walkMinMs = readInt(*backend_, kWalkMinMs, defaults.behavior.walkMinMs);
    behavior.walkMaxMs = readInt(*backend_, kWalkMaxMs, defaults.behavior.walkMaxMs);
    behavior.restMinMs = readInt(*backend_, kRestMinMs, defaults.behavior.restMinMs);
    behavior.restMaxMs = readInt(*backend_, kRestMaxMs, defaults.behavior.restMaxMs);
    behavior.restChancePercent =
        readInt(*backend_, kRestChancePercent, defaults.behavior.restChancePercent);
    behavior.approachCursorChancePercent =
        readInt(*backend_, kApproachCursorChancePercent,
                defaults.behavior.approachCursorChancePercent);
    behavior.walkSpeedPxPerSec =
        readInt(*backend_, kWalkSpeedPxPerSec, defaults.behavior.walkSpeedPxPerSec);
    behavior.returnSpeedPxPerSec =
        readInt(*backend_, kReturnSpeedPxPerSec, defaults.behavior.returnSpeedPxPerSec);
    behavior.returnDelayMs =
        readInt(*backend_, kReturnDelayMs, defaults.behavior.returnDelayMs);
    behavior.cursorSafeDistancePx =
        readInt(*backend_, kCursorSafeDistancePx, defaults.behavior.cursorSafeDistancePx);
    backend_->endGroup();

    backend_->beginGroup(QLatin1StringView(kDialogueGroup));
    DialogueSettings &dialogue = settings.dialogue;
    dialogue.chatterMinIntervalMs = readInt(*backend_, kChatterMinIntervalMs,
                                            defaults.dialogue.chatterMinIntervalMs);
    dialogue.chatterChancePercent = readInt(*backend_, kChatterChancePercent,
                                            defaults.dialogue.chatterChancePercent);
    dialogue.clickTextChancePercent = readInt(*backend_, kClickTextChancePercent,
                                              defaults.dialogue.clickTextChancePercent);
    dialogue.singlePageAutoHideMs = readInt(*backend_, kSinglePageAutoHideMs,
                                            defaults.dialogue.singlePageAutoHideMs);
    dialogue.typingMsPerChar =
        readInt(*backend_, kTypingMsPerChar, defaults.dialogue.typingMsPerChar);
    backend_->endGroup();

    backend_->beginGroup(QLatin1StringView(kAppearanceGroup));
    AppearanceSettings &appearance = settings.appearance;
    appearance.scale = readInt(*backend_, kScale, defaults.appearance.scale);
    appearance.idleFrameMs =
        readInt(*backend_, kIdleFrameMs, defaults.appearance.idleFrameMs);
    appearance.runFrameMs =
        readInt(*backend_, kRunFrameMs, defaults.appearance.runFrameMs);
    appearance.icecreamFrameMs =
        readInt(*backend_, kIcecreamFrameMs, defaults.appearance.icecreamFrameMs);
    backend_->endGroup();

    backend_->beginGroup(QLatin1StringView(kWindowGroup));
    settings.window.alwaysOnTop =
        backend_->value(QLatin1StringView(kAlwaysOnTop), defaults.window.alwaysOnTop)
            .toBool();
    backend_->endGroup();

    backend_->endGroup();

    return sanitized(settings);
}

void SettingsStore::save(const Settings &settings)
{
    if (futureSchema_) {
        qCWarning(lcSettingsStore)
            << "refusing to overwrite a configuration written by a newer schema";
        return;
    }

    const Settings valid = sanitized(settings);

    backend_->beginGroup(QLatin1StringView(kGroup));
    // 先清空自己的分组再整体写回：`1.0.0` 候选写下的扁平键没有迁移价值
    // （第 14.8 节），留在文件里只会让用户困惑。清除范围仅限本分组。
    backend_->remove(QString());
    backend_->setValue(QLatin1StringView(kSchemaVersionKey), kSchemaVersion);

    backend_->beginGroup(QLatin1StringView(kBehaviorGroup));
    const BehaviorSettings &behavior = valid.behavior;
    backend_->setValue(QLatin1StringView(kMode), activityModeId(behavior.mode));
    backend_->setValue(QLatin1StringView(kIdleMinMs), behavior.idleMinMs);
    backend_->setValue(QLatin1StringView(kIdleMaxMs), behavior.idleMaxMs);
    backend_->setValue(QLatin1StringView(kWalkMinMs), behavior.walkMinMs);
    backend_->setValue(QLatin1StringView(kWalkMaxMs), behavior.walkMaxMs);
    backend_->setValue(QLatin1StringView(kRestMinMs), behavior.restMinMs);
    backend_->setValue(QLatin1StringView(kRestMaxMs), behavior.restMaxMs);
    backend_->setValue(QLatin1StringView(kRestChancePercent), behavior.restChancePercent);
    backend_->setValue(QLatin1StringView(kApproachCursorChancePercent),
                       behavior.approachCursorChancePercent);
    backend_->setValue(QLatin1StringView(kWalkSpeedPxPerSec), behavior.walkSpeedPxPerSec);
    backend_->setValue(QLatin1StringView(kReturnSpeedPxPerSec),
                       behavior.returnSpeedPxPerSec);
    backend_->setValue(QLatin1StringView(kReturnDelayMs), behavior.returnDelayMs);
    backend_->setValue(QLatin1StringView(kCursorSafeDistancePx),
                       behavior.cursorSafeDistancePx);
    backend_->endGroup();

    backend_->beginGroup(QLatin1StringView(kDialogueGroup));
    const DialogueSettings &dialogue = valid.dialogue;
    backend_->setValue(QLatin1StringView(kChatterMinIntervalMs),
                       dialogue.chatterMinIntervalMs);
    backend_->setValue(QLatin1StringView(kChatterChancePercent),
                       dialogue.chatterChancePercent);
    backend_->setValue(QLatin1StringView(kClickTextChancePercent),
                       dialogue.clickTextChancePercent);
    backend_->setValue(QLatin1StringView(kSinglePageAutoHideMs),
                       dialogue.singlePageAutoHideMs);
    backend_->setValue(QLatin1StringView(kTypingMsPerChar), dialogue.typingMsPerChar);
    backend_->endGroup();

    backend_->beginGroup(QLatin1StringView(kAppearanceGroup));
    const AppearanceSettings &appearance = valid.appearance;
    backend_->setValue(QLatin1StringView(kScale), appearance.scale);
    backend_->setValue(QLatin1StringView(kIdleFrameMs), appearance.idleFrameMs);
    backend_->setValue(QLatin1StringView(kRunFrameMs), appearance.runFrameMs);
    backend_->setValue(QLatin1StringView(kIcecreamFrameMs), appearance.icecreamFrameMs);
    backend_->endGroup();

    backend_->beginGroup(QLatin1StringView(kWindowGroup));
    backend_->setValue(QLatin1StringView(kAlwaysOnTop), valid.window.alwaysOnTop);
    backend_->endGroup();

    backend_->endGroup();
    backend_->sync();
}

void SettingsStore::restoreDefaults()
{
    if (futureSchema_) {
        qCWarning(lcSettingsStore)
            << "refusing to remove a configuration written by a newer schema";
        return;
    }

    backend_->beginGroup(QLatin1StringView(kGroup));
    backend_->remove(QString());
    backend_->endGroup();
    backend_->sync();
}

bool SettingsStore::isFutureSchema() const
{
    return futureSchema_;
}

bool SettingsStore::firstRunNoticeShown() const
{
    return backend_->value(QLatin1StringView(kFirstRunNoticeShown), false).toBool();
}

void SettingsStore::markFirstRunNoticeShown()
{
    backend_->setValue(QLatin1StringView(kFirstRunNoticeShown), true);
    backend_->sync();
}

} // namespace mub::core
