#include "app/DiagnosticLog.h"
#include "app/DiagnosticPrivacy.h"
#include "app/RotatingLogWriter.h"

#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <QStandardPaths>
#include <QtGlobal>

#include <cstdio>
#include <memory>

namespace mub::app {

namespace {

// 日志上限。超过后转存为 .1 并重新开始，只保留两代。
constexpr qint64 kMaxLogBytes = 1024 * 1024;

std::unique_ptr<RotatingLogWriter> g_logWriter;
QMutex g_logMutex;
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
    const QString redactedMessage = redactDiagnosticText(message);
    const QString category = context.category != nullptr
        ? QString::fromLatin1(context.category)
        : QStringLiteral("default");
    const QString line =
        QStringLiteral("%1 %2 [%3] %4")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
            .arg(QLatin1String(levelName(type)))
            .arg(category)
            .arg(redactedMessage);

    const QByteArray encoded = line.toUtf8();
    std::fprintf(stderr, "%s\n", encoded.constData());
    std::fflush(stderr);

    {
        const QMutexLocker locker(&g_logMutex);
        if (g_logWriter != nullptr) {
            g_logWriter->writeLine(encoded);
        }
    }

    if (type == QtFatalMsg && g_previousHandler != nullptr) {
        // 交回给默认处理器，保持 Qt 原本的终止行为。
        g_previousHandler(type, context, message);
    }
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
        auto writer = std::make_unique<RotatingLogWriter>(path, kMaxLogBytes);
        if (writer->open()) {
            g_logWriter = std::move(writer);
        }
    }

    g_previousHandler = qInstallMessageHandler(handler);
}

QString diagnosticLogPath()
{
    const QMutexLocker locker(&g_logMutex);
    return g_logWriter != nullptr ? g_logWriter->path() : QString();
}

} // namespace mub::app
