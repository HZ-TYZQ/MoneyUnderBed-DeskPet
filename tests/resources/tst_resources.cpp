#include <QCryptographicHash>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTest>

// 资源完整性测试。
// 覆盖 docs/Plans/DevelopmentPlan.md 阶段 2 的自动检查：
// 字体文件哈希、素材文件存在性和许可文件存在性。
class TestResources final : public QObject
{
    Q_OBJECT

private slots:
    void embeddedFontMatchesFrozenHash();
    void fontSourceIsTheSingleCopy();
    void licenseFilesExist();
    void characterAssetsExist_data();
    void characterAssetsExist();
    void faceAssetsExist_data();
    void faceAssetsExist();
    void assetManifestCoversEveryAsset();

private:
    static QString sourceRoot();
    static QByteArray sha256Of(const QString &path);
};

QString TestResources::sourceRoot()
{
    return QStringLiteral(MUB_SOURCE_ROOT);
}

QByteArray TestResources::sha256Of(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return {};
    }
    return hash.result().toHex();
}

void TestResources::embeddedFontMatchesFrozenHash()
{
    // 编进 Qt 资源系统的那一份必须与决策冻结的哈希一致。
    // 校验资源而不是源文件，才能证明发行出去的字节没有被换过。
    const QByteArray actual = sha256Of(QStringLiteral(MUB_ARK_PIXEL_RESOURCE));
    QVERIFY2(!actual.isEmpty(),
             "Ark Pixel font is missing from the Qt resource system");
    QCOMPARE(QString::fromLatin1(actual),
             QStringLiteral(MUB_ARK_PIXEL_SHA256));
}

void TestResources::fontSourceIsTheSingleCopy()
{
    // docs/Decisions.md 第 4.7 节：不能在 Qt 资源和发行目录中重复放置同一字体。
    // 仓库里只允许存在一份 TTF。
    const QDir root(sourceRoot());
    QStringList found;
    QDirIterator iterator(root.absolutePath(), QStringList{QStringLiteral("*.ttf")},
                          QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        // 构建目录与被忽略的本地目录不计入。
        if (path.contains(QStringLiteral("/build")) ||
            path.contains(QStringLiteral("/Temp/")) ||
            path.contains(QStringLiteral("/Reference/"))) {
            continue;
        }
        found.append(root.relativeFilePath(path));
    }
    found.sort();

    const QStringList expected{
        QStringLiteral("third_party/ark-pixel-font/ark-pixel-12px-proportional-zh_cn.ttf")};
    QCOMPARE(found, expected);
}

void TestResources::licenseFilesExist()
{
    const QDir root(sourceRoot());
    const QStringList required{
        // 项目代码与文档：GPL-3.0-or-later。
        QStringLiteral("LICENSE"),
        // 角色素材：单独授权，不属于 GPL 范围。
        QStringLiteral("assets/LICENSE.md"),
        QStringLiteral("assets/MANIFEST.md"),
        // 第三方字体：OFL-1.1，保持独立许可证。
        QStringLiteral("third_party/ark-pixel-font/OFL.txt"),
        QStringLiteral("third_party/ark-pixel-font/README.md"),
    };

    for (const QString &relative : required) {
        const QString path = root.filePath(relative);
        QVERIFY2(QFileInfo::exists(path),
                 qPrintable(QStringLiteral("Missing license file: %1").arg(relative)));
        QVERIFY2(QFileInfo(path).size() > 0,
                 qPrintable(QStringLiteral("Empty license file: %1").arg(relative)));
    }
}

void TestResources::characterAssetsExist_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("frameCount");

    // 四个逻辑方向来自 docs/Decisions.md 第 7 节。
    for (const QString &direction : {QStringLiteral("up-left"),
                                     QStringLiteral("down-left"),
                                     QStringLiteral("up-right"),
                                     QStringLiteral("down-right")}) {
        QTest::newRow(qPrintable(QStringLiteral("idle-%1").arg(direction)))
            << QStringLiteral("idle-%1.png").arg(direction) << 9;
        QTest::newRow(qPrintable(QStringLiteral("run-%1").arg(direction)))
            << QStringLiteral("run-%1.png").arg(direction) << 8;
    }

    QTest::newRow("icecream-drop") << QStringLiteral("icecream-drop.png") << 17;
    QTest::newRow("icecream-drop-still")
        << QStringLiteral("icecream-drop-still.png") << 1;
    QTest::newRow("icecream-eat") << QStringLiteral("icecream-eat.png") << 20;
}

void TestResources::characterAssetsExist()
{
    QFETCH(QString, fileName);
    QFETCH(int, frameCount);

    const QString path =
        QDir(sourceRoot()).filePath(QStringLiteral("assets/character/") + fileName);
    QVERIFY2(QFileInfo::exists(path),
             qPrintable(QStringLiteral("Missing character asset: %1").arg(fileName)));

    // 只读 PNG 头，不引入 QImage，保持本测试脱离 Qt Gui。
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray header = file.read(24);
    QCOMPARE(header.size(), 24);
    QCOMPARE(header.left(8), QByteArray::fromHex("89504e470d0a1a0a"));

    const auto readBigEndian = [&header](const int offset) {
        return (static_cast<quint32>(static_cast<quint8>(header.at(offset))) << 24)
            | (static_cast<quint32>(static_cast<quint8>(header.at(offset + 1))) << 16)
            | (static_cast<quint32>(static_cast<quint8>(header.at(offset + 2))) << 8)
            | static_cast<quint32>(static_cast<quint8>(header.at(offset + 3)));
    };

    constexpr quint32 frameWidth = 69;
    constexpr quint32 frameHeight = 111;
    QCOMPARE(readBigEndian(20), frameHeight);
    QCOMPARE(readBigEndian(16), frameWidth * static_cast<quint32>(frameCount));
}

void TestResources::faceAssetsExist_data()
{
    QTest::addColumn<QString>("fileName");

    // 表情文件名由 docs/Decisions.md 第 4.4、4.5 节引用；改名必须同步这两处。
    const QStringList faces{
        QStringLiteral("natural.png"),
        QStringLiteral("natural-lower-eyes-brow.png"),
        QStringLiteral("serious-eyes-open.png"),
        QStringLiteral("serious-eyes-closed.png"),
        QStringLiteral("impatient.png"),
        QStringLiteral("happy.png"),
        QStringLiteral("proud.png"),
        QStringLiteral("proud-catmouth.png"),
        QStringLiteral("proud-thumb.png"),
        QStringLiteral("panic.png"),
        QStringLiteral("shadow.png"),
    };
    for (const QString &face : faces) {
        QTest::newRow(qPrintable(face)) << face;
    }
}

void TestResources::faceAssetsExist()
{
    QFETCH(QString, fileName);

    const QString path =
        QDir(sourceRoot()).filePath(QStringLiteral("assets/face/") + fileName);
    QVERIFY2(QFileInfo::exists(path),
             qPrintable(QStringLiteral("Missing face asset: %1").arg(fileName)));
}

void TestResources::assetManifestCoversEveryAsset()
{
    // docs/Decisions.md 第 12.6 节：assets/ 内文件都要登记在 MANIFEST 中。
    // 未登记的素材不能随仓库分发。
    QFile manifest(QDir(sourceRoot()).filePath(QStringLiteral("assets/MANIFEST.md")));
    QVERIFY(manifest.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(manifest.readAll());

    const QDir assetsDir(QDir(sourceRoot()).filePath(QStringLiteral("assets")));
    QDirIterator iterator(assetsDir.absolutePath(),
                          QStringList{QStringLiteral("*.png")}, QDir::Files,
                          QDirIterator::Subdirectories);
    int checked = 0;
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        const QString relative =
            QStringLiteral("assets/") + assetsDir.relativeFilePath(path);
        QVERIFY2(text.contains(relative),
                 qPrintable(QStringLiteral("Asset not listed in MANIFEST.md: %1")
                                .arg(relative)));
        ++checked;
    }
    QVERIFY2(checked > 0, "No assets were checked; the asset directory looks empty");
}

QTEST_MAIN(TestResources)
#include "tst_resources.moc"
