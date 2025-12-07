// Copyright 2025, Rune. All Rights Reserved.

#include "TrialWatermarkBlueprintLibrary.h"
#include "TrialWatermarkOverlay.h"
#include "TrialLicenseSettings.h"

namespace
{
    // Ensure overlay is initialized even in editor/PIE before using it
    void EnsureTrialWatermarkInitialized()
    {
        static bool bInitialized = false;
        if (!bInitialized)
        {
            FTrialWatermarkOverlay::Get().Initialize();
            bInitialized = true;
        }
    }
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkEnabled(bool bEnabled)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().SetEnabled(bEnabled);
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkText(const FString& NewText)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().SetBaseText(NewText);
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkAngle(float AngleDegrees)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().SetAngle(AngleDegrees);
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkTileSpacing(float TileSpacing)
{
    EnsureTrialWatermarkInitialized();
    // 向后兼容：同时设置X和Y为相同值
    FTrialWatermarkOverlay::Get().SetTileSpacing(TileSpacing, TileSpacing);
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkTileSpacingXY(float TileSpacingX, float TileSpacingY)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().SetTileSpacing(TileSpacingX, TileSpacingY);
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkOpacity(float Opacity)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().SetOpacity(Opacity);
}

void UTrialWatermarkBlueprintLibrary::SetTrialWatermarkFontSize(int32 FontSize)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().SetFontSize(FontSize);
}

bool UTrialWatermarkBlueprintLibrary::GetTrialWatermarkEnabled()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().IsEnabled();
}

FString UTrialWatermarkBlueprintLibrary::GetTrialWatermarkText()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().GetBaseText();
}

float UTrialWatermarkBlueprintLibrary::GetTrialWatermarkAngle()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().GetAngle();
}

float UTrialWatermarkBlueprintLibrary::GetTrialWatermarkTileSpacing()
{
    EnsureTrialWatermarkInitialized();
    // 向后兼容：返回X值
    return FTrialWatermarkOverlay::Get().GetTileSpacingX();
}

float UTrialWatermarkBlueprintLibrary::GetTrialWatermarkTileSpacingX()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().GetTileSpacingX();
}

float UTrialWatermarkBlueprintLibrary::GetTrialWatermarkTileSpacingY()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().GetTileSpacingY();
}

float UTrialWatermarkBlueprintLibrary::GetTrialWatermarkOpacity()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().GetOpacity();
}

int32 UTrialWatermarkBlueprintLibrary::GetTrialWatermarkFontSize()
{
    EnsureTrialWatermarkInitialized();
    return FTrialWatermarkOverlay::Get().GetFontSize();
}

void UTrialWatermarkBlueprintLibrary::ApplyTrialWatermarkFromProjectSettings()
{
    if (const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>())
    {
        EnsureTrialWatermarkInitialized();
        FTrialWatermarkOverlay& Overlay = FTrialWatermarkOverlay::Get();
        Overlay.SetEnabled(Settings->WatermarkEnabled);
        Overlay.SetBaseText(Settings->WatermarkText);
        Overlay.SetOpacity(Settings->WatermarkOpacity);
        Overlay.SetFontSize(Settings->WatermarkFontSize);
        Overlay.SetAngle(Settings->WatermarkAngle);
        Overlay.SetTileSpacing(Settings->WatermarkTileSpacingX, Settings->WatermarkTileSpacingY);
    }
}

void UTrialWatermarkBlueprintLibrary::ShowTrialExpiredDialog(const FString& Reason, float RemainingTime, bool bIsPerpetual)
{
    EnsureTrialWatermarkInitialized();
    FTrialWatermarkOverlay::Get().ShowExpiredDialog(Reason, RemainingTime, bIsPerpetual);
}
