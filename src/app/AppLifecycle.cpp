#include "app/AppLifecycle.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QWidget>

namespace mub::app {
namespace {

Q_LOGGING_CATEGORY(lcLifecycle, "mub.app.lifecycle")

} // namespace

AppLifecycle::AppLifecycle(QObject *parent)
    : QObject(parent)
{
}

std::size_t AppLifecycle::indexOf(const AuxiliaryWindow kind)
{
    return static_cast<std::size_t>(kind);
}

void AppLifecycle::takeOverQuitPolicy()
{
    // 关掉这条默认规则后，只有 `requestQuit()` 能结束进程。
    QGuiApplication::setQuitOnLastWindowClosed(false);
    quitPolicyTakenOver_ = true;
}

bool AppLifecycle::quitPolicyTakenOver() const
{
    return quitPolicyTakenOver_;
}

void AppLifecycle::setAuxiliaryWindow(const AuxiliaryWindow kind, QWidget *window)
{
    QPointer<QWidget> &slot = auxiliaryWindows_[indexOf(kind)];
    if (!slot.isNull() && slot != window) {
        // 一种辅助窗口只允许一个实例。出现第二个实例是装配错误，
        // 不是运行时状态，因此拒绝覆盖而不是悄悄换掉。
        qCWarning(lcLifecycle)
            << "refusing to replace an already registered auxiliary window"
            << static_cast<int>(kind);
        return;
    }
    slot = window;
}

QWidget *AppLifecycle::auxiliaryWindow(const AuxiliaryWindow kind) const
{
    return auxiliaryWindows_[indexOf(kind)].data();
}

void AppLifecycle::showAuxiliaryWindow(const AuxiliaryWindow kind)
{
    QWidget *const window = auxiliaryWindow(kind);
    if (window == nullptr) {
        qCWarning(lcLifecycle) << "no auxiliary window registered for"
                               << static_cast<int>(kind);
        return;
    }
    // 同一个实例反复显示。关闭只是隐藏，窗口对象一直存在，因此这里不新建。
    window->show();
    window->raise();
    window->activateWindow();
}

void AppLifecycle::requestQuit()
{
    if (quitRequested_) {
        // 角色菜单和托盘可能在同一轮里都发出退出请求，清理只做一次。
        return;
    }
    quitRequested_ = true;
    emit quitting();
}

bool AppLifecycle::quitRequested() const
{
    return quitRequested_;
}

} // namespace mub::app
