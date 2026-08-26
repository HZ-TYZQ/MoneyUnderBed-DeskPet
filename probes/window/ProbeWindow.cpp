#include "ProbeWindow.h"

#include "PlatformNative.h"

#include <QBitmap>
#include <QCloseEvent>
#include <QColor>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QScreen>
#include <QStringList>
#include <QWindow>

#include <algorithm>
#include <array>
#include <utility>

namespace {

struct CaseEntry
{
    ProbeCase value;
    const char *name;
};

constexpr std::array<CaseEntry, 15> caseTable{{
    {ProbeCase::Build, "build"},
    {ProbeCase::Screen, "screen"},
    {ProbeCase::Dpi, "dpi"},
    {ProbeCase::Render, "render"},
    {ProbeCase::Animate, "animate"},
    {ProbeCase::Move, "move"},
    {ProbeCase::DragSystem, "drag-system"},
    {ProbeCase::DragManual, "drag-manual"},
    {ProbeCase::HitTest, "hittest"},
    {ProbeCase::PassthroughQt, "passthrough-qt"},
    {ProbeCase::PassthroughNative, "passthrough-native"},
    {ProbeCase::Topmost, "topmost"},
    {ProbeCase::Focus, "focus"},
    {ProbeCase::WindowList, "windowlist"},
    {ProbeCase::Lifecycle, "lifecycle"},
}};

QString boolText(const bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString pointText(const QPoint &point)
{
    return QStringLiteral("%1,%2").arg(point.x()).arg(point.y());
}

QString rectText(const QRect &rect)
{
    return QStringLiteral("%1,%2,%3,%4")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height());
}

// 由 alpha 通道逐行生成不透明区域。69 x 111 的精灵表足够小，
// 直接逐像素扫描即可，不需要额外的几何简化。
QRegion opaqueRegion(const QImage &image, const int scale, const int threshold)
{
    QRegion region;
    for (int y = 0; y < image.height(); ++y) {
        int runStart = -1;
        for (int x = 0; x <= image.width(); ++x) {
            const bool opaque = x < image.width()
                && qAlpha(image.pixel(x, y)) >= threshold;
            if (opaque && runStart < 0) {
                runStart = x;
            } else if (!opaque && runStart >= 0) {
                region += QRect(runStart * scale, y * scale,
                                (x - runStart) * scale, scale);
                runStart = -1;
            }
        }
    }
    return region;
}

} // namespace

bool parseProbeCase(const QString &text, ProbeCase *const probeCase)
{
    for (const CaseEntry &entry : caseTable) {
        if (text == QLatin1String(entry.name)) {
            if (probeCase != nullptr) {
                *probeCase = entry.value;
            }
            return true;
        }
    }
    return false;
}

QString probeCaseName(const ProbeCase probeCase)
{
    for (const CaseEntry &entry : caseTable) {
        if (entry.value == probeCase) {
            return QString::fromLatin1(entry.name);
        }
    }
    return QStringLiteral("<unknown>");
}

QStringList knownProbeCaseNames()
{
    QStringList names;
    names.reserve(static_cast<qsizetype>(caseTable.size()));
    for (const CaseEntry &entry : caseTable) {
        names.append(QString::fromLatin1(entry.name));
    }
    return names;
}

ProbeWindow::ProbeWindow(const ProbeCase probeCase, QPixmap spriteSheet,
                         const int integerScale, QWidget *parent)
    : QWidget(parent)
    , probeCase_(probeCase)
    , spriteSheet_(std::move(spriteSheet))
    , integerScale_(integerScale)
    , frameCount_(std::max(1, spriteSheet_.width() / FrameWidth))
{
    setObjectName(QStringLiteral("deskpet-probe-window"));
    setWindowTitle(QStringLiteral("MoneyUnderBed Window Probe"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);
    setMouseTracking(true);

    Qt::WindowFlags flags = Qt::FramelessWindowHint
        | Qt::WindowStaysOnTopHint | Qt::Tool;
    if (probeCase_ == ProbeCase::Focus) {
        // 焦点用例要验证“显示与点击都不抢焦点”，因此显式声明不接受焦点。
        flags |= Qt::WindowDoesNotAcceptFocus;
        setAttribute(Qt::WA_ShowWithoutActivating, true);
    }
    setWindowFlags(flags);
    setFixedSize(FrameWidth * integerScale_, FrameHeight * integerScale_);

    animationTimer_.setTimerType(Qt::PreciseTimer);
    animationTimer_.setInterval(100);
    connect(&animationTimer_, &QTimer::timeout, this,
            [this] { advanceAnimation(); });

    motionTimer_.setTimerType(Qt::PreciseTimer);
    motionTimer_.setInterval(30);
    connect(&motionTimer_, &QTimer::timeout, this,
            [this] { advanceMotion(); });
}

void ProbeWindow::startProbe()
{
    if (windowHandle() != nullptr) {
        initialWindowId_ = static_cast<quintptr>(winId());
    }

    qInfo().noquote()
        << QStringLiteral("probe.event=case_start case=%1 window_flags=0x%2 size=%3x%4 frames=%5 scale=%6 dpr=%7")
               .arg(probeCaseName(probeCase_))
               .arg(static_cast<qulonglong>(windowFlags()), 0, 16)
               .arg(width())
               .arg(height())
               .arg(frameCount_)
               .arg(integerScale_)
               .arg(devicePixelRatioF(), 0, 'f', 2);
    logNativeState(QStringLiteral("case_start_native"));

    switch (probeCase_) {
    case ProbeCase::Animate:
        startAnimationProbe();
        break;
    case ProbeCase::Move:
        startMotionProbe();
        break;
    case ProbeCase::Dpi:
        startDpiProbe();
        break;
    case ProbeCase::Focus:
        startFocusProbe();
        break;
    case ProbeCase::HitTest:
        applyAlphaMask();
        qInfo().noquote()
            << "probe.instruction=click_the_character_body_then_click_a_transparent_corner_inside_the_same_rectangle";
        break;
    case ProbeCase::PassthroughQt:
    case ProbeCase::PassthroughNative:
        startPassthroughProbe();
        break;
    case ProbeCase::Topmost:
        startTopmostProbe();
        break;
    case ProbeCase::DragSystem:
        qInfo().noquote()
            << "probe.instruction=press_and_drag_the_character_this_case_only_uses_startSystemMove";
        break;
    case ProbeCase::DragManual:
        qInfo().noquote()
            << "probe.instruction=press_and_drag_the_character_this_case_only_uses_manual_move";
        break;
    case ProbeCase::WindowList:
        qInfo().noquote()
            << "probe.instruction=check_the_taskbar_and_press_alt_tab_the_probe_should_not_be_listed";
        break;
    case ProbeCase::Lifecycle:
        qInfo().noquote()
            << "probe.instruction=close_the_window_or_end_the_process_then_start_the_same_case_again";
        break;
    case ProbeCase::Render:
        qInfo().noquote()
            << "probe.instruction=inspect_transparency_edges_and_nearest_neighbor_pixels";
        break;
    case ProbeCase::Build:
    case ProbeCase::Screen:
        break;
    }
}

void ProbeWindow::reportSummary() const
{
    if (measuredAnimationIntervals_ > 0) {
        const double average = static_cast<double>(animationIntervalTotalMs_)
            / static_cast<double>(measuredAnimationIntervals_);
        qInfo().noquote()
            << QStringLiteral("probe.event=animation_summary samples=%1 min_ms=%2 max_ms=%3 avg_ms=%4")
                   .arg(measuredAnimationIntervals_)
                   .arg(animationIntervalMinimumMs_)
                   .arg(animationIntervalMaximumMs_)
                   .arg(average, 0, 'f', 2);
    }

    qInfo().noquote()
        << QStringLiteral("probe.event=summary case=%1 final_pos=%2 frame_geometry=%3 pet_clicks=%4 window_rebuilds=%5")
               .arg(probeCaseName(probeCase_))
               .arg(pointText(pos()))
               .arg(rectText(frameGeometry()))
               .arg(petClickCount_)
               .arg(windowRebuildCount_);
}

void ProbeWindow::closeEvent(QCloseEvent *event)
{
    qInfo().noquote() << "probe.event=window_close";
    QWidget::closeEvent(event);
}

void ProbeWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (manualDragActive_ && (event->buttons() & Qt::LeftButton) != 0) {
        const QPoint requested = event->globalPosition().toPoint()
            - manualDragOffset_;
        move(requested);
        logPosition(QStringLiteral("manual_drag_move"), requested);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void ProbeWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    ++petClickCount_;
    const QPoint local = event->position().toPoint();
    const int sampleX = std::clamp(local.x() / integerScale_, 0,
                                   FrameWidth - 1);
    const int sampleY = std::clamp(local.y() / integerScale_, 0,
                                   FrameHeight - 1);
    const QImage frame = spriteSheet_.toImage();
    const int alpha = qAlpha(frame.pixel(frameIndex_ * FrameWidth + sampleX,
                                         sampleY));

    qInfo().noquote()
        << QStringLiteral("probe.event=pet_click count=%1 global=%2 local=%3 sprite_alpha=%4")
               .arg(petClickCount_)
               .arg(pointText(event->globalPosition().toPoint()))
               .arg(pointText(local))
               .arg(alpha);

    if (probeCase_ == ProbeCase::Focus) {
        qInfo().noquote()
            << QStringLiteral("probe.event=focus_after_click active_window=%1 foreground=%2")
                   .arg(boolText(isActiveWindow()))
                   .arg(platform_native::foregroundWindowTitle());
    }

    if (probeCase_ == ProbeCase::DragSystem) {
        // 只走系统移动。不接受时直接记为失败，不静默回退到手动移动。
        const bool accepted = windowHandle() != nullptr
            && windowHandle()->startSystemMove();
        qInfo().noquote()
            << QStringLiteral("probe.event=system_drag_requested accepted=%1 fallback=none")
                   .arg(boolText(accepted));
    } else if (probeCase_ == ProbeCase::DragManual) {
        manualDragOffset_ = event->globalPosition().toPoint()
            - frameGeometry().topLeft();
        manualDragActive_ = true;
        logPosition(QStringLiteral("manual_drag_begin"), pos());
    }

    event->accept();
}

void ProbeWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && manualDragActive_) {
        manualDragActive_ = false;
        logPosition(QStringLiteral("manual_drag_end"), pos());
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void ProbeWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    const QRect source(frameIndex_ * FrameWidth, 0, FrameWidth, FrameHeight);
    painter.drawPixmap(rect(), spriteSheet_, source);
}

void ProbeWindow::advanceAnimation()
{
    const qint64 now = animationElapsed_.elapsed();
    if (previousFrameTimestampMs_ >= 0) {
        const qint64 interval = now - previousFrameTimestampMs_;
        animationIntervalTotalMs_ += interval;
        animationIntervalMinimumMs_ = animationIntervalMinimumMs_ < 0
            ? interval
            : std::min(animationIntervalMinimumMs_, interval);
        animationIntervalMaximumMs_ = std::max(animationIntervalMaximumMs_,
                                               interval);
        ++measuredAnimationIntervals_;
    }
    previousFrameTimestampMs_ = now;

    frameIndex_ = (frameIndex_ + 1) % frameCount_;
    update();

    if (frameIndex_ == 0) {
        qInfo().noquote()
            << QStringLiteral("probe.event=animation_loop elapsed_ms=%1 samples=%2")
                   .arg(now)
                   .arg(measuredAnimationIntervals_);
    }
}

void ProbeWindow::advanceMotion()
{
    constexpr int travelDistance = 300;
    constexpr int step = 6;

    motionOffset_ += motionDirection_ * step;
    if (motionOffset_ >= travelDistance) {
        motionOffset_ = travelDistance;
        motionDirection_ = -1;
    } else if (motionOffset_ <= 0) {
        motionOffset_ = 0;
        motionDirection_ = 1;
    }

    const QPoint requested = motionOrigin_ + QPoint(motionOffset_, 0);
    move(requested);
    ++motionTicks_;

    if ((motionTicks_ % 10) == 0 || motionOffset_ == 0
        || motionOffset_ == travelDistance) {
        logPosition(QStringLiteral("motion_tick"), requested);
    }
}

void ProbeWindow::applyAlphaMask()
{
    const QImage frame = spriteSheet_.toImage()
                             .copy(frameIndex_ * FrameWidth, 0, FrameWidth,
                                   FrameHeight)
                             .convertToFormat(QImage::Format_ARGB32);
    const QRegion region = opaqueRegion(frame, integerScale_, 1);
    setMask(region);

    const QRect bounds = region.boundingRect();
    qInfo().noquote()
        << QStringLiteral("probe.event=alpha_mask_applied rects=%1 bounds=%2 window=%3x%4")
               .arg(region.rectCount())
               .arg(rectText(bounds))
               .arg(width())
               .arg(height());
}

void ProbeWindow::logNativeState(const QString &eventName) const
{
    const quintptr currentId = windowHandle() != nullptr
        ? static_cast<quintptr>(const_cast<ProbeWindow *>(this)->winId())
        : 0;
    qInfo().noquote()
        << QStringLiteral("probe.event=%1 window_id=0x%2 initial_window_id=0x%3 flags=0x%4 visible=%5 native=%6")
               .arg(eventName)
               .arg(static_cast<qulonglong>(currentId), 0, 16)
               .arg(static_cast<qulonglong>(initialWindowId_), 0, 16)
               .arg(static_cast<qulonglong>(windowFlags()), 0, 16)
               .arg(boolText(isVisible()))
               .arg(platform_native::describeWindow(this));
}

void ProbeWindow::logPosition(const QString &eventName,
                              const QPoint &requested) const
{
    qInfo().noquote()
        << QStringLiteral("probe.event=%1 requested=%2 reported=%3 frame_geometry=%4")
               .arg(eventName)
               .arg(pointText(requested))
               .arg(pointText(pos()))
               .arg(rectText(frameGeometry()));
}

void ProbeWindow::setPassthroughNative(const bool enabled)
{
    const bool applied = platform_native::setNativeInputTransparent(this,
                                                                    enabled);
    qInfo().noquote()
        << QStringLiteral("probe.event=passthrough_native enabled=%1 applied=%2 platform=%3")
               .arg(boolText(enabled))
               .arg(boolText(applied))
               .arg(platform_native::platformName());
    if (!applied) {
        qWarning().noquote()
            << "probe.warning=native_passthrough_unavailable reason=not_implemented_on_this_platform";
    }
    logNativeState(QStringLiteral("passthrough_native_state"));
}

void ProbeWindow::setPassthroughQt(const bool enabled)
{
    const quintptr before = static_cast<quintptr>(winId());
    const QPoint previousPosition = pos();

    setWindowFlag(Qt::WindowTransparentForInput, enabled);
    // 修改窗口标志后 Qt 会隐藏窗口，必须重新 show()。
    show();
    move(previousPosition);
    raise();

    const quintptr after = static_cast<quintptr>(winId());
    if (after != before) {
        ++windowRebuildCount_;
    }

    qInfo().noquote()
        << QStringLiteral("probe.event=passthrough_qt enabled=%1 window_id_before=0x%2 window_id_after=0x%3 rebuilt=%4 position=%5")
               .arg(boolText(enabled))
               .arg(static_cast<qulonglong>(before), 0, 16)
               .arg(static_cast<qulonglong>(after), 0, 16)
               .arg(boolText(after != before))
               .arg(pointText(pos()));
    logNativeState(QStringLiteral("passthrough_qt_state"));
}

void ProbeWindow::setTopmost(const bool enabled)
{
    const quintptr before = static_cast<quintptr>(winId());
    const QPoint previousPosition = pos();

    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    show();
    move(previousPosition);

    const quintptr after = static_cast<quintptr>(winId());
    if (after != before) {
        ++windowRebuildCount_;
    }

    qInfo().noquote()
        << QStringLiteral("probe.event=topmost_toggled enabled=%1 window_id_before=0x%2 window_id_after=0x%3 rebuilt=%4")
               .arg(boolText(enabled))
               .arg(static_cast<qulonglong>(before), 0, 16)
               .arg(static_cast<qulonglong>(after), 0, 16)
               .arg(boolText(after != before));
    logNativeState(QStringLiteral("topmost_state"));
}

void ProbeWindow::startAnimationProbe()
{
    animationElapsed_.start();
    previousFrameTimestampMs_ = -1;
    animationTimer_.start();
    qInfo().noquote()
        << QStringLiteral("probe.event=animation_started interval_ms=%1")
               .arg(animationTimer_.interval());
}

void ProbeWindow::startDpiProbe()
{
    const auto report = [this](const QString &reason) {
        const QScreen *screen = this->screen();
        qInfo().noquote()
            << QStringLiteral("probe.event=dpi_state reason=%1 screen=%2 dpr=%3 logical_dpi=%4 window_size=%5x%6 frame_geometry=%7")
                   .arg(reason)
                   .arg(screen != nullptr ? screen->name()
                                          : QStringLiteral("<none>"))
                   .arg(devicePixelRatioF(), 0, 'f', 2)
                   .arg(screen != nullptr ? screen->logicalDotsPerInch() : 0.0,
                        0, 'f', 2)
                   .arg(width())
                   .arg(height())
                   .arg(rectText(frameGeometry()));
    };

    report(QStringLiteral("initial"));
    connect(windowHandle(), &QWindow::screenChanged, this,
            [report](QScreen *) { report(QStringLiteral("screen_changed")); });

    qInfo().noquote()
        << "probe.instruction=change_the_system_scale_or_drag_the_probe_to_another_monitor_then_compare_the_reported_values";
}

void ProbeWindow::startFocusProbe()
{
    qInfo().noquote()
        << QStringLiteral("probe.event=focus_after_show active_window=%1 foreground=%2")
               .arg(boolText(isActiveWindow()))
               .arg(platform_native::foregroundWindowTitle());
    qInfo().noquote()
        << "probe.instruction=type_into_a_text_editor_then_click_and_drag_the_character_the_caret_must_stay_in_the_editor";
}

void ProbeWindow::startMotionProbe()
{
    motionOrigin_ = pos();
    motionOffset_ = 0;
    motionDirection_ = 1;
    motionTicks_ = 0;
    logPosition(QStringLiteral("motion_start"), motionOrigin_);
    motionTimer_.start();
}

void ProbeWindow::startPassthroughProbe()
{
    qInfo().noquote()
        << "probe.instruction=click_the_character_now_then_keep_clicking_the_same_spot_while_the_state_toggles_every_4_seconds";

    toggleTimer_.setInterval(4000);
    connect(&toggleTimer_, &QTimer::timeout, this, [this] {
        toggleState_ = !toggleState_;
        if (probeCase_ == ProbeCase::PassthroughQt) {
            setPassthroughQt(toggleState_);
        } else {
            setPassthroughNative(toggleState_);
        }
    });
    toggleTimer_.start();
}

void ProbeWindow::startTopmostProbe()
{
    qInfo().noquote()
        << "probe.instruction=raise_a_terminal_and_a_browser_the_probe_toggles_topmost_every_4_seconds";

    toggleTimer_.setInterval(4000);
    connect(&toggleTimer_, &QTimer::timeout, this, [this] {
        toggleState_ = !toggleState_;
        setTopmost(!toggleState_);
    });
    toggleTimer_.start();
}

ClickTargetWindow::ClickTargetWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Probe click target"));
    setFixedSize(420, 320);
}

int ClickTargetWindow::clickCount() const
{
    return clickCount_;
}

void ClickTargetWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        ++clickCount_;
        qInfo().noquote()
            << QStringLiteral("probe.event=target_click count=%1 global=%2")
                   .arg(clickCount_)
                   .arg(pointText(event->globalPosition().toPoint()));
        update();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void ClickTargetWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor(36, 52, 71));

    constexpr int cell = 24;
    for (int y = 0; y < height(); y += cell) {
        for (int x = 0; x < width(); x += cell) {
            if (((x / cell) + (y / cell)) % 2 == 0) {
                painter.fillRect(QRect(x, y, cell, cell), QColor(53, 76, 101));
            }
        }
    }

    painter.setPen(Qt::white);
    const QString message = QStringLiteral(
                                "Underlying click target\nClicks received: %1\n"
                                "Clicks that pass through the character window "
                                "land here")
                                .arg(clickCount_);
    painter.drawText(rect().adjusted(24, 24, -24, -24),
                     Qt::AlignCenter | Qt::TextWordWrap, message);
}
