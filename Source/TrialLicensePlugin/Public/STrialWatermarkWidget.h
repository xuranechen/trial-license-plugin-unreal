// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

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
        SLATE_ATTRIBUTE(FText, Text)
        SLATE_ATTRIBUTE(float, Opacity)
        SLATE_ATTRIBUTE(int32, FontSize)
        SLATE_ATTRIBUTE(FLinearColor, Color)
        SLATE_ATTRIBUTE(float, AngleDegrees)
        SLATE_ATTRIBUTE(float, TileSpacingX)
        SLATE_ATTRIBUTE(float, TileSpacingY)
        SLATE_ATTRIBUTE(float, VerticalOffset)
        SLATE_ATTRIBUTE(FString, FontPath)
        SLATE_ATTRIBUTE(FString, ImagePath)
        SLATE_ATTRIBUTE(float, ImageScale)
        SLATE_ATTRIBUTE(bool, bImageOnly)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetText(const FText& InText);
    void SetOpacity(float InOpacity);
    void SetFontSize(int32 InSize);
    void SetColor(const FLinearColor& InColor);
    void SetAngle(float InAngleDegrees);
    void SetTileSpacing(float InTileSpacingX, float InTileSpacingY);
    void SetVerticalOffset(float InVerticalOffset);

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                          int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    FText Text;
    float Opacity = 0.12f;
    int32 FontSize = 48;
    FLinearColor Color = FLinearColor::White;
    float AngleDegrees = -30.f;
    float TileSpacingX = 0.f; // 0 = 自动调整
    float TileSpacingY = 300.f;
    float VerticalOffset = 0.f;
    FString FontPath;
    FString ImagePath;
    float ImageScale = 1.0f;
    bool bImageOnly = false;
    mutable TSharedPtr<FSlateBrush> ImageBrush;
};
