#include "app/StartupFailureReport.h"

#include "app/UiLanguage.h"
#include "core/AppMetadata.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QString>
#include <QTranslator>

#include <cstdio>
#include <memory>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcStartup, "mub.app.startup")

constexpr int kFailureExitCode = 3;

// 只有确实存在 Wayland 会话、并且能加载 Wayland 平台插件时才尝试弹窗。
// 否则 Qt 会在构造 QApplication 时 qFatal 终止进程，
// 那样连一条完整的错误说明都留不下。
bool waylandDialogLooksPossible()
{
    if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        return false;
    }
    // 构造应用对象之前调用 libraryPaths() 是安全的：构造时 Qt 会重新计算一次。
    return hasPlatformPlugin(QCoreApplication::libraryPaths(), QStringLiteral("wayland"));
}

QString translatedReason(const char *reason)
{
    return reason != nullptr ? QCoreApplication::translate("mub::platform", reason)
                             : QString();
}

void writeToStandardError(const QString &reason, const QString &detail)
{
    const QString text = QStringLiteral("%1\n\n%2\n\n%3")
                             .arg(metadata::displayName(), reason, detail);
    std::fprintf(stderr, "%s\n", text.toUtf8().constData());
    std::fflush(stderr);
}

} // namespace

bool hasPlatformPlugin(const QStringList &libraryPaths, const QString &pluginKey)
{
    const QString pattern = QStringLiteral("*%1*").arg(pluginKey);
    for (const QString &path : libraryPaths) {
        const QDir platforms(QDir(path).filePath(QStringLiteral("platforms")));
        if (!platforms.entryList({pattern}, QDir::Files).isEmpty()) {
            return true;
        }
    }
    return false;
}

int reportStartupFailure(int argc, char *argv[], const char *reason,
                         const QString &detail)
{
    // 技术细节先进日志与 stderr：之后构造应用对象仍可能失败。
    qCCritical(lcStartup).noquote()
        << QStringLiteral("window backend unavailable: %1").arg(detail);

    if (!waylandDialogLooksPossible()) {
        // 只为翻译构造一个不加载任何平台插件的 QCoreApplication。
        QCoreApplication application(argc, argv);
        const std::unique_ptr<QTranslator> translator = installUiTranslator();
        writeToStandardError(translatedReason(reason), detail);
        qCWarning(lcStartup)
            << "no usable Wayland backend for an error dialog; reported on stderr and in the log only";
        return kFailureExitCode;
    }

    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));
    QApplication application(argc, argv);
    metadata::apply();
    const std::unique_ptr<QTranslator> translator = installUiTranslator();
    const QString text = translatedReason(reason);
    writeToStandardError(text, detail);

    QMessageBox box;
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(metadata::displayName());
    box.setText(text);
    box.setDetailedText(detail);
    box.setStandardButtons(QMessageBox::Close);
    box.exec();

    return kFailureExitCode;
}

} // namespace mub::app
