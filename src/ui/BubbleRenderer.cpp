#include "ui/BubbleRenderer.h"

#include "ui/BubbleMetrics.h"
#include "ui/DialogueFont.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace mub::ui {

namespace {

// 文字为纯白；边界与分隔线是同一种白色叠加，只有不透明度不同。
constexpr QColor kTextColour(255, 255, 255);
constexpr QColor kLineBase(255, 255, 255);
// 翻页提示的内容固定是一个 □（docs/Decisions.md 第 4.8 节）。
constexpr auto kPageCueGlyph = u"□";

} // namespace

BubbleRenderer::BubbleRenderer(const double scale)
{
    setScale(scale);
}

void BubbleRenderer::setScale(const double scale)
{
    scale_ = scale > 0.0 ? scale : 1.0;
}

double BubbleRenderer::scale() const
{
    return scale_;
}

void BubbleRenderer::setContent(const QImage &face, const QString &fullText)
{
    face_ = face;
    fullText_ = fullText;
    visibleCharacters_ = static_cast<int>(fullText.size());
}

void BubbleRenderer::setVisibleCharacters(const int count)
{
    visibleCharacters_ = std::clamp(count, 0, static_cast<int>(fullText_.size()));
}

void BubbleRenderer::setTyping(const bool typing)
{
    typing_ = typing;
}

int BubbleRenderer::scaled(const int value) const
{
    return static_cast<int>(std::lround(value * scale_));
}

int BubbleRenderer::lineHeight() const
{
    // 行高是字号的倍数，不是字体自身的行距。先放大字号再乘行高倍数，
    // 保证 1.5× 下的行距与 1× 成比例，而不是逐行累积取整误差。
    return std::max(1,
                    static_cast<int>(std::lround(scaled(bubble::kFontPixelSize)
                                                 * bubble::kLineHeightPermille
                                                 / 1000.0)));
}

int BubbleRenderer::textAreaWidth() const
{
    return std::max(1,
                    scaled(bubble::kPanelWidth) - scaled(bubble::kPaddingLeft)
                        - scaled(bubble::kPaddingRight));
}

QStringList BubbleRenderer::computeWrappedLines() const
{
    const QFont font = dialogueFont(scaled(bubble::kFontPixelSize));
    const QFontMetrics metrics(font);
    const int maxWidth = textAreaWidth();

    // 中文没有词边界，逐字符折行。
    QStringList lines;
    QString current;
    for (const QChar character : fullText_) {
        const QString candidate = current + character;
        if (!current.isEmpty() && metrics.horizontalAdvance(candidate) > maxWidth) {
            lines.append(current);
            current = character;
        } else {
            current = candidate;
        }
    }
    if (!current.isEmpty() || lines.isEmpty()) {
        lines.append(current);
    }
    return lines;
}

QStringList BubbleRenderer::wrappedLines() const
{
    return computeWrappedLines();
}

int BubbleRenderer::maxLinesThatFit() const
{
    // 文字区高度由文字区最小高度与面板最小高度共同决定。
    const int available =
        std::max(scaled(bubble::kTextMinHeight),
                 scaled(bubble::kPanelMinHeight) - scaled(bubble::kPaddingTop)
                     - scaled(bubble::kPaddingBottom));
    return std::max(1, available / lineHeight());
}

bool BubbleRenderer::overflowsPanel() const
{
    return computeWrappedLines().size() > maxLinesThatFit();
}

QSize BubbleRenderer::panelSize() const
{
    // 宽度固定，这是原型 A 的核心特征。
    const int width = scaled(bubble::kPanelWidth);

    const int lines = static_cast<int>(computeWrappedLines().size());
    const int textHeight =
        std::max(scaled(bubble::kTextMinHeight), lines * lineHeight());
    const int height =
        std::max(scaled(bubble::kPanelMinHeight),
                 scaled(bubble::kPaddingTop) + textHeight
                     + scaled(bubble::kPaddingBottom));
    return {width, height};
}

void BubbleRenderer::paint(QPainter &painter) const
{
    const QSize size = panelSize();
    const int radius = scaled(bubble::kCornerRadius);

    painter.save();
    // 最近邻采样与关闭抗锯齿都是第 5.1 与 4.8 节的硬性要求。
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, bubble::kAntialiasText);

    // 面板：半透明近黑矩形，一像素低对比度边界，无投影无发光。
    const QColor panel(bubble::kPanelRed, bubble::kPanelGreen, bubble::kPanelBlue,
                       bubble::kPanelAlpha);
    QColor border = kLineBase;
    border.setAlpha(bubble::kBorderAlpha);

    const QRect panelRect(0, 0, size.width() - 1, size.height() - 1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(panel);
    if (radius > 0) {
        painter.drawRoundedRect(panelRect, radius, radius);
    } else {
        painter.drawRect(panelRect);
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(border);
    if (radius > 0) {
        painter.drawRoundedRect(panelRect, radius, radius);
    } else {
        painter.drawRect(panelRect);
    }

    // 竖分隔线：上下各内缩。
    QColor separator = kLineBase;
    separator.setAlpha(bubble::kSeparatorAlpha);
    painter.setPen(separator);
    const int separatorX = scaled(bubble::kSeparatorLeft);
    const int inset = scaled(bubble::kSeparatorInset);
    painter.drawLine(separatorX, inset, separatorX, size.height() - inset);

    // 表情：贴面板左下角。素材 120 x 144 与显示尺寸 60 x 72 同比例，
    // 因此直接按目标矩形整数缩放，不需要额外的等比适配。
    if (!face_.isNull()) {
        const int portraitWidth = scaled(bubble::kPortraitWidth);
        const int portraitHeight = scaled(bubble::kPortraitHeight);
        const int x = scaled(bubble::kPortraitLeft);
        const int y = size.height() - scaled(bubble::kPortraitBottom) - portraitHeight;
        painter.drawImage(QRect(x, y, portraitWidth, portraitHeight), face_);
    }

    // 文字：左边从内边距开始，顶部对齐上内边距。
    const QFont font = dialogueFont(scaled(bubble::kFontPixelSize));
    painter.setFont(font);
    const QFontMetrics metrics(font);

    const int textX = scaled(bubble::kPaddingLeft);
    const int step = lineHeight();
    // 行盒把字形垂直居中放在行高里，与已审核原型一致。
    int baseline =
        scaled(bubble::kPaddingTop) + (step - metrics.height()) / 2 + metrics.ascent();

    int remaining = visibleCharacters_;
    painter.setPen(kTextColour);
    for (const QString &line : computeWrappedLines()) {
        const int take = std::clamp(remaining, 0, static_cast<int>(line.size()));
        if (take > 0) {
            painter.drawText(textX, baseline, line.left(take));
        }
        remaining -= static_cast<int>(line.size());
        baseline += step;
    }

    // 翻页提示：右下角一个 □，打字过程中变淡。不显示页码。
    const QFont cueFont = dialogueFont(scaled(bubble::kPageCueFontSize));
    painter.setFont(cueFont);
    const QFontMetrics cueMetrics(cueFont);

    QColor cue = kTextColour;
    const int alpha = typing_
        ? bubble::kPageCueAlpha * bubble::kPageCueTypingAlphaPercent / 100
        : bubble::kPageCueAlpha;
    cue.setAlpha(std::clamp(alpha, 0, 255));
    painter.setPen(cue);

    const QString glyph = QString::fromUtf16(kPageCueGlyph);
    painter.drawText(size.width() - scaled(bubble::kPageCueRight)
                         - cueMetrics.horizontalAdvance(glyph),
                     size.height() - scaled(bubble::kPageCueBottom), glyph);

    painter.restore();
}

QRect BubbleRenderer::placeFor(const QRect &characterGeometry,
                               const QRect &availableGeometry) const
{
    const QSize size = panelSize();
    const int margin = scaled(bubble::kScreenMargin);

    // QRect::right() 是最后一个像素的坐标，不是右边界；边界要 +1。
    // 第 4.8 节说的是**边缘之间**的距离，按像素坐标直接相减会多出 1 px。
    const int characterRightEdge = characterGeometry.right() + 1;
    const int characterBottomEdge = characterGeometry.bottom() + 1;
    int x = characterRightEdge - scaled(bubble::kOffsetRight) - size.width();
    int y = characterBottomEdge - scaled(bubble::kOffsetBottom) - size.height();

    // 第 4.8 节：边缘避让方式是夹取，不做镜像。面板比可用区域还宽时
    // 上界会低于下界，此时以左上角对齐，宁可右侧超出也不让左侧被裁。
    const int minX = availableGeometry.left() + margin;
    const int maxX = std::max(minX, availableGeometry.right() - margin - size.width());
    x = std::clamp(x, minX, maxX);

    const int minY = availableGeometry.top() + margin;
    const int maxY = std::max(minY, availableGeometry.bottom() - margin - size.height());
    y = std::clamp(y, minY, maxY);

    return {x, y, size.width(), size.height()};
}

} // namespace mub::ui
