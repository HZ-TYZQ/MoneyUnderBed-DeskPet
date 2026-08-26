#include "core/ScreenPlacement.h"

#include <QTest>

using namespace mub::core;

class TestScreenPlacement final : public QObject
{
    Q_OBJECT

private slots:
    void anchorsToTheBottomOfTheAvailableArea();
    void honoursTheHorizontalRatio_data();
    void honoursTheHorizontalRatio();
    void respectsAnOffsetAvailableArea();
    void respectsTheBottomMargin();
    void clampsOutOfRangeRatios();
    void clampKeepsTheWindowInside();
    void clampToleratesAreasSmallerThanTheWindow();
    void distanceFromBottomIsZeroWhenAnchored();
    void distanceFromBottomGrowsUpwards();
};

void TestScreenPlacement::anchorsToTheBottomOfTheAvailableArea()
{
    const QRect area(0, 0, 1920, 1080);
    const QSize window(138, 222);
    const QPoint position = bottomAnchoredPosition(area, window, 0.5);
    QCOMPARE(position.y(), 1080 - 222);
}

void TestScreenPlacement::honoursTheHorizontalRatio_data()
{
    QTest::addColumn<double>("ratio");
    QTest::addColumn<int>("expectedX");

    // 可横向移动的距离是 1920 - 138 = 1782。
    QTest::newRow("left") << 0.0 << 0;
    QTest::newRow("centre") << 0.5 << 891;
    QTest::newRow("right") << 1.0 << 1782;
}

void TestScreenPlacement::honoursTheHorizontalRatio()
{
    QFETCH(double, ratio);
    QFETCH(int, expectedX);

    const QPoint position =
        bottomAnchoredPosition(QRect(0, 0, 1920, 1080), QSize(138, 222), ratio);
    QCOMPARE(position.x(), expectedX);
}

void TestScreenPlacement::respectsAnOffsetAvailableArea()
{
    // 第二块屏幕，或有面板占位的可用区域。
    const QRect area(1920, 40, 2560, 1400);
    const QSize window(138, 222);
    const QPoint position = bottomAnchoredPosition(area, window, 0.0);
    QCOMPARE(position, QPoint(1920, 40 + 1400 - 222));
}

void TestScreenPlacement::respectsTheBottomMargin()
{
    const QPoint position = bottomAnchoredPosition(QRect(0, 0, 800, 600),
                                                   QSize(100, 100), 0.0, 20);
    QCOMPARE(position.y(), 600 - 100 - 20);
}

void TestScreenPlacement::clampsOutOfRangeRatios()
{
    const QRect area(0, 0, 800, 600);
    const QSize window(100, 100);
    QCOMPARE(bottomAnchoredPosition(area, window, -5.0).x(), 0);
    QCOMPARE(bottomAnchoredPosition(area, window, 5.0).x(), 700);
}

void TestScreenPlacement::clampKeepsTheWindowInside()
{
    const QRect area(0, 0, 800, 600);
    const QSize window(100, 100);

    QCOMPARE(clampToAvailable(area, window, QPoint(-50, -50)), QPoint(0, 0));
    QCOMPARE(clampToAvailable(area, window, QPoint(5000, 5000)), QPoint(700, 500));
    QCOMPARE(clampToAvailable(area, window, QPoint(123, 45)), QPoint(123, 45));
}

void TestScreenPlacement::clampToleratesAreasSmallerThanTheWindow()
{
    // 可用区域比角色还小时，至少保证左上角对齐，不产生负向范围。
    const QRect area(10, 20, 50, 60);
    const QSize window(100, 100);
    QCOMPARE(clampToAvailable(area, window, QPoint(0, 0)), QPoint(10, 20));
    QCOMPARE(clampToAvailable(area, window, QPoint(999, 999)), QPoint(10, 20));
}

void TestScreenPlacement::distanceFromBottomIsZeroWhenAnchored()
{
    const QRect area(0, 0, 1920, 1080);
    const QSize window(138, 222);
    const QPoint anchored = bottomAnchoredPosition(area, window, 0.5);
    QCOMPARE(distanceFromBottom(area, window, anchored), 0);
}

void TestScreenPlacement::distanceFromBottomGrowsUpwards()
{
    const QRect area(0, 100, 1920, 1080);
    const QSize window(138, 222);
    const QPoint anchored = bottomAnchoredPosition(area, window, 0.5);
    QCOMPARE(distanceFromBottom(area, window, anchored - QPoint(0, 300)), 300);
}

QTEST_MAIN(TestScreenPlacement)
#include "tst_screenplacement.moc"
