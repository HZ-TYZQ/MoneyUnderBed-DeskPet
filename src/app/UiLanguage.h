#pragma once

#include <QLocale>
#include <QStringList>

#include <memory>

class QTranslator;

namespace mub::app {

// 界面语言。
//
// docs/Decisions.md 第 2 节：界面文本的源文本是简体中文。系统首选界面语言为中文时
// 直接显示源文本；其他任何语言，包括 C locale 与未设置语言，一律显示英文。
// 不提供语言设置项。角色台词不属于界面文本，始终显示中文原文。

// 首选界面语言是否为中文。只看第一个首选项：用户把英文排在中文前面就用英文。
bool prefersChineseUi(const QStringList &uiLanguages);

// 按首选界面语言安装界面翻译。中文不需要翻译，返回空指针；其他语言安装英文
// 译文并返回它，调用方必须让返回的对象活到不再显示任何界面为止。
// 必须在应用对象构造之后、创建任何界面之前调用。
std::unique_ptr<QTranslator> installUiTranslator(
    const QStringList &uiLanguages = QLocale::system().uiLanguages());

} // namespace mub::app
