#pragma once

namespace mub::app {

enum SelfTestFailureCode
{
    SelfTestSuccess = 0,
    SelfTestResourceFailure = 1 << 0,
    SelfTestFontFailure = 1 << 1,
    SelfTestDialogueFailure = 1 << 2,
    SelfTestConfigurationFailure = 1 << 3,
    SelfTestComponentFailure = 1 << 4,
    SelfTestPlatformFailure = 1 << 5,
};

// 每一类失败占一个稳定比特，CI 和用户只需检查非零；日志给出具体条目。
int selfTestExitCode(bool resourcesOk, bool fontOk, bool dialogueOk,
                     bool configurationOk, bool componentsOk, bool platformOk);

// 无交互自检。检查资源、字体和全部登记的精灵表后返回退出码。
//
// docs/Decisions.md 第 11.1 节：结果以进程退出码为准，详细信息写本地日志，
// 不依赖标准输出。Windows 正式版本使用 GUI 子系统，没有控制台。
//
// 返回 0 表示全部通过；非零表示至少一项失败，失败细节已写入日志。
//
int runSelfTest();

} // namespace mub::app
