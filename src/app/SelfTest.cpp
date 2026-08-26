#include "app/SelfTest.h"

#include "character/CharacterAssets.h"
#include "character/SpriteSheet.h"

#include <QFile>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QString>
#include <QStringList>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcSelfTest, "mub.app.selftest")

bool checkSpriteSheets(QStringList &failures)
{
    bool ok = true;
    for (const character::SpriteSheetEntry &entry :
         character::registeredSpriteSheets()) {
        const QString path = QString::fromLatin1(entry.resourcePath);
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
        if (sheet.frameCount() != entry.expectedFrameCount) {
            failures.append(
                QStringLiteral("%1: frame count %2, expected %3")
                    .arg(path)
                    .arg(sheet.frameCount())
                    .arg(entry.expectedFrameCount));
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

} // namespace

int runSelfTest()
{
    QStringList failures;
    bool ok = true;
    ok = checkSpriteSheets(failures) && ok;
    ok = checkDialogueFont(failures) && ok;

    if (ok) {
        qCInfo(lcSelfTest) << "self-test passed";
        return 0;
    }

    for (const QString &failure : failures) {
        qCCritical(lcSelfTest).noquote() << QStringLiteral("self-test failure: %1").arg(failure);
    }
    qCCritical(lcSelfTest).noquote()
        << QStringLiteral("self-test failed with %1 problem(s)").arg(failures.size());
    return 1;
}

} // namespace mub::app
