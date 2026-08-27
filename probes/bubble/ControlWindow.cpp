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
    scale_->addItem(QStringLiteral("1.5x  候选"), 1.5);
    scale_->addItem(QStringLiteral("2x"), 2.0);
    scale_->setCurrentIndex(2);
    contentForm->addRow(tr("显示倍率"), scale_);
    status_ = new QLabel(contentBox);
    status_->setWordWrap(true);
    contentForm->addRow(tr("当前页"), status_);
    layout->addWidget(contentBox);

    auto *textBox = new QGroupBox(tr("文字"), this);
    auto *textForm = new QFormLayout(textBox);
    addSpin(textForm, tr("像素字号"), QStringLiteral("fontPixelSize"), 8, 48, 12,
            tr("Ark Pixel 为 12px 点阵字体，非整数倍会破坏栅格"));
    addSpin(textForm, tr("额外行距"), QStringLiteral("extraLineSpacing"), 0, 24, 2);
    addSpin(textForm, tr("文字区最大宽度"), QStringLiteral("maxTextWidth"), 60, 480, 180);
    addSpin(textForm, tr("每页最多行数"), QStringLiteral("maxLinesPerPage"), 1, 8, 3);
    antialias_ = new QCheckBox(tr("文字抗锯齿"), textBox);
    antialias_->setToolTip(tr("像素字体通常应关闭，打开只为对比差别"));
    textForm->addRow(QString(), antialias_);
    layout->addWidget(textBox);

    auto *panelBox = new QGroupBox(tr("面板"), this);
    auto *panelForm = new QFormLayout(panelBox);
    addSpin(panelForm, tr("水平内边距"), QStringLiteral("paddingHorizontal"), 0, 40, 8);
    addSpin(panelForm, tr("垂直内边距"), QStringLiteral("paddingVertical"), 0, 40, 6);
    addSpin(panelForm, tr("表情与分隔线间隔"), QStringLiteral("faceGap"), 0, 40, 6);
    addSpin(panelForm, tr("表情宽"), QStringLiteral("faceWidth"), 24, 240, 120);
    addSpin(panelForm, tr("表情高"), QStringLiteral("faceHeight"), 24, 288, 144);
    addSpin(panelForm, tr("面板不透明度"), QStringLiteral("panelAlpha"), 0, 255, 190);
    addSpin(panelForm, tr("边界不透明度"), QStringLiteral("borderAlpha"), 0, 255, 60);
    addSpin(panelForm, tr("分隔线不透明度"), QStringLiteral("separatorAlpha"), 0, 255, 45);
    addSpin(panelForm, tr("圆角半径"), QStringLiteral("cornerRadius"), 0, 8, 1,
            tr("决策要求近直角，0 为纯直角"));
    pageIndicator_ = new QCheckBox(tr("显示翻页提示"), panelBox);
    pageIndicator_->setChecked(true);
    panelForm->addRow(QString(), pageIndicator_);
    layout->addWidget(panelBox);

    auto *placeBox = new QGroupBox(tr("位置与打字"), this);
    auto *placeForm = new QFormLayout(placeBox);
    addSpin(placeForm, tr("与角色间隔"), QStringLiteral("gapToCharacter"), 0, 60, 4);
    addSpin(placeForm, tr("距屏幕边缘最小距离"), QStringLiteral("screenMargin"), 0, 60, 8);
    addSpin(placeForm, tr("打字毫秒／字符"), QStringLiteral("typingMsPerChar"), 20, 30, 25,
            tr("决策第 4.1 节限定 20–30 ms"));
    layout->addWidget(placeBox);

    auto *copy = new QPushButton(tr("复制当前参数，用于写回决策文档"), this);
    layout->addWidget(copy);
    layout->addStretch();

    for (QSpinBox *spin : std::as_const(spins_)) {
        connect(spin, &QSpinBox::valueChanged, this, [this] { emitParameters(); });
    }
    connect(antialias_, &QCheckBox::toggled, this, [this] { emitParameters(); });
    connect(pageIndicator_, &QCheckBox::toggled, this, [this] { emitParameters(); });
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
    p.fontPixelSize = value(QStringLiteral("fontPixelSize"), 12);
    p.antialiasText = antialias_->isChecked();
    p.extraLineSpacing = value(QStringLiteral("extraLineSpacing"), 2);
    p.maxTextWidth = value(QStringLiteral("maxTextWidth"), 180);
    p.maxLinesPerPage = value(QStringLiteral("maxLinesPerPage"), 3);
    p.paddingHorizontal = value(QStringLiteral("paddingHorizontal"), 8);
    p.paddingVertical = value(QStringLiteral("paddingVertical"), 6);
    p.faceGap = value(QStringLiteral("faceGap"), 6);
    p.faceWidth = value(QStringLiteral("faceWidth"), 120);
    p.faceHeight = value(QStringLiteral("faceHeight"), 144);
    p.panelAlpha = value(QStringLiteral("panelAlpha"), 190);
    p.borderAlpha = value(QStringLiteral("borderAlpha"), 60);
    p.separatorAlpha = value(QStringLiteral("separatorAlpha"), 45);
    p.cornerRadius = value(QStringLiteral("cornerRadius"), 1);
    p.gapToCharacter = value(QStringLiteral("gapToCharacter"), 4);
    p.screenMargin = value(QStringLiteral("screenMargin"), 8);
    p.showPageIndicator = pageIndicator_->isChecked();
    p.typingMsPerChar = value(QStringLiteral("typingMsPerChar"), 25);
    return p;
}

void ControlWindow::setPageStatus(const int lineCount, const bool overflows)
{
    status_->setText(overflows
                         ? tr("折行 %1 行，**超出每页上限**，该页需要人工再分页")
                               .arg(lineCount)
                         : tr("折行 %1 行").arg(lineCount));
}

void ControlWindow::emitParameters()
{
    emit parametersChanged(parameters());
}

QString ControlWindow::exportText() const
{
    const BubbleParameters p = parameters();
    return QStringLiteral(
               "气泡原型 A 冻结参数（写回 docs/Decisions.md 第 4 节）\n"
               "\n"
               "- 显示倍率：%1x\n"
               "- 像素字号：%2\n"
               "- 文字抗锯齿：%3\n"
               "- 额外行距：%4\n"
               "- 文字区最大宽度：%5\n"
               "- 每页最多行数：%6\n"
               "- 内边距：水平 %7、垂直 %8\n"
               "- 表情与分隔线间隔：%9\n"
               "- 表情显示尺寸：%10 x %11\n"
               "- 面板不透明度：%12\n"
               "- 边界不透明度：%13\n"
               "- 分隔线不透明度：%14\n"
               "- 圆角半径：%15\n"
               "- 与角色间隔：%16\n"
               "- 距屏幕边缘最小距离：%17\n"
               "- 显示翻页提示：%18\n"
               "- 打字速度：%19 ms／字符\n"
               "\n"
               "长度单位为未经倍率放大的基准像素。\n")
        .arg(p.scale)
        .arg(p.fontPixelSize)
        .arg(p.antialiasText ? QStringLiteral("开") : QStringLiteral("关"))
        .arg(p.extraLineSpacing)
        .arg(p.maxTextWidth)
        .arg(p.maxLinesPerPage)
        .arg(p.paddingHorizontal)
        .arg(p.paddingVertical)
        .arg(p.faceGap)
        .arg(p.faceWidth)
        .arg(p.faceHeight)
        .arg(p.panelAlpha)
        .arg(p.borderAlpha)
        .arg(p.separatorAlpha)
        .arg(p.cornerRadius)
        .arg(p.gapToCharacter)
        .arg(p.screenMargin)
        .arg(p.showPageIndicator ? QStringLiteral("是") : QStringLiteral("否"))
        .arg(p.typingMsPerChar);
}

} // namespace mub::bubbleprobe
