#include "core/TimeSource.h"
#include "dialogue/DialogueData.h"
#include "dialogue/DialogueSession.h"

#include <QTest>

using namespace mub::dialogue;
using mub::core::ManualTimeSource;

Q_DECLARE_METATYPE(DialogueState)

namespace {

constexpr int kMsPerChar = 25;

DialogueSessionConfig config()
{
    DialogueSessionConfig c;
    c.typingMsPerChar = kMsPerChar;
    c.idleTimeoutMs = 20000;
    c.singlePageAutoHideMs = 4000;
    return c;
}

const Dialogue &multiPage()
{
    // 冰淇淋掉落是 4 页的连续对话。
    const Dialogue *dialogue = findDialogue(QStringLiteral("icecream-drop"));
    Q_ASSERT(dialogue != nullptr);
    return *dialogue;
}

const Dialogue &singlePage()
{
    const Dialogue *dialogue = findDialogue(QStringLiteral("authored-03"));
    Q_ASSERT(dialogue != nullptr);
    return *dialogue;
}

// 一直推进到当前页打完。
void typeOutCurrentPage(ManualTimeSource &clock, DialogueSession &session)
{
    for (int step = 0; step < 500 && session.state() == DialogueState::Typing; ++step) {
        clock.advance(kMsPerChar);
        session.update();
    }
}

} // namespace

class TestDialogueSession final : public QObject
{
    Q_OBJECT

private slots:
    void startsIdle();
    void startBeginsTypingTheFirstPage();
    void textAppearsOneCharacterAtATime();
    void typingEndsAtPageComplete();
    void clickWhileTypingCompletesThePage();
    void secondClickAdvancesToTheNextPage();
    void advancingSwitchesTheFace();
    void clickOnTheLastPageEndsTheDialogue();
    void multiPageNeverAdvancesOnItsOwn();
    void idleTimeoutEndsTheDialogue();
    void interactionResetsTheIdleTimeout();
    void singlePageBubbleHidesItself();
    void multiPageDoesNotUseTheSinglePageAutoHide();
    void restartingGoesBackToTheFirstPage();
    void stopClearsEverything();
    void clicksAreIgnoredWhenNotActive();
    void everyRegisteredDialogueCanBePlayedThrough_data();
    void everyRegisteredDialogueCanBePlayedThrough();
};

void TestDialogueSession::startsIdle()
{
    ManualTimeSource clock;
    const DialogueSession session(clock, config());
    QCOMPARE(session.state(), DialogueState::Idle);
    QVERIFY(!session.isActive());
    QVERIFY(session.dialogue() == nullptr);
}

void TestDialogueSession::startBeginsTypingTheFirstPage()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());

    QCOMPARE(session.state(), DialogueState::Typing);
    QCOMPARE(session.pageIndex(), 0);
    QCOMPARE(session.pageCount(), 4);
    QVERIFY(session.visibleText().isEmpty());
    QVERIFY(!session.fullText().isEmpty());
    QVERIFY(session.isActive());
}

void TestDialogueSession::textAppearsOneCharacterAtATime()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());

    for (int expected = 1; expected <= 3; ++expected) {
        clock.advance(kMsPerChar);
        QVERIFY(session.update());
        QCOMPARE(session.visibleText().size(), expected);
        QCOMPARE(session.visibleText(), session.fullText().left(expected));
    }

    // 不足一个字符的时间不产生变化。
    clock.advance(kMsPerChar - 1);
    QVERIFY(!session.update());
    QCOMPARE(session.visibleText().size(), 3);
}

void TestDialogueSession::typingEndsAtPageComplete()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    typeOutCurrentPage(clock, session);

    QCOMPARE(session.state(), DialogueState::PageComplete);
    QCOMPARE(session.visibleText(), session.fullText());
}

void TestDialogueSession::clickWhileTypingCompletesThePage()
{
    // docs/Decisions.md 第 4.1 节：当前页仍在打字时，
    // 第一次点击立即补全当前页。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    clock.advance(kMsPerChar * 2);
    session.update();
    QCOMPARE(session.state(), DialogueState::Typing);

    QVERIFY(session.click());
    QCOMPARE(session.state(), DialogueState::PageComplete);
    QCOMPARE(session.visibleText(), session.fullText());
    // 补全没有翻页。
    QCOMPARE(session.pageIndex(), 0);
}

void TestDialogueSession::secondClickAdvancesToTheNextPage()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());

    QVERIFY(session.click());
    QCOMPARE(session.pageIndex(), 0);
    QVERIFY(session.click());
    QCOMPARE(session.pageIndex(), 1);
    QCOMPARE(session.state(), DialogueState::Typing);
    QVERIFY(session.visibleText().isEmpty());
}

void TestDialogueSession::advancingSwitchesTheFace()
{
    // 进入下一页时，同时切换到该页预先指定的表情。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());

    QCOMPARE(session.faceId(), QStringLiteral("panic"));
    session.click();
    session.click();
    QCOMPARE(session.pageIndex(), 1);
    QCOMPARE(session.faceId(), QStringLiteral("impatient"));
}

void TestDialogueSession::clickOnTheLastPageEndsTheDialogue()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());

    for (int page = 0; page < session.pageCount(); ++page) {
        session.click(); // 补全
        if (page + 1 < session.pageCount()) {
            session.click(); // 翻页
        }
    }
    QVERIFY(session.isLastPage());
    QCOMPARE(session.state(), DialogueState::PageComplete);

    QVERIFY(session.click());
    QCOMPARE(session.state(), DialogueState::Finished);
    QVERIFY(!session.isActive());
}

void TestDialogueSession::multiPageNeverAdvancesOnItsOwn()
{
    // 连续对话不自动翻页。
    ManualTimeSource clock;
    DialogueSessionConfig c = config();
    c.idleTimeoutMs = 0; // 关掉超时，单独验证「不自动翻页」
    DialogueSession session(clock, c);
    session.start(multiPage());
    typeOutCurrentPage(clock, session);

    for (int step = 0; step < 1000; ++step) {
        clock.advance(100);
        session.update();
    }
    QCOMPARE(session.pageIndex(), 0);
    QCOMPARE(session.state(), DialogueState::PageComplete);
}

void TestDialogueSession::idleTimeoutEndsTheDialogue()
{
    // 用户持续 20 s 没有操作时，对话自动结束。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    session.click();
    QCOMPARE(session.state(), DialogueState::PageComplete);

    clock.advance(19999);
    session.update();
    QCOMPARE(session.state(), DialogueState::PageComplete);

    clock.advance(1);
    QVERIFY(session.update());
    QCOMPARE(session.state(), DialogueState::Finished);
}

void TestDialogueSession::interactionResetsTheIdleTimeout()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    session.click();

    clock.advance(19000);
    session.update();
    // 一次点击翻页，超时重新计时。
    session.click();
    typeOutCurrentPage(clock, session);

    clock.advance(19000);
    session.update();
    QVERIFY(session.isActive());
}

void TestDialogueSession::singlePageBubbleHidesItself()
{
    // 普通单页气泡在文字完成后开始自动消失计时。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(singlePage());
    QCOMPARE(session.pageCount(), 1);
    typeOutCurrentPage(clock, session);
    QCOMPARE(session.state(), DialogueState::PageComplete);

    clock.advance(3999);
    session.update();
    QCOMPARE(session.state(), DialogueState::PageComplete);

    clock.advance(1);
    QVERIFY(session.update());
    QCOMPARE(session.state(), DialogueState::Finished);
}

void TestDialogueSession::multiPageDoesNotUseTheSinglePageAutoHide()
{
    // 多页对话不能因为单页自动消失的时长而提前结束。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    typeOutCurrentPage(clock, session);

    clock.advance(4001);
    session.update();
    QCOMPARE(session.state(), DialogueState::PageComplete);
    QVERIFY(session.isActive());
}

void TestDialogueSession::restartingGoesBackToTheFirstPage()
{
    // 下次重新触发同一段对话时从第一页开始。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    session.click();
    session.click();
    QCOMPARE(session.pageIndex(), 1);

    session.start(multiPage());
    QCOMPARE(session.pageIndex(), 0);
    QCOMPARE(session.state(), DialogueState::Typing);
    QVERIFY(session.visibleText().isEmpty());
}

void TestDialogueSession::stopClearsEverything()
{
    // 隐藏与退出都不保留待恢复的对话页面。
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(multiPage());
    session.click();
    session.stop();

    QCOMPARE(session.state(), DialogueState::Idle);
    QVERIFY(!session.isActive());
    QVERIFY(session.dialogue() == nullptr);
    QVERIFY(session.visibleText().isEmpty());
}

void TestDialogueSession::clicksAreIgnoredWhenNotActive()
{
    ManualTimeSource clock;
    DialogueSession session(clock, config());
    QVERIFY(!session.click());
    QVERIFY(!session.update());

    session.start(singlePage());
    session.click();
    session.click();
    QCOMPARE(session.state(), DialogueState::Finished);
    // 结束后的点击不再被对话消费，交回给上层。
    QVERIFY(!session.click());
}

void TestDialogueSession::everyRegisteredDialogueCanBePlayedThrough_data()
{
    QTest::addColumn<QString>("id");
    for (const Dialogue &dialogue : registeredDialogues()) {
        QTest::newRow(dialogue.id) << QString::fromLatin1(dialogue.id);
    }
}

void TestDialogueSession::everyRegisteredDialogueCanBePlayedThrough()
{
    QFETCH(QString, id);

    const Dialogue *dialogue = findDialogue(id);
    QVERIFY(dialogue != nullptr);

    ManualTimeSource clock;
    DialogueSession session(clock, config());
    session.start(*dialogue);

    int guard = 0;
    while (session.isActive() && guard++ < 200) {
        typeOutCurrentPage(clock, session);
        QCOMPARE(session.visibleText(), session.fullText());
        QVERIFY(!session.faceId().isEmpty());
        session.click();
    }
    QCOMPARE(session.state(), DialogueState::Finished);
    QCOMPARE(session.pageIndex(), session.pageCount() - 1);
}

QTEST_APPLESS_MAIN(TestDialogueSession)
#include "tst_dialoguesession.moc"
