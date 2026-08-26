#pragma once

#include <QElapsedTimer>
#include <QPixmap>
#include <QPoint>
#include <QRegion>
#include <QString>
#include <QTimer>
#include <QWidget>

class QCloseEvent;
class QMouseEvent;
class QPaintEvent;

// 探针用例。每个用例只测一件事，不互相回退，
// 以免一条路径成功被记成两条都成功。
enum class ProbeCase
{
    Build,             // 只构造 QApplication 并退出
    Screen,            // 屏幕与后端信息
    Dpi,               // 缩放信息与屏幕切换跟踪
    Render,            // 透明与整数倍最近邻绘制
    Animate,           // 帧序与定时
    Move,              // 自主移动，请求位置与实际位置对照
    DragSystem,        // 只用 startSystemMove()，失败即记失败
    DragManual,        // 只用手动 move()，不调用 startSystemMove()
    HitTest,           // 可见像素命中，透明像素穿透
    PassthroughQt,     // 整窗穿透：Qt::WindowTransparentForInput 路径
    PassthroughNative, // 整窗穿透：只改原生扩展样式的候选路径
    Topmost,           // 置顶开关切换
    Focus,             // 显示、点击、拖动是否抢焦点
    WindowList,        // 任务栏与 Alt+Tab 表现
    Lifecycle,         // 关闭、退出与重复启动
};

bool parseProbeCase(const QString &text, ProbeCase *probeCase);
QString probeCaseName(ProbeCase probeCase);
QStringList knownProbeCaseNames();

class ProbeWindow final : public QWidget
{
    Q_OBJECT

public:
    static constexpr int FrameWidth = 69;
    static constexpr int FrameHeight = 111;

    ProbeWindow(ProbeCase probeCase, QPixmap spriteSheet, int integerScale,
                QWidget *parent = nullptr);

    void startProbe();
    void reportSummary() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void advanceAnimation();
    void advanceMotion();
    void applyAlphaMask();
    void logNativeState(const QString &eventName) const;
    void logPosition(const QString &eventName, const QPoint &requested) const;
    void setPassthroughNative(bool enabled);
    void setPassthroughQt(bool enabled);
    void setTopmost(bool enabled);
    void startAnimationProbe();
    void startDpiProbe();
    void startFocusProbe();
    void startMotionProbe();
    void startPassthroughProbe();
    void startTopmostProbe();

    ProbeCase probeCase_;
    QPixmap spriteSheet_;
    int integerScale_ = 1;
    int frameCount_ = 1;
    int frameIndex_ = 0;
    // 记录首次拿到的原生句柄。之后若发生变化，说明窗口被重建。
    quintptr initialWindowId_ = 0;

    QTimer animationTimer_;
    QElapsedTimer animationElapsed_;
    qint64 previousFrameTimestampMs_ = -1;
    qint64 animationIntervalTotalMs_ = 0;
    qint64 animationIntervalMinimumMs_ = -1;
    qint64 animationIntervalMaximumMs_ = -1;
    int measuredAnimationIntervals_ = 0;

    QTimer motionTimer_;
    QPoint motionOrigin_;
    int motionOffset_ = 0;
    int motionDirection_ = 1;
    int motionTicks_ = 0;

    QTimer toggleTimer_;
    bool toggleState_ = false;
    int windowRebuildCount_ = 0;

    bool manualDragActive_ = false;
    QPoint manualDragOffset_;
    int petClickCount_ = 0;
};

// 位于角色窗口下方的点击靶。用于区分“角色吃掉了点击”和“点击穿到了下层”。
class ClickTargetWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit ClickTargetWindow(QWidget *parent = nullptr);

    int clickCount() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    int clickCount_ = 0;
};
