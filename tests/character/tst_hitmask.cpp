#include "character/HitMask.h"
#include "character/SpriteSheet.h"

#include <QImage>
#include <QRegion>
#include <QTest>

using mub::character::opaqueRegion;

namespace {

// 左半不透明、右半透明的测试帧。
QImage halfOpaque(const int width, const int height)
{
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width / 2; ++x) {
            image.setPixelColor(x, y, QColor(255, 0, 0, 255));
        }
    }
    return image;
}

} // namespace

class TestHitMask final : public QObject
{
    Q_OBJECT

private slots:
    void fullyTransparentFrameProducesEmptyRegion();
    void fullyOpaqueFrameCoversTheWholeFrame();
    void onlyOpaquePixelsAreIncluded();
    void regionScalesWithIntegerScale_data();
    void regionScalesWithIntegerScale();
    void thresholdControlsWhatCountsAsOpaque();
    void invalidInputProducesEmptyRegion();
    void realCharacterFrameHasTransparentCorners();
};

void TestHitMask::fullyTransparentFrameProducesEmptyRegion()
{
    QImage image(10, 10, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QVERIFY(opaqueRegion(image, 1).isEmpty());
}

void TestHitMask::fullyOpaqueFrameCoversTheWholeFrame()
{
    QImage image(10, 8, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 255));
    const QRegion region = opaqueRegion(image, 1);
    QCOMPARE(region.boundingRect(), QRect(0, 0, 10, 8));
    QVERIFY(region.contains(QPoint(0, 0)));
    QVERIFY(region.contains(QPoint(9, 7)));
}

void TestHitMask::onlyOpaquePixelsAreIncluded()
{
    const QRegion region = opaqueRegion(halfOpaque(10, 4), 1);
    QCOMPARE(region.boundingRect(), QRect(0, 0, 5, 4));
    QVERIFY(region.contains(QPoint(4, 2)));
    // 透明的右半必须穿透，不能形成不可见的矩形阻挡区。
    QVERIFY(!region.contains(QPoint(5, 2)));
    QVERIFY(!region.contains(QPoint(9, 0)));
}

void TestHitMask::regionScalesWithIntegerScale_data()
{
    QTest::addColumn<int>("scale");
    QTest::newRow("1x") << 1;
    QTest::newRow("2x") << 2;
    QTest::newRow("3x") << 3;
    QTest::newRow("4x") << 4;
}

void TestHitMask::regionScalesWithIntegerScale()
{
    QFETCH(int, scale);

    const QRegion region = opaqueRegion(halfOpaque(10, 4), scale);
    QCOMPARE(region.boundingRect(), QRect(0, 0, 5 * scale, 4 * scale));

    // 缩放后仍然保持左实右空。
    QVERIFY(region.contains(QPoint(5 * scale - 1, 4 * scale - 1)));
    QVERIFY(!region.contains(QPoint(5 * scale, 0)));
}

void TestHitMask::thresholdControlsWhatCountsAsOpaque()
{
    QImage image(4, 1, QImage::Format_ARGB32);
    image.setPixelColor(0, 0, QColor(0, 0, 0, 0));
    image.setPixelColor(1, 0, QColor(0, 0, 0, 10));
    image.setPixelColor(2, 0, QColor(0, 0, 0, 128));
    image.setPixelColor(3, 0, QColor(0, 0, 0, 255));

    const QRegion lenient = opaqueRegion(image, 1, 1);
    QCOMPARE(lenient.boundingRect(), QRect(1, 0, 3, 1));

    const QRegion strict = opaqueRegion(image, 1, 129);
    QCOMPARE(strict.boundingRect(), QRect(3, 0, 1, 1));
}

void TestHitMask::invalidInputProducesEmptyRegion()
{
    QVERIFY(opaqueRegion(QImage(), 1).isEmpty());
    QVERIFY(opaqueRegion(halfOpaque(10, 4), 0).isEmpty());
    QVERIFY(opaqueRegion(halfOpaque(10, 4), -2).isEmpty());
}

void TestHitMask::realCharacterFrameHasTransparentCorners()
{
    const auto sheet = mub::character::SpriteSheet::load(
        QStringLiteral(":/assets/character/idle-down-left.png"));
    QVERIFY(sheet.isValid());

    const QRegion region = opaqueRegion(sheet.frame(0), 1);
    QVERIFY(!region.isEmpty());

    // 角色不会填满整个 69 x 111 的矩形，否则像素级穿透就没有意义。
    const QRect frameRect(0, 0, 69, 111);
    QVERIFY(region.boundingRect() != frameRect);
    QVERIFY(!region.contains(QPoint(0, 0)));
}

QTEST_MAIN(TestHitMask)
#include "tst_hitmask.moc"
