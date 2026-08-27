#pragma once

#include "core/ActivityMode.h"
#include "core/BubbleFrequency.h"

#include <QMetaType>
#include <QString>
#include <QStringView>

#include <span>

namespace mub::core {

// Linux 工作区可见性。docs/Decisions.md 第 3.4 节：
// Linux 默认显示在所有工作区，设置中允许改为当前工作区。
// Windows 没有公开稳定的「固定到所有虚拟桌面」接口，因此该项在 Windows 上不适用，
// 由平台能力自述决定是否显示，而不是在界面里判断当前系统。
enum class WorkspaceVisibility
{
    AllWorkspaces,
    CurrentWorkspace,
};

// 用户设置。
//
// 只包含第 5.1 节列出的设置项。**暂停与隐藏不在这里**：
// 第 2.2 节规定暂停只对当前运行周期有效，第 2.3 节规定隐藏状态不保存。
// 角色位置、互动记录和行为历史同样不保存（第 2.3 节），本结构不得扩张到那些内容。
struct Settings
{
    // 第 2.2 节：首次启动默认安静模式。
    ActivityMode mode = ActivityMode::Quiet;
    // 第 4 节：气泡默认低频。
    BubbleFrequency bubble = BubbleFrequency::Low;
    // 第 3.4 节：默认始终置顶。
    bool alwaysOnTop = true;
    // 第 5.1 节：整数倍率。
    int scale = 2;
    // 第 3.4 节：Linux 默认显示在所有工作区。
    WorkspaceVisibility workspace = WorkspaceVisibility::AllWorkspaces;

    friend bool operator==(const Settings &, const Settings &) = default;
};

// 允许的整数倍率档位。
//
// 第 5.1 节只冻结了「必须是整数」，完整档位集合仍在第 13 节的未定清单里，
// 第一版至少验收 `1×` 与 `2×`。这里给出的是当前实现集合，改动需同时改决策文档。
std::span<const int> allowedScales();
bool isAllowedScale(int scale);

// 配置文件里使用的稳定标识。
//
// 存字符串而不是枚举序号：枚举顺序将来若变化，序号会静默错位成另一档设置。
QString activityModeId(ActivityMode mode);
QString bubbleFrequencyId(BubbleFrequency frequency);
QString workspaceVisibilityId(WorkspaceVisibility visibility);

// 解析。无法识别时返回 `fallback`，不抛异常也不中断启动。
ActivityMode activityModeFromId(QStringView id, ActivityMode fallback);
BubbleFrequency bubbleFrequencyFromId(QStringView id, BubbleFrequency fallback);
WorkspaceVisibility workspaceVisibilityFromId(QStringView id,
                                              WorkspaceVisibility fallback);

// 把越界或损坏的取值拉回合法范围。
//
// 配置文件是用户可编辑的普通文件，也可能被上一版程序写坏。
// 读到非法值时回落到默认值，不让程序带着非法状态运行。
Settings sanitized(Settings settings);

} // namespace mub::core

// 设置要经由信号传递，必须能放进 QVariant。
Q_DECLARE_METATYPE(mub::core::Settings)
