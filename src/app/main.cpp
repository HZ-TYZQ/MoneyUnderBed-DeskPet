#include "core/AppMetadata.h"
#include "ui/PlaceholderWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    mub::metadata::applyTo(application);

    QCommandLineParser parser;
    parser.setApplicationDescription(mub::metadata::unofficialNotice());
    parser.addHelpOption();
    parser.addVersionOption();

    // 阶段 8 会把 --self-test 扩展为资源、字体、台词和配置初始化的完整检查。
    // 当前只验证程序能完成启动组装并正常退出。
    // 结果以退出码为准，不依赖标准输出（docs/Decisions.md 第 11.1 节）。
    const QCommandLineOption selfTestOption(
        QStringLiteral("self-test"),
        QCoreApplication::translate("main", "运行无交互自检后退出，结果以退出码为准。"));
    parser.addOption(selfTestOption);
    parser.process(application);

    if (parser.isSet(selfTestOption)) {
        return 0;
    }

    mub::ui::PlaceholderWindow window;
    window.show();

    return application.exec();
}
