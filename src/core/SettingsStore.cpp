#include "core/SettingsStore.h"

#include <QSettings>
#include <QString>

namespace mub::core {

namespace {

// 键名是用户配置文件的一部分，改名等于丢掉用户已保存的设置。
constexpr auto kGroup = "settings";
constexpr auto kMode = "mode";
constexpr auto kBubble = "bubble";
constexpr auto kAlwaysOnTop = "alwaysOnTop";
constexpr auto kScale = "scale";
// 第一版撤回过的工作区设置。保存时顺手清掉旧构建留下的死键。
constexpr auto kLegacyWorkspace = "workspace";

// 首次启动标记放在设置分组之外，因此不会被「恢复默认设置」清掉。
constexpr auto kFirstRunNoticeShown = "state/firstRunNoticeShown";

} // namespace

SettingsStore::SettingsStore(QSettings &backend)
    : backend_(&backend)
{
}

Settings SettingsStore::load() const
{
    const Settings defaults;
    Settings settings;

    backend_->beginGroup(QLatin1StringView(kGroup));
    settings.mode = activityModeFromId(
        backend_->value(QLatin1StringView(kMode)).toString(), defaults.mode);
    settings.bubble = bubbleFrequencyFromId(
        backend_->value(QLatin1StringView(kBubble)).toString(), defaults.bubble);
    settings.alwaysOnTop =
        backend_->value(QLatin1StringView(kAlwaysOnTop), defaults.alwaysOnTop).toBool();
    settings.scale = backend_->value(QLatin1StringView(kScale), defaults.scale).toInt();
    backend_->endGroup();

    return sanitized(settings);
}

void SettingsStore::save(const Settings &settings)
{
    const Settings valid = sanitized(settings);

    backend_->beginGroup(QLatin1StringView(kGroup));
    backend_->setValue(QLatin1StringView(kMode), activityModeId(valid.mode));
    backend_->setValue(QLatin1StringView(kBubble), bubbleFrequencyId(valid.bubble));
    backend_->setValue(QLatin1StringView(kAlwaysOnTop), valid.alwaysOnTop);
    backend_->setValue(QLatin1StringView(kScale), valid.scale);
    backend_->remove(QLatin1StringView(kLegacyWorkspace));
    backend_->endGroup();
    backend_->sync();
}

void SettingsStore::restoreDefaults()
{
    backend_->beginGroup(QLatin1StringView(kGroup));
    backend_->remove(QString());
    backend_->endGroup();
    backend_->sync();
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
