#include "app/StartupFailureReport.h"

#include "core/AppMetadata.h"

#include <QApplication>
#include <QByteArray>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QString>

#include <cstdio>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcStartup, "mub.app.startup")

constexpr int kFailureExitCode = 3;

// 只有确实存在 Wayland 会话时才尝试用 Wayland 后端弹窗。
// 否则 Qt 会在构造 QApplication 时 qFatal 终止进程，
// 那样连一条完整的错误说明都留不下。
bool waylandSessionLooksAvailable()
{
    return !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
}

void writeToStandardError(const QString &reason, const QString &detail)
{
    const QString text = QStringLiteral("%1\n\n%2\n\n%3")
                             .arg(metadata::displayName(), reason, detail);
    std::fprintf(stderr, "%s\n", text.toUtf8().constData());
    std::fflush(stderr);
}

} // namespace

int reportStartupFailure(int argc, char *argv[], const QString &reason,
                         const QString &detail)
{
    qCCritical(lcStartup).noquote()
        << QStringLiteral("window backend unavailable: %1").arg(detail);
    writeToStandardError(reason, detail);

    if (!waylandSessionLooksAvailable()) {
        qCWarning(lcStartup)
            << "no Wayland session available; reported on stderr and in the log only";
        return kFailureExitCode;
    }

    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));
    QApplication application(argc, argv);
    metadata::apply();

    QMessageBox box;
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(metadata::displayName());
    box.setText(reason);
    box.setDetailedText(detail);
    box.setStandardButtons(QMessageBox::Close);
    box.exec();

    return kFailureExitCode;
}

} // namespace mub::app
