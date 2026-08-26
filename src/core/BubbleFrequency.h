#pragma once

namespace mub::core {

// 气泡频率设置。docs/Decisions.md 第 4 节：只有关闭、低频和正常三档，
// 默认低频。安静模式完全抑制气泡，与本设置无关。
enum class BubbleFrequency
{
    Off,
    Low,
    Normal,
};

} // namespace mub::core
