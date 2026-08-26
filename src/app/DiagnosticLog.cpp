#include "app/DiagnosticLog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGlobal>

#include <cstdio>

namespace mub::app {

namespace {

// 日志上限。超过后转存为 .1 并重新开始，只保留两代。
constexpr qint64 kMaxLogBytes = 1024 * 1024;

QFile g_logFile;
QTextStream g_logStream;
QtMessageHandler g_previousHandler = nullptr;

const char *levelName(const QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return "debug";
    case QtInfoMsg:
        return "info";
    case QtWarningMsg:
        return "warning";
    case QtCriticalMsg:
        return "critical";
    case QtFatalMsg:
        return "fatal";
    }
    return "info";
}

void handler(const QtMsgType type, const QMessageLogContext &context,
             const QString &message)
{
    const QString category = context.category != nullptr
        ? QString::fromLatin1(context.category)
        : QStringLiteral("default");
    const QString line =
        QStringLiteral("%1 %2 [%3] %4")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
            .arg(QLatin1String(levelName(type)))
            .arg(category)
            .arg(message);

    const QByteArray encoded = line.toUtf8();
    std::fprintf(stderr, "%s\n", encoded.constData());
    std::fflush(stderr);

    if (g_logFile.isOpen()) {
        g_logStream << line << Qt::endl;
    }

    if (type == QtFatalMsg && g_previousHandler != nullptr) {
        // 交回给默认处理器，保持 Qt 原本的终止行为。
        g_previousHandler(type, context, message);
    }
}

void rotateIfNeeded(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || info.size() < kMaxLogBytes) {
        return;
    }
    const QString previous = path + QStringLiteral(".1");
    QFile::remove(previous);
    QFile::rename(path, previous);
}

} // namespace

void installDiagnosticLog()
{
    // AppDataLocation 依赖 organizationName 与 applicationName，
    // 因此调用方必须先执行 mub::metadata::apply()。
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!directory.isEmpty() && QDir().mkpath(directory)) {
        const QString path = QDir(directory).filePath(QStringLiteral("deskpet.log"));
        rotateIfNeeded(path);
        g_logFile.setFileName(path);
        if (g_logFile.open(QIODevice::WriteOnly | QIODevice::Append
                           | QIODevice::Text)) {
            g_logStream.setDevice(&g_logFile);
        }
    }

    g_previousHandler = qInstallMessageHandler(handler);
}

QString diagnosticLogPath()
{
    return g_logFile.isOpen() ? g_logFile.fileName() : QString();
}

} // namespace mub::app
