#include "core/RandomSource.h"
#include "dialogue/DialogueData.h"
#include "dialogue/LineSelection.h"

#include <QSet>
#include <QString>
#include <QTest>

using mub::core::ScriptedRandomSource;
using mub::dialogue::findDialogue;
using mub::dialogue::LineTrigger;
using mub::dialogue::lineIdsFor;
using mub::dialogue::selectLineId;

namespace {

const QList<LineTrigger> kTriggers{
    LineTrigger::ClickFeedback,
    LineTrigger::AutonomousChatter,
};

} // namespace

class TestLineSelection final : public QObject
{
    Q_OBJECT

private slots:
    void everyTriggerHasLines();
    void everyIdResolvesToASinglePageDialogue();
    void noDuplicatesWithinATrigger();
    void selectionStaysInsideTheTriggerPool();
    void selectionCoversTheWholePool();
};

void TestLineSelection::everyTriggerHasLines()
{
    for (const LineTrigger trigger : kTriggers) {
        QVERIFY(!lineIdsFor(trigger).empty());
    }
}

// 单页气泡只显示一页。多页台词必须走连续对话，不能被随机气泡选中。
void TestLineSelection::everyIdResolvesToASinglePageDialogue()
{
    for (const LineTrigger trigger : kTriggers) {
        for (const char *const id : lineIdsFor(trigger)) {
            const QString name = QString::fromLatin1(id);
            const mub::dialogue::Dialogue *entry = findDialogue(name);
            QVERIFY2(entry != nullptr, qPrintable(name));
            QCOMPARE(entry->pages.size(), std::size_t{1});
            // 第 4 节：每个显示页面都必须有经过人工审核的表情。
            QVERIFY(mub::dialogue::isRegularFace(
                QString::fromLatin1(entry->pages.front().faceId)));
        }
    }
}

void TestLineSelection::noDuplicatesWithinATrigger()
{
    for (const LineTrigger trigger : kTriggers) {
        QSet<QString> seen;
        for (const char *const id : lineIdsFor(trigger)) {
            const QString name = QString::fromLatin1(id);
            QVERIFY2(!seen.contains(name), qPrintable(name));
            seen.insert(name);
        }
    }
}

void TestLineSelection::selectionStaysInsideTheTriggerPool()
{
    for (const LineTrigger trigger : kTriggers) {
        const auto ids = lineIdsFor(trigger);
        QSet<QString> pool;
        for (const char *const id : ids) {
            pool.insert(QString::fromLatin1(id));
        }

        // 越界的下标必须被夹取，不能读到池子外面去。
        ScriptedRandomSource random(QList<int>{-5, 0, 1, 999}, QList<double>{});
        for (int i = 0; i < 4; ++i) {
            QVERIFY(pool.contains(selectLineId(trigger, random)));
        }
    }
}

void TestLineSelection::selectionCoversTheWholePool()
{
    for (const LineTrigger trigger : kTriggers) {
        const auto ids = lineIdsFor(trigger);
        QList<int> indices;
        for (std::size_t i = 0; i < ids.size(); ++i) {
            indices.append(static_cast<int>(i));
        }
        ScriptedRandomSource random(indices, QList<double>{});

        for (const char *const id : ids) {
            QCOMPARE(selectLineId(trigger, random), QString::fromLatin1(id));
        }
    }
}

QTEST_MAIN(TestLineSelection)
#include "tst_lineselection.moc"
