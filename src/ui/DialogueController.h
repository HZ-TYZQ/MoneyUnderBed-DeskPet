#pragma once

#include "core/EventCoordinator.h"
#include "dialogue/DialogueSession.h"
#include "ui/BubbleHost.h"

#include <QImage>
#include <QObject>
#include <QString>
#include <QTimer>

namespace mub::core {
class RandomSource;
class TimeSource;
}

namespace mub::dialogue {
struct Dialogue;
}

namespace mub::platform {
class DeskPetWindowBackend;
}

namespace mub::ui {

class BubbleWindow;
class CharacterPresenter;
class CharacterWindow;

// 正式对话系统。
//
// 把台词数据、DialogueSession 的状态机和气泡窗口接起来，并按
// docs/Decisions.md 第 4.2 节的所有权约定持有事件。
//
// 职责边界：本类不裁决优先级，也不决定何时触发对话 —— 那是协调器和
// CharacterPresenter 的职责；本类只负责「事件已经批下来之后」的显示与结束。
class DialogueController final : public QObject, public BubbleHost
{
    Q_OBJECT

public:
    DialogueController(CharacterPresenter &presenter, CharacterWindow &character,
                       const core::TimeSource &timeSource, core::RandomSource &random,
                       platform::DeskPetWindowBackend *backend,
                       QObject *parent = nullptr);
    ~DialogueController() override;

    // 气泡与角色使用同一整数倍率（第 4.8 节）。
    void setScale(int scale);
    int scale() const;

    // 系统会话不可交互时冻结打字、超时和自动隐藏计时。
    void setSessionSuspended(bool suspended);

    // BubbleHost。
    bool consumeCharacterClick() override;
    bool showChatterBubble(core::EventKind kind) override;
    bool startDialogue(const QString &dialogueId) override;

    // 立即结束并收起气泡，不保留待恢复的页面（第 4.2 节的隐藏与退出）。
    // 已持有的事件也一并结束。
    void stop();

    bool isShowing() const;
    core::EventKind ownedEvent() const;
    const dialogue::DialogueSession &session() const;
    const BubbleWindow &bubble() const;

private:
    bool begin(const dialogue::Dialogue &entry, core::EventKind kind);
    void handleBubbleClick();
    void handleEventReplaced(core::EventKind oldKind);
    void tick();
    void refreshContent();
    void reposition();
    // 收起气泡但不结束事件。事件被更高优先级替换时走这条路径。
    void hideBubble();

    CharacterPresenter *presenter_;
    CharacterWindow *character_;
    core::RandomSource *random_;
    dialogue::DialogueSession session_;
    BubbleWindow *bubble_ = nullptr;
    QTimer timer_;
    core::EventKind ownedKind_ = core::EventKind::None;
    // 当前页已加载的表情与文本，用来避免每一帧都重读资源、重新折行。
    QString faceId_;
    QImage face_;
    QString pageText_;
};

} // namespace mub::ui
