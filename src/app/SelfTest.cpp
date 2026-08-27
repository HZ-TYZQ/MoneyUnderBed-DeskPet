#include "app/SelfTest.h"

#include "character/AnimationClip.h"
#include "character/SpriteSheet.h"
#include "core/EventCoordinator.h"
#include "core/SettingsStore.h"
#include "core/TimeSource.h"
#include "dialogue/DialogueData.h"
#include "dialogue/DialogueSession.h"

#include <QFile>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcSelfTest, "mub.app.selftest")

bool checkSpriteSheets(QStringList &failures)
{
    bool ok = true;
    for (const character::AnimationClip &entry : character::registeredClips()) {
        const QString path = character::clipAssetPath(
            QString::fromLatin1(entry.id));
        character::SpriteSheetError error = character::SpriteSheetError::None;
        const character::SpriteSheet sheet =
            character::SpriteSheet::load(path, &error);

        if (!sheet.isValid()) {
            failures.append(QStringLiteral("%1: %2")
                                .arg(path,
                                     character::describeSpriteSheetError(error)));
            ok = false;
            continue;
        }
        if (sheet.frameCount() != entry.frameCount) {
            failures.append(QStringLiteral("%1: frame count %2, expected %3")
                                .arg(path)
                                .arg(sheet.frameCount())
                                .arg(entry.frameCount));
            ok = false;
            continue;
        }
        if (entry.frameDurationMs <= 0) {
            failures.append(QStringLiteral("%1: frame duration must be positive").arg(path));
            ok = false;
            continue;
        }
        qCInfo(lcSelfTest).noquote()
            << QStringLiteral("sprite sheet ok id=%1 frames=%2")
                   .arg(QLatin1String(entry.id))
                   .arg(sheet.frameCount());
    }
    return ok;
}

bool checkFaceResources(QStringList &failures)
{
    bool ok = true;
    for (const char *faceId : dialogue::regularFaceIds()) {
        const QString path = dialogue::faceAssetPath(QString::fromLatin1(faceId));
        if (!QFile::exists(path)) {
            failures.append(QStringLiteral("%1: missing face resource").arg(path));
            ok = false;
        }
    }
    // shadow 不进入常规表情池，但按决策保留完整图标库，自检仍要防止漏包。
    const QString shadow = dialogue::faceAssetPath(QStringLiteral("shadow"));
    if (!QFile::exists(shadow)) {
        failures.append(QStringLiteral("%1: missing preserved face resource").arg(shadow));
        ok = false;
    }
    if (ok) {
        qCInfo(lcSelfTest) << "face resources ok regular="
                           << dialogue::regularFaceIds().size() << "plus shadow";
    }
    return ok;
}

bool checkDialogueFont(QStringList &failures)
{
    const QString path = QStringLiteral(MUB_ARK_PIXEL_RESOURCE);
    if (!QFile::exists(path)) {
        failures.append(QStringLiteral("%1: missing from the resource system").arg(path));
        return false;
    }
    const int id = QFontDatabase::addApplicationFont(path);
    if (id < 0) {
        failures.append(QStringLiteral("%1: could not be registered as a font").arg(path));
        return false;
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) {
        failures.append(QStringLiteral("%1: registered but reports no font family").arg(path));
        return false;
    }
    qCInfo(lcSelfTest).noquote()
        << QStringLiteral("dialogue font ok family=%1").arg(families.constFirst());
    return true;
}

bool checkDialogues(QStringList &failures)
{
    bool ok = true;
    QSet<QString> ids;
    for (const dialogue::Dialogue &entry : dialogue::registeredDialogues()) {
        const QString id = QString::fromLatin1(entry.id);
        if (id.isEmpty() || ids.contains(id)) {
            failures.append(QStringLiteral("dialogue id is empty or duplicated: %1").arg(id));
            ok = false;
        }
        ids.insert(id);
        if (entry.sourceLineCount <= 0 || entry.pages.empty()) {
            failures.append(QStringLiteral("dialogue %1 has no source line or page").arg(id));
            ok = false;
        }
        for (const dialogue::DialoguePage &page : entry.pages) {
            const QString text = QString::fromUtf8(
                reinterpret_cast<const char *>(page.text));
            const QString face = QString::fromLatin1(page.faceId);
            if (text.isEmpty() || !dialogue::isRegularFace(face)
                || !QFile::exists(dialogue::faceAssetPath(face))) {
                failures.append(QStringLiteral("dialogue %1 has invalid text or face %2")
                                    .arg(id, face));
                ok = false;
            }
        }
    }
    if (dialogue::totalSourceLineCount() != 29 || dialogue::totalPageCount() != 36) {
        failures.append(QStringLiteral("dialogue totals differ from the frozen 29 lines / 36 pages"));
        ok = false;
    }
    if (ok) {
        qCInfo(lcSelfTest) << "dialogue registry ok entries="
                           << dialogue::registeredDialogues().size();
    }
    return ok;
}

bool checkConfiguration(QStringList &failures)
{
    QTemporaryDir directory;
    if (!directory.isValid()) {
        failures.append(QStringLiteral("could not create a temporary configuration directory"));
        return false;
    }

    QSettings backend(directory.filePath(QStringLiteral("self-test.ini")),
                      QSettings::IniFormat);
    core::SettingsStore store(backend);
    if (store.load() != core::Settings{}) {
        failures.append(QStringLiteral("empty configuration did not load safe defaults"));
        return false;
    }

    core::Settings expected;
    expected.mode = core::ActivityMode::Active;
    expected.bubble = core::BubbleFrequency::Normal;
    expected.alwaysOnTop = false;
    expected.scale = 1;
    store.save(expected);
    if (backend.status() != QSettings::NoError || store.load() != expected) {
        failures.append(QStringLiteral("configuration could not be saved and reloaded"));
        return false;
    }

    backend.setValue(QStringLiteral("settings/scale"), 999);
    backend.setValue(QStringLiteral("settings/mode"), QStringLiteral("damaged"));
    backend.sync();
    const core::Settings recovered = store.load();
    if (recovered.scale != core::Settings{}.scale
        || recovered.mode != core::Settings{}.mode
        || recovered.bubble != expected.bubble) {
        failures.append(QStringLiteral("damaged configuration did not recover per field"));
        return false;
    }
    qCInfo(lcSelfTest) << "configuration initialization and recovery ok";
    return true;
}

bool checkKeyComponents(QStringList &failures)
{
    core::EventCoordinator coordinator;
    if (coordinator.request(core::EventKind::AutonomousChatter)
            != core::EventDecision::Accepted
        || coordinator.request(core::EventKind::Feeding)
            != core::EventDecision::Replaced
        || coordinator.request(core::EventKind::ClickFeedback)
            != core::EventDecision::Suppressed) {
        failures.append(QStringLiteral("event priority coordinator invariant failed"));
        return false;
    }

    const dialogue::Dialogue *entry = dialogue::findDialogue(
        QStringLiteral("icecream-drop"));
    core::ManualTimeSource clock;
    dialogue::DialogueSession session(clock);
    if (entry == nullptr) {
        failures.append(QStringLiteral("key dialogue is unavailable"));
        return false;
    }
    session.start(*entry);
    if (!session.isActive() || !session.click()
        || session.state() != dialogue::DialogueState::PageComplete) {
        failures.append(QStringLiteral("dialogue session state transition failed"));
        return false;
    }
    qCInfo(lcSelfTest) << "key component state transitions ok";
    return true;
}

} // namespace

int selfTestExitCode(const bool resourcesOk, const bool fontOk,
                     const bool dialogueOk, const bool configurationOk,
                     const bool componentsOk)
{
    int code = SelfTestSuccess;
    if (!resourcesOk) {
        code |= SelfTestResourceFailure;
    }
    if (!fontOk) {
        code |= SelfTestFontFailure;
    }
    if (!dialogueOk) {
        code |= SelfTestDialogueFailure;
    }
    if (!configurationOk) {
        code |= SelfTestConfigurationFailure;
    }
    if (!componentsOk) {
        code |= SelfTestComponentFailure;
    }
    return code;
}

int runSelfTest()
{
    QStringList failures;
    bool resourcesOk = checkSpriteSheets(failures);
    resourcesOk = checkFaceResources(failures) && resourcesOk;
    const bool fontOk = checkDialogueFont(failures);
    const bool dialogueOk = checkDialogues(failures);
    const bool configurationOk = checkConfiguration(failures);
    const bool componentsOk = checkKeyComponents(failures);
    const int exitCode = selfTestExitCode(resourcesOk, fontOk, dialogueOk,
                                         configurationOk, componentsOk);

    if (exitCode == SelfTestSuccess) {
        qCInfo(lcSelfTest) << "self-test passed";
        return 0;
    }

    for (const QString &failure : failures) {
        qCCritical(lcSelfTest).noquote() << QStringLiteral("self-test failure: %1").arg(failure);
    }
    qCCritical(lcSelfTest).noquote()
        << QStringLiteral("self-test failed with %1 problem(s)").arg(failures.size());
    return exitCode;
}

} // namespace mub::app
