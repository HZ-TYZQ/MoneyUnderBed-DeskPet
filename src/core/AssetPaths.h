#pragma once

#include <QString>
#include <QStringView>

namespace mub::core {

// 角色素材作为独立数据文件放在可执行文件旁的 assets/ 目录。
// 开发构建、Windows ZIP 和 AppImage 使用同一套定位规则。
QString assetRootPath();
QString assetFilePath(QStringView relativePath);

} // namespace mub::core
