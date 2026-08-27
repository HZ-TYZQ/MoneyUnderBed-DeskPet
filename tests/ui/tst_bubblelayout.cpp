#include "dialogue/DialogueData.h"
#include "ui/BubbleMetrics.h"
#include "ui/BubbleRenderer.h"

#include <QFile>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTest>

using mub::ui::BubbleRenderer;
namespace bubble = mub::ui::bubble;

namespace {

// 角色帧尺寸。所有角色素材同尺寸（docs/Decisions.md 第 7 节）。
constexpr int kFrameWidth = 69;
constexpr int kFrameHeight = 111;

// 一块常见的桌面可用区域。角落用例都相对它取。
const QRect kAvailable(0, 0, 1920, 1080);

// 第 5.1 节只允许整数倍率。`1×` 与 `2×` 是第一版的验收档，
// `3×` 一并覆盖，确认布局不是只在两档上凑对。
const QList<int> kScales{1, 2, 3};

int scaled(const int value, const int scale)
{
    return value * scale;
}

QRect characterAt(const QPoint &topLeft, const int scale)
{
    return {topLeft, QSize(scaled(kFrameWidth, scale), scaled(kFrameHeight, scale))};
}

QString decisionsText()
{
    QFile file(QStringLiteral(MUB_SOURCE_ROOT "/docs/Decisions.md"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

BubbleRenderer rendererWith(const QString &text, const int scale)
{
    BubbleRenderer renderer(scale);
    renderer.setContent(QImage(), text);
    return renderer;
}

} // namespace

class TestBubbleLayout final : public QObject
{
    Q_OBJECT

private slots:
    void frozenValuesMatchTheDecisionRecord();
    void panelWidthIsFixedRegardlessOfTextLength();
    void panelIsNeverSmallerThanTheFrozenMinimum();
    void placesTheBubbleAboveAndLeftOfTheCharacter();
    void keepsThePanelInsideTheAvailableAreaAtEveryCorner();
    void clampsInsteadOfMirroringNearTheLeftEdge();
    void everyPageFitsWithoutOverflowing();
};

// 第 4.8 节冻结的取值与 BubbleMetrics.h 必须一致。
// 任何一方单独改动都会让这个用例失败，决策文档因此是唯一事实来源。
void TestBubbleLayout::frozenValuesMatchTheDecisionRecord()
{
    const QString doc = decisionsText();
    QVERIFY2(!doc.isEmpty(), "docs/Decisions.md could not be read");

    const QStringList expected{
        QStringLiteral("固定宽度 `%1`").arg(bubble::kPanelWidth),
        QStringLiteral("最小高度 `%1`").arg(bubble::kPanelMinHeight),
        QStringLiteral("内边距：上 `%1`、右 `%2`、下 `%3`、左 `%4`")
            .arg(bubble::kPaddingTop)
            .arg(bubble::kPaddingRight)
            .arg(bubble::kPaddingBottom)
            .arg(bubble::kPaddingLeft),
        QStringLiteral("圆角半径 `%1`").arg(bubble::kCornerRadius),
        QStringLiteral("底色 `rgb(%1, %2, %3)`，不透明度 `%4`")
            .arg(bubble::kPanelRed)
            .arg(bubble::kPanelGreen)
            .arg(bubble::kPanelBlue)
            .arg(bubble::kPanelAlpha),
        QStringLiteral("单像素边界不透明度 `%1`").arg(bubble::kBorderAlpha),
        QStringLiteral("表情显示尺寸 `%1 x %2`")
            .arg(bubble::kPortraitWidth)
            .arg(bubble::kPortraitHeight),
        QStringLiteral("表情距面板左 `%1`、距面板底 `%2`")
            .arg(bubble::kPortraitLeft)
            .arg(bubble::kPortraitBottom),
        QStringLiteral("竖分隔线位于 `%1`，上下各内缩 `%2`，不透明度 `%3`")
            .arg(bubble::kSeparatorLeft)
            .arg(bubble::kSeparatorInset)
            .arg(bubble::kSeparatorAlpha),
        QStringLiteral("像素字号 `%1`").arg(bubble::kFontPixelSize),
        QStringLiteral("行高为字号的 `%1` 倍")
            .arg(QString::number(bubble::kLineHeightPermille / 1000.0, 'f', 2)),
        QStringLiteral("文字区最小高度 `%1`").arg(bubble::kTextMinHeight),
        QStringLiteral("距右 `%1`、距底 `%2`，字号 `%3`，不透明度 `%4`")
            .arg(bubble::kPageCueRight)
            .arg(bubble::kPageCueBottom)
            .arg(bubble::kPageCueFontSize)
            .arg(bubble::kPageCueAlpha),
        QStringLiteral("打字过程中不透明度降为 `%1%`")
            .arg(bubble::kPageCueTypingAlphaPercent),
        QStringLiteral("面板右缘距角色右缘 `%1`，面板下缘距角色下缘 `%2`")
            .arg(bubble::kOffsetRight)
            .arg(bubble::kOffsetBottom),
        QStringLiteral("面板距屏幕可用区域边缘至少 `%1`").arg(bubble::kScreenMargin),
    };

    for (const QString &needle : expected) {
        QVERIFY2(doc.contains(needle),
                 qPrintable(QStringLiteral("docs/Decisions.md 第 4.8 节缺少：%1")
                                .arg(needle)));
    }

    // 边缘避让方式与层次同样是冻结项，改了必须同时改实现。
    QVERIFY(doc.contains(QStringLiteral("屏幕边缘避让方式为夹取，不做镜像")));
    QVERIFY(doc.contains(QStringLiteral("角色绘制在气泡之上")));
    QVERIFY(doc.contains(QStringLiteral("关闭文字抗锯齿")));
    QCOMPARE(bubble::kAntialiasText, false);
}

// 固定宽度是原型 A 与其他两版的关键区别。
void TestBubbleLayout::panelWidthIsFixedRegardlessOfTextLength()
{
    for (const int scale : kScales) {
        const int expected = scaled(bubble::kPanelWidth, scale);
        QCOMPARE(rendererWith(QStringLiteral("好吃！"), scale).panelSize().width(),
                 expected);
        QCOMPARE(rendererWith(QStringLiteral("可恶，一定是这个家伙害我掉了甜筒！"), scale)
                     .panelSize()
                     .width(),
                 expected);
        QCOMPARE(rendererWith(QString(), scale).panelSize().width(), expected);
    }
}

void TestBubbleLayout::panelIsNeverSmallerThanTheFrozenMinimum()
{
    for (const int scale : kScales) {
        const QSize size = rendererWith(QStringLiteral("好吃！"), scale).panelSize();
        QCOMPARE(size.width(), scaled(bubble::kPanelWidth, scale));
        QVERIFY(size.height() >= scaled(bubble::kPanelMinHeight, scale));
    }
}

// 面板右缘距角色右缘 kOffsetRight，下缘距角色下缘 kOffsetBottom。
// 角色在屏幕右下角时不需要夹取，可以直接核对公式本身。
void TestBubbleLayout::placesTheBubbleAboveAndLeftOfTheCharacter()
{
    for (const int scale : kScales) {
        const BubbleRenderer renderer =
            rendererWith(QStringLiteral("你要陪我！"), scale);
        const QRect character =
            characterAt({kAvailable.right() - scaled(kFrameWidth, scale) + 1,
                         kAvailable.bottom() - scaled(kFrameHeight, scale) + 1},
                        scale);
        const QRect place = renderer.placeFor(character, kAvailable);
        const QSize panel = renderer.panelSize();

        QCOMPARE(place.size(), panel);
        QCOMPARE(place.right(), character.right() - scaled(bubble::kOffsetRight, scale));
        QCOMPARE(place.bottom(),
                 character.bottom() - scaled(bubble::kOffsetBottom, scale));
        // 位于角色左上方。
        QVERIFY(place.left() < character.left());
        QVERIFY(place.top() < character.top());
    }
}

void TestBubbleLayout::keepsThePanelInsideTheAvailableAreaAtEveryCorner()
{
    for (const int scale : kScales) {
        const BubbleRenderer renderer =
            rendererWith(QStringLiteral("我已经没有钱买第二个了……"), scale);
        const QSize panel = renderer.panelSize();
        const int margin = scaled(bubble::kScreenMargin, scale);
        const int width = scaled(kFrameWidth, scale);
        const int height = scaled(kFrameHeight, scale);

        const QList<QPoint> corners{
            {kAvailable.left(), kAvailable.top()},
            {kAvailable.right() - width + 1, kAvailable.top()},
            {kAvailable.left(), kAvailable.bottom() - height + 1},
            {kAvailable.right() - width + 1, kAvailable.bottom() - height + 1},
        };

        for (const QPoint &corner : corners) {
            const QRect place = renderer.placeFor(characterAt(corner, scale), kAvailable);
            QCOMPARE(place.size(), panel);
            QVERIFY2(place.left() >= kAvailable.left() + margin,
                     qPrintable(QStringLiteral("scale=%1 corner=%2,%3 left=%4")
                                    .arg(scale)
                                    .arg(corner.x())
                                    .arg(corner.y())
                                    .arg(place.left())));
            QVERIFY(place.top() >= kAvailable.top() + margin);
            QVERIFY(place.right() <= kAvailable.right() - margin);
            QVERIFY(place.bottom() <= kAvailable.bottom() - margin);
        }
    }
}

// 第 4.8 节：靠近屏幕左缘时夹取回可用区域，不镜像到角色右上方。
void TestBubbleLayout::clampsInsteadOfMirroringNearTheLeftEdge()
{
    for (const int scale : kScales) {
        const BubbleRenderer renderer = rendererWith(QStringLiteral("怎么啦？"), scale);
        const QRect character = characterAt({kAvailable.left(), 600}, scale);
        const QRect place = renderer.placeFor(character, kAvailable);

        const int margin = scaled(bubble::kScreenMargin, scale);
        QCOMPARE(place.left(), kAvailable.left() + margin);
        // 镜像的话左缘会落在角色右侧，那是被明确否决的做法。
        QVERIFY(place.left() < character.right());
        QVERIFY(place.left() != character.left() + scaled(bubble::kOffsetRight, scale));
    }
}

// 退出门：所有台词在各倍率下都要能读，不允许被面板裁掉。
void TestBubbleLayout::everyPageFitsWithoutOverflowing()
{
    for (const int scale : kScales) {
        BubbleRenderer renderer(scale);
        for (const mub::dialogue::Dialogue &entry : mub::dialogue::registeredDialogues()) {
            for (const mub::dialogue::DialoguePage &page : entry.pages) {
                const QString text =
                    QString::fromUtf8(reinterpret_cast<const char *>(page.text));
                renderer.setContent(QImage(), text);
                QVERIFY2(!renderer.overflowsPanel(),
                         qPrintable(QStringLiteral("scale=%1 id=%2 lines=%3 max=%4: %5")
                                        .arg(scale)
                                        .arg(QString::fromLatin1(entry.id))
                                        .arg(renderer.wrappedLines().size())
                                        .arg(renderer.maxLinesThatFit())
                                        .arg(text)));
            }
        }
    }
}

QTEST_MAIN(TestBubbleLayout)
#include "tst_bubblelayout.moc"
