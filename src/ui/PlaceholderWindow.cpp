#include "ui/PlaceholderWindow.h"

#include "core/AppMetadata.h"

#include "mub/Version.h"

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>
#include <QStringList>
#include <QtGlobal>

namespace mub::ui {

namespace {

QString backendName()
{
    return QGuiApplication::platformName();
}

// 只注册一次。paintEvent 每次重绘都会调用本函数。
QString fontStatus()
{
    static const QString status = [] {
        const int id = QFontDatabase::addApplicationFont(
            QStringLiteral(MUB_ARK_PIXEL_RESOURCE));
        if (id < 0) {
            return PlaceholderWindow::tr("对话字体加载失败");
        }
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (families.isEmpty()) {
            return PlaceholderWindow::tr("对话字体没有字族名");
        }
        return families.constFirst();
    }();
    return status;
}

} // namespace

PlaceholderWindow::PlaceholderWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(metadata::displayName());
    setMinimumSize(520, 260);
}

void PlaceholderWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor(24, 26, 32));
    painter.setPen(QColor(226, 230, 238));

    const QScreen *currentScreen = screen();
    const QStringList lines{
        metadata::displayName(),
        metadata::unofficialNotice(),
        QString(),
        tr("版本：%1").arg(metadata::versionString()),
        tr("Qt：%1（API 基线 %2）")
            .arg(QString::fromLatin1(qVersion()))
            .arg(QStringLiteral(MUB_QT_API_BASELINE)),
        tr("窗口后端：%1").arg(backendName()),
        tr("屏幕缩放：%1")
            .arg(currentScreen != nullptr ? currentScreen->devicePixelRatio()
                                          : 1.0,
                 0, 'f', 2),
        tr("对话字体：%1").arg(fontStatus()),
        QString(),
        tr("这是阶段 2 的构建验证窗口，不是角色窗口。"),
    };

    painter.drawText(rect().adjusted(24, 24, -24, -24),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     lines.join(QLatin1Char('\n')));
}

} // namespace mub::ui
