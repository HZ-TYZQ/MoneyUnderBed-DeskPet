#include "character/AnimationClip.h"
#include "character/Direction.h"

#include <QTest>

using namespace mub::character;

Q_DECLARE_METATYPE(Facing)
Q_DECLARE_METATYPE(MotionState)

class TestDirection final : public QObject
{
    Q_OBJECT

private slots:
    void quadrantsMapToTheFourSpriteDirections_data();
    void quadrantsMapToTheFourSpriteDirections();
    void spriteIdsExistInTheClipRegistry_data();
    void spriteIdsExistInTheClipRegistry();
    void stoppingKeepsTheLastFacing();
    void deadZoneBoundary_data();
    void deadZoneBoundary();
    void pureHorizontalUsesTheFrontFacing_data();
    void pureHorizontalUsesTheFrontFacing();
    void pureVerticalKeepsTheHorizontalFacing_data();
    void pureVerticalKeepsTheHorizontalFacing();
    void hysteresisPreventsFlippingNearTheThreshold();
    void hysteresisAllowsFlippingOnceClearlyExceeded();
    void facingHelpersAgreeWithTheEnum_data();
    void facingHelpersAgreeWithTheEnum();
};

void TestDirection::quadrantsMapToTheFourSpriteDirections_data()
{
    QTest::addColumn<QPointF>("velocity");
    QTest::addColumn<Facing>("expected");

    // 屏幕坐标：x 向右为正，y 向下为正（docs/Decisions.md 第 7 节）。
    QTest::newRow("dx<0 dy<0") << QPointF(-50, -50) << Facing::UpLeft;
    QTest::newRow("dx<0 dy>0") << QPointF(-50, 50) << Facing::DownLeft;
    QTest::newRow("dx>0 dy<0") << QPointF(50, -50) << Facing::UpRight;
    QTest::newRow("dx>0 dy>0") << QPointF(50, 50) << Facing::DownRight;
}

void TestDirection::quadrantsMapToTheFourSpriteDirections()
{
    QFETCH(QPointF, velocity);
    QFETCH(Facing, expected);

    DirectionResolver resolver;
    QCOMPARE(resolver.update(velocity), expected);
    QCOMPARE(resolver.motionState(), MotionState::Running);
}

void TestDirection::spriteIdsExistInTheClipRegistry_data()
{
    QTest::addColumn<MotionState>("motion");
    QTest::addColumn<Facing>("facing");

    for (const auto motion : {MotionState::Idle, MotionState::Running}) {
        for (const auto facing : {Facing::UpLeft, Facing::DownLeft,
                                  Facing::UpRight, Facing::DownRight}) {
            QTest::newRow(qPrintable(spriteIdFor(motion, facing)))
                << motion << facing;
        }
    }
}

void TestDirection::spriteIdsExistInTheClipRegistry()
{
    QFETCH(MotionState, motion);
    QFETCH(Facing, facing);

    // 八种组合都必须在登记表里有对应素材，否则运行时会找不到动画。
    const QString id = spriteIdFor(motion, facing);
    QVERIFY2(findClip(id) != nullptr, qPrintable(id));
}

void TestDirection::stoppingKeepsTheLastFacing()
{
    DirectionResolver resolver;
    resolver.update(QPointF(-50, -50));
    QCOMPARE(resolver.facing(), Facing::UpLeft);

    // 角色停止后保持最后移动方向。
    QCOMPARE(resolver.update(QPointF(0, 0)), Facing::UpLeft);
    QCOMPARE(resolver.motionState(), MotionState::Idle);
}

void TestDirection::deadZoneBoundary_data()
{
    QTest::addColumn<double>("speed");
    QTest::addColumn<MotionState>("expected");

    // 默认死区为 6.0。
    QTest::newRow("well below") << 1.0 << MotionState::Idle;
    QTest::newRow("just below") << 5.9 << MotionState::Idle;
    QTest::newRow("exactly at") << 6.0 << MotionState::Running;
    QTest::newRow("above") << 20.0 << MotionState::Running;
}

void TestDirection::deadZoneBoundary()
{
    QFETCH(double, speed);
    QFETCH(MotionState, expected);

    DirectionResolver resolver;
    resolver.update(QPointF(speed, 0));
    QCOMPARE(resolver.motionState(), expected);
}

void TestDirection::pureHorizontalUsesTheFrontFacing_data()
{
    QTest::addColumn<double>("dx");
    QTest::addColumn<Facing>("expected");

    // 纯水平移动默认使用正面方向：向左为 down-left，向右为 down-right。
    QTest::newRow("left") << -50.0 << Facing::DownLeft;
    QTest::newRow("right") << 50.0 << Facing::DownRight;
}

void TestDirection::pureHorizontalUsesTheFrontFacing()
{
    QFETCH(double, dx);
    QFETCH(Facing, expected);

    DirectionResolver resolver;
    // 先朝上，验证纯水平移动会把垂直分量拉回正面而不是沿用向上。
    resolver.update(QPointF(dx, -50));
    QVERIFY(facesUp(resolver.facing()));

    QCOMPARE(resolver.update(QPointF(dx, 0)), expected);
}

void TestDirection::pureVerticalKeepsTheHorizontalFacing_data()
{
    QTest::addColumn<Facing>("initial");
    QTest::addColumn<double>("dy");
    QTest::addColumn<Facing>("expected");

    // 纯垂直移动沿用上一次的左右朝向。
    QTest::newRow("left then up") << Facing::DownLeft << -50.0 << Facing::UpLeft;
    QTest::newRow("left then down") << Facing::UpLeft << 50.0 << Facing::DownLeft;
    QTest::newRow("right then up") << Facing::DownRight << -50.0 << Facing::UpRight;
    QTest::newRow("right then down") << Facing::UpRight << 50.0 << Facing::DownRight;
}

void TestDirection::pureVerticalKeepsTheHorizontalFacing()
{
    QFETCH(Facing, initial);
    QFETCH(double, dy);
    QFETCH(Facing, expected);

    DirectionResolver resolver;
    resolver.setFacing(initial);
    QCOMPARE(resolver.update(QPointF(0, dy)), expected);
}

void TestDirection::hysteresisPreventsFlippingNearTheThreshold()
{
    // 死区 6、滞后 4，因此改变朝向需要越过 10。
    DirectionResolver resolver(DirectionConfig{6.0, 4.0});
    resolver.update(QPointF(-50, 0));
    QVERIFY(facesLeft(resolver.facing()));

    // 反向速度越过死区但没越过滞后阈值，不应转身。
    resolver.update(QPointF(7.0, 0));
    QVERIFY2(facesLeft(resolver.facing()),
             "character flipped inside the hysteresis band");
    resolver.update(QPointF(9.9, 0));
    QVERIFY(facesLeft(resolver.facing()));
}

void TestDirection::hysteresisAllowsFlippingOnceClearlyExceeded()
{
    DirectionResolver resolver(DirectionConfig{6.0, 4.0});
    resolver.update(QPointF(-50, 0));
    QVERIFY(facesLeft(resolver.facing()));

    resolver.update(QPointF(10.0, 0));
    QVERIFY(!facesLeft(resolver.facing()));
}

void TestDirection::facingHelpersAgreeWithTheEnum_data()
{
    QTest::addColumn<Facing>("facing");
    QTest::addColumn<bool>("left");
    QTest::addColumn<bool>("up");

    QTest::newRow("up-left") << Facing::UpLeft << true << true;
    QTest::newRow("down-left") << Facing::DownLeft << true << false;
    QTest::newRow("up-right") << Facing::UpRight << false << true;
    QTest::newRow("down-right") << Facing::DownRight << false << false;
}

void TestDirection::facingHelpersAgreeWithTheEnum()
{
    QFETCH(Facing, facing);
    QFETCH(bool, left);
    QFETCH(bool, up);

    QCOMPARE(facesLeft(facing), left);
    QCOMPARE(facesUp(facing), up);
    QCOMPARE(makeFacing(left, up), facing);
    QVERIFY(!facingId(facing).isEmpty());
}

QTEST_APPLESS_MAIN(TestDirection)
#include "tst_direction.moc"
