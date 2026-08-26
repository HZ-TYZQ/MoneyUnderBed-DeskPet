#pragma once

#include <QImage>
#include <QSize>
#include <QString>

namespace mub::character {

// 精灵表校验失败的原因。
// 这些值参与自检退出码判定，因此是稳定契约，不要随意改动顺序。
enum class SpriteSheetError
{
    None,
    FileMissing,        // 文件不存在
    DecodeFailed,       // 文件存在但不是可解码的图像
    WrongFrameHeight,   // 帧高不等于 111
    WidthNotFrameMultiple, // 宽度不是帧宽 69 的整数倍
    NoFrames,           // 宽度不足一帧
    NoAlphaChannel,     // 没有透明通道，无法做像素级命中与穿透
};

QString describeSpriteSheetError(SpriteSheetError error);

// 一张按固定帧尺寸横向排布的角色精灵表。
//
// 帧尺寸由作者素材决定，见 docs/Decisions.md 第 6 节与 assets/MANIFEST.md。
// 本类只做校验与取帧，不决定播放哪一帧，也不决定角色行为。
class SpriteSheet
{
public:
    static constexpr int FrameWidth = 69;
    static constexpr int FrameHeight = 111;

    SpriteSheet() = default;

    // 从文件加载并校验。失败时返回无效对象，`error` 给出原因。
    static SpriteSheet load(const QString &path, SpriteSheetError *error = nullptr);

    // 从已有图像校验。用于测试和内存中的素材。
    static SpriteSheet fromImage(const QImage &image, SpriteSheetError *error = nullptr);

    bool isValid() const;
    int frameCount() const;
    QSize frameSize() const;

    // 取第 index 帧。index 越界时返回空图像。
    QImage frame(int index) const;

    const QImage &image() const;

private:
    QImage image_;
    int frameCount_ = 0;
};

} // namespace mub::character
