#include "BubbleRenderer.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace mub::bubbleprobe {

namespace {

// .dialogue-text { color: #fff }
constexpr QColor kTextColour(255, 255, 255);
// 边界与分隔线都是白色叠加，只有不透明度不同。
constexpr QColor kLineBase(255, 255, 255);
// .page-cue::before { content: "□" }
constexpr auto kPageCueGlyph = u"□";

} // namespace

BubbleRenderer::BubbleRenderer(BubbleParameters parameters)
    : parameters_(parameters)
{
}

void BubbleRenderer::setParameters(const BubbleParameters &parameters)
{
    parameters_ = parameters;
}

const BubbleParameters &BubbleRenderer::parameters() const
{
    return parameters_;
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
    return static_cast<int>(std::lround(value * parameters_.scale));
}

int BubbleRenderer::lineHeight() const
{
    // CSS 的 line-height 是字号的倍数，不是字体自身的行距。
    return std::max(1,
                    static_cast<int>(std::lround(
                        scaled(parameters_.fontPixelSize)
                        * parameters_.lineHeightPermille / 1000.0)));
}

int BubbleRenderer::textAreaWidth() const
{
    return std::max(1,
                    scaled(parameters_.panelWidth) - scaled(parameters_.paddingLeft)
                        - scaled(parameters_.paddingRight));
}

QStringList BubbleRenderer::computeWrappedLines() const
{
    QFont font;
    font.setPixelSize(scaled(parameters_.fontPixelSize));
    const QFontMetrics metrics(font);
    const int maxWidth = textAreaWidth();

    // 中文没有词边界，按字符逐个折行，与浏览器的 normal 换行一致。
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
    // 文字区高度由 min-height 与面板最小高度共同决定。
    const int available = std::max(
        scaled(parameters_.textMinHeight),
        scaled(parameters_.panelMinHeight) - scaled(parameters_.paddingTop)
            - scaled(parameters_.paddingBottom));
    return std::max(1, available / lineHeight());
}

bool BubbleRenderer::overflowsPanel() const
{
    return computeWrappedLines().size() > maxLinesThatFit();
}

QSize BubbleRenderer::panelSize() const
{
    // 宽度固定，这是原型 A 的核心特征。
    const int width = scaled(parameters_.panelWidth);

    const int lines = static_cast<int>(computeWrappedLines().size());
    const int textHeight =
        std::max(scaled(parameters_.textMinHeight), lines * lineHeight());
    const int height = std::max(
        scaled(parameters_.panelMinHeight),
        scaled(parameters_.paddingTop) + textHeight + scaled(parameters_.paddingBottom));
    return {width, height};
}

void BubbleRenderer::paint(QPainter &painter) const
{
    const QSize size = panelSize();
    const int radius = scaled(parameters_.cornerRadius);

    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, parameters_.antialiasText);

    // .dialogue-panel：半透明近黑矩形，1px 低对比度边界，无投影无发光。
    const QColor panel(parameters_.panelRed, parameters_.panelGreen,
                       parameters_.panelBlue, parameters_.panelAlpha);
    QColor border = kLineBase;
    border.setAlpha(parameters_.borderAlpha);

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

    // .dialogue-panel::before：竖分隔线，上下各内缩。
    QColor separator = kLineBase;
    separator.setAlpha(parameters_.separatorAlpha);
    painter.setPen(separator);
    const int separatorX = scaled(parameters_.separatorLeft);
    const int inset = scaled(parameters_.separatorInset);
    painter.drawLine(separatorX, inset, separatorX, size.height() - inset);

    // .dialogue-portrait：绝对定位在面板左下，object-fit: contain 已由
    // 60 x 72 与素材 120 x 144 的同比例保证，这里直接整数缩放。
    if (!face_.isNull()) {
        const int portraitWidth = scaled(parameters_.portraitWidth);
        const int portraitHeight = scaled(parameters_.portraitHeight);
        const int x = scaled(parameters_.portraitLeft);
        const int y = size.height() - scaled(parameters_.portraitBottom) - portraitHeight;
        painter.drawImage(QRect(x, y, portraitWidth, portraitHeight), face_);
    }

    // .dialogue-text：从 padding-left 开始，顶部对齐 padding-top。
    QFont font;
    font.setPixelSize(scaled(parameters_.fontPixelSize));
    painter.setFont(font);
    const QFontMetrics metrics(font);

    const int textX = scaled(parameters_.paddingLeft);
    const int step = lineHeight();
    // CSS 的行盒把字形垂直居中放在行高里。
    int baseline = scaled(parameters_.paddingTop) + (step - metrics.height()) / 2
        + metrics.ascent();

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

    // .page-cue：右下角一个 □，打字过程中变淡。原型 A 不显示页码。
    if (parameters_.showPageCue) {
        QFont cueFont;
        cueFont.setPixelSize(scaled(parameters_.pageCueFontSize));
        painter.setFont(cueFont);
        const QFontMetrics cueMetrics(cueFont);

        QColor cue = kTextColour;
        const int alpha = typing_
            ? parameters_.pageCueAlpha * parameters_.pageCueTypingAlphaPercent / 100
            : parameters_.pageCueAlpha;
        cue.setAlpha(std::clamp(alpha, 0, 255));
        painter.setPen(cue);

        const QString glyph = QString::fromUtf16(kPageCueGlyph);
        painter.drawText(size.width() - scaled(parameters_.pageCueRight)
                             - cueMetrics.horizontalAdvance(glyph),
                         size.height() - scaled(parameters_.pageCueBottom), glyph);
    }

    painter.restore();
}

QRect BubbleRenderer::placeFor(const QRect &characterGeometry,
                               const QRect &availableGeometry) const
{
    const QSize size = panelSize();
    const int margin = scaled(parameters_.screenMargin);

    // 审核冻结值：right 38px、bottom 90px，相对角色框。
    // 面板右边缘在角色右边缘左侧 38px 处，下边缘在角色下边缘上方 90px 处，
    // 因此气泡位于角色左上方并与角色顶部略有重叠。
    int right = characterGeometry.right() - scaled(parameters_.offsetRight);
    int x = right - size.width();
    int y = characterGeometry.bottom() - scaled(parameters_.offsetBottom) - size.height();

    const int minX = availableGeometry.left() + margin;
    const int maxX = std::max(minX, availableGeometry.right() - margin - size.width());

    if (parameters_.mirrorNearEdge && x < minX) {
        // 镜像到角色右上方：左边缘距角色左边缘 offsetRight。
        const int mirrored = characterGeometry.left() + scaled(parameters_.offsetRight);
        if (mirrored + size.width() <= availableGeometry.right() - margin) {
            x = mirrored;
        }
    }
    x = std::clamp(x, minX, maxX);

    const int minY = availableGeometry.top() + margin;
    const int maxY = std::max(minY, availableGeometry.bottom() - margin - size.height());
    y = std::clamp(y, minY, maxY);

    return {x, y, size.width(), size.height()};
}

} // namespace mub::bubbleprobe
