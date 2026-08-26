#pragma once

namespace mub::app {

// 无交互自检。检查资源、字体和全部登记的精灵表后返回退出码。
//
// docs/Decisions.md 第 11.1 节：结果以进程退出码为准，详细信息写本地日志，
// 不依赖标准输出。Windows 正式版本使用 GUI 子系统，没有控制台。
//
// 返回 0 表示全部通过；非零表示至少一项失败，失败细节已写入日志。
//
// 阶段 8 会扩展为覆盖台词数据、配置初始化和关键组件。
int runSelfTest();

} // namespace mub::app
