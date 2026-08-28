#include "app/SelfTest.h"

#include "app/SettingsController.h"
#include "character/AnimationClip.h"
#include "character/SpriteSheet.h"
#include "core/EventCoordinator.h"
#include "core/SettingsPresets.h"
#include "core/SettingsStore.h"
#include "core/TimeSource.h"
#include "dialogue/DialogueData.h"
#include "dialogue/DialogueSession.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
        if (character::frameDurationFor(entry, character::AnimationTiming{}) <= 0) {
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
    expected.behavior.mode = core::ActivityMode::Active;
    expected.dialogue.typingMsPerChar = 40;
    expected.window.alwaysOnTop = false;
    expected.appearance.scale = 1;
    store.save(expected);
    if (backend.status() != QSettings::NoError || store.load() != expected) {
        failures.append(QStringLiteral("configuration could not be saved and reloaded"));
        return false;
    }

    // 逐字段恢复：坏掉的键回到默认值，没坏的键保持用户的取值。
    backend.setValue(QStringLiteral("settings/appearance/scale"), 999);
    backend.setValue(QStringLiteral("settings/behavior/mode"), QStringLiteral("damaged"));
    backend.sync();
    const core::Settings recovered = store.load();
    const core::Settings defaults;
    if (recovered.appearance.scale != defaults.appearance.scale
        || recovered.behavior.mode != defaults.behavior.mode
        || recovered.dialogue.typingMsPerChar != expected.dialogue.typingMsPerChar) {
        failures.append(QStringLiteral("damaged configuration did not recover per field"));
        return false;
    }

    // 成对约束：最小值大于最大值时整对回到默认值（第 14.8 节）。
    backend.setValue(QStringLiteral("settings/behavior/idleMinMs"), 20000);
    backend.setValue(QStringLiteral("settings/behavior/idleMaxMs"), 1000);
    backend.sync();
    const core::Settings pairRecovered = store.load();
    if (pairRecovered.behavior.idleMinMs != defaults.behavior.idleMinMs
        || pairRecovered.behavior.idleMaxMs != defaults.behavior.idleMaxMs) {
        failures.append(QStringLiteral("invalid duration pair did not recover"));
        return false;
    }
    // schema 版本必须真的写进文件：没有它，1.0.0 的平铺键会被当成有效配置读，
    // 而不是触发默认值重建（第 14.8 节）。
    if (backend.value(QStringLiteral("settings/schemaVersion")).toInt()
        != core::SettingsStore::kSchemaVersion) {
        failures.append(QStringLiteral("saved configuration does not record schema %1")
                            .arg(core::SettingsStore::kSchemaVersion));
        return false;
    }

    // 未来 schema 只读：读到更高版本时用默认值运行，并且**不得回写**，
    // 否则新版本写的配置会被旧版本降级覆盖。
    backend.setValue(QStringLiteral("settings/schemaVersion"),
                     core::SettingsStore::kSchemaVersion + 1);
    backend.sync();
    core::SettingsStore future(backend);
    if (future.load() != core::Settings{}) {
        failures.append(QStringLiteral("a future schema did not fall back to defaults"));
        return false;
    }
    core::Settings attempted;
    attempted.appearance.scale = 3;
    future.save(attempted);
    backend.sync();
    if (backend.value(QStringLiteral("settings/schemaVersion")).toInt()
        != core::SettingsStore::kSchemaVersion + 1) {
        failures.append(QStringLiteral("a future schema was overwritten by this version"));
        return false;
    }
    qCInfo(lcSelfTest) << "configuration initialization, recovery and schema"
                       << core::SettingsStore::kSchemaVersion << "ok";
    return true;
}

// 设置控制器的装配：产品路径上唯一的运行时设置持有者。自检要证明它确实把改动
// 分发到了领域信号并落到了盘上——这两条断了，界面看起来正常但什么都不生效。
bool checkSettingsController(QStringList &failures)
{
    QTemporaryDir directory;
    if (!directory.isValid()) {
        failures.append(QStringLiteral("could not create a temporary configuration directory"));
        return false;
    }

    QSettings backend(directory.filePath(QStringLiteral("self-test-controller.ini")),
                      QSettings::IniFormat);
    core::SettingsStore store(backend);
    SettingsController controller(store);

    // 不用 QSignalSpy：那是 Qt6::Test 的设施，产品二进制不链接测试模块。
    int behaviorNotices = 0;
    int dialogueNotices = 0;
    QObject::connect(&controller, &SettingsController::behaviorChanged,
                     &controller, [&behaviorNotices] { ++behaviorNotices; });
    QObject::connect(&controller, &SettingsController::dialogueChanged,
                     &controller, [&dialogueNotices] { ++dialogueNotices; });

    core::Settings changed = controller.settings();
    core::applyActivityTempo(changed.behavior, core::ActivityTempo::High);
    core::applySpeechFrequency(changed.dialogue, core::SpeechFrequency::High);
    controller.applyAndPersist(changed);

    if (behaviorNotices == 0 || dialogueNotices == 0) {
        failures.append(QStringLiteral("settings controller did not notify its domains"));
        return false;
    }
    if (controller.settings() != changed) {
        failures.append(QStringLiteral("settings controller did not adopt the applied values"));
        return false;
    }
    if (store.load() != changed) {
        failures.append(QStringLiteral("settings controller did not persist the applied values"));
        return false;
    }

    // 只进运行时的路径（`--scale`）不得回写配置文件。
    core::Settings thisRunOnly = changed;
    thisRunOnly.appearance.scale = changed.appearance.scale == 1 ? 2 : 1;
    controller.applyForThisRunOnly(thisRunOnly);
    controller.flush();
    if (store.load().appearance.scale != changed.appearance.scale) {
        failures.append(QStringLiteral("a run-only override leaked into the configuration file"));
        return false;
    }

    // 档位必须能反向匹配回去，否则界面会把刚写进去的值显示成「自定义」。
    if (core::matchActivityTempo(controller.settings().behavior)
            != core::ActivityTempo::High
        || core::matchSpeechFrequency(controller.settings().dialogue)
            != core::SpeechFrequency::High) {
        failures.append(QStringLiteral("preset values do not match back to their level"));
        return false;
    }
    qCInfo(lcSelfTest) << "settings controller assembly ok";
    return true;
}

// 运行平台依赖：发行包必须带上桌面平台插件。打包自检在 offscreen 下运行，
// 因此漏掉 xcb/windows 插件不会让自检失败——除非在这里显式检查
// （第 8.2 节、计划第 10 节）。
bool checkPlatformPlugins(QStringList &failures)
{
#if defined(Q_OS_WIN)
    const QString required = QStringLiteral("qwindows.dll");
#elif defined(Q_OS_LINUX)
    const QString required = QStringLiteral("libqxcb.so");
#else
    const QString required;
#endif
    if (required.isEmpty()) {
        qCInfo(lcSelfTest) << "no desktop platform plugin requirement on this platform";
        return true;
    }

    // 必须走 Qt 自己的插件搜索路径，不能只看 QLibraryInfo::PluginsPath：
    // windeployqt 把 platforms/ 直接放在 exe 旁边，AppImage 放在 usr/plugins/ 下，
    // 开发构建又在 Qt 安装目录里。libraryPaths() 正是 Qt 加载插件时实际查的那组
    // 目录，因此这里检查的就是真实加载路径，而不是猜一个目录。
    const QStringList searchPaths = QCoreApplication::libraryPaths();
    for (const QString &searchPath : searchPaths) {
        const QString path =
            QDir(searchPath).filePath(QStringLiteral("platforms/") + required);
        if (QFileInfo::exists(path)) {
            qCInfo(lcSelfTest).noquote()
                << QStringLiteral("desktop platform plugin ok %1").arg(path);
            return true;
        }
    }
    failures.append(QStringLiteral("%1: desktop platform plugin missing from %2")
                        .arg(required, searchPaths.join(QLatin1Char(';'))));
    return false;
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
                     const bool componentsOk, const bool platformOk)
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
    if (!platformOk) {
        code |= SelfTestPlatformFailure;
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
    bool configurationOk = checkConfiguration(failures);
    configurationOk = checkSettingsController(failures) && configurationOk;
    const bool componentsOk = checkKeyComponents(failures);
    const bool platformOk = checkPlatformPlugins(failures);
    const int exitCode = selfTestExitCode(resourcesOk, fontOk, dialogueOk,
                                         configurationOk, componentsOk,
                                         platformOk);

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
