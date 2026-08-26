#include "character/HitMask.h"

#include <QImage>
#include <QRect>

#include <algorithm>

namespace mub::character {

QRegion opaqueRegion(const QImage &frame, const int integerScale,
                     const int alphaThreshold)
{
    if (frame.isNull() || integerScale <= 0) {
        return {};
    }

    const int threshold = std::clamp(alphaThreshold, 1, 255);
    const QImage source = frame.format() == QImage::Format_ARGB32
        ? frame
        : frame.convertToFormat(QImage::Format_ARGB32);

    // 逐行扫描连续的不透明区间。单帧只有 69 x 111 像素，
    // 直接扫描即可，不需要额外的几何简化。
    QRegion region;
    for (int y = 0; y < source.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        int runStart = -1;
        for (int x = 0; x <= source.width(); ++x) {
            const bool opaque =
                x < source.width() && qAlpha(line[x]) >= threshold;
            if (opaque && runStart < 0) {
                runStart = x;
            } else if (!opaque && runStart >= 0) {
                region += QRect(runStart * integerScale, y * integerScale,
                                (x - runStart) * integerScale, integerScale);
                runStart = -1;
            }
        }
    }
    return region;
}

} // namespace mub::character
