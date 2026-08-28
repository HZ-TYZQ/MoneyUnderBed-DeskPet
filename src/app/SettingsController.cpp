#include "app/SettingsController.h"

#include "core/SettingsStore.h"

namespace mub::app {

namespace {

// 连续输入合并成一次写入的窗口。取值只需要「比人连续操作的间隔长、
// 比人察觉不到的延迟短」，不是行为参数，因此不进设置。
constexpr int kDefaultPersistDelayMs = 400;

} // namespace

SettingsController::SettingsController(core::SettingsStore &store, QObject *parent)
    : QObject(parent)
    , store_(&store)
    , settings_(store.load())
{
    persistTimer_.setSingleShot(true);
    persistTimer_.setInterval(kDefaultPersistDelayMs);
    connect(&persistTimer_, &QTimer::timeout, this, &SettingsController::persistNow);
}

const core::Settings &SettingsController::settings() const
{
    return settings_;
}

void SettingsController::apply(const core::Settings &next)
{
    applyInternal(next, false);
}

void SettingsController::applyAndPersist(const core::Settings &next)
{
    applyInternal(next, true);
}

void SettingsController::applyInternal(const core::Settings &next,
                                       const bool persistNowRequested)
{
    // 校验只在这里做一次。界面和领域模块都拿不到未经校验的值。
    const bool changed = adopt(core::sanitized(next));

    if (persistNowRequested) {
        // 一次编辑完成就写一次。值没变但还有待写的去抖，也在这里落地，
        // 否则「完成编辑立即落盘」会被前一次拖动的计时器拖后。
        if (changed || persistTimer_.isActive()) {
            persistTimer_.stop();
            persistNow();
        }
        return;
    }
    if (changed) {
        // 每次变更都重新计时：一次拖动只在停下来之后写一次。
        persistTimer_.start();
    }
}

bool SettingsController::adopt(const core::Settings &valid)
{
    if (valid == settings_) {
        // 值没变就不通知：滑块拖回原处不应该产生一次变更。
        return false;
    }

    const core::Settings previous = settings_;
    settings_ = valid;

    if (previous.behavior != settings_.behavior) {
        emit behaviorChanged(settings_.behavior);
    }
    if (previous.dialogue != settings_.dialogue) {
        emit dialogueChanged(settings_.dialogue);
    }
    if (previous.appearance != settings_.appearance) {
        emit appearanceChanged(settings_.appearance);
    }
    if (previous.window != settings_.window) {
        emit windowChanged(settings_.window);
    }
    emit settingsChanged(settings_);
    return true;
}

void SettingsController::applyForThisRunOnly(const core::Settings &next)
{
    // 只进运行时，不排队落盘：命令行覆盖不该悄悄改掉用户保存的配置。
    adopt(core::sanitized(next));
}

void SettingsController::persistNow()
{
    store_->save(settings_);
    emit persisted();
}

void SettingsController::flush()
{
    if (!persistTimer_.isActive()) {
        return;
    }
    persistTimer_.stop();
    persistNow();
}

bool SettingsController::hasPendingWrite() const
{
    return persistTimer_.isActive();
}

void SettingsController::resetGroup(const core::SettingsGroup group)
{
    const core::Settings defaults;
    core::Settings next = settings_;
    switch (group) {
    case core::SettingsGroup::Behavior:
        next.behavior = defaults.behavior;
        break;
    case core::SettingsGroup::Dialogue:
        next.dialogue = defaults.dialogue;
        break;
    case core::SettingsGroup::Appearance:
        next.appearance = defaults.appearance;
        break;
    case core::SettingsGroup::Window:
        next.window = defaults.window;
        break;
    }
    applyAndPersist(next);
}

void SettingsController::resetAll()
{
    // 走 store 的清除路径而不是写一份默认值：清掉自己的分组后，下次读取自然
    // 得到默认值，也不会把死键留在配置文件里。首次启动标记在设置分组之外，
    // 不受影响（第 5.2 节、第 14.8 节）。
    persistTimer_.stop();
    store_->restoreDefaults();
    adopt(core::Settings{});
    emit persisted();
}

void SettingsController::setPersistDelayMs(const int delayMs)
{
    persistTimer_.setInterval(delayMs);
}

int SettingsController::persistDelayMs() const
{
    return persistTimer_.interval();
}

} // namespace mub::app
