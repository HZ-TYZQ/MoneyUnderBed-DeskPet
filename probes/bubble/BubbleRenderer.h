#pragma once

#include "BubbleParameters.h"

#include <QImage>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

class QPainter;

namespace mub::bubbleprobe {

// 原型 A 的布局与绘制。
//
// 结构逐条对应 Temp/dialogue-bubble-designs/prototype.css 的
// `body[data-layout="embedded"]`：
//
// - 面板固定 260 px 宽，不随文字长短变化。
// - 表情绝对定位在面板左下，60 x 72，贴底。
// - 竖分隔线在 left: 66px，上下各内缩 8px，一像素。
// - 文字区由 padding-left: 72px 让出，右侧留 17px。
// - 翻页提示是右下角一个 □，打字过程中变淡。
// - 面板近直角、无投影、无发光。
//
// 可以调的只有 BubbleParameters 里的数值，结构本身不提供改动入口。
class BubbleRenderer
{
public:
    explicit BubbleRenderer(BubbleParameters parameters = {});

    void setParameters(const BubbleParameters &parameters);
    const BubbleParameters &parameters() const;

    // face 为空表示不画表情区。
    void setContent(const QImage &face, const QString &fullText);
    void setVisibleCharacters(int count);
    // 打字未完成时翻页提示变淡。
    void setTyping(bool typing);

    // 按当前参数与内容算出的面板尺寸，已含倍率。宽度恒为 panelWidth。
    QSize panelSize() const;

    // 文字按文字区宽度折行后的结果。
    QStringList wrappedLines() const;
    // 折行后是否超出文字区可容纳的高度。
    bool overflowsPanel() const;
    // 文字区在当前参数下最多能容纳几行。
    int maxLinesThatFit() const;

    void paint(QPainter &painter) const;

    // 面板相对角色窗口的位置。
    //
    // 默认按 CSS 的固定偏移放在角色左上方并略有重叠；
    // 超出可用区域时按 mirrorNearEdge 决定镜像还是夹取。
    QRect placeFor(const QRect &characterGeometry, const QRect &availableGeometry) const;

private:
    int scaled(int value) const;
    int lineHeight() const;
    int textAreaWidth() const;
    QStringList computeWrappedLines() const;

    BubbleParameters parameters_;
    QImage face_;
    QString fullText_;
    int visibleCharacters_ = 0;
    bool typing_ = false;
};

} // namespace mub::bubbleprobe
