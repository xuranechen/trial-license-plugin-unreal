// Copyright 2025, Rune. All Rights Reserved.

#include "TrialWatermarkOverlay.h"
#include "TrialLicenseSettings.h"
#include "UTrialLicenseLibrary.h"
#include "FTrialLicenseInfo.h"
#include "STrialExpiredWidget.h"
#include "TrialExpiredWidgetBase.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "STrialWatermarkWidget.h"
#include "Styling/SlateColor.h"
#include "Framework/Application/SlateApplication.h"
#include "Containers/Ticker.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

static bool GWatermarkEnabled = true;
static float GWatermarkOpacity = 0.12f;
static int32 GWatermarkFontSize = 48;
static FLinearColor GWatermarkColor = FLinearColor(1.f,1.f,1.f,1.f);
static float GWatermarkAngle = -30.f;
static float GWatermarkTileSpacingX = 0.f; // 0 = 自动调整
static float GWatermarkTileSpacingY = 300.f;
static float GWatermarkVerticalOffset = 0.f;

FTrialWatermarkOverlay& FTrialWatermarkOverlay::Get()
{
    static FTrialWatermarkOverlay Instance;
    return Instance;
}

void FTrialWatermarkOverlay::Initialize()
{
    // Prefer project settings
    if (const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>())
    {
        GWatermarkEnabled = Settings->WatermarkEnabled;
        CachedLicenseKey = Settings->LicenseKey;
        CachedPassPhrase = Settings->PassPhrase;
        BaseText = Settings->WatermarkText;
        GWatermarkOpacity = Settings->WatermarkOpacity;
        GWatermarkFontSize = Settings->WatermarkFontSize;
        GWatermarkAngle = Settings->WatermarkAngle;
        GWatermarkTileSpacingX = Settings->WatermarkTileSpacingX;
        GWatermarkTileSpacingY = Settings->WatermarkTileSpacingY;
        GWatermarkVerticalOffset = Settings->WatermarkVerticalOffset;
    }
    else if (GConfig)
    {
        GConfig->GetBool(TEXT("TrialLicensePlugin"), TEXT("WatermarkEnabled"), GWatermarkEnabled, GGameIni);
        GConfig->GetString(TEXT("TrialLicensePlugin"), TEXT("LicenseKey"), CachedLicenseKey, GGameIni);
        GConfig->GetString(TEXT("TrialLicensePlugin"), TEXT("PassPhrase"), CachedPassPhrase, GGameIni);
        GConfig->GetString(TEXT("TrialLicensePlugin"), TEXT("WatermarkText"), BaseText, GGameIni);
        GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("WatermarkOpacity"), GWatermarkOpacity, GGameIni);
        GConfig->GetInt(TEXT("TrialLicensePlugin"), TEXT("WatermarkFontSize"), GWatermarkFontSize, GGameIni);
        GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("WatermarkAngle"), GWatermarkAngle, GGameIni);
        GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("WatermarkTileSpacingX"), GWatermarkTileSpacingX, GGameIni);
        GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("WatermarkTileSpacingY"), GWatermarkTileSpacingY, GGameIni);
        GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("WatermarkVerticalOffset"), GWatermarkVerticalOffset, GGameIni);
    }

    bEnabled = GWatermarkEnabled;
    Opacity = GWatermarkOpacity;
    FontSize = GWatermarkFontSize;
    AngleDegrees = GWatermarkAngle;
    TileSpacingX = GWatermarkTileSpacingX;
    TileSpacingY = GWatermarkTileSpacingY;
    VerticalOffset = GWatermarkVerticalOffset;

    UE_LOG(LogTemp, Log, TEXT("[TrialWatermark] Initialize: Enabled=%d, Text='%s', Opacity=%.2f, FontSize=%d"), bEnabled, *BaseText, Opacity, FontSize);

    TryAttachToViewport();

    if (!TickerHandle.IsValid())
    {
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FTrialWatermarkOverlay::Tick), 1.0f);
        UE_LOG(LogTemp, Log, TEXT("[TrialWatermark] Ticker started"));
    }
}

void FTrialWatermarkOverlay::Shutdown()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle = FTSTicker::FDelegateHandle();
    }
    DetachFromViewport();
}

bool FTrialWatermarkOverlay::Tick(float /*DeltaTime*/)
{
    if (!bEnabled)
    {
        if (RootOverlayWidget.IsValid())
        {
            RootOverlayWidget.Pin()->SetVisibility(EVisibility::Collapsed);
        }
        return true; // keep ticking to handle re-enable
    }

    if (!bAttached)
    {
        TryAttachToViewport();
    }

    UpdateWatermarkText();

    // 定期检查证书是否过期（每10秒检查一次）
    const double CurrentTime = FPlatformTime::Seconds();
    if (!bExpiredDialogShown && (CurrentTime - LastExpireCheckTime > 10.0))
    {
        LastExpireCheckTime = CurrentTime;

        if (!CachedLicenseKey.IsEmpty() && !CachedPassPhrase.IsEmpty())
        {
            const float RemainingTime = UTrialLicenseLibrary::GetTrialRemainingTime(CachedLicenseKey, CachedPassPhrase);
            const FTrialLicenseInfo Info = UTrialLicenseLibrary::ParseLicenseKey(CachedLicenseKey, CachedPassPhrase);

            // 如果试用时间已耗尽（且不是永久授权）
            if (!Info.bPerpetual && RemainingTime <= 0.f)
            {
                ShowExpiredDialog(
                    TEXT("您的试用期已到期，请联系开发者获取正式许可证。\n\nYour trial period has expired. Please contact the developer for a full license."),
                    RemainingTime,
                    Info.bPerpetual
                );
            }
            // 如果证书验证失败
            else if (!UTrialLicenseLibrary::ValidateLicense(CachedLicenseKey, CachedPassPhrase))
            {
                ShowExpiredDialog(
                    TEXT("许可证验证失败，可能是设备不匹配或证书已被篡改。\n\nLicense validation failed. Device mismatch or certificate tampering detected."),
                    RemainingTime,
                    Info.bPerpetual
                );
            }
        }
    }

    return true;
}

void FTrialWatermarkOverlay::TryAttachToViewport()
{
    if (bAttached)
    {
        return;
    }

#if WITH_EDITOR
    // 在编辑器模式下,仅在PIE运行时显示水印
    if (GEditor && !GEditor->IsPlayingSessionInEditor())
    {
        return; // 静默返回,不输出警告
    }
#endif

    if (!GEngine || !GEngine->GameViewport)
    {
        // 仅在非编辑器模式或PIE模式下输出警告
#if WITH_EDITOR
        if (GEditor && GEditor->IsPlayingSessionInEditor())
        {
            UE_LOG(LogTemp, Warning, TEXT("[TrialWatermark] GameViewport not ready yet, will retry..."));
        }
#else
        UE_LOG(LogTemp, Warning, TEXT("[TrialWatermark] GameViewport not ready yet, will retry..."));
#endif
        return;
    }

    TSharedPtr<STrialWatermarkWidget> Widget;
    const UTrialLicenseSettings* LocalSettings = GetDefault<UTrialLicenseSettings>();
    TSharedRef<SOverlay> Overlay = SNew(SOverlay)
    + SOverlay::Slot()
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Fill)
    [
        SAssignNew(Widget, STrialWatermarkWidget)
        .Text(FText::FromString(TEXT("")))
        .Opacity(Opacity)
        .FontSize(FontSize)
        .Color(GWatermarkColor)
        .AngleDegrees(AngleDegrees)
        .TileSpacingX(TileSpacingX)
        .TileSpacingY(TileSpacingY)
        .VerticalOffset(VerticalOffset)
        .FontPath(LocalSettings ? LocalSettings->WatermarkFontPath.FilePath : TEXT(""))
        .ImagePath(LocalSettings ? LocalSettings->WatermarkImagePath.FilePath : TEXT(""))
        .ImageScale(LocalSettings ? LocalSettings->WatermarkImageScale : 1.0f)
        .bImageOnly(LocalSettings ? LocalSettings->bWatermarkImageOnly : false)
    ];

    RootOverlayWidget = Overlay;
    WatermarkWidget = Widget;

    GEngine->GameViewport->AddViewportWidgetContent(Overlay, 10);
    bAttached = true;
    
    UE_LOG(LogTemp, Log, TEXT("[TrialWatermark] Successfully attached to viewport. Enabled=%d, Opacity=%.2f"), bEnabled, Opacity);
}

void FTrialWatermarkOverlay::DetachFromViewport()
{
    if (bAttached && GEngine && GEngine->GameViewport)
    {
        if (RootOverlayWidget.IsValid())
        {
            GEngine->GameViewport->RemoveViewportWidgetContent(RootOverlayWidget.Pin().ToSharedRef());
        }
    }
    bAttached = false;
    RootOverlayWidget.Reset();
    WatermarkWidget.Reset();
}

void FTrialWatermarkOverlay::UpdateWatermarkText()
{
    if (!WatermarkWidget.IsValid())
    {
        // 仅在应该显示水印但Widget无效时才输出警告
#if WITH_EDITOR
        if (GEditor && GEditor->IsPlayingSessionInEditor())
        {
            UE_LOG(LogTemp, Warning, TEXT("[TrialWatermark] UpdateWatermarkText: Widget is not valid"));
        }
#else
        UE_LOG(LogTemp, Warning, TEXT("[TrialWatermark] UpdateWatermarkText: Widget is not valid"));
#endif
        return;
    }

    FString DisplayBase = BaseText.IsEmpty() ? TEXT("Trial Version") : BaseText;

    if (CachedLicenseKey.IsEmpty() || CachedPassPhrase.IsEmpty())
    {
        const FString Text = FString::Printf(TEXT("%s | 未配置许可证/口令"), *DisplayBase);
        WatermarkWidget.Pin()->SetText(FText::FromString(Text));
        return;
    }

    const FTrialLicenseInfo Info = UTrialLicenseLibrary::ParseLicenseKey(CachedLicenseKey, CachedPassPhrase);

    if (Info.bPerpetual)
    {
        // 永久授权隐藏
        if (RootOverlayWidget.IsValid())
        {
            RootOverlayWidget.Pin()->SetVisibility(EVisibility::Collapsed);
        }
        return;
    }
    else
    {
        if (RootOverlayWidget.IsValid())
        {
            RootOverlayWidget.Pin()->SetVisibility(EVisibility::HitTestInvisible);
        }
    }

    const float RemainingSeconds = UTrialLicenseLibrary::GetTrialRemainingTime(CachedLicenseKey, CachedPassPhrase);

    int32 RemainH = 0, RemainM = 0, RemainS = 0;
    if (RemainingSeconds > 0.f)
    {
        RemainH = (int32)(RemainingSeconds / 3600.0f);
        RemainM = (int32)((FMath::Fmod(RemainingSeconds, 3600.0f)) / 60.0f);
        RemainS = (int32)FMath::Fmod(RemainingSeconds, 60.0f);
    }

    const FString ExpireStr = Info.ExpireTime.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
    FString RemainStr;
    if (RemainingSeconds < 0.f)
    {
        RemainStr = TEXT("无限制");
    }
    else
    {
        RemainStr = FString::Printf(TEXT("%02d:%02d:%02d"), RemainH, RemainM, RemainS);
    }

    const FString Text = FString::Printf(TEXT("%s | 剩余: %s | 到期: %s"), *DisplayBase, *RemainStr, *ExpireStr);
    WatermarkWidget.Pin()->SetText(FText::FromString(Text));
    UE_LOG(LogTemp, Verbose, TEXT("[TrialWatermark] Text updated: %s"), *Text);
}

void FTrialWatermarkOverlay::SetEnabled(bool bInEnabled)
{
    bEnabled = bInEnabled;
    if (!bEnabled)
    {
        if (RootOverlayWidget.IsValid())
        {
            RootOverlayWidget.Pin()->SetVisibility(EVisibility::Collapsed);
        }
    }
    else
    {
        if (RootOverlayWidget.IsValid())
        {
            RootOverlayWidget.Pin()->SetVisibility(EVisibility::HitTestInvisible);
        }
    }
}

void FTrialWatermarkOverlay::SetBaseText(const FString& InText)
{
    BaseText = InText;
}

void FTrialWatermarkOverlay::SetAngle(float InAngleDegrees)
{
    AngleDegrees = InAngleDegrees;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetAngle(AngleDegrees);
    }
}

void FTrialWatermarkOverlay::SetTileSpacing(float InTileSpacingX, float InTileSpacingY)
{
    TileSpacingX = InTileSpacingX;
    TileSpacingY = InTileSpacingY;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetTileSpacing(TileSpacingX, TileSpacingY);
    }
}

void FTrialWatermarkOverlay::SetVerticalOffset(float InVerticalOffset)
{
    VerticalOffset = InVerticalOffset;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetVerticalOffset(VerticalOffset);
    }
}

void FTrialWatermarkOverlay::SetOpacity(float InOpacity)
{
    Opacity = InOpacity;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetOpacity(Opacity);
    }
}

void FTrialWatermarkOverlay::SetFontSize(int32 InFontSize)
{
    FontSize = InFontSize;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetFontSize(FontSize);
    }
}

void FTrialWatermarkOverlay::ShowExpiredDialog(const FString& Reason, float RemainingTime, bool bIsPerpetual)
{
    // 避免重复弹出
    if (bExpiredDialogShown)
    {
        return;
    }

    const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
    if (!Settings || !Settings->bAutoShowExpiredDialog)
    {
        return;
    }

    // 确保在主线程
    if (!IsInGameThread())
    {
        return;
    }

    // 判断使用UMG还是Slate（默认使用Slate）
    if (Settings->bUseUMGWidget)
    {
        // 使用UMG自定义Widget
        if (!GEngine || !GEngine->GameViewport)
        {
            UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Cannot show UMG dialog: GameViewport not available, fallback to Slate"));
            // 无法使用UMG，直接跳到Slate逻辑（不修改Settings，因为它是const）
        }
        else
        {
            // 加载UMG Widget类
            UClass* WidgetClass = nullptr;
            if (!Settings->ExpiredWidgetClass.IsNull())
            {
                WidgetClass = Settings->ExpiredWidgetClass.TryLoadClass<UUserWidget>();
            }

            // 如果没有指定类，使用默认UMG基类
            if (!WidgetClass)
            {
                WidgetClass = UTrialExpiredWidgetBase::StaticClass();
                UE_LOG(LogTemp, Log, TEXT("[TrialLicense] No custom UMG widget specified, using default UTrialExpiredWidgetBase"));
            }

            // 创建UMG Widget实例
            if (WidgetClass && WidgetClass->IsChildOf(UTrialExpiredWidgetBase::StaticClass()))
            {
                UTrialExpiredWidgetBase* ExpiredWidget = CreateWidget<UTrialExpiredWidgetBase>(GEngine->GameViewport->GetWorld(), WidgetClass);
                if (ExpiredWidget)
                {
                    // 初始化数据
                    ExpiredWidget->InitializeExpiredWidget(Reason, RemainingTime, bIsPerpetual);
                    
                    // 添加到视口
                    ExpiredWidget->AddToViewport(1000); // 高层级确保显示在最前面

                    bExpiredDialogShown = true;
                    UE_LOG(LogTemp, Log, TEXT("[TrialLicense] UMG expired dialog shown. Reason: %s"), *Reason);
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[TrialLicense] Invalid UMG widget class. Must inherit from UTrialExpiredWidgetBase. Fallback to Slate."));
            }
        }
    }

    // 使用默认Slate Widget（功能完整的默认弹窗）
    if (!FSlateApplication::IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Slate not initialized, cannot show expired dialog"));
        return;
    }

    // 创建默认Slate弹窗Widget
    TSharedPtr<SWidget> ExpiredWidget = SNew(STrialExpiredWidget)
        .ExpireReason(Reason)
        .RemainingTime(RemainingTime)
        .bIsPerpetual(bIsPerpetual);

    // 创建弹窗窗口
    TSharedRef<SWindow> ExpiredWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("试用许可证提示 | Trial License Notice")))
        .ClientSize(FVector2D(650.f, 500.f))
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .IsTopmostWindow(true)
        .SizingRule(ESizingRule::FixedSize)
        [
            ExpiredWidget.ToSharedRef()
        ];

    // 显示为模态窗口
    FSlateApplication::Get().AddModalWindow(ExpiredWindow, FSlateApplication::Get().GetActiveTopLevelWindow());

    bExpiredDialogShown = true;
    UE_LOG(LogTemp, Log, TEXT("[TrialLicense] Default Slate expired dialog shown. Reason: %s"), *Reason);
}
