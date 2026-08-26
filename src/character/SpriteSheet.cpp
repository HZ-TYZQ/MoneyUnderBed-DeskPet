#include "character/SpriteSheet.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QRect>

namespace mub::character {

namespace {

void assign(SpriteSheetError *slot, const SpriteSheetError value)
{
    if (slot != nullptr) {
        *slot = value;
    }
}

} // namespace

QString describeSpriteSheetError(const SpriteSheetError error)
{
    const auto translate = [](const char *text) {
        return QCoreApplication::translate("mub::character", text);
    };

    switch (error) {
    case SpriteSheetError::None:
        return translate("没有错误");
    case SpriteSheetError::FileMissing:
        return translate("精灵表文件不存在");
    case SpriteSheetError::DecodeFailed:
        return translate("精灵表文件无法解码为图像");
    case SpriteSheetError::WrongFrameHeight:
        return translate("精灵表高度不等于单帧高度");
    case SpriteSheetError::WidthNotFrameMultiple:
        return translate("精灵表宽度不是单帧宽度的整数倍");
    case SpriteSheetError::NoFrames:
        return translate("精灵表宽度不足一帧");
    case SpriteSheetError::NoAlphaChannel:
        return translate("精灵表没有透明通道");
    }
    return translate("未知错误");
}

SpriteSheet SpriteSheet::load(const QString &path, SpriteSheetError *error)
{
    if (!QFileInfo::exists(path)) {
        assign(error, SpriteSheetError::FileMissing);
        return {};
    }

    QImage image;
    if (!image.load(path)) {
        assign(error, SpriteSheetError::DecodeFailed);
        return {};
    }
    return fromImage(image, error);
}

SpriteSheet SpriteSheet::fromImage(const QImage &image, SpriteSheetError *error)
{
    if (image.isNull()) {
        assign(error, SpriteSheetError::DecodeFailed);
        return {};
    }
    if (image.height() != FrameHeight) {
        assign(error, SpriteSheetError::WrongFrameHeight);
        return {};
    }
    if (image.width() < FrameWidth) {
        assign(error, SpriteSheetError::NoFrames);
        return {};
    }
    if ((image.width() % FrameWidth) != 0) {
        assign(error, SpriteSheetError::WidthNotFrameMultiple);
        return {};
    }
    // 没有 alpha 就无法实现可见像素命中与透明区域穿透
    // （docs/Decisions.md 第 3.4 节），因此直接判为非法素材。
    if (!image.hasAlphaChannel()) {
        assign(error, SpriteSheetError::NoAlphaChannel);
        return {};
    }

    SpriteSheet sheet;
    // 统一转成带 alpha 的固定格式，使后续取帧和掩码计算不依赖源文件格式。
    sheet.image_ = image.convertToFormat(QImage::Format_ARGB32);
    sheet.frameCount_ = image.width() / FrameWidth;
    assign(error, SpriteSheetError::None);
    return sheet;
}

bool SpriteSheet::isValid() const
{
    return frameCount_ > 0 && !image_.isNull();
}

int SpriteSheet::frameCount() const
{
    return frameCount_;
}

QSize SpriteSheet::frameSize() const
{
    return {FrameWidth, FrameHeight};
}

QImage SpriteSheet::frame(const int index) const
{
    if (index < 0 || index >= frameCount_) {
        return {};
    }
    return image_.copy(QRect(index * FrameWidth, 0, FrameWidth, FrameHeight));
}

const QImage &SpriteSheet::image() const
{
    return image_;
}

} // namespace mub::character
