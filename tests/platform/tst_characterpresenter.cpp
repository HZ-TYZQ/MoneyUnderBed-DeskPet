#include "FakeWindowBackend.h"

#include "character/SpriteSheet.h"
#include "core/EventCoordinator.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"
#include "ui/BubbleHost.h"
#include "ui/CharacterPresenter.h"
#include "ui/CharacterWindow.h"

#include <QSignalSpy>
#include <QTest>

using mub::character::SpriteSheet;
using mub::core::EventDecision;
using mub::core::EventKind;
using mub::core::ManualTimeSource;
using mub::core::ScriptedRandomSource;
using mub::testing::FakeWindowBackend;
using mub::ui::CharacterPresenter;
using mub::ui::CharacterWindow;

namespace {

SpriteSheet loadIdleSheet()
{
    return SpriteSheet::load(QStringLiteral(":/assets/character/idle-down-left.png"));
}

class RecordingBubbleHost final : public mub::ui::BubbleHost
{
public:
    bool consumeCharacterClick() override { return false; }

    bool showChatterBubble(const EventKind kind) override
    {
        ++chatterCount;
        lastKind = kind;
        return true;
    }

    bool startDialogue(const QString &) override { return false; }

    int chatterCount = 0;
    EventKind lastKind = EventKind::None;
};

} // namespace

class TestCharacterPresenter final : public QObject
{
    Q_OBJECT

private slots:
    void droppedIcecreamDoesNotAcquireAnUnownedDialogueEvent();
    void replacingAnEventNotifiesItsOwner();
    void bubbleOffSuppressesAutonomousChatter();
    void clickChangesTheVisibleFrameWhenBubbleIsOff();
};

void TestCharacterPresenter::droppedIcecreamDoesNotAcquireAnUnownedDialogueEvent()
{
    ManualTimeSource clock;
    // feeding chance() 读取 double；0.0 确定进入 15% 的掉落分支。
    ScriptedRandomSource random(QList<int>{}, QList<double>{0.0});
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 1, &backend);
    CharacterPresenter presenter(window, clock, random);
    QSignalSpy dialogueRequests(&presenter, &CharacterPresenter::dialogueRequested);

    presenter.start();
    presenter.feed();
    QCOMPARE(presenter.coordinator().current(), EventKind::Feeding);

    // 掉落动画共 17 帧、每帧 100 ms，需要 1700 ms。推进量必须同时低于
    // AnimationPlayer::TimeJumpThresholdMs（2000 ms），否则会被当成锁屏恢复
    // 而只推进一帧，动画永远走不完。推进后等待 Presenter 的 16 ms 节拍处理。
    clock.advance(1900);
    QTRY_COMPARE_WITH_TIMEOUT(dialogueRequests.count(), 1, 250);

    QCOMPARE(dialogueRequests.constFirst().constFirst().toString(),
             QStringLiteral("icecream-drop"));
    // 当前没有正式对话控制器。触发请求不能留下无人负责完成的 Dialogue 事件。
    QCOMPARE(presenter.coordinator().current(), EventKind::None);
    QCOMPARE(presenter.requestEvent(EventKind::ClickFeedback), EventDecision::Accepted);
    presenter.stop();
}

void TestCharacterPresenter::replacingAnEventNotifiesItsOwner()
{
    ManualTimeSource clock;
    ScriptedRandomSource random(QList<int>{}, QList<double>{0.5});
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 1, &backend);
    CharacterPresenter presenter(window, clock, random);

    EventKind replaced = EventKind::None;
    connect(&presenter, &CharacterPresenter::eventReplaced, this,
            [&replaced](const EventKind kind) { replaced = kind; });

    QCOMPARE(presenter.requestEvent(EventKind::Dialogue), EventDecision::Accepted);
    QCOMPARE(presenter.requestEvent(EventKind::Feeding), EventDecision::Replaced);
    QCOMPARE(replaced, EventKind::Dialogue);
    QCOMPARE(presenter.coordinator().current(), EventKind::Feeding);

    // 旧所有者的迟到完成通知不能清除替换上来的投喂事件。
    presenter.finishEvent(EventKind::Dialogue);
    QCOMPARE(presenter.coordinator().current(), EventKind::Feeding);
    presenter.finishEvent(EventKind::Feeding);
    QCOMPARE(presenter.coordinator().current(), EventKind::None);
}

void TestCharacterPresenter::bubbleOffSuppressesAutonomousChatter()
{
    ManualTimeSource clock;
    // 待机 2000 ms；休息和接近鼠标判定均为假，因此到点后请求自主闲聊。
    ScriptedRandomSource random(QList<int>{2000}, QList<double>{0.5, 0.5});
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 1, &backend);
    CharacterPresenter presenter(window, clock, random);
    RecordingBubbleHost bubbles;
    presenter.setBubbleHost(&bubbles);
    presenter.setMode(mub::core::ActivityMode::Active);
    presenter.setBubbleFrequency(mub::core::BubbleFrequency::Off);

    // 与产品启动路径一致，先把角色放到屏幕底部，避免测试先进入返回底部状态。
    window.moveToCursorScreenBottom();
    presenter.start();
    presenter.behavior().update();
    clock.advance(2000);
    QTest::qWait(50);

    QCOMPARE(bubbles.chatterCount, 0);
    QCOMPARE(presenter.coordinator().current(), EventKind::None);
    presenter.stop();
}

void TestCharacterPresenter::clickChangesTheVisibleFrameWhenBubbleIsOff()
{
    ManualTimeSource clock;
    ScriptedRandomSource random(QList<int>{2000}, QList<double>{0.5});
    FakeWindowBackend backend;
    CharacterWindow window(loadIdleSheet(), 1, &backend);
    CharacterPresenter presenter(window, clock, random);
    presenter.setBubbleFrequency(mub::core::BubbleFrequency::Off);

    window.moveToCursorScreenBottom();
    presenter.start();
    const int before = window.frameIndex();

    emit window.clicked();

    QVERIFY(window.frameIndex() != before);
    QCOMPARE(presenter.coordinator().current(), EventKind::None);
    presenter.stop();
}

QTEST_MAIN(TestCharacterPresenter)
#include "tst_characterpresenter.moc"
