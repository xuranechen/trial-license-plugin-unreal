// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Containers/Ticker.h"

class STrialWatermarkWidget;

class TRIALLICENSEPLUGIN_API FTrialWatermarkOverlay
{
public:
    static FTrialWatermarkOverlay& Get();

    void Initialize();
    void Shutdown();
    void DetachFromViewport();

    void SetEnabled(bool bInEnabled);
    void SetBaseText(const FString& InText);
    void SetAngle(float InAngleDegrees);
    void SetTileSpacing(float InTileSpacingX, float InTileSpacingY);
    void SetVerticalOffset(float InVerticalOffset);
    void SetOpacity(float InOpacity);
    void SetFontSize(int32 InFontSize);

    /** 显示证书过期弹窗 */
    void ShowExpiredDialog(const FString& Reason = TEXT(""), float RemainingTime = 0.f, bool bIsPerpetual = false);

    bool IsEnabled() const { return bEnabled; }
    FString GetBaseText() const { return BaseText; }
    float GetAngle() const { return AngleDegrees; }
    float GetTileSpacingX() const { return TileSpacingX; }
    float GetTileSpacingY() const { return TileSpacingY; }
    float GetVerticalOffset() const { return VerticalOffset; }
    float GetOpacity() const { return Opacity; }
    int32 GetFontSize() const { return FontSize; }

private:
    FTrialWatermarkOverlay() = default;

    void TryAttachToViewport();
    void UpdateWatermarkText();

    bool Tick(float DeltaTime);

private:
    TWeakPtr<SWidget> RootOverlayWidget;
    TWeakPtr<STrialWatermarkWidget> WatermarkWidget;
    FTSTicker::FDelegateHandle TickerHandle;
    bool bAttached = false;
    FString CachedLicenseKey;
    FString CachedPassPhrase;
    FString BaseText;
    bool bEnabled = true;
    float AngleDegrees = -30.f;
    float TileSpacingX = 0.f; // 0 = 自动调整
    float TileSpacingY = 300.f;
    float VerticalOffset = 0.f;
    float Opacity = 0.12f;
    int32 FontSize = 48;

    /** 过期弹窗是否已显示 */
    bool bExpiredDialogShown = false;

    /** 上次检查过期的时间戳 */
    double LastExpireCheckTime = 0.0;
};
