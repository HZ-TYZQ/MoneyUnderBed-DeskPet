#pragma once

namespace mub::core {

// 活动模式。docs/Decisions.md 第 2.2 节：
// 只有安静与活跃两档，第一版不再增加额外的行为强度档位。
enum class ActivityMode
{
    // 仍播放待机动画，但不主动接近鼠标，也不主动显示气泡。
    Quiet,
    // 允许自主移动、偶尔接近鼠标，并按气泡设置的频率显示气泡。
    Active,
};

} // namespace mub::core
