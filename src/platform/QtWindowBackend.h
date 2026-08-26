#pragma once

#include "platform/DeskPetWindowBackend.h"

namespace mub::platform {

// 只使用 Qt 公共 API 的实现。
//
// docs/Decisions.md 第 8.4 节：优先使用 Qt，Qt 无法满足窗口细节时才调用平台 API。
// 因此本类是默认实现，平台专用实现在此基础上只覆盖确实需要的部分。
//
// Linux XCB 路径的全部能力已由 docs/FeasibilityResults.md 实测通过。
class QtWindowBackend : public DeskPetWindowBackend
{
public:
    BackendCapabilities capabilities() const override;
    void configureAsDeskPet(QWindow *window) override;
    void setAlwaysOnTop(QWindow *window, bool enabled) override;
    void setInputPassthrough(QWindow *window, bool enabled) override;
    void setHitMask(QWindow *window, const QRegion &region) override;
    bool startSystemDrag(QWindow *window) override;

protected:
    // 桌宠窗口的基础标志。平台实现可以在此基础上增减。
    static Qt::WindowFlags deskPetFlags();
};

} // namespace mub::platform
