#include "ControlWindow.h"

#include "dialogue/DialogueData.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace mub::bubbleprobe {

namespace {

QString dialogueLabel(const dialogue::Dialogue &entry)
{
    const QString source = entry.source == dialogue::LineSource::OriginalDemo
        ? QStringLiteral("原作")
        : QStringLiteral("新增");
    const QString first =
        QString::fromUtf8(reinterpret_cast<const char *>(entry.pages[0].text));
    return QStringLiteral("[%1] %2 (%3 页) %4")
        .arg(source, QLatin1String(entry.id))
        .arg(entry.pages.size())
        .arg(first.left(12));
}

} // namespace

ControlWindow::ControlWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("气泡原型 A 参数审核"));

    auto *layout = new QVBoxLayout(this);

    auto *contentBox = new QGroupBox(tr("内容"), this);
    auto *contentForm = new QFormLayout(contentBox);
    dialogue_ = new QComboBox(contentBox);
    for (const dialogue::Dialogue &entry : dialogue::registeredDialogues()) {
        dialogue_->addItem(dialogueLabel(entry), QString::fromLatin1(entry.id));
    }
    contentForm->addRow(tr("台词"), dialogue_);
    scale_ = new QComboBox(contentBox);
    scale_->addItem(QStringLiteral("1x"), 1.0);
    scale_->addItem(QStringLiteral("1.5x"), 1.5);
    scale_->addItem(QStringLiteral("2x"), 2.0);
    scale_->setCurrentIndex(2);
    contentForm->addRow(tr("显示倍率"), scale_);
    status_ = new QLabel(contentBox);
    status_->setWordWrap(true);
    contentForm->addRow(tr("当前页"), status_);
    layout->addWidget(contentBox);

    auto *panelBox = new QGroupBox(tr("面板  .dialogue-panel"), this);
    auto *panelForm = new QFormLayout(panelBox);
    addSpin(panelForm, tr("面板固定宽度"), QStringLiteral("panelWidth"), 120, 480, 260,
            tr("原型 A 的面板宽度固定，不随文字长短变化"));
    addSpin(panelForm, tr("面板最小高度"), QStringLiteral("panelMinHeight"), 40, 240, 78);
    addSpin(panelForm, tr("上内边距"), QStringLiteral("paddingTop"), 0, 40, 13);
    addSpin(panelForm, tr("右内边距"), QStringLiteral("paddingRight"), 0, 40, 17);
    addSpin(panelForm, tr("下内边距"), QStringLiteral("paddingBottom"), 0, 40, 17);
    addSpin(panelForm, tr("左内边距"), QStringLiteral("paddingLeft"), 0, 160, 72,
            tr("给左侧表情让出的位置"));
    addSpin(panelForm, tr("圆角半径"), QStringLiteral("cornerRadius"), 0, 8, 1);
    addSpin(panelForm, tr("底色不透明度"), QStringLiteral("panelAlpha"), 0, 255, 219,
            tr("CSS 为 86%"));
    addSpin(panelForm, tr("边界不透明度"), QStringLiteral("borderAlpha"), 0, 255, 41,
            tr("CSS 为 16%"));
    layout->addWidget(panelBox);

    auto *faceBox = new QGroupBox(tr("表情  .dialogue-portrait"), this);
    auto *faceForm = new QFormLayout(faceBox);
    addSpin(faceForm, tr("表情宽"), QStringLiteral("portraitWidth"), 20, 240, 60);
    addSpin(faceForm, tr("表情高"), QStringLiteral("portraitHeight"), 20, 288, 72);
    addSpin(faceForm, tr("距面板左"), QStringLiteral("portraitLeft"), 0, 60, 6);
    addSpin(faceForm, tr("距面板底"), QStringLiteral("portraitBottom"), 0, 60, 3);
    addSpin(faceForm, tr("分隔线位置"), QStringLiteral("separatorLeft"), 0, 160, 66);
    addSpin(faceForm, tr("分隔线上下内缩"), QStringLiteral("separatorInset"), 0, 40, 8);
    addSpin(faceForm, tr("分隔线不透明度"), QStringLiteral("separatorAlpha"), 0, 255, 36,
            tr("CSS 为 14%"));
    layout->addWidget(faceBox);

    auto *textBox = new QGroupBox(tr("文字  .dialogue-text"), this);
    auto *textForm = new QFormLayout(textBox);
    addSpin(textForm, tr("像素字号"), QStringLiteral("fontPixelSize"), 8, 48, 12,
            tr("Ark Pixel 为 12px 点阵字体，非整数倍会破坏栅格"));
    addSpin(textForm, tr("行高千分比"), QStringLiteral("lineHeightPermille"), 800, 3000, 1620,
            tr("CSS 的 line-height: 1.62，即字号的 1.62 倍"));
    addSpin(textForm, tr("文字区最小高度"), QStringLiteral("textMinHeight"), 0, 200, 46);
    antialias_ = new QCheckBox(tr("文字抗锯齿"), textBox);
    antialias_->setToolTip(tr("像素字体通常应关闭，打开只为对比差别"));
    textForm->addRow(QString(), antialias_);
    layout->addWidget(textBox);

    auto *cueBox = new QGroupBox(tr("翻页提示  .page-cue"), this);
    auto *cueForm = new QFormLayout(cueBox);
    pageCue_ = new QCheckBox(tr("显示右下角的 □"), cueBox);
    pageCue_->setChecked(true);
    pageCue_->setToolTip(tr("原型 A 只显示一个 □，不显示页码"));
    cueForm->addRow(QString(), pageCue_);
    addSpin(cueForm, tr("距右"), QStringLiteral("pageCueRight"), 0, 40, 7);
    addSpin(cueForm, tr("距底"), QStringLiteral("pageCueBottom"), 0, 40, 7);
    addSpin(cueForm, tr("字号"), QStringLiteral("pageCueFontSize"), 4, 24, 7);
    addSpin(cueForm, tr("不透明度"), QStringLiteral("pageCueAlpha"), 0, 255, 179,
            tr("CSS 为 70%"));
    addSpin(cueForm, tr("打字时不透明度百分比"), QStringLiteral("pageCueTypingAlphaPercent"),
            0, 100, 28, tr("CSS 为 opacity 0.28"));
    layout->addWidget(cueBox);

    auto *placeBox = new QGroupBox(tr("位置与打字"), this);
    auto *placeForm = new QFormLayout(placeBox);
    addSpin(placeForm, tr("面板右缘距角色右缘"), QStringLiteral("offsetRight"), -200, 200, 38,
            tr("CSS 的 right: 38px"));
    addSpin(placeForm, tr("面板下缘距角色下缘"), QStringLiteral("offsetBottom"), 0, 300, 90,
            tr("Qt 原型审核冻结为 bottom: 90px"));
    addSpin(placeForm, tr("距屏幕边缘最小距离"), QStringLiteral("screenMargin"), 0, 60, 8,
            tr("HTML 原型没有边缘避让，这项是为决策第 11.3 节新增的"));
    mirror_ = new QCheckBox(tr("靠近左缘时镜像到角色右上方"), placeBox);
    mirror_->setToolTip(tr("关闭则只做夹取。避让方式在决策第 13 节仍未确定"));
    placeForm->addRow(QString(), mirror_);
    addSpin(placeForm, tr("打字毫秒／字符"), QStringLiteral("typingMsPerChar"), 20, 30, 28,
            tr("HTML 原型为 28 ms，决策第 4.1 节限定 20–30 ms"));
    layout->addWidget(placeBox);

    auto *copy = new QPushButton(tr("复制当前参数，用于写回决策文档"), this);
    layout->addWidget(copy);
    layout->addStretch();

    for (QSpinBox *spin : std::as_const(spins_)) {
        connect(spin, &QSpinBox::valueChanged, this, [this] { emitParameters(); });
    }
    connect(antialias_, &QCheckBox::toggled, this, [this] { emitParameters(); });
    connect(pageCue_, &QCheckBox::toggled, this, [this] { emitParameters(); });
    connect(mirror_, &QCheckBox::toggled, this, [this] { emitParameters(); });
    connect(scale_, &QComboBox::currentIndexChanged, this, [this] { emitParameters(); });
    connect(dialogue_, &QComboBox::currentIndexChanged, this, [this] {
        emit dialogueRequested(dialogue_->currentData().toString());
    });
    connect(copy, &QPushButton::clicked, this,
            [this] { QApplication::clipboard()->setText(exportText()); });
}

QSpinBox *ControlWindow::addSpin(QFormLayout *form, const QString &label,
                                 const QString &key, const int minimum,
                                 const int maximum, const int value,
                                 const QString &hint)
{
    auto *spin = new QSpinBox(this);
    spin->setRange(minimum, maximum);
    spin->setValue(value);
    if (!hint.isEmpty()) {
        spin->setToolTip(hint);
    }
    form->addRow(label, spin);
    spins_.insert(key, spin);
    return spin;
}

BubbleParameters ControlWindow::parameters() const
{
    BubbleParameters p;
    const auto value = [this](const QString &key, const int fallback) {
        const auto it = spins_.constFind(key);
        return it != spins_.constEnd() ? (*it)->value() : fallback;
    };
    p.scale = scale_->currentData().toDouble();

    p.panelWidth = value(QStringLiteral("panelWidth"), 260);
    p.panelMinHeight = value(QStringLiteral("panelMinHeight"), 78);
    p.paddingTop = value(QStringLiteral("paddingTop"), 13);
    p.paddingRight = value(QStringLiteral("paddingRight"), 17);
    p.paddingBottom = value(QStringLiteral("paddingBottom"), 17);
    p.paddingLeft = value(QStringLiteral("paddingLeft"), 72);
    p.cornerRadius = value(QStringLiteral("cornerRadius"), 1);
    p.panelAlpha = value(QStringLiteral("panelAlpha"), 219);
    p.borderAlpha = value(QStringLiteral("borderAlpha"), 41);

    p.portraitWidth = value(QStringLiteral("portraitWidth"), 60);
    p.portraitHeight = value(QStringLiteral("portraitHeight"), 72);
    p.portraitLeft = value(QStringLiteral("portraitLeft"), 6);
    p.portraitBottom = value(QStringLiteral("portraitBottom"), 3);
    p.separatorLeft = value(QStringLiteral("separatorLeft"), 66);
    p.separatorInset = value(QStringLiteral("separatorInset"), 8);
    p.separatorAlpha = value(QStringLiteral("separatorAlpha"), 36);

    p.fontPixelSize = value(QStringLiteral("fontPixelSize"), 12);
    p.lineHeightPermille = value(QStringLiteral("lineHeightPermille"), 1620);
    p.textMinHeight = value(QStringLiteral("textMinHeight"), 46);
    p.antialiasText = antialias_->isChecked();

    p.showPageCue = pageCue_->isChecked();
    p.pageCueRight = value(QStringLiteral("pageCueRight"), 7);
    p.pageCueBottom = value(QStringLiteral("pageCueBottom"), 7);
    p.pageCueFontSize = value(QStringLiteral("pageCueFontSize"), 7);
    p.pageCueAlpha = value(QStringLiteral("pageCueAlpha"), 179);
    p.pageCueTypingAlphaPercent =
        value(QStringLiteral("pageCueTypingAlphaPercent"), 28);

    p.offsetRight = value(QStringLiteral("offsetRight"), 38);
    p.offsetBottom = value(QStringLiteral("offsetBottom"), 90);
    p.screenMargin = value(QStringLiteral("screenMargin"), 8);
    p.mirrorNearEdge = mirror_->isChecked();
    p.typingMsPerChar = value(QStringLiteral("typingMsPerChar"), 28);
    return p;
}

void ControlWindow::setPageStatus(const int lineCount, const int maxLines,
                                  const bool overflows, const int pageNumber,
                                  const int pageCount)
{
    const QString base = tr("第 %1／%2 页，折行 %3 行，面板可容纳 %4 行")
                             .arg(pageNumber)
                             .arg(pageCount)
                             .arg(lineCount)
                             .arg(maxLines);
    status_->setText(overflows ? base + tr("\n超出可容纳行数，该页需要人工再分页")
                               : base);
}

void ControlWindow::emitParameters()
{
    emit parametersChanged(parameters());
}

QString ControlWindow::exportText() const
{
    const BubbleParameters p = parameters();
    const auto yesNo = [](const bool value) {
        return value ? QStringLiteral("是") : QStringLiteral("否");
    };
    return QStringLiteral(
               "## 气泡原型 A 冻结参数\n"
               "\n"
               "长度单位为未经倍率放大的基准像素，与角色一同按显示倍率缩放。\n"
               "结构不可改，见 docs/Decisions.md 第 4 节。\n"
               "\n"
               "### 面板\n"
               "\n"
               "- 固定宽度：%1（不随文字长短变化）\n"
               "- 最小高度：%2\n"
               "- 内边距：上 %3、右 %4、下 %5、左 %6\n"
               "- 圆角半径：%7\n"
               "- 底色：`rgb(%8, %9, %10)`，不透明度 %11\n"
               "- 单像素边界不透明度：%12\n"
               "\n"
               "### 表情与分隔线\n"
               "\n"
               "- 表情显示尺寸：%13 x %14\n"
               "- 表情距面板左 %15、距面板底 %16\n"
               "- 竖分隔线位置 %17，上下各内缩 %18，不透明度 %19\n"
               "\n"
               "### 文字\n"
               "\n"
               "- 像素字号：%20\n"
               "- 行高：字号的 %21 倍\n"
               "- 文字区最小高度：%22\n"
               "- 文字抗锯齿：%23\n"
               "- 打字速度：%24 ms／字符\n"
               "\n"
               "### 翻页提示\n"
               "\n"
               "- 显示：%25（原型 A 只显示一个 □，不显示页码）\n"
               "- 距右 %26、距底 %27，字号 %28，不透明度 %29\n"
               "- 打字过程中不透明度降为 %30%\n"
               "\n"
               "### 位置\n"
               "\n"
               "- 显示倍率：%31x\n"
               "- 面板右缘距角色右缘：%32\n"
               "- 面板下缘距角色下缘：%33\n"
               "- 距屏幕边缘最小距离：%34\n"
               "- 靠近左缘时镜像到角色右上方：%35\n"
               "- 角色绘制在气泡之上\n")
        .arg(p.panelWidth)
        .arg(p.panelMinHeight)
        .arg(p.paddingTop)
        .arg(p.paddingRight)
        .arg(p.paddingBottom)
        .arg(p.paddingLeft)
        .arg(p.cornerRadius)
        .arg(p.panelRed)
        .arg(p.panelGreen)
        .arg(p.panelBlue)
        .arg(p.panelAlpha)
        .arg(p.borderAlpha)
        .arg(p.portraitWidth)
        .arg(p.portraitHeight)
        .arg(p.portraitLeft)
        .arg(p.portraitBottom)
        .arg(p.separatorLeft)
        .arg(p.separatorInset)
        .arg(p.separatorAlpha)
        .arg(p.fontPixelSize)
        .arg(p.lineHeightPermille / 1000.0)
        .arg(p.textMinHeight)
        .arg(yesNo(p.antialiasText))
        .arg(p.typingMsPerChar)
        .arg(yesNo(p.showPageCue))
        .arg(p.pageCueRight)
        .arg(p.pageCueBottom)
        .arg(p.pageCueFontSize)
        .arg(p.pageCueAlpha)
        .arg(p.pageCueTypingAlphaPercent)
        .arg(p.scale)
        .arg(p.offsetRight)
        .arg(p.offsetBottom)
        .arg(p.screenMargin)
        .arg(yesNo(p.mirrorNearEdge));
}

} // namespace mub::bubbleprobe
