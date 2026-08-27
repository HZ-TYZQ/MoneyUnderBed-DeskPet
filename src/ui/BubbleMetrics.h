#pragma once

namespace mub::ui::bubble {

// 气泡渲染参数。
//
// 每一条都直接对应 docs/Decisions.md 第 4.8 节冻结的取值，
// 单位是**未经倍率放大的基准像素**；气泡与角色按同一倍率一起缩放。
//
// 这些是决策值，不是可调参数：改动等于改动产品外观，必须先改决策文档。
// tests/ui/tst_bubblelayout.cpp 会把本文件逐条与决策文档核对，
// 任何一方单独改动都会让测试失败。
//
// 第 4 节的布局结构不因这些数值可调而改变：面板固定宽度、表情嵌在左侧、
// 中间一条低对比度竖分隔线、近直角、无投影无发光、翻页提示在右下角。

// --- 面板 ---
// 宽度固定，不随文字长短变化。这是原型 A 与其他两版的关键区别。
inline constexpr int kPanelWidth = 260;
inline constexpr int kPanelMinHeight = 78;
inline constexpr int kPaddingTop = 13;
inline constexpr int kPaddingRight = 17;
inline constexpr int kPaddingBottom = 17;
// 左侧较宽是为表情让出位置。
inline constexpr int kPaddingLeft = 72;
inline constexpr int kCornerRadius = 1;
inline constexpr int kPanelRed = 12;
inline constexpr int kPanelGreen = 11;
inline constexpr int kPanelBlue = 14;
inline constexpr int kPanelAlpha = 219;
inline constexpr int kBorderAlpha = 41;

// --- 表情与分隔线 ---
// 素材原尺寸 120 x 144，比例一致，为整数二分之一缩放。
inline constexpr int kPortraitWidth = 60;
inline constexpr int kPortraitHeight = 72;
inline constexpr int kPortraitLeft = 6;
inline constexpr int kPortraitBottom = 3;
inline constexpr int kSeparatorLeft = 66;
inline constexpr int kSeparatorInset = 8;
inline constexpr int kSeparatorAlpha = 36;

// --- 文字 ---
inline constexpr int kFontPixelSize = 12;
// 行高为字号的 1.62 倍，按千分之一存储以免引入浮点常量。
inline constexpr int kLineHeightPermille = 1620;
inline constexpr int kTextMinHeight = 46;
// 像素字体关闭抗锯齿。
inline constexpr bool kAntialiasText = false;

// --- 翻页提示 ---
// 内容只有一个 □。原型 A 不显示页码，页码是原型 C 的做法。
inline constexpr int kPageCueRight = 7;
inline constexpr int kPageCueBottom = 7;
inline constexpr int kPageCueFontSize = 7;
inline constexpr int kPageCueAlpha = 179;
// 打字过程中提示变淡。
inline constexpr int kPageCueTypingAlphaPercent = 28;

// --- 位置与层次 ---
// 面板右缘距角色右缘 38，面板下缘距角色下缘 90，
// 即气泡位于角色左上方并与角色顶部略有重叠。角色绘制在气泡之上。
inline constexpr int kOffsetRight = 38;
inline constexpr int kOffsetBottom = 90;
inline constexpr int kScreenMargin = 8;

} // namespace mub::ui::bubble
