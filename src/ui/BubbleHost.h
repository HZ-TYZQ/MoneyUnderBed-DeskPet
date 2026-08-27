#pragma once

#include "core/EventCoordinator.h"

#include <QString>

namespace mub::ui {

// 气泡侧的窄接口。
//
// CharacterPresenter 只需要知道三件事：一次点击是否已经被正在显示的对话消费、
// 单页气泡是否真的显示出来了、连续对话是否真的开始了。它不关心气泡怎么画，
// 也不持有对话数据。
//
// 事件所有权约定（docs/Decisions.md 第 4.2 节）：
// Presenter 先向协调器申请事件，再调用本接口。
//
// - 返回 `true`：气泡接管该事件，由气泡侧在关闭时调用
//   CharacterPresenter::finishEvent()。
// - 返回 `false`：Presenter 立即自行结束该事件。
//
// 两条路径都必然结束事件，因此不存在「气泡没起来，事件却永远占着」的情况。
// 没有注入气泡宿主时接口整体缺席，所有事件都由 Presenter 即刻结束。
class BubbleHost
{
public:
    virtual ~BubbleHost();

    // 角色被点击。返回 true 表示点击已被当前对话消费掉
    // （第 4.1 节：对话期间点击只补全或推进台词，不另外触发单击反馈）。
    virtual bool consumeCharacterClick() = 0;

    // 为 `kind` 显示一次单页气泡。`kind` 只会是单击反馈或自主闲聊。
    virtual bool showChatterBubble(core::EventKind kind) = 0;

    // 开始一段连续对话。找不到该标识时返回 false。
    virtual bool startDialogue(const QString &dialogueId) = 0;
};

} // namespace mub::ui
