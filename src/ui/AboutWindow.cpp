#include "ui/AboutWindow.h"

#include "core/SettingsPresets.h"

#include "core/AppMetadata.h"

#include <QClipboard>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QStringList>
#include <QSysInfo>
#include <QVBoxLayout>

#include <utility>

namespace mub::ui {

namespace {

// 第 12.3 节记录的素材来源。这些不是装饰性说明，是发行必须保留的内容。
constexpr auto kAssetAuthor = "_U5B_";
constexpr auto kAssetVideoUrl = "https://www.bilibili.com/video/BV1XwhV6TEXQ/";

QLabel *richLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    return label;
}

} // namespace

QString diagnosticsText(const QString &backendName, const bool trayAvailable,
                        const core::Settings &settings)
{
    QStringList lines;
    lines.append(QStringLiteral("version: %1").arg(metadata::versionString()));
    lines.append(QStringLiteral("qt: %1").arg(QString::fromLatin1(qVersion())));
    lines.append(QStringLiteral("system: %1 %2 (%3)")
                     .arg(QSysInfo::prettyProductName(),
                          QSysInfo::kernelVersion(),
                          QSysInfo::currentCpuArchitecture()));
    lines.append(QStringLiteral("platform: %1").arg(QGuiApplication::platformName()));
    lines.append(QStringLiteral("backend: %1").arg(backendName));
    lines.append(QStringLiteral("tray: %1")
                     .arg(trayAvailable ? QStringLiteral("available")
                                        : QStringLiteral("unavailable")));

    // 设置摘要只取解释常见报告所必需的四项，不写配置路径与用户名。
    const std::optional<core::SpeechFrequency> speech =
        core::matchSpeechFrequency(settings.dialogue);
    lines.append(
        QStringLiteral("settings: mode=%1 speech=%2 scale=%3 always_on_top=%4")
            .arg(core::activityModeId(settings.behavior.mode),
                 speech ? QString::number(static_cast<int>(*speech))
                        : QStringLiteral("custom"))
            .arg(settings.appearance.scale)
            .arg(settings.window.alwaysOnTop ? QStringLiteral("yes")
                                             : QStringLiteral("no")));

    // 屏幕信息只取几何、DPR 和刷新率，不取显示器序列号一类可识别信息。
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (qsizetype i = 0; i < screens.size(); ++i) {
        const QScreen *screen = screens.at(i);
        const QRect geometry = screen->geometry();
        const QRect available = screen->availableGeometry();
        lines.append(QStringLiteral("screen[%1]: %2x%3+%4+%5 available=%6x%7 dpr=%8 hz=%9")
                         .arg(i)
                         .arg(geometry.width())
                         .arg(geometry.height())
                         .arg(geometry.x())
                         .arg(geometry.y())
                         .arg(available.width())
                         .arg(available.height())
                         .arg(screen->devicePixelRatio())
                         .arg(screen->refreshRate()));
    }

    return lines.join(QLatin1Char('\n'));
}

AboutWindow::AboutWindow(QString backendName, const bool trayAvailable, QWidget *parent)
    : QDialog(parent)
    , backendName_(std::move(backendName))
    , trayAvailable_(trayAvailable)
{
    setObjectName(QStringLiteral("mub-about-window"));
    setWindowTitle(tr("关于"));

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(richLabel(
        QStringLiteral("<b>%1</b> %2")
            .arg(metadata::displayName().toHtmlEscaped(),
                 metadata::versionString().toHtmlEscaped()),
        this));

    // 非官方声明必须展示（第 1.2 节）。
    layout->addWidget(richLabel(metadata::unofficialNotice().toHtmlEscaped(), this));

    layout->addWidget(richLabel(
        tr("程序代码采用 GPL-3.0-or-later。项目主页：<a href=\"%1\">%1</a>")
            .arg(metadata::homepageUrl()),
        this));

    layout->addWidget(richLabel(
        tr("对话字体为 Ark Pixel（方舟像素字体），采用 SIL Open Font License 1.1。"),
        this));

    layout->addWidget(richLabel(
        tr("角色素材来自《床下有罐钱》作者 %1 发布的二创素材包，"
           "原视频：<a href=\"%2\">%2</a>")
            .arg(QString::fromLatin1(kAssetAuthor),
                 QString::fromLatin1(kAssetVideoUrl)),
        this));

    // 第 12.3、12.4 节的限制原样展示，不概括、不弱化。
    layout->addWidget(richLabel(
        tr("素材限制：仅用于二次创作，不可商用，禁止制作 R18 内容，"
           "禁止用于 AI 训练。角色素材不属于 GPL；包含这些素材的发行版本必须保持非商业。"),
        this));

    auto *buttons = new QDialogButtonBox(this);
    QPushButton *copy =
        buttons->addButton(tr("复制诊断信息"), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    connect(copy, &QPushButton::clicked, this, &AboutWindow::copyDiagnostics);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
}

void AboutWindow::setSettings(const core::Settings &settings)
{
    settings_ = settings;
}

void AboutWindow::copyDiagnostics()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr) {
        return;
    }
    clipboard->setText(diagnosticsText(backendName_, trayAvailable_, settings_));
}

} // namespace mub::ui
