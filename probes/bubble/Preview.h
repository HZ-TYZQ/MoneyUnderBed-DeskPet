#pragma once

#include "BubbleParameters.h"
#include "BubbleRenderer.h"

#include "character/SpriteSheet.h"
#include "core/TimeSource.h"
#include "dialogue/DialogueSession.h"

#include <QImage>
#include <QObject>
#include <QPoint>
#include <QTimer>
#include <QWidget>

namespace mub::dialogue {
struct Dialogue;
}

namespace mub::bubbleprobe {

// 只画气泡面板的无边框半透明窗口。
class BubbleOverlay final : public QWidget
{
    Q_OBJECT

public:
    explicit BubbleOverlay(QWidget *parent = nullptr);
    BubbleRenderer &renderer();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    BubbleRenderer renderer_;
};

// 只画角色的无边框半透明窗口，可拖动。
class CharacterOverlay final : public QWidget
{
    Q_OBJECT

public:
    CharacterOverlay(character::SpriteSheet sheet, QWidget *parent = nullptr);

    void setScale(double scale);

signals:
    void moved();
    void clicked();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    character::SpriteSheet sheet_;
    double scale_ = 2.0;
    bool dragging_ = false;
    QPoint dragOffset_;
};

// 把台词数据、对话状态机和两个窗口接起来。
class PreviewController final : public QObject
{
    Q_OBJECT

public:
    explicit PreviewController(QObject *parent = nullptr);
    ~PreviewController() override;

    void show();
    void setParameters(const BubbleParameters &parameters);
    void playDialogue(const QString &dialogueId);

signals:
    // 供控制窗口显示当前页的折行行数与是否超出每页行数上限。
    void pageStatusChanged(int lineCount, bool overflows);

private:
    void tick();
    void refreshContent();
    void reposition();

    core::MonotonicTimeSource clock_;
    dialogue::DialogueSession session_;
    BubbleParameters parameters_;
    CharacterOverlay *character_ = nullptr;
    BubbleOverlay *bubble_ = nullptr;
    QTimer timer_;
    QString dialogueId_;
};

} // namespace mub::bubbleprobe
