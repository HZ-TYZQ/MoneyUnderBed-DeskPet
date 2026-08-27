#pragma once

namespace mub::bubbleprobe {

// 原型 A「表情嵌入面板」的全部可调参数。
//
// 计划第 11.1 节要求项目所有者对比实际字号、抗锯齿、行高、面板最大宽度、
// 每页行数、翻页提示和表情切换方式后冻结取值。本结构就是待冻结的清单：
// 审核结束后这些数值写回 docs/Decisions.md，正式气泡按冻结值实现。
//
// 所有长度单位都是**未经倍率放大的基准像素**，绘制时统一乘以 scale。
struct BubbleParameters
{
    // 显示倍率。1 与 2 是正式基线，1.5 是候选档位（docs/Decisions.md 第 13 节）。
    double scale = 2.0;

    // 字体像素字号。Ark Pixel 12px 为像素点阵字体，非整数倍会破坏栅格。
    int fontPixelSize = 12;
    // 像素字体默认关闭抗锯齿。打开只是为了让项目所有者亲眼比较差别。
    bool antialiasText = false;
    // 行间额外增加的像素，0 表示使用字体自身行高。
    int extraLineSpacing = 2;

    // 文字区最大宽度。超过即换行。
    int maxTextWidth = 180;
    // 每页最多显示多少行。超过时原型会提示该页需要人工再分页。
    int maxLinesPerPage = 3;

    // 面板内边距。
    int paddingHorizontal = 8;
    int paddingVertical = 6;
    // 表情与竖分隔线、竖分隔线与文字之间的间隔。
    int faceGap = 6;

    // 表情显示尺寸。素材原尺寸为 120 x 144。
    int faceWidth = 120;
    int faceHeight = 144;

    // 面板底色不透明度，0 到 255。
    int panelAlpha = 190;
    // 单像素边界与竖分隔线的不透明度，均为低对比度。
    int borderAlpha = 60;
    int separatorAlpha = 45;
    // 近直角。0 为纯直角。
    int cornerRadius = 1;

    // 面板与角色之间的垂直间隔。
    int gapToCharacter = 4;
    // 面板距屏幕边缘的最小距离。
    int screenMargin = 8;

    // 翻页提示，位于面板右下角。
    bool showPageIndicator = true;

    // 每字符打字毫秒数。docs/Decisions.md 第 4.1 节限定 20–30 ms。
    int typingMsPerChar = 25;
};

} // namespace mub::bubbleprobe
