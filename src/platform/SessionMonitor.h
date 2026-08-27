#pragma once

#include "core/SessionSuspension.h"

#include <QObject>

class QWindow;

namespace mub::platform {

// 平台会话事件的窄入口。平台实现只报告各自观察到的原因，基类负责聚合，
// 避免一个恢复信号覆盖另一个仍然有效的暂停原因。
class SessionMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SessionMonitor(QObject *parent = nullptr);
    ~SessionMonitor() override;

    virtual bool start(QWindow *notificationWindow) = 0;
    bool isSuspended() const;

signals:
    void suspendedChanged(bool suspended);

protected:
    void setReason(core::SessionSuspendReason reason, bool suspended);

private:
    core::SessionSuspensionState state_;
};

} // namespace mub::platform
