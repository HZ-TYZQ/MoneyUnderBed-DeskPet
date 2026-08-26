#pragma once

#include <memory>

namespace mub::platform {

class DeskPetWindowBackend;

// 平台选择的唯一位置。除本文件的实现外，产品代码中不应再出现
// 按操作系统分支的条件编译（docs/Decisions.md 第 8.4 节）。
std::unique_ptr<DeskPetWindowBackend> createWindowBackend();

} // namespace mub::platform
