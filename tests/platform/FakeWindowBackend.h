#pragma once

#include "platform/DeskPetWindowBackend.h"

#include <QList>
#include <QRegion>
#include <QString>

namespace mub::testing {

// 平台接口的假实现。
//
// 计划第 8.2 节要求「平台接口使用假实现测试，不在纯逻辑测试中调用桌面 API」。
// 本类记录调用，不接触任何真实窗口。
class FakeWindowBackend final : public platform::DeskPetWindowBackend
{
public:
    struct Calls
    {
        int deskPetWindowFlags = 0;
        int configureAsDeskPet = 0;
        int setAlwaysOnTop = 0;
        int setInputPassthrough = 0;
        int setHitMask = 0;
        int startSystemDrag = 0;
    };

    explicit FakeWindowBackend(platform::BackendCapabilities capabilities = {})
        : capabilities_(std::move(capabilities))
    {
        if (capabilities_.name.isEmpty()) {
            capabilities_.name = QStringLiteral("fake");
            capabilities_.alwaysOnTop = true;
            capabilities_.inputPassthrough = true;
            capabilities_.pixelHitMask = true;
            capabilities_.systemDrag = true;
            capabilities_.excludeFromWindowList = true;
        }
    }

    platform::BackendCapabilities capabilities() const override
    {
        return capabilities_;
    }

    Qt::WindowFlags deskPetWindowFlags() const override
    {
        ++calls_.deskPetWindowFlags;
        return Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus
            | Qt::NoDropShadowWindowHint;
    }

    void configureAsDeskPet(QWindow *window) override
    {
        Q_UNUSED(window)
        // 标志必须在原生窗口创建之前取用，也就是在这一步之前。
        flagsTakenBeforeConfigure_ = calls_.deskPetWindowFlags > 0;
        ++calls_.configureAsDeskPet;
    }

    void setAlwaysOnTop(QWindow *window, const bool enabled) override
    {
        Q_UNUSED(window)
        ++calls_.setAlwaysOnTop;
        alwaysOnTop_ = enabled;
    }

    void setInputPassthrough(QWindow *window, const bool enabled) override
    {
        Q_UNUSED(window)
        ++calls_.setInputPassthrough;
        passthrough_ = enabled;
    }

    void setHitMask(QWindow *window, const QRegion &region) override
    {
        Q_UNUSED(window)
        ++calls_.setHitMask;
        lastMask_ = region;
    }

    bool startSystemDrag(QWindow *window) override
    {
        Q_UNUSED(window)
        ++calls_.startSystemDrag;
        return systemDragAccepted_;
    }

    const Calls &calls() const { return calls_; }
    bool flagsTakenBeforeConfigure() const { return flagsTakenBeforeConfigure_; }
    bool alwaysOnTop() const { return alwaysOnTop_; }
    bool passthrough() const { return passthrough_; }
    QRegion lastMask() const { return lastMask_; }
    void setSystemDragAccepted(const bool accepted) { systemDragAccepted_ = accepted; }

private:
    platform::BackendCapabilities capabilities_;
    mutable Calls calls_;
    bool flagsTakenBeforeConfigure_ = false;
    bool alwaysOnTop_ = false;
    bool passthrough_ = false;
    bool systemDragAccepted_ = true;
    QRegion lastMask_;
};

} // namespace mub::testing
