#pragma once

#include <QString>
#include <QtGlobal>

namespace mub::core {
class TimeSource;
}

namespace mub::dialogue {

struct Dialogue;

// 对话会话的可见状态。
enum class DialogueState
{
    Idle,          // 没有对话
    Typing,        // 当前页正在逐字打出
    PageComplete,  // 当前页已打完，等待点击或超时
    Finished,      // 对话结束
};

struct DialogueSessionConfig
{
    // docs/Decisions.md 第 4.1 节：`28 ms` 由 2026-08-27 的 Qt 原型审核确定。
    // 第 14.4 节把它开放为设置项，该值继续作为默认值。
    int typingMsPerChar = 28;
    // 第 4.1 节：用户持续 20 s 没有操作时对话自动结束。
    // 第 14.7 节：**保持冻结**，不开放为用户设置。
    int idleTimeoutMs = 20000;
    // 第 4 节：普通单句气泡在文字完成后开始自动消失计时。第 14.4 节开放为设置。
    int singlePageAutoHideMs = 4000;
};

// 可由用户设置的两个对话节奏参数。`idleTimeoutMs` 不在其中。
struct DialoguePacing
{
    int typingMsPerChar = DialogueSessionConfig{}.typingMsPerChar;
    int singlePageAutoHideMs = DialogueSessionConfig{}.singlePageAutoHideMs;

    friend bool operator==(const DialoguePacing &, const DialoguePacing &) = default;
};

// 连续对话的打字、翻页与超时状态机。
//
// 纯逻辑：不接触任何窗口、字体或素材，时间由注入的 TimeSource 提供，
// 因此测试不依赖真实等待。
//
// 本类不决定何时触发对话，也不做优先级裁决 —— 那是 core::EventCoordinator 的职责。
class DialogueSession
{
public:
    explicit DialogueSession(const core::TimeSource &timeSource,
                             DialogueSessionConfig config = {});

    // 对话节奏。第 14.8 节：新值在**下一次对话开始**时生效，正在进行的打字与
    // 自动消失计时不受影响，也不会被重启。
    void setPacing(const DialoguePacing &pacing);
    // 待生效的取值。
    const DialoguePacing &pendingPacing() const;
    // 当前这一段对话实际在用的取值。
    DialoguePacing activePacing() const;

    // 开始一段对话。重复触发同一段对话时从第一页重新开始
    // （docs/Decisions.md 第 4.1 节）。
    void start(const Dialogue &dialogue);

    // 立即结束。用于隐藏与退出：不保留待恢复的对话页面。
    void stop();

    // 会话锁定、睡眠或显示器关闭期间冻结所有对话计时。
    // 恢复时平移各计时基准，避免补打文字、立即超时或自动消失。
    void setSuspended(bool suspended);
    bool isSuspended() const;

    // 按当前时间推进。返回 true 表示可见内容或状态发生了变化。
    bool update();

    // 用户点击角色或面板。
    //
    // 打字中：立即补全当前页。
    // 已打完且不是最后一页：进入下一页，同时切换到该页指定的表情。
    // 已打完且是最后一页：结束对话。
    //
    // 返回 true 表示本次点击被对话消费掉了。
    bool click();

    DialogueState state() const;
    bool isActive() const;

    const Dialogue *dialogue() const;
    int pageIndex() const;
    int pageCount() const;
    bool isLastPage() const;

    // 当前页已经打出的部分。
    QString visibleText() const;
    // 当前页的完整文本。
    QString fullText() const;
    // 当前页对应的表情标识。
    QString faceId() const;

private:
    void beginPage(int index);
    void completeCurrentPage();
    void finish();

    const core::TimeSource *timeSource_;
    DialogueSessionConfig config_;
    // 待生效的节奏。下一次 start() 时并入 config_。
    DialoguePacing pendingPacing_;

    const Dialogue *dialogue_ = nullptr;
    DialogueState state_ = DialogueState::Idle;
    int pageIndex_ = 0;
    QString fullText_;
    int visibleCharacters_ = 0;

    qint64 pageStartedMs_ = 0;
    qint64 pageCompletedMs_ = 0;
    qint64 lastInteractionMs_ = 0;
    qint64 suspendedStartedMs_ = 0;
    bool suspended_ = false;
};

} // namespace mub::dialogue
