#pragma once

#include <QString>
#include <QStringView>

#include <span>

namespace mub::dialogue {

// 台词来源。docs/Decisions.md 第 4.3 节要求原作 demo 台词与项目新增文案
// 分组保存，逐条标记来源，不能混为同一来源。
enum class LineSource
{
    OriginalDemo,     // 原作 demo 直接收录
    ProjectAuthored,  // 项目新增
};

// 一个显示页面。
//
// docs/Decisions.md 第 4 节：所有显示台词的页面都必须同时显示该页经过
// 人工审核的对应表情。因此表情不是可选项，每页都有。
struct DialoguePage
{
    // UTF-8 文本。构建时以 /utf-8 与 -finput-charset 保证两平台一致。
    const char8_t *text;
    // 表情标识，对应 assets/face/<faceId>.png。
    const char *faceId;
};

// 一段对话。可能只有一页，也可能是需要点击推进的连续对话。
struct Dialogue
{
    const char *id;
    LineSource source;
    // 触发场景，原文照录自决策文档，供审查与后续接线使用。
    const char8_t *trigger;
    // 这段对话由几条来源台词构成。
    // 冰淇淋掉落是 4 条来源台词组成的一段连续对话，其余都是 1 条。
    int sourceLineCount;
    std::span<const DialoguePage> pages;
};

std::span<const Dialogue> registeredDialogues();

// 按标识查找。找不到时返回 nullptr。
const Dialogue *findDialogue(QStringView id);

// 常规表情池，不含 `shadow`（docs/Decisions.md 第 4.6 节）。
std::span<const char *const> regularFaceIds();
bool isRegularFace(QStringView faceId);

QString faceResourcePath(QStringView faceId);

// 全部来源台词条数与显示页面数。决策文档第 4.5 节声明为 29 与 36。
int totalSourceLineCount();
int totalPageCount();

} // namespace mub::dialogue
