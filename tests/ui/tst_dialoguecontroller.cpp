#include "FakeWindowBackend.h"

#include "character/SpriteSheet.h"
#include "core/EventCoordinator.h"
#include "core/RandomSource.h"
#include "core/TimeSource.h"
#include "dialogue/DialogueData.h"
#include "dialogue/DialogueSession.h"
#include "ui/CharacterPresenter.h"
#include "ui/CharacterWindow.h"
#include "ui/DialogueController.h"

#include <QTest>

using mub::character::SpriteSheet;
using mub::core::EventDecision;
using mub::core::EventKind;
using mub::core::ManualTimeSource;
using mub::core::ScriptedRandomSource;
using mub::dialogue::DialogueState;
using mub::testing::FakeWindowBackend;
using mub::ui::CharacterPresenter;
using mub::ui::CharacterWindow;
using mub::ui::DialogueController;

namespace {

constexpr auto kDropDialogue = "icecream-drop";

SpriteSheet loadIdleSheet()
{
    return SpriteSheet::load(QStringLiteral(":/assets/character/idle-down-left.png"));
}

// 一整套产品对象。测试用假后端，窗口不会真的出现在桌面上。
struct Fixture
{
    Fixture()
        : random(QList<int>{0}, QList<double>{0.5})
        , window(loadIdleSheet(), 1, &backend)
        , presenter(window, clock, random)
        , controller(presenter, window, clock, random, &backend)
    {
        presenter.setBubbleHost(&controller);
    }

    // 事件已经批下来之后才轮到控制器。这是 BubbleHost 的约定，
    // 产品里由 CharacterPresenter::handOffToBubble() 保证。
    bool beginDialogue(const QString &id = QString::fromLatin1(kDropDialogue))
    {
        if (presenter.requestEvent(EventKind::Dialogue) == EventDecision::Suppressed) {
            return false;
        }
        return controller.startDialogue(id);
    }

    ManualTimeSource clock;
    ScriptedRandomSource random;
    FakeWindowBackend backend;
    CharacterWindow window;
    CharacterPresenter presenter;
    DialogueController controller;
};

} // namespace

class TestDialogueController final : public QObject
{
    Q_OBJECT

private slots:
    void startingADialogueTakesOwnershipOfTheEvent();
    void unknownDialogueIdIsRefusedAndLeavesNoEvent();
    void dialogueSuppressesAutonomousChatter();
    void clicksAdvanceThroughEveryPageThenReleaseTheEvent();
    void feedingReplacesTheDialogueWithoutClearingTheNewEvent();
    void stopEndsTheDialogueAndReleasesTheEvent();
    void idleTimeoutReleasesTheEvent();
    void continuousDialoguePausesAutonomousBehaviour();
    void singlePageBubbleDoesNotPauseAutonomousBehaviour();
    void singlePageBubbleReleasesItsEventWhenItAutoHides();
    void sessionSuspensionFreezesTheDialogue();
};

void TestDialogueController::sessionSuspensionFreezesTheDialogue()
{
    Fixture f;
    QVERIFY(f.beginDialogue());

    f.controller.setSessionSuspended(true);
    f.presenter.setSessionSuspended(true);
    const QString visible = f.controller.session().visibleText();
    f.clock.advance(mub::dialogue::DialogueSessionConfig{}.idleTimeoutMs + 1000);
    QTest::qWait(30);

    QVERIFY(f.controller.isShowing());
    QCOMPARE(f.controller.session().visibleText(), visible);
    QVERIFY(f.presenter.behavior().isPaused());

    f.controller.setSessionSuspended(false);
    f.presenter.setSessionSuspended(false);
    f.clock.advance(mub::dialogue::DialogueSessionConfig{}.typingMsPerChar);
    QTRY_VERIFY_WITH_TIMEOUT(f.controller.session().visibleText().size()
                                 > visible.size(),
                             1000);
}

void TestDialogueController::startingADialogueTakesOwnershipOfTheEvent()
{
    Fixture f;
    QVERIFY(f.beginDialogue());

    QVERIFY(f.controller.isShowing());
    QCOMPARE(f.controller.ownedEvent(), EventKind::Dialogue);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::Dialogue);
    QCOMPARE(f.controller.session().state(), DialogueState::Typing);
    QCOMPARE(f.controller.session().pageIndex(), 0);
}

void TestDialogueController::unknownDialogueIdIsRefusedAndLeavesNoEvent()
{
    Fixture f;
    QCOMPARE(f.presenter.requestEvent(EventKind::Dialogue), EventDecision::Accepted);
    QVERIFY(!f.controller.startDialogue(QStringLiteral("no-such-dialogue")));

    // 控制器没有接管，事件必须由 Presenter 结束。产品里 handOffToBubble()
    // 做这件事；这里手工补上同一步，确认协调器随后确实是空闲的。
    QCOMPARE(f.controller.ownedEvent(), EventKind::None);
    f.presenter.finishEvent(EventKind::Dialogue);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::None);
}

void TestDialogueController::dialogueSuppressesAutonomousChatter()
{
    Fixture f;
    QVERIFY(f.beginDialogue());

    // 第 4.2 节：连续对话期间抑制新的随机闲聊，被抑制者不补播。
    QCOMPARE(f.presenter.requestEvent(EventKind::AutonomousChatter),
             EventDecision::Suppressed);
    QCOMPARE(f.presenter.requestEvent(EventKind::ClickFeedback),
             EventDecision::Suppressed);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::Dialogue);
    QVERIFY(f.controller.isShowing());
}

void TestDialogueController::clicksAdvanceThroughEveryPageThenReleaseTheEvent()
{
    Fixture f;
    QVERIFY(f.beginDialogue());
    const int pages = f.controller.session().pageCount();
    QVERIFY(pages > 1);

    for (int page = 0; page < pages; ++page) {
        QCOMPARE(f.controller.session().pageIndex(), page);
        // 打字中第一次点击补全当前页。
        QVERIFY(f.controller.consumeCharacterClick());
        QCOMPARE(f.controller.session().state(), DialogueState::PageComplete);
        // 再次点击翻页；最后一页则结束对话。
        QVERIFY(f.controller.consumeCharacterClick());
    }

    QVERIFY(!f.controller.isShowing());
    QCOMPARE(f.controller.ownedEvent(), EventKind::None);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::None);
    // 对话结束后点击重新回到单击反馈路径，不再被对话消费。
    QVERIFY(!f.controller.consumeCharacterClick());
}

void TestDialogueController::feedingReplacesTheDialogueWithoutClearingTheNewEvent()
{
    Fixture f;
    QVERIFY(f.beginDialogue());

    // 第 4.2 节：用户选择投喂时结束当前对话并立即进入投喂事件。
    f.presenter.feed();

    QCOMPARE(f.presenter.coordinator().current(), EventKind::Feeding);
    QVERIFY(!f.controller.isShowing());
    // 被替换者不得再调用 finish()，否则会误清刚换上来的投喂事件。
    QCOMPARE(f.controller.ownedEvent(), EventKind::None);
    QVERIFY(!f.presenter.isDialogueActive());
}

void TestDialogueController::stopEndsTheDialogueAndReleasesTheEvent()
{
    Fixture f;
    QVERIFY(f.beginDialogue());

    // 隐藏与退出：立即结束，不保留待恢复的对话页面。
    f.controller.stop();

    QVERIFY(!f.controller.isShowing());
    QCOMPARE(f.controller.ownedEvent(), EventKind::None);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::None);
    QVERIFY(!f.presenter.isDialogueActive());
}

void TestDialogueController::idleTimeoutReleasesTheEvent()
{
    Fixture f;
    QVERIFY(f.beginDialogue());

    // 第 4.1 节：持续 20 s 没有操作时对话自动结束。
    f.clock.advance(mub::dialogue::DialogueSessionConfig{}.idleTimeoutMs + 1);
    QTRY_VERIFY_WITH_TIMEOUT(!f.controller.isShowing(), 1000);

    QCOMPARE(f.controller.ownedEvent(), EventKind::None);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::None);
}

void TestDialogueController::continuousDialoguePausesAutonomousBehaviour()
{
    Fixture f;
    QVERIFY(!f.presenter.isDialogueActive());
    QVERIFY(f.beginDialogue());

    // 第 4.1 节：显示对话期间暂停自主移动和自主行为。
    QVERIFY(f.presenter.isDialogueActive());
    QVERIFY(f.presenter.behavior().isPaused());

    f.controller.stop();
    QVERIFY(!f.presenter.isDialogueActive());
    QVERIFY(!f.presenter.behavior().isPaused());
}

void TestDialogueController::singlePageBubbleDoesNotPauseAutonomousBehaviour()
{
    Fixture f;
    QCOMPARE(f.presenter.requestEvent(EventKind::ClickFeedback), EventDecision::Accepted);
    QVERIFY(f.controller.showChatterBubble(EventKind::ClickFeedback));

    QCOMPARE(f.controller.ownedEvent(), EventKind::ClickFeedback);
    QCOMPARE(f.controller.session().pageCount(), 1);
    // 单页气泡不冻结角色：可以边走边说。
    QVERIFY(!f.presenter.isDialogueActive());
    QVERIFY(!f.presenter.behavior().isPaused());
}

void TestDialogueController::singlePageBubbleReleasesItsEventWhenItAutoHides()
{
    Fixture f;
    QCOMPARE(f.presenter.requestEvent(EventKind::AutonomousChatter),
             EventDecision::Accepted);
    QVERIFY(f.controller.showChatterBubble(EventKind::AutonomousChatter));

    // 第 4 节：普通单句气泡在**文字完成后**才开始自动消失计时。
    // 因此假时钟必须分两步推进：一次推完打字，等状态机结账，再推自动消失。
    // 一次推到底只会让打字和计时起点落在同一时刻，气泡永远不会收起。
    const mub::dialogue::DialogueSessionConfig config;
    const int typingMs = static_cast<int>(f.controller.session().fullText().size())
        * config.typingMsPerChar;

    f.clock.advance(typingMs + 1);
    QTRY_COMPARE_WITH_TIMEOUT(f.controller.session().state(),
                              DialogueState::PageComplete, 1000);
    QVERIFY(f.controller.isShowing());

    f.clock.advance(config.singlePageAutoHideMs + 1);
    QTRY_VERIFY_WITH_TIMEOUT(!f.controller.isShowing(), 1000);
    QCOMPARE(f.controller.ownedEvent(), EventKind::None);
    QCOMPARE(f.presenter.coordinator().current(), EventKind::None);
}

QTEST_MAIN(TestDialogueController)
#include "tst_dialoguecontroller.moc"
