#include "dialogue/DialogueSession.h"

#include "core/TimeSource.h"
#include "dialogue/DialogueData.h"

#include <algorithm>

namespace mub::dialogue {

namespace {

QString textOf(const DialoguePage &page)
{
    return QString::fromUtf8(reinterpret_cast<const char *>(page.text));
}

} // namespace

DialogueSession::DialogueSession(const core::TimeSource &timeSource,
                                 DialogueSessionConfig config)
    : timeSource_(&timeSource)
    , config_(config)
{
    config_.typingMsPerChar = std::max(1, config_.typingMsPerChar);
    config_.idleTimeoutMs = std::max(0, config_.idleTimeoutMs);
    config_.singlePageAutoHideMs = std::max(0, config_.singlePageAutoHideMs);
    pendingPacing_ = activePacing();
}

void DialogueSession::setPacing(const DialoguePacing &pacing)
{
    // 只记下来。正在进行的对话继续用它开始时的取值（第 14.8 节）。
    pendingPacing_.typingMsPerChar = std::max(1, pacing.typingMsPerChar);
    pendingPacing_.singlePageAutoHideMs = std::max(0, pacing.singlePageAutoHideMs);
}

const DialoguePacing &DialogueSession::pendingPacing() const
{
    return pendingPacing_;
}

DialoguePacing DialogueSession::activePacing() const
{
    return {config_.typingMsPerChar, config_.singlePageAutoHideMs};
}

void DialogueSession::start(const Dialogue &dialogue)
{
    if (dialogue.pages.empty()) {
        stop();
        return;
    }
    dialogue_ = &dialogue;
    // 第 14.8 节：打字速度与单页自动消失时间在这里取一次快照，整段对话都用它。
    // `idleTimeoutMs` 不在其中，第 14.7 节保持冻结。
    config_.typingMsPerChar = pendingPacing_.typingMsPerChar;
    config_.singlePageAutoHideMs = pendingPacing_.singlePageAutoHideMs;
    lastInteractionMs_ = timeSource_->nowMs();
    if (suspended_) {
        suspendedStartedMs_ = lastInteractionMs_;
    }
    beginPage(0);
}

void DialogueSession::stop()
{
    dialogue_ = nullptr;
    state_ = DialogueState::Idle;
    pageIndex_ = 0;
    fullText_.clear();
    visibleCharacters_ = 0;
}

void DialogueSession::setSuspended(const bool suspended)
{
    if (suspended_ == suspended) {
        return;
    }

    const qint64 now = timeSource_->nowMs();
    suspended_ = suspended;
    if (suspended) {
        suspendedStartedMs_ = now;
        return;
    }

    if (dialogue_ == nullptr) {
        return;
    }

    const qint64 pausedMs = std::max<qint64>(0, now - suspendedStartedMs_);
    pageStartedMs_ += pausedMs;
    pageCompletedMs_ += pausedMs;
    lastInteractionMs_ += pausedMs;
}

bool DialogueSession::isSuspended() const
{
    return suspended_;
}

bool DialogueSession::update()
{
    if (suspended_ || dialogue_ == nullptr || state_ == DialogueState::Idle
        || state_ == DialogueState::Finished) {
        return false;
    }

    const qint64 now = timeSource_->nowMs();

    if (state_ == DialogueState::Typing) {
        const qint64 elapsed = now - pageStartedMs_;
        const int expected = static_cast<int>(
            std::min<qint64>(fullText_.size(), elapsed / config_.typingMsPerChar));
        if (expected >= fullText_.size()) {
            completeCurrentPage();
            return true;
        }
        if (expected != visibleCharacters_) {
            visibleCharacters_ = expected;
            return true;
        }
        return false;
    }

    // PageComplete。
    // 单页气泡在文字完成后开始自动消失计时，不需要用户点击。
    if (pageCount() == 1 && now - pageCompletedMs_ >= config_.singlePageAutoHideMs) {
        finish();
        return true;
    }
    // 连续对话不自动翻页，但持续无操作会整体结束。
    if (config_.idleTimeoutMs > 0 && now - lastInteractionMs_ >= config_.idleTimeoutMs) {
        finish();
        return true;
    }
    return false;
}

bool DialogueSession::click()
{
    if (suspended_ || dialogue_ == nullptr || state_ == DialogueState::Idle
        || state_ == DialogueState::Finished) {
        return false;
    }

    lastInteractionMs_ = timeSource_->nowMs();

    if (state_ == DialogueState::Typing) {
        // 第一次点击立即补全当前页；再次点击才进入下一页。
        completeCurrentPage();
        return true;
    }

    if (isLastPage()) {
        // 最后一页显示完成后，再次点击结束对话。
        finish();
        return true;
    }

    beginPage(pageIndex_ + 1);
    return true;
}

DialogueState DialogueSession::state() const
{
    return state_;
}

bool DialogueSession::isActive() const
{
    return state_ == DialogueState::Typing || state_ == DialogueState::PageComplete;
}

const Dialogue *DialogueSession::dialogue() const
{
    return dialogue_;
}

int DialogueSession::pageIndex() const
{
    return pageIndex_;
}

int DialogueSession::pageCount() const
{
    return dialogue_ != nullptr ? static_cast<int>(dialogue_->pages.size()) : 0;
}

bool DialogueSession::isLastPage() const
{
    return dialogue_ != nullptr && pageIndex_ + 1 >= pageCount();
}

QString DialogueSession::visibleText() const
{
    return fullText_.left(visibleCharacters_);
}

QString DialogueSession::fullText() const
{
    return fullText_;
}

QString DialogueSession::faceId() const
{
    if (dialogue_ == nullptr || pageIndex_ >= pageCount()) {
        return {};
    }
    return QString::fromLatin1(dialogue_->pages[static_cast<size_t>(pageIndex_)].faceId);
}

void DialogueSession::beginPage(const int index)
{
    pageIndex_ = index;
    // 进入下一页时同时切换到该页预先指定的表情，由 faceId() 反映。
    fullText_ = textOf(dialogue_->pages[static_cast<size_t>(index)]);
    visibleCharacters_ = 0;
    state_ = DialogueState::Typing;
    pageStartedMs_ = timeSource_->nowMs();
}

void DialogueSession::completeCurrentPage()
{
    visibleCharacters_ = static_cast<int>(fullText_.size());
    state_ = DialogueState::PageComplete;
    pageCompletedMs_ = timeSource_->nowMs();
}

void DialogueSession::finish()
{
    state_ = DialogueState::Finished;
}

} // namespace mub::dialogue
