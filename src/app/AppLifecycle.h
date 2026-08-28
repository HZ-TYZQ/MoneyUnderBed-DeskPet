#pragma once

#include <QObject>
#include <QPointer>

#include <array>
#include <cstddef>

class QWidget;

namespace mub::app {

// 应用进程的生命周期与辅助窗口所有权。
//
// docs/Decisions.md 第 14.6 节：应用进程的生命周期由应用层显式管理，不由 Qt 的
// 「最后一个普通窗口关闭」规则决定；关闭设置或关于窗口只关闭该辅助窗口，不得
// 连带退出桌宠；同一种辅助窗口同时只允许一个实例，关闭后必须能再次打开；只有
// 用户从右键菜单、托盘或其他正式退出入口明确选择「退出」才结束进程。
//
// **根因**：`QApplication::quitOnLastWindowClosed` 默认为真，而角色窗口与气泡
// 窗口按 `deskPetWindowFlags()` 带 `Qt::Tool`，不计入「最后一个窗口」；设置与
// 关于窗口是 `QDialog`，计入。因此关闭辅助窗口时 Qt 认为最后一个普通窗口已经
// 关闭并退出应用。修复的位置是启动层从未接管这条策略，而不是辅助窗口的关闭
// 事件——把关闭改写成隐藏只会掩盖它，见 `DevelopmentStatus-1.1.md` 的探针记录。
//
// 本类只管窗口所有权与退出路径。设置的校验、应用与持久化不属于这里，
// 由阶段 2 的设置控制器承担。
class AppLifecycle final : public QObject
{
    Q_OBJECT

public:
    enum class AuxiliaryWindow
    {
        Settings,
        About,
    };

    explicit AppLifecycle(QObject *parent = nullptr);

    // 接管退出策略。必须在创建任何辅助窗口之前调用一次。
    void takeOverQuitPolicy();
    bool quitPolicyTakenOver() const;

    // 登记一个辅助窗口。窗口由调用方持有，本类只保存弱引用。
    // 同一种窗口重复登记不同实例是装配错误，会被拒绝并记录警告。
    void setAuxiliaryWindow(AuxiliaryWindow kind, QWidget *window);
    QWidget *auxiliaryWindow(AuxiliaryWindow kind) const;

    // 显示、前置并激活该辅助窗口。已经打开时只前置，不新建实例。
    void showAuxiliaryWindow(AuxiliaryWindow kind);

    // 所有正式退出入口都经过这里。重复请求只产生一次退出。
    void requestQuit();
    bool quitRequested() const;

signals:
    // 退出前的唯一通知。调用方在这里做清理并结束事件循环。
    void quitting();

private:
    static constexpr std::size_t kAuxiliaryWindowCount = 2;
    static std::size_t indexOf(AuxiliaryWindow kind);

    std::array<QPointer<QWidget>, kAuxiliaryWindowCount> auxiliaryWindows_;
    bool quitPolicyTakenOver_ = false;
    bool quitRequested_ = false;
};

} // namespace mub::app
