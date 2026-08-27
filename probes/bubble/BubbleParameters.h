#pragma once

namespace mub::bubbleprobe {

// 原型 A「表情嵌入面板」的参数。
//
// 默认值**逐条取自** `Temp/dialogue-bubble-designs/prototype.css` 中
// `body[data-layout="embedded"]` 的实际取值，不是重新设计的。
// 该 HTML 原型是项目所有者已经选定的那一版，因此它是布局的事实基线。
//
// 计划第 11.1 节要求项目所有者对比后冻结取值，写回 docs/Decisions.md，
// 正式气泡再按冻结值实现。本结构就是待冻结的清单。
//
// 所有长度单位都是**未经倍率放大的基准像素**。
// HTML 原型里整个 `.pet-system` 连同气泡一起 `transform: scale()`，
// 所以气泡与角色始终等比例，这里保持同样的做法。
struct BubbleParameters
{
    // 显示倍率。1 与 2 是正式基线，1.5 是候选档位。
    double scale = 2.0;

    // --- 面板：.dialogue-panel ---
    // 固定宽度，不随文字长短变化。这是原型 A 与我先前实现的关键区别。
    int panelWidth = 260;
    int panelMinHeight = 78;
    int paddingTop = 13;
    int paddingRight = 17;
    int paddingBottom = 17;
    // embedded 覆盖了通用的 17，给左侧表情让出位置。
    int paddingLeft = 72;
    int cornerRadius = 1;
    // background: rgb(12 11 14 / 86%)
    int panelRed = 12;
    int panelGreen = 11;
    int panelBlue = 14;
    int panelAlpha = 219;
    // border: 1px solid rgb(255 255 255 / 16%)
    int borderAlpha = 41;

    // --- 表情：.dialogue-portrait ---
    // 素材 120 x 144 按 object-fit: contain 缩到 60 x 72，比例正好一致。
    int portraitWidth = 60;
    int portraitHeight = 72;
    // 绝对定位在面板内，left: 6px，bottom: 3px。
    int portraitLeft = 6;
    int portraitBottom = 3;

    // --- 竖分隔线：.dialogue-panel::before ---
    // left: 66px，上下各内缩 8px，1px 宽，rgb(255 255 255 / 14%)
    int separatorLeft = 66;
    int separatorInset = 8;
    int separatorAlpha = 36;

    // --- 文字：.dialogue-text ---
    int fontPixelSize = 12;
    // line-height: 1.62，按千分之一存储以便用整数控件调节。
    int lineHeightPermille = 1620;
    int textMinHeight = 46;
    // 像素字体默认关闭抗锯齿。打开只为让项目所有者亲眼比较差别。
    bool antialiasText = false;

    // --- 翻页提示：.page-cue ---
    // 原型 A 只显示一个 □，不显示页码；页码是原型 C 的做法。
    bool showPageCue = true;
    int pageCueRight = 8;
    int pageCueBottom = 7;
    int pageCueFontSize = 7;
    int pageCueAlpha = 179;
    // 打字过程中提示变淡：opacity 0.28。
    int pageCueTypingAlphaPercent = 28;

    // --- 相对角色的位置：body[data-layout="embedded"] .dialogue-shell ---
    // 面板右边缘距角色右边缘 38px，面板下边缘距角色下边缘 107px，
    // 即气泡位于角色左上方并略有重叠。角色画在气泡之上。
    int offsetRight = 38;
    int offsetBottom = 107;
    // 面板距屏幕边缘的最小距离。HTML 原型没有边缘避让，
    // 这一项是为满足 docs/Decisions.md 第 11.3 节新增的。
    int screenMargin = 8;
    // 靠近屏幕左缘放不下时，是否把气泡镜像到角色右上方。
    // 关闭则只做夹取。避让方式在决策第 13 节仍未确定，交由审核决定。
    bool mirrorNearEdge = false;

    // --- 打字 ---
    // HTML 原型用的是 28 ms，落在决策第 4.1 节的 20–30 ms 区间内。
    int typingMsPerChar = 28;
};

} // namespace mub::bubbleprobe
