#pragma once

#include "BubbleParameters.h"

#include <QHash>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QSpinBox;

namespace mub::bubbleprobe {

// 参数调节窗口。
//
// 计划第 11.1 节要求项目所有者对比字号、抗锯齿、行高、面板最大宽度、
// 每页行数、翻页提示和表情切换方式后冻结取值。本窗口把这些做成可实时调节的控件，
// 并提供一键导出，导出结果直接贴回 docs/Decisions.md。
class ControlWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit ControlWindow(QWidget *parent = nullptr);

    BubbleParameters parameters() const;

public slots:
    void setPageStatus(int lineCount, bool overflows);

signals:
    void parametersChanged(const BubbleParameters &parameters);
    void dialogueRequested(const QString &dialogueId);

private:
    QSpinBox *addSpin(QFormLayout *form, const QString &label, const QString &key,
                      int minimum, int maximum, int value, const QString &hint = {});
    void emitParameters();
    QString exportText() const;

    QHash<QString, QSpinBox *> spins_;
    QComboBox *scale_ = nullptr;
    QComboBox *dialogue_ = nullptr;
    QCheckBox *antialias_ = nullptr;
    QCheckBox *pageIndicator_ = nullptr;
    QLabel *status_ = nullptr;
};

} // namespace mub::bubbleprobe
