#include "core/AssetPaths.h"

#include <QCoreApplication>
#include <QDir>

namespace mub::core {

QString assetRootPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("assets"));
}

QString assetFilePath(const QStringView relativePath)
{
    return QDir(assetRootPath()).filePath(relativePath.toString());
}

} // namespace mub::core
