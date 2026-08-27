#pragma once

#include <QFont>

namespace mub::ui {

// 对话气泡使用的字体。
//
// docs/Decisions.md 第 4.7 节：固定使用打包内的 Ark Pixel，不依赖系统字体，
// 也不因为系统缺字而回退到别的字族。
//
// 只注册一次，之后复用。注册失败时返回一个像素字号正确的默认字体，
// 让气泡仍然能画出来，同时在日志中留下记录 —— 缺字体属于素材问题，
// 由自检（--self-test）负责判定失败，不在绘制路径上中断产品。
QFont dialogueFont(int pixelSize);

// 已注册的字族名。注册失败时为空。供诊断与测试使用。
QString dialogueFontFamily();

} // namespace mub::ui
