#include "dialogue/DialogueData.h"

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QRawFont>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTest>

using namespace mub::dialogue;

namespace {

QString decisionsText()
{
    QFile file(QDir(QStringLiteral(MUB_SOURCE_ROOT))
                   .filePath(QStringLiteral("docs/Decisions.md")));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString pageText(const DialoguePage &page)
{
    return QString::fromUtf8(reinterpret_cast<const char *>(page.text));
}

QSet<uint> distinctCharacters()
{
    QSet<uint> characters;
    for (const Dialogue &dialogue : registeredDialogues()) {
        for (const DialoguePage &page : dialogue.pages) {
            for (const uint code : pageText(page).toUcs4()) {
                characters.insert(code);
            }
        }
    }
    return characters;
}

} // namespace

class TestDialogueData final : public QObject
{
    Q_OBJECT

private slots:
    void countsMatchTheDecisionRecord();
    void sourceCategoriesMatchTheDecisionRecord();
    void everyPageTextAppearsInTheDecisionRecord_data();
    void everyPageTextAppearsInTheDecisionRecord();
    void everyTriggerAppearsInTheDecisionRecord_data();
    void everyTriggerAppearsInTheDecisionRecord();
    void everyDialogueHasAtLeastOnePage_data();
    void everyDialogueHasAtLeastOnePage();
    void dialogueIdsAreUnique();
    void everyPageHasAFaceAndTheAssetExists_data();
    void everyPageHasAFaceAndTheAssetExists();
    void shadowIsNotInTheRegularFacePool();
    void regularFacePoolMatchesWhatTheDialoguesUse();
    void distinctCharacterCountMatchesTheDecisionRecord();
    void dialogueFontCoversEveryCharacter();
    void icecreamDropDialogueIsTheFourPageSequence();
    void lookupFindsEveryDialogue_data();
    void lookupFindsEveryDialogue();
    void lookupRejectsUnknownIds();
};

void TestDialogueData::countsMatchTheDecisionRecord()
{
    // docs/Decisions.md 第 4.5 节：共有 29 条来源台词、36 个显示页面。
    QCOMPARE(totalSourceLineCount(), 29);
    QCOMPARE(totalPageCount(), 36);
}

void TestDialogueData::sourceCategoriesMatchTheDecisionRecord()
{
    // 第 4.3 节要求原作台词与新增文案分组保存，逐条标记来源。
    int original = 0;
    int authored = 0;
    for (const Dialogue &dialogue : registeredDialogues()) {
        if (dialogue.source == LineSource::OriginalDemo) {
            original += dialogue.sourceLineCount;
        } else {
            authored += dialogue.sourceLineCount;
        }
    }
    QCOMPARE(original, 8);
    QCOMPARE(authored, 21);
}

void TestDialogueData::everyPageTextAppearsInTheDecisionRecord_data()
{
    QTest::addColumn<QString>("text");
    for (const Dialogue &dialogue : registeredDialogues()) {
        int index = 0;
        for (const DialoguePage &page : dialogue.pages) {
            QTest::newRow(qPrintable(
                QStringLiteral("%1-p%2").arg(QLatin1String(dialogue.id)).arg(++index)))
                << pageText(page);
        }
    }
}

void TestDialogueData::everyPageTextAppearsInTheDecisionRecord()
{
    QFETCH(QString, text);

    // 决策文档是台词的唯一来源。任何一侧改动而另一侧没跟上，这里就会失败。
    const QString decisions = decisionsText();
    QVERIFY2(!decisions.isEmpty(), "could not read docs/Decisions.md");
    QVERIFY(!text.isEmpty());
    QVERIFY2(decisions.contains(text),
             qPrintable(QStringLiteral("not found in docs/Decisions.md: %1").arg(text)));
}

void TestDialogueData::everyTriggerAppearsInTheDecisionRecord_data()
{
    QTest::addColumn<QString>("trigger");
    for (const Dialogue &dialogue : registeredDialogues()) {
        QTest::newRow(dialogue.id)
            << QString::fromUtf8(reinterpret_cast<const char *>(dialogue.trigger));
    }
}

void TestDialogueData::everyTriggerAppearsInTheDecisionRecord()
{
    QFETCH(QString, trigger);

    const QString decisions = decisionsText();
    QVERIFY(!trigger.isEmpty());
    QVERIFY2(decisions.contains(trigger),
             qPrintable(QStringLiteral("trigger not found in docs/Decisions.md: %1")
                            .arg(trigger)));
}

void TestDialogueData::everyDialogueHasAtLeastOnePage_data()
{
    QTest::addColumn<int>("pageCount");
    QTest::addColumn<int>("sourceLineCount");
    for (const Dialogue &dialogue : registeredDialogues()) {
        QTest::newRow(dialogue.id)
            << static_cast<int>(dialogue.pages.size()) << dialogue.sourceLineCount;
    }
}

void TestDialogueData::everyDialogueHasAtLeastOnePage()
{
    QFETCH(int, pageCount);
    QFETCH(int, sourceLineCount);

    QVERIFY(pageCount >= 1);
    QVERIFY(sourceLineCount >= 1);
}

void TestDialogueData::dialogueIdsAreUnique()
{
    QSet<QString> seen;
    for (const Dialogue &dialogue : registeredDialogues()) {
        const QString id = QString::fromLatin1(dialogue.id);
        QVERIFY2(!seen.contains(id), qPrintable(QStringLiteral("duplicate id: %1").arg(id)));
        seen.insert(id);
    }
    QCOMPARE(seen.size(), static_cast<qsizetype>(registeredDialogues().size()));
}

void TestDialogueData::everyPageHasAFaceAndTheAssetExists_data()
{
    QTest::addColumn<QString>("faceId");
    for (const Dialogue &dialogue : registeredDialogues()) {
        int index = 0;
        for (const DialoguePage &page : dialogue.pages) {
            QTest::newRow(qPrintable(
                QStringLiteral("%1-p%2").arg(QLatin1String(dialogue.id)).arg(++index)))
                << QString::fromLatin1(page.faceId);
        }
    }
}

void TestDialogueData::everyPageHasAFaceAndTheAssetExists()
{
    QFETCH(QString, faceId);

    // 第 4 节：所有显示台词的页面都必须同时显示经过人工审核的对应表情。
    QVERIFY(!faceId.isEmpty());
    QVERIFY2(isRegularFace(faceId),
             qPrintable(QStringLiteral("%1 is not in the regular face pool").arg(faceId)));
    QVERIFY2(QFile::exists(faceResourcePath(faceId)),
             qPrintable(QStringLiteral("missing asset: %1").arg(faceResourcePath(faceId))));
}

void TestDialogueData::shadowIsNotInTheRegularFacePool()
{
    // 第 4.6 节：shadow.png 不进入第一版常规表情池，
    // 也不参与任何随机表情选择。素材存在不代表可以自动注册为常规表情。
    QVERIFY(!isRegularFace(QStringLiteral("shadow")));
    // 素材本身仍随包分发，只是不参与选择。
    QVERIFY(QFile::exists(faceResourcePath(QStringLiteral("shadow"))));

    for (const Dialogue &dialogue : registeredDialogues()) {
        for (const DialoguePage &page : dialogue.pages) {
            QVERIFY2(QLatin1String(page.faceId) != QLatin1String("shadow"),
                     qPrintable(QStringLiteral("%1 uses shadow").arg(QLatin1String(dialogue.id))));
        }
    }
}

void TestDialogueData::regularFacePoolMatchesWhatTheDialoguesUse()
{
    // 表情池不得多出没有任何页面使用的条目，也不得少于实际用到的。
    QSet<QString> used;
    for (const Dialogue &dialogue : registeredDialogues()) {
        for (const DialoguePage &page : dialogue.pages) {
            used.insert(QString::fromLatin1(page.faceId));
        }
    }
    QSet<QString> pool;
    for (const char *face : regularFaceIds()) {
        pool.insert(QString::fromLatin1(face));
    }
    QCOMPARE(pool, used);
}

void TestDialogueData::distinctCharacterCountMatchesTheDecisionRecord()
{
    // 第 4.7 节：当前 36 个显示页面共使用 137 个不同字符。
    QCOMPARE(distinctCharacters().size(), 137);
}

void TestDialogueData::dialogueFontCoversEveryCharacter()
{
    // 第 4.7 节与计划第 11.2 节：逐字验证 36 个页面，
    // 缺字、加载失败或哈希变化时测试失败。
    const int id = QFontDatabase::addApplicationFont(QStringLiteral(MUB_ARK_PIXEL_RESOURCE));
    QVERIFY2(id >= 0, "the Ark Pixel font could not be registered");
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    QVERIFY(!families.isEmpty());

    QFont font(families.constFirst());
    font.setPixelSize(12);
    const QRawFont rawFont = QRawFont::fromFont(font);
    QVERIFY2(rawFont.isValid(), "QRawFont could not be created from the dialogue font");

    QStringList missing;
    for (const uint code : distinctCharacters()) {
        if (!rawFont.supportsCharacter(code)) {
            const char32_t codePoint = code;
            missing.append(QStringLiteral("U+%1 (%2)")
                               .arg(code, 4, 16, QLatin1Char('0'))
                               .arg(QString::fromUcs4(&codePoint, 1)));
        }
    }
    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("font is missing %1 character(s): %2")
                            .arg(missing.size())
                            .arg(missing.join(QStringLiteral(", ")))));
}

void TestDialogueData::icecreamDropDialogueIsTheFourPageSequence()
{
    // 掉落事件结束后进入的连续对话，标识与 core::FeedingSelector 对齐。
    const Dialogue *dialogue = findDialogue(QStringLiteral("icecream-drop"));
    QVERIFY(dialogue != nullptr);
    QCOMPARE(dialogue->pages.size(), static_cast<size_t>(4));
    QCOMPARE(dialogue->sourceLineCount, 4);
    QCOMPARE(dialogue->source, LineSource::OriginalDemo);
    QCOMPARE(QString::fromLatin1(dialogue->pages[0].faceId), QStringLiteral("panic"));
    QCOMPARE(QString::fromLatin1(dialogue->pages[3].faceId), QStringLiteral("impatient"));
}

void TestDialogueData::lookupFindsEveryDialogue_data()
{
    QTest::addColumn<QString>("id");
    for (const Dialogue &dialogue : registeredDialogues()) {
        QTest::newRow(dialogue.id) << QString::fromLatin1(dialogue.id);
    }
}

void TestDialogueData::lookupFindsEveryDialogue()
{
    QFETCH(QString, id);
    const Dialogue *dialogue = findDialogue(id);
    QVERIFY(dialogue != nullptr);
    QCOMPARE(QString::fromLatin1(dialogue->id), id);
}

void TestDialogueData::lookupRejectsUnknownIds()
{
    QVERIFY(findDialogue(QStringLiteral("no-such-dialogue")) == nullptr);
    QVERIFY(findDialogue(QString()) == nullptr);
}

QTEST_MAIN(TestDialogueData)
#include "tst_dialoguedata.moc"
