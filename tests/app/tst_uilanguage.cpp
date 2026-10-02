// 界面语言与报错通道（docs/Decisions.md 第 2 节）。

#include "app/StartupFailureReport.h"
#include "app/UiLanguage.h"
#include "core/AppMetadata.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTranslator>
#include <QXmlStreamReader>

using mub::app::hasPlatformPlugin;
using mub::app::installUiTranslator;
using mub::app::prefersChineseUi;

class TestUiLanguage final : public QObject
{
    Q_OBJECT

private slots:
    void chineseFirstMeansSourceText();
    void everythingElseMeansEnglish();
    void englishTranslationIsInstalled();
    void chineseInstallsNoTranslator();
    void everyUiTextHasAnEnglishTranslation();
    void platformPluginLookupFindsOnlyThatPlugin();
};

void TestUiLanguage::chineseFirstMeansSourceText()
{
    for (const QString &language : {QStringLiteral("zh"), QStringLiteral("zh-CN"),
                                    QStringLiteral("zh_TW"),
                                    QStringLiteral("zh-Hans-CN")}) {
        QVERIFY2(prefersChineseUi({language, QStringLiteral("en-US")}),
                 qPrintable(language));
    }
}

// AppImageHub 的测试用 C locale 启动程序，必须看到英文界面。
void TestUiLanguage::everythingElseMeansEnglish()
{
    QVERIFY(!prefersChineseUi({}));
    QVERIFY(!prefersChineseUi({QStringLiteral("C")}));
    QVERIFY(!prefersChineseUi({QStringLiteral("en-US"), QStringLiteral("zh-CN")}));
    QVERIFY(!prefersChineseUi({QStringLiteral("ja-JP")}));
    // 只是前缀相同的其他语言标签不算中文。
    QVERIFY(!prefersChineseUi({QStringLiteral("zha")}));
}

void TestUiLanguage::englishTranslationIsInstalled()
{
    const auto translator = installUiTranslator({QStringLiteral("C")});
    QVERIFY(translator != nullptr);
    QCOMPARE(mub::metadata::displayName(), QStringLiteral("MoneyUnderBed DeskPet (unofficial)"));
    QCOMPARE(QCoreApplication::translate("mub::ui::CharacterPresenter", "退出"),
             QStringLiteral("Quit"));
}

void TestUiLanguage::chineseInstallsNoTranslator()
{
    QVERIFY(installUiTranslator({QStringLiteral("zh-CN")}) == nullptr);
    QCOMPARE(mub::metadata::displayName(), QStringLiteral("《床下有罐钱》非官方桌宠"));
}

// 新增界面文本后忘了补译文，英文界面就会夹着中文。lupdate 把新文本标成
// unfinished，这里据此失败。
void TestUiLanguage::everyUiTextHasAnEnglishTranslation()
{
    const QString path = QStringLiteral(
        MUB_SOURCE_ROOT "/resources/translations/money-under-bed-deskpet_en.ts");
    QFile file(path);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));

    QXmlStreamReader xml(&file);
    int messages = 0;
    QString source;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) {
            continue;
        }
        if (xml.name() == QLatin1String("source")) {
            source = xml.readElementText();
            ++messages;
        } else if (xml.name() == QLatin1String("translation")) {
            const QString type = xml.attributes().value(QLatin1String("type")).toString();
            const QString text = xml.readElementText();
            QVERIFY2(type.isEmpty() && !text.isEmpty(),
                     qPrintable(QStringLiteral("untranslated: %1").arg(source)));
        }
    }
    QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
    QVERIFY(messages > 0);
}

void TestUiLanguage::platformPluginLookupFindsOnlyThatPlugin()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(QDir(root.path()).mkpath(QStringLiteral("platforms")));
    QFile xcb(root.filePath(QStringLiteral("platforms/libqxcb.so")));
    QVERIFY(xcb.open(QIODevice::WriteOnly));
    xcb.close();

    // 正式 AppImage 的布局：只有 xcb，没有 wayland。
    QVERIFY(!hasPlatformPlugin({root.path()}, QStringLiteral("wayland")));
    QVERIFY(hasPlatformPlugin({root.path()}, QStringLiteral("xcb")));
    QVERIFY(!hasPlatformPlugin({}, QStringLiteral("xcb")));

    QFile wayland(root.filePath(QStringLiteral("platforms/libqwayland.so")));
    QVERIFY(wayland.open(QIODevice::WriteOnly));
    wayland.close();
    QVERIFY(hasPlatformPlugin({QStringLiteral("/nonexistent"), root.path()},
                              QStringLiteral("wayland")));
}

QTEST_GUILESS_MAIN(TestUiLanguage)
#include "tst_uilanguage.moc"
