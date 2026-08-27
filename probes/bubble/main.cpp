#include "ControlWindow.h"
#include "Preview.h"

#include "core/AppMetadata.h"
#include "dialogue/DialogueData.h"

#include "BubbleRenderer.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QImage>
#include <QPainter>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    mub::metadata::apply();
    QCoreApplication::setApplicationName(QStringLiteral("mub-bubble-prototype"));

    // 固定使用打包内的 Ark Pixel，不依赖系统字体。
    const int fontId =
        QFontDatabase::addApplicationFont(QStringLiteral(MUB_ARK_PIXEL_RESOURCE));
    if (fontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QApplication::setFont(QFont(families.constFirst()));
        }
    }

    QCommandLineParser parser;
    parser.addHelpOption();
    // 无头渲染一张对照图，便于在没有桌面的环境下检查布局，也便于附图讨论。
    const QCommandLineOption renderOption(
        QStringLiteral("render"),
        QCoreApplication::translate("main", "把当前默认参数下的各页渲染成 PNG 后退出。"),
        QCoreApplication::translate("main", "输出目录"));
    const QCommandLineOption dialogueOption(
        QStringLiteral("dialogue"),
        QCoreApplication::translate("main", "要渲染的对话标识。"),
        QCoreApplication::translate("main", "标识"),
        QStringLiteral("icecream-drop"));
    parser.addOption(renderOption);
    parser.addOption(dialogueOption);
    parser.process(application);

    if (parser.isSet(renderOption)) {
        const mub::dialogue::Dialogue *entry =
            mub::dialogue::findDialogue(parser.value(dialogueOption));
        if (entry == nullptr) {
            return 2;
        }
        mub::bubbleprobe::BubbleRenderer renderer;
        int index = 0;
        for (const mub::dialogue::DialoguePage &page : entry->pages) {
            const QImage face(mub::dialogue::faceResourcePath(
                QString::fromLatin1(page.faceId)));
            renderer.setContent(
                face, QString::fromUtf8(reinterpret_cast<const char *>(page.text)));
            renderer.setPageIndicator(
                entry->pages.size() > 1
                    ? QStringLiteral("%1/%2").arg(index + 1).arg(entry->pages.size())
                    : QString());

            QImage canvas(renderer.panelSize(), QImage::Format_ARGB32_Premultiplied);
            // 用中灰底衬出半透明面板，纯透明背景看不出实际观感。
            canvas.fill(QColor(90, 95, 105));
            QPainter painter(&canvas);
            renderer.paint(painter);
            painter.end();

            const QString path = QStringLiteral("%1/%2-p%3.png")
                                     .arg(parser.value(renderOption),
                                          QLatin1String(entry->id))
                                     .arg(++index);
            if (!canvas.save(path)) {
                return 3;
            }
        }
        return 0;
    }

    mub::bubbleprobe::ControlWindow controls;
    mub::bubbleprobe::PreviewController preview;

    QObject::connect(&controls, &mub::bubbleprobe::ControlWindow::parametersChanged,
                     &preview, &mub::bubbleprobe::PreviewController::setParameters);
    QObject::connect(&controls, &mub::bubbleprobe::ControlWindow::dialogueRequested,
                     &preview, &mub::bubbleprobe::PreviewController::playDialogue);
    QObject::connect(&preview, &mub::bubbleprobe::PreviewController::pageStatusChanged,
                     &controls, &mub::bubbleprobe::ControlWindow::setPageStatus);

    preview.show();
    preview.setParameters(controls.parameters());
    // 默认播四页的冰淇淋掉落，一上来就能看到翻页与表情切换。
    preview.playDialogue(QStringLiteral("icecream-drop"));

    controls.show();
    return application.exec();
}
