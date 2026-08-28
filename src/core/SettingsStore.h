#pragma once

#include "core/Settings.h"

class QSettings;

namespace mub::core {

// 设置的持久化。
//
// docs/Decisions.md 第 5.1 节与第 14.8 节：修改后立即生效并保存，不设「应用」阶段；
// 提供「恢复默认设置」；使用各系统标准用户配置目录，不把配置写在 EXE、AppImage 或
// 当前工作目录旁边——后者由调用方构造 `QSettings` 时保证。
//
// **本类只做 schema 编解码，不认识任何运行时对象。** 校验、向领域模块应用配置和
// 决定何时落盘都属于应用层的设置控制器（第 14.2 节）。
//
// 第 14.8 节的 schema 规则：
//
// - `1.1.0` 的设置结构从 `schemaVersion = 1` 开始。
// - 没有 schema 的配置是 `1.0.0` 候选留下的开发期数据，**不迁移旧字段**，
//   按 `1.1.0` 默认设置重新建立。
// - schema 高于本程序支持的版本时，用安全默认值启动并记录警告，且本次运行
//   **不再写入配置文件**，以免降级运行破坏未来版本的数据。
class SettingsStore
{
public:
    // 本程序写入并能够读取的 schema 版本。
    static constexpr int kSchemaVersion = 1;

    explicit SettingsStore(QSettings &backend);

    // 读取。缺项用默认值补齐，非法值按 sanitized() 回落，不中断启动。
    Settings load() const;

    // 写入并落盘。读到过未来 schema 时不写入，只记录一次警告。
    void save(const Settings &settings);

    // 恢复默认：删除本程序写入的全部设置键，使下次读取得到默认值。
    // 只删自己的分组，不清空整个配置文件。
    void restoreDefaults();

    // 本次运行是否读到了高于 `kSchemaVersion` 的配置。为真时 `save()` 与
    // `restoreDefaults()` 都不写盘。
    bool isFutureSchema() const;

    // 首次启动提示是否已经显示过。
    //
    // 这不是用户设置，因此不在 Settings 里，也**不受「恢复默认设置」和 schema
    // 重建影响** —— 恢复默认是把设置调回出厂值，不是把程序变回从没运行过。
    bool firstRunNoticeShown() const;
    void markFirstRunNoticeShown();

private:
    QSettings *backend_;
    // `load()` 是 const，但需要把「读到未来 schema」这个事实留给后续的写入路径。
    mutable bool futureSchema_ = false;
};

} // namespace mub::core
