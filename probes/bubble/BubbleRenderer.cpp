#include "BubbleRenderer.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace mub::bubbleprobe {

namespace {

// 面板与分隔线的基色。docs/Decisions.md 第 4 节：半透明黑色对话框。
constexpr QColor kPanelBase(0, 0, 0);
constexpr QColor kTextColour(240, 240, 240);
constexpr QColor kLineBase(255, 255, 255);

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

void BubbleRenderer::setPageIndicator(const QString &text)
{
    pageIndicator_ = text;
}

int BubbleRenderer::scaled(const int value) const
{
    return static_cast<int>(std::lround(value * parameters_.scale));
}

QStringList BubbleRenderer::computeWrappedLines() const
{
    QFont font;
    font.setPixelSize(scaled(parameters_.fontPixelSize));
    const QFontMetrics metrics(font);
    const int maxWidth = scaled(parameters_.maxTextWidth);

    // 中文没有词边界，按字符逐个折行。
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

bool BubbleRenderer::overflowsPage() const
{
    return computeWrappedLines().size() > parameters_.maxLinesPerPage;
}

QSize BubbleRenderer::panelSize() const
{
    QFont font;
    font.setPixelSize(scaled(parameters_.fontPixelSize));
    const QFontMetrics metrics(font);
    const int lineHeight = metrics.height() + scaled(parameters_.extraLineSpacing);

    const QStringList lines = computeWrappedLines();
    int textWidth = 0;
    for (const QString &line : lines) {
        textWidth = std::max(textWidth, metrics.horizontalAdvance(line));
    }
    textWidth = std::min(textWidth, scaled(parameters_.maxTextWidth));

    const int visibleLines =
        std::min(static_cast<int>(lines.size()), parameters_.maxLinesPerPage);
    int textHeight = lineHeight * std::max(1, visibleLines);
    if (parameters_.showPageIndicator) {
        textHeight += lineHeight;
    }

    const bool hasFace = !face_.isNull();
    const int faceWidth = hasFace ? scaled(parameters_.faceWidth) : 0;
    const int faceHeight = hasFace ? scaled(parameters_.faceHeight) : 0;
    // 表情与竖分隔线各占一次间隔，分隔线本身 1 像素。
    const int faceBlock = hasFace ? faceWidth + scaled(parameters_.faceGap) * 2 + 1 : 0;

    const int width = scaled(parameters_.paddingHorizontal) * 2 + faceBlock + textWidth;
    const int height = scaled(parameters_.paddingVertical) * 2
        + std::max(faceHeight, textHeight);
    return {width, height};
}

void BubbleRenderer::paint(QPainter &painter) const
{
    const QSize size = panelSize();
    const int radius = scaled(parameters_.cornerRadius);

    painter.save();
    // 像素画与像素字体都不能被平滑处理。
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, parameters_.antialiasText);

    // 面板底与单像素边界。不加投影、发光或装饰。
    QColor panel = kPanelBase;
    panel.setAlpha(parameters_.panelAlpha);
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

    const int padX = scaled(parameters_.paddingHorizontal);
    const int padY = scaled(parameters_.paddingVertical);
    int cursorX = padX;

    // 表情完整嵌在面板左侧，垂直居中。
    if (!face_.isNull()) {
        const int faceWidth = scaled(parameters_.faceWidth);
        const int faceHeight = scaled(parameters_.faceHeight);
        const int faceY = (size.height() - faceHeight) / 2;
        painter.drawImage(QRect(cursorX, faceY, faceWidth, faceHeight), face_);
        cursorX += faceWidth + scaled(parameters_.faceGap);

        // 低对比度竖分隔线，一像素。
        QColor separator = kLineBase;
        separator.setAlpha(parameters_.separatorAlpha);
        painter.setPen(separator);
        painter.drawLine(cursorX, padY, cursorX, size.height() - padY);
        cursorX += 1 + scaled(parameters_.faceGap);
    }

    QFont font;
    font.setPixelSize(scaled(parameters_.fontPixelSize));
    painter.setFont(font);
    const QFontMetrics metrics(font);
    const int lineHeight = metrics.height() + scaled(parameters_.extraLineSpacing);

    const QStringList lines = computeWrappedLines();
    const int visibleLines =
        std::min(static_cast<int>(lines.size()), parameters_.maxLinesPerPage);
    int blockHeight = lineHeight * std::max(1, visibleLines);
    if (parameters_.showPageIndicator) {
        blockHeight += lineHeight;
    }
    int y = (size.height() - blockHeight) / 2 + metrics.ascent();

    // 打字效果：按已打出的字符数逐行截断。
    int remaining = visibleCharacters_;
    painter.setPen(kTextColour);
    for (int index = 0; index < visibleLines; ++index) {
        const QString &line = lines.at(index);
        const int take = std::clamp(remaining, 0, static_cast<int>(line.size()));
        if (take > 0) {
            painter.drawText(cursorX, y, line.left(take));
        }
        remaining -= static_cast<int>(line.size());
        y += lineHeight;
    }

    // 翻页提示位于面板右下角。
    if (parameters_.showPageIndicator && !pageIndicator_.isEmpty()) {
        QColor hint = kTextColour;
        hint.setAlpha(150);
        painter.setPen(hint);
        const int width = metrics.horizontalAdvance(pageIndicator_);
        painter.drawText(size.width() - padX - width,
                         size.height() - padY - metrics.descent(), pageIndicator_);
    }

    painter.restore();
}

QRect BubbleRenderer::placeFor(const QRect &characterGeometry,
                               const QRect &availableGeometry) const
{
    const QSize size = panelSize();
    const int gap = scaled(parameters_.gapToCharacter);
    const int margin = scaled(parameters_.screenMargin);

    // 默认放在角色正上方并水平居中。
    int x = characterGeometry.center().x() - size.width() / 2;
    int y = characterGeometry.top() - gap - size.height();

    // 顶部放不下就改放角色下方。
    if (y < availableGeometry.top() + margin) {
        y = characterGeometry.bottom() + gap;
    }
    // 下方也放不下时贴住可用区域顶部，保证面板与表情都不被裁出屏幕。
    if (y + size.height() > availableGeometry.bottom() - margin) {
        y = std::max(availableGeometry.top() + margin,
                     availableGeometry.bottom() - margin - size.height());
    }

    const int minX = availableGeometry.left() + margin;
    const int maxX = std::max(minX, availableGeometry.right() - margin - size.width());
    x = std::clamp(x, minX, maxX);

    return {x, y, size.width(), size.height()};
}

} // namespace mub::bubbleprobe
