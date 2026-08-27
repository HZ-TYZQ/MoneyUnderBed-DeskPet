#pragma once

#include <QRegion>
#include <QString>

class QWindow;

namespace mub::platform {

// 平台能力自述。UI 依据它决定是否隐藏某项设置或走降级路径，
// 而不是自己判断当前运行在哪个系统上。
struct BackendCapabilities
{
    QString name;                       // 后端名，用于日志和诊断信息
    bool alwaysOnTop = false;           // 能否让窗口保持置顶
    bool inputPassthrough = false;      // 能否整窗穿透鼠标输入
    bool pixelHitMask = false;          // 能否按像素设置命中区域
    bool systemDrag = false;            // 能否请求由窗口管理器接管拖动
    bool excludeFromWindowList = false; // 能否不出现在任务栏与窗口切换列表
};

// 窄平台接口。
//
// docs/Decisions.md 第 8.4 节要求平台相关能力集中在窄接口中，
// 不把条件编译散落到动画、行为和角色逻辑里。
// 本接口是唯一允许出现平台分叉的地方。
//
// 所有方法都接受 QWindow，不接受 QWidget：接口不依赖 Qt Widgets，
// 也避免 QWidget 改标志时的隐藏与重新显示。
class DeskPetWindowBackend
{
public:
    DeskPetWindowBackend() = default;
    virtual ~DeskPetWindowBackend();

    DeskPetWindowBackend(const DeskPetWindowBackend &) = delete;
    DeskPetWindowBackend &operator=(const DeskPetWindowBackend &) = delete;
    DeskPetWindowBackend(DeskPetWindowBackend &&) = delete;
    DeskPetWindowBackend &operator=(DeskPetWindowBackend &&) = delete;

    virtual BackendCapabilities capabilities() const = 0;

    // 桌宠窗口的窗口标志。
    //
    // 调用方必须在原生窗口**创建之前**把它套到 QWidget 上。Windows 上先按
    // 普通窗口创建、再改标志的窗口会保留 DWM 的圆角边框与系统背景材质，
    // 结果是角色四周出现一个可见的矩形框；探针之所以没有这个问题，正是因为
    // 它在窗口创建前就设定了最终标志。
    virtual Qt::WindowFlags deskPetWindowFlags() const = 0;

    // 一次性把窗口配置成桌宠窗口：无边框、透明、不进任务栏、不抢焦点。
    // 必须在窗口首次显示之前调用。
    virtual void configureAsDeskPet(QWindow *window) = 0;

    virtual void setAlwaysOnTop(QWindow *window, bool enabled) = 0;

    // 整窗穿透。开启后窗口不接收任何鼠标输入。
    // 这与按像素命中是两件事：前者整窗关闭输入，后者只让透明像素穿透。
    virtual void setInputPassthrough(QWindow *window, bool enabled) = 0;

    // 按像素设置命中区域。空区域表示恢复为整窗接收。
    virtual void setHitMask(QWindow *window, const QRegion &region) = 0;

    // 请求由窗口管理器接管拖动。返回是否被接受。
    // 不被接受时由调用方自行做手动移动，本接口不代为回退。
    virtual bool startSystemDrag(QWindow *window) = 0;

};

} // namespace mub::platform
