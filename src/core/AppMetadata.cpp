#include "core/AppMetadata.h"

#include "mub/Version.h"

#include <QCoreApplication>
#include <QLatin1String>

namespace mub::metadata {

namespace {

// 界面文本一律走翻译，不硬编码（docs/Decisions.md 第 5.1 节）。
// 第一版不提供语言设置，但资源结构保持可加载翻译文件。
QString translate(const char *sourceText)
{
    return QCoreApplication::translate("mub::metadata", sourceText);
}

} // namespace

QString applicationName()
{
    return QStringLiteral(MUB_APPLICATION_NAME);
}

QString executableName()
{
    return QStringLiteral(MUB_EXECUTABLE_NAME);
}

QString applicationId()
{
    return QStringLiteral(MUB_APPLICATION_ID);
}

QString organizationName()
{
    return QStringLiteral(MUB_ORGANIZATION_NAME);
}

QString organizationDomain()
{
    return QStringLiteral(MUB_ORGANIZATION_DOMAIN);
}

QString homepageUrl()
{
    return QStringLiteral(MUB_HOMEPAGE_URL);
}

QString versionString()
{
    const QString base = QStringLiteral(MUB_VERSION_STRING);
    const QString suffix = QStringLiteral(MUB_VERSION_SUFFIX);
    if (suffix.isEmpty()) {
        return base;
    }
    return base + QLatin1Char('-') + suffix;
}

QString displayName()
{
    return translate("《床下有罐钱》非官方桌宠");
}

QString unofficialNotice()
{
    return translate(
        "本程序是非官方、非商业的二次创作项目，与《床下有罐钱》的开发者没有隶属关系，"
        "也不由其发布或背书。");
}

void applyTo(QCoreApplication &application)
{
    Q_UNUSED(application)

    QCoreApplication::setApplicationName(applicationName());
    QCoreApplication::setApplicationVersion(versionString());
    QCoreApplication::setOrganizationName(organizationName());
    QCoreApplication::setOrganizationDomain(organizationDomain());
}

} // namespace mub::metadata
