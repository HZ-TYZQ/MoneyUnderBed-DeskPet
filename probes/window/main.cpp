#include "PlatformNative.h"
#include "ProbeWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QTextStream>
#include <QTimer>

#include <cstdio>
#include <memory>

namespace {

// 探针在 Windows 上保持控制台子系统，日志直接可见。
// 同时写一份日志文件，方便项目所有者把完整输出附回结果文档。
QFile g_logFile;
QTextStream g_logStream;

void messageHandler(const QtMsgType type, const QMessageLogContext &context,
                    const QString &message)
{
    Q_UNUSED(context)

    const char *level = "info";
    switch (type) {
    case QtDebugMsg:
        level = "debug";
        break;
    case QtInfoMsg:
        level = "info";
        break;
    case QtWarningMsg:
        level = "warning";
        break;
    case QtCriticalMsg:
        level = "critical";
        break;
    case QtFatalMsg:
        level = "fatal";
        break;
    }

    const QString line = QStringLiteral("%1 level=%2 %3")
                             .arg(QDateTime::currentDateTime().toString(
                                 Qt::ISODateWithMs))
                             .arg(QLatin1String(level))
                             .arg(message);

    const QByteArray encoded = line.toUtf8();
    std::fprintf(stderr, "%s\n", encoded.constData());
    std::fflush(stderr);

    if (g_logFile.isOpen()) {
        g_logStream << line << Qt::endl;
    }
}

bool openLogFile(const QString &requestedPath, const QString &caseName)
{
    QString path = requestedPath;
    if (path.isEmpty()) {
        path = QDir::current().filePath(
            QStringLiteral("deskpet-probe-%1.log").arg(caseName));
    }

    g_logFile.setFileName(path);
    if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate
                        | QIODevice::Text)) {
        // 当前目录不可写时退到临时目录，不因为日志失败而放弃测试。
        g_logFile.setFileName(QDir::temp().filePath(
            QStringLiteral("deskpet-probe-%1.log").arg(caseName)));
        if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate
                            | QIODevice::Text)) {
            return false;
        }
    }
    g_logStream.setDevice(&g_logFile);
    return true;
}

QString rectText(const QRect &rect)
{
    return QStringLiteral("%1,%2,%3,%4")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height());
}

// 默认素材是仓库内的 assets/character/idle-down-left.png。
// 打包后该文件与可执行文件一起分发，因此先在可执行文件目录查找，
// 再回退到从仓库根运行的相对路径。不使用被 Git 忽略的 Reference/。
QString resolveDefaultSprite()
{
    const QString relative =
        QStringLiteral("assets/character/idle-down-left.png");
    const QDir applicationDir(QCoreApplication::applicationDirPath());

    const QStringList candidates{
        applicationDir.filePath(relative),
        applicationDir.filePath(QStringLiteral("idle-down-left.png")),
        applicationDir.filePath(QStringLiteral("../") + relative),
        QDir::current().filePath(relative),
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }
    return QString();
}

void logEnvironment()
{
    qInfo().noquote()
        << QStringLiteral("probe.event=environment qt_version=%1 platform=%2 native_platform=%3 xdg_session_type=%4 display=%5 wayland_display=%6")
               .arg(QString::fromLatin1(qVersion()))
               .arg(QGuiApplication::platformName())
               .arg(platform_native::platformName())
               .arg(qEnvironmentVariable("XDG_SESSION_TYPE",
                                         QStringLiteral("<unset>")))
               .arg(qEnvironmentVariable("DISPLAY", QStringLiteral("<unset>")))
               .arg(qEnvironmentVariable("WAYLAND_DISPLAY",
                                         QStringLiteral("<unset>")));

    const auto screens = QGuiApplication::screens();
    for (int index = 0; index < screens.size(); ++index) {
        const QScreen *screen = screens.at(index);
        qInfo().noquote()
            << QStringLiteral("probe.event=screen index=%1 name=%2 geometry=%3 available=%4 dpr=%5 logical_dpi=%6 physical_dpi=%7 refresh_hz=%8")
                   .arg(index)
                   .arg(screen->name())
                   .arg(rectText(screen->geometry()))
                   .arg(rectText(screen->availableGeometry()))
                   .arg(screen->devicePixelRatio(), 0, 'f', 2)
                   .arg(screen->logicalDotsPerInch(), 0, 'f', 2)
                   .arg(screen->physicalDotsPerInch(), 0, 'f', 2)
                   .arg(screen->refreshRate(), 0, 'f', 2);
    }
}

bool caseNeedsSprite(const ProbeCase probeCase)
{
    return probeCase != ProbeCase::Build && probeCase != ProbeCase::Screen;
}

bool caseNeedsClickTarget(const ProbeCase probeCase)
{
    return probeCase == ProbeCase::HitTest
        || probeCase == ProbeCase::PassthroughQt
        || probeCase == ProbeCase::PassthroughNative;
}

} // namespace

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);

    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("deskpet-probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2"));
    application.setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "MoneyUnderBed cross-platform window probe. Feasibility code only; "
        "it does not share source with the product."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption caseOption(
        {QStringLiteral("c"), QStringLiteral("case")},
        QStringLiteral("Probe case. Use --list-cases for the full list."),
        QStringLiteral("name"), QStringLiteral("screen"));
    const QCommandLineOption listCasesOption(
        QStringLiteral("list-cases"),
        QStringLiteral("Print every probe case name and exit."));
    const QCommandLineOption spriteOption(
        {QStringLiteral("s"), QStringLiteral("sprite")},
        QStringLiteral("Sprite-sheet PNG path. Defaults to the bundled "
                       "assets/character/idle-down-left.png."),
        QStringLiteral("path"));
    const QCommandLineOption scaleOption(
        QStringLiteral("scale"), QStringLiteral("Integer pixel scale, 1 to 8."),
        QStringLiteral("factor"), QStringLiteral("2"));
    const QCommandLineOption durationOption(
        {QStringLiteral("d"), QStringLiteral("duration")},
        QStringLiteral("Automatic exit delay in seconds; zero runs until "
                       "closed."),
        QStringLiteral("seconds"), QStringLiteral("15"));
    const QCommandLineOption logOption(
        {QStringLiteral("l"), QStringLiteral("log")},
        QStringLiteral("Log file path. Defaults to "
                       "./deskpet-probe-<case>.log."),
        QStringLiteral("path"));

    parser.addOption(caseOption);
    parser.addOption(listCasesOption);
    parser.addOption(spriteOption);
    parser.addOption(scaleOption);
    parser.addOption(durationOption);
    parser.addOption(logOption);
    parser.process(application);

    if (parser.isSet(listCasesOption)) {
        const QByteArray encoded =
            knownProbeCaseNames().join(QLatin1Char('\n')).toUtf8();
        std::fprintf(stdout, "%s\n", encoded.constData());
        return 0;
    }

    const QString caseText = parser.value(caseOption).trimmed().toLower();
    ProbeCase probeCase = ProbeCase::Screen;
    if (!parseProbeCase(caseText, &probeCase)) {
        qCritical().noquote()
            << QStringLiteral("probe.error=unknown_case value=%1 known=%2")
                   .arg(caseText)
                   .arg(knownProbeCaseNames().join(QLatin1Char(',')));
        return 2;
    }

    if (!openLogFile(parser.value(logOption), caseText)) {
        std::fprintf(stderr, "level=warning probe.warning=log_file_unavailable\n");
    } else {
        std::fprintf(stderr, "level=info probe.event=log_file path=%s\n",
                     g_logFile.fileName().toUtf8().constData());
    }

    bool scaleOk = false;
    const int integerScale = parser.value(scaleOption).toInt(&scaleOk);
    if (!scaleOk || integerScale <= 0 || integerScale > 8) {
        qCritical().noquote()
            << QStringLiteral("probe.error=invalid_scale value=%1 expected=1..8")
                   .arg(parser.value(scaleOption));
        return 2;
    }

    bool durationOk = false;
    const int durationSeconds = parser.value(durationOption).toInt(&durationOk);
    if (!durationOk || durationSeconds < 0) {
        qCritical().noquote()
            << QStringLiteral("probe.error=invalid_duration value=%1")
                   .arg(parser.value(durationOption));
        return 2;
    }

    logEnvironment();

    if (probeCase == ProbeCase::Build) {
        qInfo().noquote() << "probe.event=build_ok";
        return 0;
    }
    if (probeCase == ProbeCase::Screen) {
        qInfo().noquote() << "probe.event=screen_ok";
        return 0;
    }

    QString spritePath = parser.value(spriteOption);
    if (spritePath.isEmpty()) {
        spritePath = resolveDefaultSprite();
    }
    if (spritePath.isEmpty() && caseNeedsSprite(probeCase)) {
        qCritical().noquote()
            << "probe.error=missing_sprite hint=default_asset_not_found_pass_--sprite_<path>";
        return 3;
    }

    QPixmap spriteSheet(spritePath);
    if (spriteSheet.isNull()) {
        qCritical().noquote()
            << QStringLiteral("probe.error=sprite_load_failed path=%1")
                   .arg(spritePath);
        return 3;
    }
    if (spriteSheet.height() != ProbeWindow::FrameHeight
        || spriteSheet.width() < ProbeWindow::FrameWidth
        || (spriteSheet.width() % ProbeWindow::FrameWidth) != 0) {
        qCritical().noquote()
            << QStringLiteral("probe.error=invalid_sprite_dimensions path=%1 size=%2x%3 expected_height=%4 expected_width_multiple=%5")
                   .arg(spritePath)
                   .arg(spriteSheet.width())
                   .arg(spriteSheet.height())
                   .arg(ProbeWindow::FrameHeight)
                   .arg(ProbeWindow::FrameWidth);
        return 3;
    }

    qInfo().noquote()
        << QStringLiteral("probe.event=sprite_loaded path=%1 size=%2x%3 frames=%4 scale=%5")
               .arg(spritePath)
               .arg(spriteSheet.width())
               .arg(spriteSheet.height())
               .arg(spriteSheet.width() / ProbeWindow::FrameWidth)
               .arg(integerScale);

    std::unique_ptr<ClickTargetWindow> clickTarget;
    if (caseNeedsClickTarget(probeCase)) {
        clickTarget = std::make_unique<ClickTargetWindow>();
    }

    auto probeWindow = std::make_unique<ProbeWindow>(
        probeCase, std::move(spriteSheet), integerScale);

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        qCritical().noquote() << "probe.error=no_primary_screen";
        return 4;
    }

    const QRect available = screen->availableGeometry();
    if (clickTarget) {
        const QPoint targetPosition(
            available.center().x() - clickTarget->width() / 2,
            available.center().y() - clickTarget->height() / 2);
        clickTarget->move(targetPosition);
        clickTarget->show();

        probeWindow->move(
            targetPosition.x() + (clickTarget->width() - probeWindow->width()) / 2,
            targetPosition.y() + (clickTarget->height() - probeWindow->height()) / 2);
    } else {
        probeWindow->move(available.center().x() - probeWindow->width() / 2,
                          available.center().y() - probeWindow->height() / 2);
    }

    probeWindow->show();
    probeWindow->raise();

    QTimer::singleShot(250, probeWindow.get(),
                       [&probeWindow] { probeWindow->startProbe(); });

    QObject::connect(&application, &QCoreApplication::aboutToQuit,
                     [&probeWindow, &clickTarget] {
                         probeWindow->reportSummary();
                         if (clickTarget) {
                             qInfo().noquote()
                                 << QStringLiteral("probe.event=click_target_summary clicks=%1")
                                        .arg(clickTarget->clickCount());
                         }
                         qInfo().noquote() << "probe.event=exit";
                     });

    if (durationSeconds > 0) {
        QTimer::singleShot(durationSeconds * 1000, &application,
                           &QCoreApplication::quit);
        qInfo().noquote()
            << QStringLiteral("probe.event=auto_exit_scheduled seconds=%1")
                   .arg(durationSeconds);
    }

    return application.exec();
}
