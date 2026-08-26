#include "core/ScreenPlacement.h"

#include <algorithm>
#include <cmath>

namespace mub::core {

QPoint bottomAnchoredPosition(const QRect &availableGeometry,
                              const QSize &windowSize,
                              const double horizontalRatio,
                              const int bottomMargin)
{
    const double ratio = std::clamp(horizontalRatio, 0.0, 1.0);
    const int travel = std::max(0, availableGeometry.width() - windowSize.width());
    const int x = availableGeometry.x()
        + static_cast<int>(std::lround(ratio * static_cast<double>(travel)));
    const int y = availableGeometry.y() + availableGeometry.height()
        - windowSize.height() - bottomMargin;
    return clampToAvailable(availableGeometry, windowSize, QPoint(x, y));
}

QPoint clampToAvailable(const QRect &availableGeometry, const QSize &windowSize,
                        const QPoint &position)
{
    // 可用区域比窗口还小时，至少保证左上角对齐，不产生负向范围。
    const int maxX = std::max(availableGeometry.x(),
                              availableGeometry.x() + availableGeometry.width()
                                  - windowSize.width());
    const int maxY = std::max(availableGeometry.y(),
                              availableGeometry.y() + availableGeometry.height()
                                  - windowSize.height());
    return {std::clamp(position.x(), availableGeometry.x(), maxX),
            std::clamp(position.y(), availableGeometry.y(), maxY)};
}

int distanceFromBottom(const QRect &availableGeometry, const QSize &windowSize,
                       const QPoint &position)
{
    const int windowBottom = position.y() + windowSize.height();
    const int areaBottom = availableGeometry.y() + availableGeometry.height();
    return areaBottom - windowBottom;
}

} // namespace mub::core
