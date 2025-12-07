// Copyright 2025, Rune. All Rights Reserved.

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

    // 加载图片
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

void STrialWatermarkWidget::SetText(const FText& InText)
{
    Text = InText;
}

void STrialWatermarkWidget::SetOpacity(float InOpacity)
{
    Opacity = InOpacity;
}

void STrialWatermarkWidget::SetFontSize(int32 InSize)
{
    FontSize = InSize;
}

void STrialWatermarkWidget::SetColor(const FLinearColor& InColor)
{
    Color = InColor;
}

void STrialWatermarkWidget::SetAngle(float InAngleDegrees)
{
    AngleDegrees = InAngleDegrees;
}

void STrialWatermarkWidget::SetTileSpacing(float InTileSpacingX, float InTileSpacingY)
{
    TileSpacingX = InTileSpacingX;
    TileSpacingY = InTileSpacingY;
}

void STrialWatermarkWidget::SetVerticalOffset(float InVerticalOffset)
{
    VerticalOffset = InVerticalOffset;
}

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
    // 解析字体路径（支持绝对路径或相对项目Content路径）
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

    // 测量文本尺寸
    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    FVector2D TextSize = FontMeasure->Measure(Text, FontInfo);

    // 计算实际内容尺寸（用于自动间距）
    float ContentWidth = TextSize.X;
    float ContentHeight = TextSize.Y;
    
    if (ImageBrush.IsValid() && ImageBrush->HasUObject())
    {
        FVector2D ImageSize = ImageBrush->ImageSize * ImageScale;
        if (bImageOnly)
        {
            // 仅图片模式：使用图片尺寸
            ContentWidth = ImageSize.X;
            ContentHeight = ImageSize.Y;
        }
        else
        {
            // 图片+文本模式：图片宽度 + 间距 + 文本宽度，高度取较大值
            ContentWidth = ImageSize.X + 10.f + TextSize.X;
            ContentHeight = FMath::Max(ImageSize.Y, TextSize.Y);
        }
    }

    // 使用配置的平铺间距，如果设为0则自动调整
    float SpacingX = (TileSpacingX > 0.f) ? TileSpacingX : ContentWidth * 1.1f;
    float SpacingY = (TileSpacingY > 0.f) ? TileSpacingY : ContentHeight * 1.5f;

    float AngleRad = FMath::DegreesToRadians(AngleDegrees);

    // 应用垂直偏移
    for (float x = -Size.X; x < Size.X * 1.5f; x += SpacingX)
    {
        for (float y = -Size.Y + VerticalOffset; y < Size.Y * 1.5f + VerticalOffset; y += SpacingY)
        {
            // 如果有图片，渲染图片
            if (ImageBrush.IsValid() && ImageBrush->HasUObject())
            {
                FVector2D ImageSize = ImageBrush->ImageSize * ImageScale;
                FVector2D ImagePivot = ImageSize * 0.5f;

                const FSlateRenderTransform ImageTransform = Concatenate(
                    FSlateRenderTransform(FVector2D(x, y)),
                    FSlateRenderTransform(FQuat2D(AngleRad)),
                    FSlateRenderTransform(-ImagePivot)
                );

                FPaintGeometry ImagePG = AllottedGeometry.ToPaintGeometry(
                    ImageSize, FSlateLayoutTransform(), ImageTransform
                );

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

                const FSlateRenderTransform TextTransform = Concatenate(
                    FSlateRenderTransform(FVector2D(x, y) + TextOffset),
                    FSlateRenderTransform(FQuat2D(AngleRad)),
                    FSlateRenderTransform(-TextPivot)
                );

                FPaintGeometry TextPG = AllottedGeometry.ToPaintGeometry(
                    TextSize, FSlateLayoutTransform(), TextTransform
                );

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

                const FSlateRenderTransform Transform = Concatenate(
                    FSlateRenderTransform(FVector2D(x, y)),
                    FSlateRenderTransform(FQuat2D(AngleRad)),
                    FSlateRenderTransform(-Pivot)
                );

                FPaintGeometry PG = AllottedGeometry.ToPaintGeometry(
                    TextSize, FSlateLayoutTransform(), Transform
                );

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

