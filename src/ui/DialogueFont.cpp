#include "ui/DialogueFont.h"

#include <QFontDatabase>
#include <QLoggingCategory>
#include <QString>
#include <QStringList>

namespace mub::ui {

namespace {

Q_LOGGING_CATEGORY(lcFont, "mub.ui.font")

// 首次使用时注册，之后复用同一个字族名。
QString registerArkPixel()
{
    const QString path = QStringLiteral(MUB_ARK_PIXEL_RESOURCE);
    const int id = QFontDatabase::addApplicationFont(path);
    if (id < 0) {
        qCCritical(lcFont).noquote()
            << QStringLiteral("could not register the dialogue font: %1").arg(path);
        return {};
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) {
        qCCritical(lcFont).noquote()
            << QStringLiteral("%1 registered but reports no font family").arg(path);
        return {};
    }
    qCInfo(lcFont).noquote()
        << QStringLiteral("dialogue font family=%1").arg(families.constFirst());
    return families.constFirst();
}

} // namespace

QString dialogueFontFamily()
{
    static const QString family = registerArkPixel();
    return family;
}

QFont dialogueFont(const int pixelSize)
{
    QFont font;
    const QString family = dialogueFontFamily();
    if (!family.isEmpty()) {
        font.setFamily(family);
    }
    font.setPixelSize(pixelSize > 0 ? pixelSize : 1);
    // 像素字体不做字距调整，否则字形会脱离像素栅格。
    // 抗锯齿由绘制方按 BubbleMetrics 的冻结值决定，这里不预设。
    font.setKerning(false);
    return font;
}

} // namespace mub::ui
