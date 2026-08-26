#include "core/GestureRecognizer.h"

#include <QTest>

using mub::core::GestureRecognizer;
using Release = mub::core::GestureRecognizer::Release;

class TestGestureRecognizer final : public QObject
{
    Q_OBJECT

private slots:
    void releaseWithoutPressReportsNothing();
    void pressAloneIsNotYetAClick();
    void smallMovementIsAClick();
    void movementBeyondThresholdIsADrag();
    void thresholdBoundary_data();
    void thresholdBoundary();
    void dragIsDetectedOnReleaseWhenNoMoveArrived();
    void dragStaysADragAfterMovingBack();
    void cancelDiscardsTheGesture();
    void dragOffsetIsRelativeToTheWindow();
    void diagonalMovementUsesTheLargerAxis();
};

void TestGestureRecognizer::releaseWithoutPressReportsNothing()
{
    GestureRecognizer gesture;
    QCOMPARE(gesture.release(QPoint(10, 10)), Release::None);
}

void TestGestureRecognizer::pressAloneIsNotYetAClick()
{
    // 计划第 10.2 节：按下时不提前计为单击。
    GestureRecognizer gesture;
    gesture.press(QPoint(100, 100));
    QVERIFY(gesture.isPressed());
    QVERIFY(!gesture.isDragging());
}

void TestGestureRecognizer::smallMovementIsAClick()
{
    GestureRecognizer gesture(4);
    gesture.press(QPoint(100, 100));
    QVERIFY(!gesture.move(QPoint(101, 102)));
    QVERIFY(!gesture.isDragging());
    QCOMPARE(gesture.release(QPoint(102, 101)), Release::Click);
    QVERIFY(!gesture.isPressed());
}

void TestGestureRecognizer::movementBeyondThresholdIsADrag()
{
    GestureRecognizer gesture(4);
    gesture.press(QPoint(100, 100));
    QVERIFY(gesture.move(QPoint(110, 100)));
    QVERIFY(gesture.isDragging());
    // 已经进入拖动后，后续移动不再是「首次跨过阈值」。
    QVERIFY(!gesture.move(QPoint(120, 100)));
    QCOMPARE(gesture.release(QPoint(120, 100)), Release::DragEnd);
}

void TestGestureRecognizer::thresholdBoundary_data()
{
    QTest::addColumn<int>("delta");
    QTest::addColumn<bool>("expectedDrag");

    QTest::newRow("below") << 3 << false;
    QTest::newRow("exactly at threshold") << 4 << true;
    QTest::newRow("above") << 5 << true;
}

void TestGestureRecognizer::thresholdBoundary()
{
    QFETCH(int, delta);
    QFETCH(bool, expectedDrag);

    GestureRecognizer gesture(4);
    gesture.press(QPoint(0, 0));
    gesture.move(QPoint(delta, 0));
    QCOMPARE(gesture.isDragging(), expectedDrag);
}

void TestGestureRecognizer::dragIsDetectedOnReleaseWhenNoMoveArrived()
{
    // 快速拖动时某些平台可能不产生中间的移动事件。
    GestureRecognizer gesture(4);
    gesture.press(QPoint(0, 0));
    QCOMPARE(gesture.release(QPoint(200, 0)), Release::DragEnd);
}

void TestGestureRecognizer::dragStaysADragAfterMovingBack()
{
    // 拖出去又拖回原点，仍然是拖动，不能变回单击。
    GestureRecognizer gesture(4);
    gesture.press(QPoint(50, 50));
    gesture.move(QPoint(200, 50));
    QCOMPARE(gesture.release(QPoint(50, 50)), Release::DragEnd);
}

void TestGestureRecognizer::cancelDiscardsTheGesture()
{
    GestureRecognizer gesture(4);
    gesture.press(QPoint(0, 0));
    gesture.move(QPoint(100, 0));
    gesture.cancel();
    QVERIFY(!gesture.isPressed());
    QVERIFY(!gesture.isDragging());
    QCOMPARE(gesture.release(QPoint(100, 0)), Release::None);
}

void TestGestureRecognizer::dragOffsetIsRelativeToTheWindow()
{
    GestureRecognizer gesture;
    gesture.press(QPoint(150, 240));
    QCOMPARE(gesture.dragOffsetFrom(QPoint(100, 200)), QPoint(50, 40));
}

void TestGestureRecognizer::diagonalMovementUsesTheLargerAxis()
{
    GestureRecognizer gesture(10);
    gesture.press(QPoint(0, 0));
    // 两轴都不足阈值，不算拖动。
    gesture.move(QPoint(9, 9));
    QVERIFY(!gesture.isDragging());
    // 任一轴达到阈值即算拖动。
    gesture.move(QPoint(9, 10));
    QVERIFY(gesture.isDragging());
}

QTEST_MAIN(TestGestureRecognizer)
#include "tst_gesture.moc"
