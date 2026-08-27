#pragma once

#include "core/Settings.h"

class QSettings;

namespace mub::core {

// 设置的持久化。
//
// docs/Decisions.md 第 5.1 节：修改后立即生效并保存，不设「应用」阶段；
// 提供「恢复默认设置」；使用各系统标准用户配置目录，
// 不把配置写在 EXE、AppImage 或当前工作目录旁边 —— 后者由调用方构造
// `QSettings` 时保证（默认构造的 QSettings 已经走 QStandardPaths）。
//
// 本类不拥有 `QSettings`，测试可以传入指向临时文件的实例。
class SettingsStore
{
public:
    explicit SettingsStore(QSettings &backend);

    // 读取。缺项用默认值补齐，非法值按 sanitized() 回落，不中断启动。
    Settings load() const;

    // 写入并落盘。
    void save(const Settings &settings);

    // 恢复默认：删除本程序写入的全部设置键，使下次读取得到默认值。
    // 只删自己的分组，不清空整个配置文件。
    void restoreDefaults();

    // 首次启动提示是否已经显示过。
    //
    // 这不是用户设置，因此不在 Settings 里，也**不受「恢复默认设置」影响** ——
    // 恢复默认是把设置调回出厂值，不是把程序变回从没运行过。
    bool firstRunNoticeShown() const;
    void markFirstRunNoticeShown();

private:
    QSettings *backend_;
};

} // namespace mub::core
