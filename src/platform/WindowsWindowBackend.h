#pragma once

#include "platform/QtWindowBackend.h"

namespace mub::platform {

// Windows 专用实现。只覆盖纯 Qt 路径不足以确定的部分，其余继承自 QtWindowBackend。
//
// 整窗穿透有两条候选路径，取舍尚未由真实 Windows 测试决定
// （docs/WindowsFeasibilityResults.md 仍为未实测）。因此两条都实现，
// 由环境变量 `MUB_WIN_PASSTHROUGH` 在运行时选择：
//
//   qt      使用 Qt::WindowTransparentForInput（默认）
//   native  只修改 WS_EX_TRANSPARENT 扩展样式，不动 Qt 窗口标志
//
// 默认走 Qt 路径符合 docs/Decisions.md 第 8.4 节「优先使用 Qt」。
// 一次人工校验就能覆盖两种可能，不必错了再出一次包。
// 取得结论后删除未采用的那条，并把结论写回决策文档。
class WindowsWindowBackend final : public QtWindowBackend
{
public:
    enum class PassthroughStrategy
    {
        QtWindowFlag,
        NativeExtendedStyle,
    };

    WindowsWindowBackend();

    static PassthroughStrategy strategyFromEnvironment();

    BackendCapabilities capabilities() const override;
    void configureAsDeskPet(QWindow *window) override;
    void setInputPassthrough(QWindow *window, bool enabled) override;

    PassthroughStrategy passthroughStrategy() const;

private:
    PassthroughStrategy strategy_;
};

} // namespace mub::platform
