// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TrialWatermarkBlueprintLibrary.generated.h"

UCLASS()
class TRIALLICENSEPLUGIN_API UTrialWatermarkBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkText(const FString& NewText);

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkAngle(float AngleDegrees);

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkTileSpacing(float TileSpacing);

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkTileSpacingXY(float TileSpacingX, float TileSpacingY);

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkOpacity(float Opacity);

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void SetTrialWatermarkFontSize(int32 FontSize);

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static bool GetTrialWatermarkEnabled();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static FString GetTrialWatermarkText();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static float GetTrialWatermarkAngle();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static float GetTrialWatermarkTileSpacing();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static float GetTrialWatermarkTileSpacingX();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static float GetTrialWatermarkTileSpacingY();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static float GetTrialWatermarkOpacity();

    UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
    static int32 GetTrialWatermarkFontSize();

    UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
    static void ApplyTrialWatermarkFromProjectSettings();

    /**
     * 手动显示证书过期弹窗
     * @param Reason 过期原因文本（可选）
     * @param RemainingTime 剩余时间（秒），0表示已过期
     * @param bIsPerpetual 是否为永久授权
     */
    UFUNCTION(BlueprintCallable, Category = "Trial License|Dialog")
    static void ShowTrialExpiredDialog(const FString& Reason = TEXT(""), float RemainingTime = 0.f, bool bIsPerpetual = false);
};
