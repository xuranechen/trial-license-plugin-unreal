// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * @brief 试用水印Widget（Slate版本）
 *
 * 该类实现了在游戏视口上显示试用水印的功能，支持文本和图片两种水印模式。
 * 水印可以旋转、平铺显示，支持自定义字体、颜色、不透明度等属性。
 * 用于在试用版本中显示试用标识，提醒用户当前使用的是试用版本。
 */
class STrialWatermarkWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STrialWatermarkWidget)
        : _Text(FText::FromString(TEXT("Trial Version")))
        , _Opacity(0.12f)
        , _FontSize(48)
        , _Color(FLinearColor::White)
        , _AngleDegrees(-30.f)
        , _TileSpacingX(0.f) // 0 = 自动调整
        , _TileSpacingY(300.f)
        , _VerticalOffset(0.f)
        , _FontPath(TEXT(""))
        , _ImagePath(TEXT(""))
        , _ImageScale(1.0f)
        , _bImageOnly(false)
    {}
        /** @brief 水印文本内容 */
        SLATE_ATTRIBUTE(FText, Text)
        /** @brief 水印不透明度（0.0-1.0） */
        SLATE_ATTRIBUTE(float, Opacity)
        /** @brief 字体大小（像素） */
        SLATE_ATTRIBUTE(int32, FontSize)
        /** @brief 文本颜色 */
        SLATE_ATTRIBUTE(FLinearColor, Color)
        /** @brief 旋转角度（度） */
        SLATE_ATTRIBUTE(float, AngleDegrees)
        /** @brief 水平平铺间距（像素），0表示自动调整 */
        SLATE_ATTRIBUTE(float, TileSpacingX)
        /** @brief 垂直平铺间距（像素） */
        SLATE_ATTRIBUTE(float, TileSpacingY)
        /** @brief 垂直偏移量（像素） */
        SLATE_ATTRIBUTE(float, VerticalOffset)
        /** @brief 自定义字体文件路径 */
        SLATE_ATTRIBUTE(FString, FontPath)
        /** @brief 水印图片文件路径 */
        SLATE_ATTRIBUTE(FString, ImagePath)
        /** @brief 图片缩放比例 */
        SLATE_ATTRIBUTE(float, ImageScale)
        /** @brief 是否仅显示图片（不显示文本） */
        SLATE_ATTRIBUTE(bool, bImageOnly)
    SLATE_END_ARGS()

    /**
     * @brief 构造函数，初始化水印Widget
     *
     * 根据传入的参数初始化水印的显示属性。
     *
     * @param InArgs Slate构造参数，包含所有水印配置选项
     */
    void Construct(const FArguments& InArgs);

    /**
     * @brief 设置水印文本
     *
     * 更新水印显示的文本内容。
     *
     * @param InText 新的水印文本
     */
    void SetText(const FText& InText);

    /**
     * @brief 设置水印不透明度
     *
     * 更新水印的透明度级别。
     *
     * @param InOpacity 不透明度值（0.0-1.0）
     */
    void SetOpacity(float InOpacity);

    /**
     * @brief 设置字体大小
     *
     * 更新水印文本的字体大小。
     *
     * @param InSize 字体大小（像素）
     */
    void SetFontSize(int32 InSize);

    /**
     * @brief 设置文本颜色
     *
     * 更新水印文本的显示颜色。
     *
     * @param InColor 新的文本颜色
     */
    void SetColor(const FLinearColor& InColor);

    /**
     * @brief 设置旋转角度
     *
     * 更新水印的旋转角度。
     *
     * @param InAngleDegrees 旋转角度（度）
     */
    void SetAngle(float InAngleDegrees);

    /**
     * @brief 设置平铺间距
     *
     * 更新水印在X和Y方向上的平铺间距。
     *
     * @param InTileSpacingX X方向间距（像素），0表示自动调整
     * @param InTileSpacingY Y方向间距（像素）
     */
    void SetTileSpacing(float InTileSpacingX, float InTileSpacingY);

    /**
     * @brief 设置垂直偏移量
     *
     * 更新水印在垂直方向上的偏移量。
     *
     * @param InVerticalOffset 垂直偏移量（像素），正值向下，负值向上
     */
    void SetVerticalOffset(float InVerticalOffset);

    /**
     * @brief 绘制水印内容
     *
     * 重写父类方法，实现水印的绘制逻辑，包括文本和图片的渲染。
     *
     * @param Args 绘制参数
     * @param AllottedGeometry 分配的几何空间
     * @param MyCullingRect 裁剪矩形
     * @param OutDrawElements 输出绘制元素列表
     * @param LayerId 图层ID
     * @param InWidgetStyle Widget样式
     * @param bParentEnabled 父级是否启用
     * @return 使用的图层ID
     */
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                          int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    /** @brief 水印文本内容 */
    FText Text;
    /** @brief 水印不透明度（0.0-1.0） */
    float Opacity = 0.12f;
    /** @brief 字体大小（像素） */
    int32 FontSize = 48;
    /** @brief 文本颜色 */
    FLinearColor Color = FLinearColor::White;
    /** @brief 旋转角度（度） */
    float AngleDegrees = -30.f;
    /** @brief 水平平铺间距（像素），0表示自动调整 */
    float TileSpacingX = 0.f;
    /** @brief 垂直平铺间距（像素） */
    float TileSpacingY = 300.f;
    /** @brief 垂直偏移量（像素） */
    float VerticalOffset = 0.f;
    /** @brief 自定义字体文件路径 */
    FString FontPath;
    /** @brief 水印图片文件路径 */
    FString ImagePath;
    /** @brief 图片缩放比例 */
    float ImageScale = 1.0f;
    /** @brief 是否仅显示图片（不显示文本） */
    bool bImageOnly = false;
    /** @brief 缓存的图片画刷，用于优化性能 */
    mutable TSharedPtr<FSlateBrush> ImageBrush;
};
