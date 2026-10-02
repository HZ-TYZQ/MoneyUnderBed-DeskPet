#include "app/UiLanguage.h"

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTranslator>

namespace mub::app {

namespace {

Q_LOGGING_CATEGORY(lcLanguage, "mub.app.language")

} // namespace

bool prefersChineseUi(const QStringList &uiLanguages)
{
    if (uiLanguages.isEmpty()) {
        return false;
    }
    // 形如 zh、zh-CN、zh_TW、zh-Hans-CN。
    const QString first = uiLanguages.constFirst();
    return first.compare(QStringLiteral("zh"), Qt::CaseInsensitive) == 0
        || first.startsWith(QStringLiteral("zh-"), Qt::CaseInsensitive)
        || first.startsWith(QStringLiteral("zh_"), Qt::CaseInsensitive);
}

std::unique_ptr<QTranslator> installUiTranslator(const QStringList &uiLanguages)
{
    if (prefersChineseUi(uiLanguages)) {
        qCInfo(lcLanguage) << "ui language: zh (source text)" << uiLanguages;
        return nullptr;
    }

    auto translator = std::make_unique<QTranslator>();
    const QString path = QStringLiteral(MUB_UI_TRANSLATION_RESOURCE);
    if (!translator->load(path)) {
        // 译文随程序编进资源，加载失败说明构建有问题。仍可用中文源文本继续运行。
        qCCritical(lcLanguage).noquote()
            << QStringLiteral("could not load the English ui translation: %1").arg(path);
        return nullptr;
    }
    QCoreApplication::installTranslator(translator.get());
    qCInfo(lcLanguage) << "ui language: en" << uiLanguages;
    return translator;
}

} // namespace mub::app
