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
// 布局结构由 docs/Decisions.md 第 4 节冻结，本类不得改变：
// 表情完整嵌在面板左侧，文字位于右侧，中间一条低对比度竖分隔线；
// 面板是近直角的半透明黑色矩形，只有低对比度单像素边界；
// 不加投影、发光、装饰标题栏或角色名标签；翻页提示位于右下角。
//
// 可以调的只有 BubbleParameters 里的数值。
class BubbleRenderer
{
public:
    explicit BubbleRenderer(BubbleParameters parameters = {});

    void setParameters(const BubbleParameters &parameters);
    const BubbleParameters &parameters() const;

    // 设置当前页内容。face 为空表示不画表情区。
    void setContent(const QImage &face, const QString &fullText);
    // 已打出的字符数，用于打字效果。
    void setVisibleCharacters(int count);
    void setPageIndicator(const QString &text);

    // 按当前参数与内容算出的面板尺寸，已含倍率。
    QSize panelSize() const;

    // 当前文本按 maxTextWidth 折行后的结果。
    QStringList wrappedLines() const;
    // 折行后的行数是否超过 maxLinesPerPage。
    bool overflowsPage() const;

    // 在 painter 的 (0,0) 处绘制整个面板。
    void paint(QPainter &painter) const;

    // 面板相对角色窗口的位置。
    //
    // 默认放在角色正上方并水平居中；顶部放不下时改放角色下方；
    // 左右超出可用区域时夹回，保证面板与表情都不被裁出屏幕。
    QRect placeFor(const QRect &characterGeometry, const QRect &availableGeometry) const;

private:
    int scaled(int value) const;
    QStringList computeWrappedLines() const;

    BubbleParameters parameters_;
    QImage face_;
    QString fullText_;
    QString pageIndicator_;
    int visibleCharacters_ = 0;
};

} // namespace mub::bubbleprobe
