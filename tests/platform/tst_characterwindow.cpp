#include "FakeWindowBackend.h"

#include "character/HitMask.h"
#include "character/SpriteSheet.h"
#include "platform/BackendFactory.h"
#include "platform/DeskPetWindowBackend.h"
#include "ui/CharacterWindow.h"

#include <QImage>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using mub::character::SpriteSheet;
using mub::testing::FakeWindowBackend;
using mub::ui::CharacterWindow;

namespace {

SpriteSheet loadIdleSheet()
{
    return SpriteSheet::load(QStringLiteral(":/assets/character/idle-down-left.png"));
}

} // namespace

class TestCharacterWindow final : public QObject
{
    Q_OBJECT

private slots:
    void windowSizeIsTheFrameSizeTimesTheScale_data();
    void windowSizeIsTheFrameSizeTimesTheScale();
    void invalidScaleFallsBackToOne();
    void configuresTheBackendBeforeFirstShowAndOnlyOnce();
    void alwaysOnTopIsForwardedToTheBackend();
    void hitMaskIsAppliedAndMatchesTheFrameAlpha();
    void changingTheFrameReappliesTheHitMask();
    void frameIndexWrapsAround();
    void bottomPlacementUsesTheAvailableArea();
    void renderingUsesNearestNeighbour_data();
    void renderingUsesNearestNeighbour();
    void backendWithoutHitMaskSupportIsNotCalled();
    void realBackendReportsTheCapabilitiesTheProductNeeds();
    void realBackendAppliesDeskPetFlagsBeforeShow();
};

void TestCharacterWindow::windowSizeIsTheFrameSizeTimesTheScale_data()
{
    QTest::addColumn<int>("scale");
    QTest::newRow("1x") << 1;
    QTest::newRow("2x") << 2;
    QTest::newRow("3x") << 3;
    QTest::newRow("4x") << 4;
}

void TestCharacterWindow::windowSizeIsTheFrameSizeTimesTheScale()
{
    QFETCH(int, scale);

    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), scale, &backend);

    QCOMPARE(window.integerScale(), scale);
    QCOMPARE(window.size(), QSize(69 * scale, 111 * scale));
}

void TestCharacterWindow::invalidScaleFallsBackToOne()
{
    FakeWindowBackend backend;
    CharacterWindow zero(loadIdleSheet(), 0, &backend);
    CharacterWindow negative(loadIdleSheet(), -3, &backend);

    QCOMPARE(zero.integerScale(), 1);
    QCOMPARE(negative.integerScale(), 1);
}

void TestCharacterWindow::configuresTheBackendBeforeFirstShowAndOnlyOnce()
{
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 2, &backend);

    // 窗口类型与焦点策略是映射时属性，必须在 show() 前交给后端配置。
    QCOMPARE(backend.calls().configureAsDeskPet, 1);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(backend.calls().configureAsDeskPet, 1);

    window.hide();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    // 重复显示不应重新配置窗口。
    QCOMPARE(backend.calls().configureAsDeskPet, 1);
}

void TestCharacterWindow::alwaysOnTopIsForwardedToTheBackend()
{
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 2, &backend);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // 默认置顶（docs/Decisions.md 第 3.4 节）。
    QVERIFY(window.isAlwaysOnTop());
    QVERIFY(backend.alwaysOnTop());

    window.setAlwaysOnTop(false);
    QVERIFY(!window.isAlwaysOnTop());
    QVERIFY(!backend.alwaysOnTop());

    window.setAlwaysOnTop(true);
    QVERIFY(backend.alwaysOnTop());
}

void TestCharacterWindow::hitMaskIsAppliedAndMatchesTheFrameAlpha()
{
    FakeWindowBackend backend;
    const SpriteSheet sheet = loadIdleSheet();
    QVERIFY(sheet.isValid());

    CharacterWindow window(sheet, 2, &backend);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(backend.calls().setHitMask > 0);
    QVERIFY(!window.hitRegion().isEmpty());
    QCOMPARE(backend.lastMask(), window.hitRegion());
    QCOMPARE(window.hitRegion(),
             mub::character::opaqueRegion(sheet.frame(0), 2));

    // 透明的左上角必须穿透。
    QVERIFY(!window.hitRegion().contains(QPoint(0, 0)));
}

void TestCharacterWindow::changingTheFrameReappliesTheHitMask()
{
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 2, &backend);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const int before = backend.calls().setHitMask;
    window.setFrameIndex(4);
    QCOMPARE(window.frameIndex(), 4);
    QVERIFY(backend.calls().setHitMask > before);
}

void TestCharacterWindow::frameIndexWrapsAround()
{
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 1, &backend);

    // 待机表 9 帧。
    window.setFrameIndex(9);
    QCOMPARE(window.frameIndex(), 0);
    window.setFrameIndex(10);
    QCOMPARE(window.frameIndex(), 1);
    window.setFrameIndex(-1);
    QCOMPARE(window.frameIndex(), 8);
}

void TestCharacterWindow::bottomPlacementUsesTheAvailableArea()
{
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 2, &backend);

    window.moveToBottomOf(QRect(0, 0, 1920, 1080), 0.5);
    QCOMPARE(window.pos().y(), 1080 - 111 * 2);
}

void TestCharacterWindow::renderingUsesNearestNeighbour_data()
{
    QTest::addColumn<int>("scale");
    QTest::newRow("2x") << 2;
    QTest::newRow("4x") << 4;
}

void TestCharacterWindow::renderingUsesNearestNeighbour()
{
    QFETCH(int, scale);

    FakeWindowBackend backend;
    const SpriteSheet sheet = loadIdleSheet();
    CharacterWindow window(sheet, scale, &backend);

    QImage rendered(window.size(), QImage::Format_ARGB32);
    rendered.fill(Qt::transparent);
    window.render(&rendered);

    const QImage source = sheet.frame(0).convertToFormat(QImage::Format_ARGB32);

    // 最近邻放大意味着每个源像素变成一个 scale x scale 的纯色块。
    // 出现插值时块内会有渐变，下面的比较就会失败。
    int comparedBlocks = 0;
    for (int y = 0; y < source.height(); ++y) {
        for (int x = 0; x < source.width(); ++x) {
            const QRgb expected = rendered.pixel(x * scale, y * scale);
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    QCOMPARE(rendered.pixel(x * scale + dx, y * scale + dy),
                             expected);
                }
            }
            ++comparedBlocks;
        }
    }
    QCOMPARE(comparedBlocks, source.width() * source.height());
}

void TestCharacterWindow::backendWithoutHitMaskSupportIsNotCalled()
{
    mub::platform::BackendCapabilities limited;
    limited.name = QStringLiteral("no-mask");
    limited.alwaysOnTop = true;
    limited.pixelHitMask = false;

    FakeWindowBackend backend(limited);
    CharacterWindow window(loadIdleSheet(), 2, &backend);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // 后端自述不支持像素掩码时不应调用，而是记录警告并继续。
    QCOMPARE(backend.calls().setHitMask, 0);
    // 窗口自己仍然算出了区域，供上层决定是否降级。
    QVERIFY(!window.hitRegion().isEmpty());
}

void TestCharacterWindow::realBackendReportsTheCapabilitiesTheProductNeeds()
{
    const std::unique_ptr<mub::platform::DeskPetWindowBackend> backend =
        mub::platform::createWindowBackend();
    QVERIFY(backend != nullptr);

    const mub::platform::BackendCapabilities caps = backend->capabilities();
    QVERIFY(!caps.name.isEmpty());
    // docs/Decisions.md 第 3.4 节与第 8.1 节把这些列为核心能力。
    QVERIFY(caps.alwaysOnTop);
    QVERIFY(caps.inputPassthrough);
    QVERIFY(caps.pixelHitMask);
    QVERIFY(caps.excludeFromWindowList);
}

void TestCharacterWindow::realBackendAppliesDeskPetFlagsBeforeShow()
{
    const std::unique_ptr<mub::platform::DeskPetWindowBackend> backend =
        mub::platform::createWindowBackend();
    CharacterWindow window(loadIdleSheet(), 1, backend.get());

    QWindow *handle = window.windowHandle();
    QVERIFY(handle != nullptr);
    QVERIFY(handle->flags().testFlag(Qt::Tool));
    QVERIFY(handle->flags().testFlag(Qt::WindowDoesNotAcceptFocus));
    QVERIFY(!window.isVisible());
}

QTEST_MAIN(TestCharacterWindow)
#include "tst_characterwindow.moc"
