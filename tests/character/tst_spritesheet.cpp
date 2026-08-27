#include "character/AnimationClip.h"
#include "character/SpriteSheet.h"

#include <QImage>
#include <QTest>

using mub::character::SpriteSheet;
using mub::character::SpriteSheetError;

namespace {

QImage makeSheet(const int width, const int height,
                 const QImage::Format format = QImage::Format_ARGB32)
{
    QImage image(width, height, format);
    image.fill(format == QImage::Format_RGB32 ? Qt::red : Qt::transparent);
    return image;
}

} // namespace

class TestSpriteSheet final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsValidSheets_data();
    void acceptsValidSheets();
    void rejectsInvalidSheets_data();
    void rejectsInvalidSheets();
    void frameExtractionIsInBounds();
    void everyRegisteredSheetLoadsWithTheExpectedFrameCount_data();
    void everyRegisteredSheetLoadsWithTheExpectedFrameCount();
    void everyErrorHasADescription_data();
    void everyErrorHasADescription();
};

void TestSpriteSheet::acceptsValidSheets_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("expectedFrames");

    QTest::newRow("single frame") << 69 << 1;
    QTest::newRow("run sheet") << 69 * 8 << 8;
    QTest::newRow("idle sheet") << 69 * 9 << 9;
    QTest::newRow("icecream eat") << 69 * 20 << 20;
}

void TestSpriteSheet::acceptsValidSheets()
{
    QFETCH(int, width);
    QFETCH(int, expectedFrames);

    SpriteSheetError error = SpriteSheetError::DecodeFailed;
    const SpriteSheet sheet = SpriteSheet::fromImage(makeSheet(width, 111), &error);

    QCOMPARE(error, SpriteSheetError::None);
    QVERIFY(sheet.isValid());
    QCOMPARE(sheet.frameCount(), expectedFrames);
    QCOMPARE(sheet.frameSize(), QSize(69, 111));
}

void TestSpriteSheet::rejectsInvalidSheets_data()
{
    QTest::addColumn<QImage>("image");
    QTest::addColumn<SpriteSheetError>("expectedError");

    QTest::newRow("null image") << QImage() << SpriteSheetError::DecodeFailed;
    QTest::newRow("too short") << makeSheet(69, 110)
                               << SpriteSheetError::WrongFrameHeight;
    QTest::newRow("too tall") << makeSheet(69, 112)
                              << SpriteSheetError::WrongFrameHeight;
    QTest::newRow("narrower than one frame")
        << makeSheet(68, 111) << SpriteSheetError::NoFrames;
    QTest::newRow("width not a multiple")
        << makeSheet(69 * 3 + 1, 111) << SpriteSheetError::WidthNotFrameMultiple;
    QTest::newRow("no alpha channel")
        << makeSheet(69, 111, QImage::Format_RGB32)
        << SpriteSheetError::NoAlphaChannel;
}

void TestSpriteSheet::rejectsInvalidSheets()
{
    QFETCH(QImage, image);
    QFETCH(SpriteSheetError, expectedError);

    SpriteSheetError error = SpriteSheetError::None;
    const SpriteSheet sheet = SpriteSheet::fromImage(image, &error);

    QCOMPARE(error, expectedError);
    QVERIFY(!sheet.isValid());
    QCOMPARE(sheet.frameCount(), 0);
}

void TestSpriteSheet::frameExtractionIsInBounds()
{
    const SpriteSheet sheet = SpriteSheet::fromImage(makeSheet(69 * 3, 111));
    QVERIFY(sheet.isValid());

    for (int index = 0; index < sheet.frameCount(); ++index) {
        QCOMPARE(sheet.frame(index).size(), QSize(69, 111));
    }
    QVERIFY(sheet.frame(-1).isNull());
    QVERIFY(sheet.frame(sheet.frameCount()).isNull());
    QVERIFY(sheet.frame(1000).isNull());
}

void TestSpriteSheet::everyRegisteredSheetLoadsWithTheExpectedFrameCount_data()
{
    QTest::addColumn<QString>("assetPath");
    QTest::addColumn<int>("expectedFrames");

    for (const auto &entry : mub::character::registeredClips()) {
        QTest::newRow(entry.id)
            << mub::character::clipAssetPath(QString::fromLatin1(entry.id))
            << entry.frameCount;
    }
}

void TestSpriteSheet::everyRegisteredSheetLoadsWithTheExpectedFrameCount()
{
    QFETCH(QString, assetPath);
    QFETCH(int, expectedFrames);

    SpriteSheetError error = SpriteSheetError::None;
    const SpriteSheet sheet = SpriteSheet::load(assetPath, &error);

    QVERIFY2(sheet.isValid(),
             qPrintable(mub::character::describeSpriteSheetError(error)));
    QCOMPARE(sheet.frameCount(), expectedFrames);
    // 素材必须带透明通道，否则做不到可见像素命中与透明区域穿透。
    QVERIFY(sheet.image().hasAlphaChannel());
}

void TestSpriteSheet::everyErrorHasADescription_data()
{
    QTest::addColumn<SpriteSheetError>("error");

    QTest::newRow("None") << SpriteSheetError::None;
    QTest::newRow("FileMissing") << SpriteSheetError::FileMissing;
    QTest::newRow("DecodeFailed") << SpriteSheetError::DecodeFailed;
    QTest::newRow("WrongFrameHeight") << SpriteSheetError::WrongFrameHeight;
    QTest::newRow("WidthNotFrameMultiple") << SpriteSheetError::WidthNotFrameMultiple;
    QTest::newRow("NoFrames") << SpriteSheetError::NoFrames;
    QTest::newRow("NoAlphaChannel") << SpriteSheetError::NoAlphaChannel;
}

void TestSpriteSheet::everyErrorHasADescription()
{
    QFETCH(SpriteSheetError, error);
    // 自检失败时必须能写出可读原因，不能只有一个枚举值。
    QVERIFY(!mub::character::describeSpriteSheetError(error).isEmpty());
}

QTEST_MAIN(TestSpriteSheet)
#include "tst_spritesheet.moc"
