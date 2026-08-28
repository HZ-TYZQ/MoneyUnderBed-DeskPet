#pragma once

#include "core/Settings.h"

#include <QObject>
#include <QTimer>

namespace mub::core {
class SettingsStore;
}

namespace mub::app {

// 唯一运行时设置的持有者。
//
// docs/Decisions.md 第 14.2 节：设置界面只产生设置变更，不直接读写 `QSettings`，
// 各行为与对话模块也不接触持久化后端；应用层统一负责校验、向领域模块应用配置
// 和自动保存。
//
// 第 14.8 节把三件事分开：
//
// - **接收**：合法值一变化就进入运行时配置。
// - **作用**：正在执行的行为持有启动时的快照，从下一次对应行为开始使用新值。
//   这条边界由各领域模块自己保证（阶段 3），控制器只负责把新值送到。
// - **落盘**：滑块在 `sliderReleased`、数字框在 `editingFinished` 时写一次，
//   键盘等连续变更经短去抖后写一次。一次拖动不得逐帧同步配置文件。
class SettingsController final : public QObject
{
    Q_OBJECT

public:
    enum class Group
    {
        Behavior,
        Dialogue,
        Appearance,
        Window,
    };

    // 构造时从 `store` 读入一次，之后 `settings()` 就是唯一真相。
    explicit SettingsController(core::SettingsStore &store, QObject *parent = nullptr);

    const core::Settings &settings() const;

    // 立即生效，落盘经过去抖。用于滑块拖动和键盘连续输入。
    void apply(const core::Settings &next);

    // 立即生效并立即落盘。用于一次编辑完成（`sliderReleased`、`editingFinished`、
    // 下拉框选择、复选框切换）。
    void applyAndPersist(const core::Settings &next);

    // 把等待中的去抖落盘立刻写掉。退出前调用，避免丢掉最后一次修改。
    void flush();

    // 是否还有未落盘的修改。
    bool hasPendingWrite() const;

    // 按组恢复默认值。界面必须先取得用户确认，控制器收到即执行（第 14.2 节）。
    void resetGroup(Group group);
    // 全部恢复默认值。
    void resetAll();

    // 去抖时长。测试注入更短的值，产品用默认值。
    void setPersistDelayMs(int delayMs);
    int persistDelayMs() const;

signals:
    // 领域变更通知。运行时模块只订阅自己需要的那一个。
    void behaviorChanged(const mub::core::BehaviorSettings &behavior);
    void dialogueChanged(const mub::core::DialogueSettings &dialogue);
    void appearanceChanged(const mub::core::AppearanceSettings &appearance);
    void windowChanged(const mub::core::WindowSettings &window);

    // 整体变更。设置界面据此回填，托盘据此更新活动模式。
    void settingsChanged(const mub::core::Settings &settings);

    // 实际写入配置文件。运行时接收与落盘要能分别断言（计划第 6.3 节）。
    void persisted();

private:
    void applyInternal(const core::Settings &next, bool persistNowRequested);
    // 采纳一个已经校验过的值并发出领域通知。返回是否真的变了。
    bool adopt(const core::Settings &valid);
    void persistNow();

    core::SettingsStore *store_;
    core::Settings settings_;
    QTimer persistTimer_;
};

} // namespace mub::app
