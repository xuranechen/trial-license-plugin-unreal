// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用水印Slate Widget实现
 *
 * 该类实现了试用版水印的渲染逻辑，支持文本和图片的平铺显示。
 * 提供旋转、透明度、间距等视觉效果配置，支持自定义字体和图片资源。
 */
#include "STrialWatermarkWidget.h"

#include "Rendering/DrawElements.h"
#include "Styling/SlateColor.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Brushes/SlateImageBrush.h"
#include "Slate/SlateTextures.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"

/**
 * @brief 构造水印Widget
 *
 * 初始化水印Widget的所有属性，并加载指定的图片资源。
 * 支持相对路径和绝对路径的图片加载。
 *
 * @param InArgs 构造参数，包含所有水印配置信息
 */
void STrialWatermarkWidget::Construct(const FArguments& InArgs)
{
    Text = InArgs._Text.Get();
    Opacity = InArgs._Opacity.Get();
    FontSize = InArgs._FontSize.Get();
    Color = InArgs._Color.Get();
    AngleDegrees = InArgs._AngleDegrees.Get();
    TileSpacingX = InArgs._TileSpacingX.Get();
    TileSpacingY = InArgs._TileSpacingY.Get();
    VerticalOffset = InArgs._VerticalOffset.Get();
    FontPath = InArgs._FontPath.Get();
    ImagePath = InArgs._ImagePath.Get();
    ImageScale = InArgs._ImageScale.Get();
    bImageOnly = InArgs._bImageOnly.Get();

    // 加载图片资源，支持相对路径和绝对路径
    if (!ImagePath.IsEmpty())
    {
        FString ResolvedImagePath = ImagePath;
        if (FPaths::IsRelative(ResolvedImagePath))
        {
            ResolvedImagePath = FPaths::ProjectContentDir() / ResolvedImagePath;
        }
        ResolvedImagePath = FPaths::ConvertRelativePathToFull(ResolvedImagePath);

        if (FPaths::FileExists(ResolvedImagePath))
        {
            ImageBrush = MakeShared<FSlateDynamicImageBrush>(FName(*ResolvedImagePath), FVector2D(256, 256));
        }
    }
}

/**
 * @brief 设置水印文本内容
 *
 * @param InText 新的水印文本
 */
void STrialWatermarkWidget::SetText(const FText& InText)
{
    Text = InText;
}

/**
 * @brief 设置水印透明度
 *
 * @param InOpacity 透明度值（0.0-1.0）
 */
void STrialWatermarkWidget::SetOpacity(float InOpacity)
{
    Opacity = InOpacity;
}

/**
 * @brief 设置水印字体大小
 *
 * @param InSize 字体大小（像素）
 */
void STrialWatermarkWidget::SetFontSize(int32 InSize)
{
    FontSize = InSize;
}

/**
 * @brief 设置水印颜色
 *
 * @param InColor 水印颜色
 */
void STrialWatermarkWidget::SetColor(const FLinearColor& InColor)
{
    Color = InColor;
}

/**
 * @brief 设置水印旋转角度
 *
 * @param InAngleDegrees 旋转角度（度）
 */
void STrialWatermarkWidget::SetAngle(float InAngleDegrees)
{
    AngleDegrees = InAngleDegrees;
}

/**
 * @brief 设置水印平铺间距
 *
 * @param InTileSpacingX X方向间距
 * @param InTileSpacingY Y方向间距
 */
void STrialWatermarkWidget::SetTileSpacing(float InTileSpacingX, float InTileSpacingY)
{
    TileSpacingX = InTileSpacingX;
    TileSpacingY = InTileSpacingY;
}

/**
 * @brief 设置水印垂直偏移
 *
 * @param InVerticalOffset 垂直偏移量（像素）
 */
void STrialWatermarkWidget::SetVerticalOffset(float InVerticalOffset)
{
    VerticalOffset = InVerticalOffset;
}

/**
 * @brief 渲染水印内容
 *
 * 实现水印的平铺渲染逻辑，支持文本和图片的组合显示。
 * 通过双重循环在整个视口区域平铺水印，应用旋转、透明度等视觉效果。
 *
 * @param Args 绘制参数
 * @param AllottedGeometry 分配的几何信息
 * @param MyCullingRect 裁剪矩形
 * @param OutDrawElements 绘制元素列表
 * @param LayerId 渲染层ID
 * @param InWidgetStyle Widget样式
 * @param bParentEnabled 父级是否启用
 * @return 下一个可用的层ID
 */
int32 STrialWatermarkWidget::OnPaint(const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    FVector2D Size = AllottedGeometry.GetLocalSize();

    FLinearColor FinalColor(Color.R, Color.G, Color.B, Opacity);

    // 解析字体路径，支持绝对路径和相对项目Content路径
    FString ResolvedFontPath = FontPath;
    if (!ResolvedFontPath.IsEmpty() && !FPaths::FileExists(ResolvedFontPath))
    {
        const FString ProjectCandidate = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / ResolvedFontPath);
        if (FPaths::FileExists(ProjectCandidate))
        {
            ResolvedFontPath = ProjectCandidate;
        }
    }

    FSlateFontInfo FontInfo(
        ResolvedFontPath.IsEmpty() ? (FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf")) : ResolvedFontPath,
        FontSize
    );

    // 测量文本尺寸，用于计算布局
    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    FVector2D TextSize = FontMeasure->Measure(Text, FontInfo);

    // 计算实际内容尺寸，用于自动间距计算
    float ContentWidth = TextSize.X;
    float ContentHeight = TextSize.Y;

    if (ImageBrush.IsValid() && ImageBrush->HasUObject())
    {
        FVector2D ImageSize = ImageBrush->ImageSize * ImageScale;
        if (bImageOnly)
        {
            // 仅图片模式：使用图片尺寸作为内容尺寸
            ContentWidth = ImageSize.X;
            ContentHeight = ImageSize.Y;
        }
        else
        {
            // 图片+文本模式：计算组合内容的尺寸
            ContentWidth = ImageSize.X + 10.f + TextSize.X;
            ContentHeight = FMath::Max(ImageSize.Y, TextSize.Y);
        }
    }

    // 计算平铺间距，如果配置为0则自动调整
    float SpacingX = (TileSpacingX > 0.f) ? TileSpacingX : ContentWidth * 1.1f;
    float SpacingY = (TileSpacingY > 0.f) ? TileSpacingY : ContentHeight * 1.5f;

    float AngleRad = FMath::DegreesToRadians(AngleDegrees);

    // 双重循环在整个视口区域平铺水印，应用垂直偏移
    for (float x = -Size.X; x < Size.X * 1.5f; x += SpacingX)
    {
        for (float y = -Size.Y + VerticalOffset; y < Size.Y * 1.5f + VerticalOffset; y += SpacingY)
        {
            // 优先渲染图片（如果有的话）
            if (ImageBrush.IsValid() && ImageBrush->HasUObject())
            {
                FVector2D ImageSize = ImageBrush->ImageSize * ImageScale;
                FVector2D ImagePivot = ImageSize * 0.5f;

                // 创建图片的旋转变换
                const FSlateRenderTransform ImageTransform = Concatenate(
                    FSlateRenderTransform(FVector2D(x, y)),
                    FSlateRenderTransform(FQuat2D(AngleRad)),
                    FSlateRenderTransform(-ImagePivot)
                );

                FPaintGeometry ImagePG = AllottedGeometry.ToPaintGeometry(
                    ImageSize, FSlateLayoutTransform(), ImageTransform
                );

                // 添加图片绘制元素
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId,
                    ImagePG,
                    ImageBrush.Get(),
                    ESlateDrawEffect::None,
                    FLinearColor(1.f, 1.f, 1.f, Opacity)
                );

                // 如果仅显示图片，跳过文本渲染
                if (bImageOnly)
                {
                    continue;
                }

                // 图片+文本模式：文本显示在图片右侧
                FVector2D TextOffset(ImageSize.X + 10.f, (ImageSize.Y - TextSize.Y) * 0.5f);
                FVector2D TextPivot = TextSize * 0.5f;

                // 创建文本的旋转变换
                const FSlateRenderTransform TextTransform = Concatenate(
                    FSlateRenderTransform(FVector2D(x, y) + TextOffset),
                    FSlateRenderTransform(FQuat2D(AngleRad)),
                    FSlateRenderTransform(-TextPivot)
                );

                FPaintGeometry TextPG = AllottedGeometry.ToPaintGeometry(
                    TextSize, FSlateLayoutTransform(), TextTransform
                );

                // 添加文本绘制元素
                FSlateDrawElement::MakeText(
                    OutDrawElements,
                    LayerId + 1,
                    TextPG,
                    Text,
                    FontInfo,
                    ESlateDrawEffect::None,
                    FinalColor
                );
            }
            else
            {
                // 仅文本模式
                FVector2D Pivot = TextSize * 0.5f;

                // 创建文本的旋转变换
                const FSlateRenderTransform Transform = Concatenate(
                    FSlateRenderTransform(FVector2D(x, y)),
                    FSlateRenderTransform(FQuat2D(AngleRad)),
                    FSlateRenderTransform(-Pivot)
                );

                FPaintGeometry PG = AllottedGeometry.ToPaintGeometry(
                    TextSize, FSlateLayoutTransform(), Transform
                );

                // 添加文本绘制元素
                FSlateDrawElement::MakeText(
                    OutDrawElements,
                    LayerId,
                    PG,
                    Text,
                    FontInfo,
                    ESlateDrawEffect::None,
                    FinalColor
                );
            }
        }
    }

    return LayerId + 2;
}
