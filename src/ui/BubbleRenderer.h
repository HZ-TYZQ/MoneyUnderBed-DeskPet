#pragma once

#include <QImage>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

class QPainter;

namespace mub::ui {

// 对话气泡的布局与绘制。
//
// 结构由 docs/Decisions.md 第 4 节冻结，取值由第 4.8 节冻结
// （见 BubbleMetrics.h）。本类不提供改动结构的入口，
// 唯一可变的是显示倍率 —— 那是第 5.1 节的用户设置项。
//
// 不持有窗口，也不接触平台 API：布局可以在无头环境下逐项测试。
class BubbleRenderer
{
public:
    explicit BubbleRenderer(double scale = 1.0);

    // 显示倍率。第 5.1 节：`1×`、`1.5×`、`2×` 等离散档位，一律最近邻采样。
    void setScale(double scale);
    double scale() const;

    // `face` 为空表示该页没有可用表情素材，此时只画文字区。
    // 正常内容里每页都有表情（第 4 节），空表情属于素材缺失的降级路径。
    void setContent(const QImage &face, const QString &fullText);
    // 已经打出的字符数，由 DialogueSession 的可见文本长度决定。
    void setVisibleCharacters(int count);
    // 打字未完成时翻页提示变淡。
    void setTyping(bool typing);

    // 当前内容下的面板尺寸，已含倍率。宽度恒为 kPanelWidth × 倍率。
    QSize panelSize() const;

    // 文字按文字区宽度折行后的结果。
    QStringList wrappedLines() const;
    // 文字区在当前倍率下最多能容纳几行。
    int maxLinesThatFit() const;
    // 折行结果是否超出面板。用于人工审核和测试，不改变绘制行为。
    bool overflowsPanel() const;

    void paint(QPainter &painter) const;

    // 面板相对角色窗口的位置，结果已限制在可用区域内。
    //
    // 第 4.8 节：面板右缘距角色右缘 kOffsetRight，下缘距角色下缘 kOffsetBottom，
    // 因此气泡位于角色左上方并与角色顶部略有重叠；靠近屏幕边缘时**夹取**回
    // 可用区域，不镜像到角色另一侧。
    QRect placeFor(const QRect &characterGeometry, const QRect &availableGeometry) const;

private:
    int scaled(int value) const;
    int lineHeight() const;
    int textAreaWidth() const;
    QStringList computeWrappedLines() const;

    double scale_ = 1.0;
    QImage face_;
    QString fullText_;
    int visibleCharacters_ = 0;
    bool typing_ = false;
};

} // namespace mub::ui
