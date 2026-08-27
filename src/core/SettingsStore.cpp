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
constexpr auto kWorkspace = "workspace";

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
    settings.workspace = workspaceVisibilityFromId(
        backend_->value(QLatin1StringView(kWorkspace)).toString(), defaults.workspace);
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
    backend_->setValue(QLatin1StringView(kWorkspace),
                       workspaceVisibilityId(valid.workspace));
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

} // namespace mub::core
