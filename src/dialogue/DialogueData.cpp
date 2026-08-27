#include "dialogue/DialogueData.h"

#include "core/AssetPaths.h"

#include <QLatin1String>

#include <array>

namespace mub::dialogue {

namespace {

// 本文件的内容由 docs/Decisions.md 第 4.4 与 4.5 节逐条转写而来。
// tests/dialogue/tst_dialoguedata 会把每一页文本回查决策文档，
// 因此两边任何一侧改动而另一侧没跟上，测试都会失败。
//
// 分页与表情映射是内容数据的一部分，不在运行时按字数、标点或文件名
// 自动生成（docs/Decisions.md 第 4 节）。

constexpr std::array<DialoguePage, 4> kIcecreamDropPages{{
    {u8"我刚刚掉了甜筒……", "panic"},
    {u8"可恶，一定是这个家伙害我掉了甜筒！", "impatient"},
    {u8"我已经没有钱买第二个了……", "serious-eyes-closed"},
    {u8"你要赔我！", "impatient"},
}};
constexpr std::array<DialoguePage, 1> kOriginal01Pages{{
    {u8"不管啦，能出多大事呢，今天可以三顿都吃零食啦！", "proud"},
}};
constexpr std::array<DialoguePage, 1> kOriginal02Pages{{
    {u8"我才不穿校服。", "serious-eyes-open"},
}};
constexpr std::array<DialoguePage, 1> kOriginal03Pages{{
    {u8"哎呀，不用这么麻烦，直接给我零花钱就可以啦。", "proud-catmouth"},
}};
constexpr std::array<DialoguePage, 1> kOriginal04Pages{{
    {u8"啊！作业给我留一份，我还要抄！", "happy"},
}};
constexpr std::array<DialoguePage, 1> kAuthored01Pages{{
    {u8"零食放着也是会过期的，我现在吃完不是正好吗？", "proud-thumb"},
}};
constexpr std::array<DialoguePage, 2> kAuthored02Pages{{
    {u8"刚刚是不是有什么事来着？", "natural-lower-eyes-brow"},
    {u8"算啦，应该不重要！", "proud-catmouth"},
}};
constexpr std::array<DialoguePage, 1> kAuthored03Pages{{
    {u8"你找我？有零食吗？", "natural"},
}};
constexpr std::array<DialoguePage, 1> kAuthored04Pages{{
    {u8"还有吗？我觉得我还能再吃一个！", "happy"},
}};
constexpr std::array<DialoguePage, 1> kAuthored05Pages{{
    {u8"咦？我刚刚不是还在那边吗？", "natural-lower-eyes-brow"},
}};
constexpr std::array<DialoguePage, 1> kAuthored06Pages{{
    {u8"你在忙什么？给我也看看！", "happy"},
}};
constexpr std::array<DialoguePage, 2> kAuthored07Pages{{
    {u8"先休息一下吧", "serious-eyes-closed"},
    {u8"反正麻烦又不会自己跑掉。", "proud-catmouth"},
}};
constexpr std::array<DialoguePage, 2> kAuthored08Pages{{
    {u8"我好像刚吃过零食……", "natural-lower-eyes-brow"},
    {u8"不过那是刚刚的事啦！", "proud-catmouth"},
}};
constexpr std::array<DialoguePage, 3> kAuthored09Pages{{
    {u8"甜筒怎么自己掉下去了？", "panic"},
    {u8"它也太不小心了吧！", "impatient"},
    {u8"那只能再买一支啦～", "proud-catmouth"},
}};
constexpr std::array<DialoguePage, 2> kAuthored10Pages{{
    {u8"我刚刚想到一个好办法！", "proud-thumb"},
    {u8"……等一下，是什么来着？", "natural-lower-eyes-brow"},
}};
constexpr std::array<DialoguePage, 1> kAuthored11Pages{{
    {u8"那边好像有好吃的，我去看看！", "happy"},
}};
constexpr std::array<DialoguePage, 1> kAuthored12Pages{{
    {u8"咦，什么都没有……难道是我看错了？", "natural-lower-eyes-brow"},
}};
constexpr std::array<DialoguePage, 1> kAuthored13Pages{{
    {u8"你也没事做吗？那正好，一起偷懒吧！", "proud"},
}};
constexpr std::array<DialoguePage, 1> kAuthored14Pages{{
    {u8"好累啊……", "serious-eyes-closed"},
}};
constexpr std::array<DialoguePage, 2> kAuthored15Pages{{
    {u8"今天应该不会有什么麻烦吧？", "natural-lower-eyes-brow"},
    {u8"算啦，有了再说！", "proud-catmouth"},
}};
constexpr std::array<DialoguePage, 1> kAuthored16Pages{{
    {u8"零食？！", "happy"},
}};
constexpr std::array<DialoguePage, 1> kAuthored17Pages{{
    {u8"今天吃什么好呢……", "natural-lower-eyes-brow"},
}};
constexpr std::array<DialoguePage, 1> kAuthored18Pages{{
    {u8"咦，要去哪儿？", "panic"},
}};
constexpr std::array<DialoguePage, 1> kAuthored19Pages{{
    {u8"怎么啦？", "natural"},
}};
constexpr std::array<DialoguePage, 1> kAuthored20Pages{{
    {u8"好吃！", "happy"},
}};
constexpr std::array<DialoguePage, 1> kAuthored21Pages{{
    {u8"甜筒最好吃了！", "proud"},
}};

constexpr std::array<Dialogue, 26> kDialogues{{
    {"icecream-drop", LineSource::OriginalDemo, u8"冰淇淋掉落事件", 4,
     {kIcecreamDropPages.data(), kIcecreamDropPages.size()}},
    {"original-01", LineSource::OriginalDemo, u8"活跃模式随机闲聊", 1,
     {kOriginal01Pages.data(), kOriginal01Pages.size()}},
    {"original-02", LineSource::OriginalDemo, u8"单击或低频闲聊", 1,
     {kOriginal02Pages.data(), kOriginal02Pages.size()}},
    {"original-03", LineSource::OriginalDemo, u8"低概率单击反馈；不关联任何付费入口", 1,
     {kOriginal03Pages.data(), kOriginal03Pages.size()}},
    {"original-04", LineSource::OriginalDemo, u8"低概率单击反馈", 1,
     {kOriginal04Pages.data(), kOriginal04Pages.size()}},
    {"authored-01", LineSource::ProjectAuthored, u8"活跃模式闲聊", 1,
     {kAuthored01Pages.data(), kAuthored01Pages.size()}},
    {"authored-02", LineSource::ProjectAuthored, u8"待机闲聊", 1,
     {kAuthored02Pages.data(), kAuthored02Pages.size()}},
    {"authored-03", LineSource::ProjectAuthored, u8"单击反馈", 1,
     {kAuthored03Pages.data(), kAuthored03Pages.size()}},
    {"authored-04", LineSource::ProjectAuthored, u8"投喂完成", 1,
     {kAuthored04Pages.data(), kAuthored04Pages.size()}},
    {"authored-05", LineSource::ProjectAuthored, u8"被拖动后", 1,
     {kAuthored05Pages.data(), kAuthored05Pages.size()}},
    {"authored-06", LineSource::ProjectAuthored, u8"活跃模式接近鼠标", 1,
     {kAuthored06Pages.data(), kAuthored06Pages.size()}},
    {"authored-07", LineSource::ProjectAuthored, u8"进入休息状态", 1,
     {kAuthored07Pages.data(), kAuthored07Pages.size()}},
    {"authored-08", LineSource::ProjectAuthored, u8"活跃模式闲聊", 1,
     {kAuthored08Pages.data(), kAuthored08Pages.size()}},
    {"authored-09", LineSource::ProjectAuthored, u8"冰淇淋掉落的替代反应", 1,
     {kAuthored09Pages.data(), kAuthored09Pages.size()}},
    {"authored-10", LineSource::ProjectAuthored, u8"待机闲聊", 1,
     {kAuthored10Pages.data(), kAuthored10Pages.size()}},
    {"authored-11", LineSource::ProjectAuthored, u8"开始自主行走", 1,
     {kAuthored11Pages.data(), kAuthored11Pages.size()}},
    {"authored-12", LineSource::ProjectAuthored, u8"文案 11 对应的抵达反馈", 1,
     {kAuthored12Pages.data(), kAuthored12Pages.size()}},
    {"authored-13", LineSource::ProjectAuthored, u8"单击反馈", 1,
     {kAuthored13Pages.data(), kAuthored13Pages.size()}},
    {"authored-14", LineSource::ProjectAuthored, u8"行走结束", 1,
     {kAuthored14Pages.data(), kAuthored14Pages.size()}},
    {"authored-15", LineSource::ProjectAuthored, u8"活跃模式闲聊", 1,
     {kAuthored15Pages.data(), kAuthored15Pages.size()}},
    {"authored-16", LineSource::ProjectAuthored, u8"开始投喂", 1,
     {kAuthored16Pages.data(), kAuthored16Pages.size()}},
    {"authored-17", LineSource::ProjectAuthored, u8"待机闲聊", 1,
     {kAuthored17Pages.data(), kAuthored17Pages.size()}},
    {"authored-18", LineSource::ProjectAuthored, u8"被拖动时", 1,
     {kAuthored18Pages.data(), kAuthored18Pages.size()}},
    {"authored-19", LineSource::ProjectAuthored, u8"高频单击反馈", 1,
     {kAuthored19Pages.data(), kAuthored19Pages.size()}},
    {"authored-20", LineSource::ProjectAuthored, u8"高频投喂完成反馈", 1,
     {kAuthored20Pages.data(), kAuthored20Pages.size()}},
    {"authored-21", LineSource::ProjectAuthored, u8"吃冰淇淋时", 1,
     {kAuthored21Pages.data(), kAuthored21Pages.size()}},
}};

// 常规表情池。docs/Decisions.md 第 4.6 节：`shadow` 是特殊剧情素材，
// 不进入第一版常规表情池，也不参与任何随机表情选择。
constexpr std::array<const char *, 10> kRegularFaces{{
    "happy",
    "impatient",
    "natural",
    "natural-lower-eyes-brow",
    "panic",
    "proud",
    "proud-catmouth",
    "proud-thumb",
    "serious-eyes-closed",
    "serious-eyes-open",
}};

} // namespace

std::span<const Dialogue> registeredDialogues()
{
    return {kDialogues.data(), kDialogues.size()};
}

const Dialogue *findDialogue(const QStringView id)
{
    for (const Dialogue &dialogue : kDialogues) {
        if (id == QLatin1String(dialogue.id)) {
            return &dialogue;
        }
    }
    return nullptr;
}

std::span<const char *const> regularFaceIds()
{
    return {kRegularFaces.data(), kRegularFaces.size()};
}

bool isRegularFace(const QStringView faceId)
{
    for (const char *face : kRegularFaces) {
        if (faceId == QLatin1String(face)) {
            return true;
        }
    }
    return false;
}

QString faceAssetPath(const QStringView faceId)
{
    return core::assetFilePath(QStringLiteral("face/") + faceId.toString()
                               + QStringLiteral(".png"));
}

int totalSourceLineCount()
{
    int total = 0;
    for (const Dialogue &dialogue : kDialogues) {
        total += dialogue.sourceLineCount;
    }
    return total;
}

int totalPageCount()
{
    int total = 0;
    for (const Dialogue &dialogue : kDialogues) {
        total += static_cast<int>(dialogue.pages.size());
    }
    return total;
}

} // namespace mub::dialogue
