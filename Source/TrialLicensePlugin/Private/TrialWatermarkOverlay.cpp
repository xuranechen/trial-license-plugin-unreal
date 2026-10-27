// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用水印覆盖层
 *
 * 负责在游戏视口渲染水印、定期更新剩余时间，并在过期/验证失败时弹出对话框。
 * 支持从项目设置读取参数，兼容UMG或Slate弹窗。
 */
#include "TrialWatermarkOverlay.h"
#include "TrialLicenseSettings.h"
#include "UTrialLicenseSubsystem.h"
#include "FTrialLicenseInfo.h"
#include "STrialExpiredWidget.h"
#include "TrialExpiredWidgetBase.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
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

// 辅助函数：获取试用许可证子系统 (水印覆盖层版本)
/**
 * @brief 获取试用许可证子系统（覆盖层版本）
 *
 * 优先从当前播放世界的GameInstance获取，若不可用则退回从GEngine的GameViewport获取。
 *
 * @return 子系统指针，失败返回nullptr
 */
static UTrialLicenseSubsystem* GetTrialLicenseSubsystemForOverlay()
{
	if (!GEngine)
	{
		return nullptr;
	}

	if (UWorld* World = GEngine->GetCurrentPlayWorld())
	{
		if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World))
		{
			if (UTrialLicenseSubsystem* Subsystem = GameInstance->GetSubsystem<UTrialLicenseSubsystem>())
			{
				return Subsystem;
			}
		}
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (UWorld* World = WorldContext.World())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UTrialLicenseSubsystem* Subsystem = GameInstance->GetSubsystem<UTrialLicenseSubsystem>())
				{
					return Subsystem;
				}
			}
		}
	}

	// 回退方案：尝试从GEngine获取
	if (GEngine->GameViewport)
	{
		if (UGameInstance* GameInstance = GEngine->GameViewport->GetGameInstance())
		{
			if (UTrialLicenseSubsystem* Subsystem = GameInstance->GetSubsystem<UTrialLicenseSubsystem>())
			{
				return Subsystem;
			}
		}
	}
	return nullptr;
}

/**
 * @brief 单例访问
 */
FTrialWatermarkOverlay& FTrialWatermarkOverlay::Get()
{
    static FTrialWatermarkOverlay Instance;
    return Instance;
}

/**
 * @brief 初始化覆盖层
 *
 * 从项目设置或配置文件读取水印参数，附加到视口并启动定时刷新。
 */
void FTrialWatermarkOverlay::Initialize()
{
    // 重置过期对话框标志位，确保每次启动都能重新显示
    bExpiredDialogShown = false;

    // Prefer project settings
    if (const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>())
    {
        GWatermarkEnabled = Settings->WatermarkEnabled;
        CachedLicenseKey = Settings->LicenseKey;
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

/**
 * @brief 停止覆盖层
 *
 * 移除定时器并从视口分离。
 */
void FTrialWatermarkOverlay::Shutdown()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle = FTSTicker::FDelegateHandle();
    }
    DetachFromViewport();
}

/**
 * @brief 定时刷新
 *
 * 维护水印显示、更新文本，并周期检测许可证过期状态。
 */
bool FTrialWatermarkOverlay::Tick(float /*DeltaTime*/)
{
	if (!bEnabled)
	{
		if (RootOverlayWidget.IsValid())
		{
			RootOverlayWidget.Pin()->SetVisibility(EVisibility::Collapsed);
		}
	}
	else if (RootOverlayWidget.IsValid())
	{
		RootOverlayWidget.Pin()->SetVisibility(EVisibility::Visible);
	}

	if (bEnabled && !bAttached)
	{
		TryAttachToViewport();
	}

	if (bEnabled)
	{
		UpdateWatermarkText();
	}

    return true;
}

/**
 * @brief 尝试附加到游戏视口
 *
 * 构建水印Widget并添加到Viewport，未就绪时静默退出等待下一次尝试。
 */
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

/**
 * @brief 从视口移除水印Overlay
 */
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

/**
 * @brief 更新水印显示文本
 *
 * 根据许可证状态展示剩余时间、到期时间或提示未配置。
 */
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

    if (CachedLicenseKey.IsEmpty())
    {
        const FString Text = FString::Printf(TEXT("%s | 未配置许可证"), *DisplayBase);
        WatermarkWidget.Pin()->SetText(FText::FromString(Text));
        return;
    }

    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForOverlay())
    {
        const FTrialLicenseInfo Info = Subsystem->ParseLicenseKey(CachedLicenseKey);

        if (!Info.bShowWatermark)
        {
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

        const float RemainingSeconds = Subsystem->GetTrialRemainingTime(CachedLicenseKey);

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
}

/**
 * @brief 设置水印启用状态
 */
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

/**
 * @brief 设置水印基础文本
 */
void FTrialWatermarkOverlay::SetBaseText(const FString& InText)
{
    BaseText = InText;
}

/**
 * @brief 设置水印旋转角度
 */
void FTrialWatermarkOverlay::SetAngle(float InAngleDegrees)
{
    AngleDegrees = InAngleDegrees;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetAngle(AngleDegrees);
    }
}

/**
 * @brief 设置水印平铺间距
 *
 * 可分别指定X/Y方向间距。
 */
void FTrialWatermarkOverlay::SetTileSpacing(float InTileSpacingX, float InTileSpacingY)
{
    TileSpacingX = InTileSpacingX;
    TileSpacingY = InTileSpacingY;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetTileSpacing(TileSpacingX, TileSpacingY);
    }
}

/**
 * @brief 设置水印垂直偏移
 */
void FTrialWatermarkOverlay::SetVerticalOffset(float InVerticalOffset)
{
    VerticalOffset = InVerticalOffset;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetVerticalOffset(VerticalOffset);
    }
}

/**
 * @brief 设置水印不透明度
 */
void FTrialWatermarkOverlay::SetOpacity(float InOpacity)
{
    Opacity = InOpacity;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetOpacity(Opacity);
    }
}

/**
 * @brief 设置水印字体大小
 */
void FTrialWatermarkOverlay::SetFontSize(int32 InFontSize)
{
    FontSize = InFontSize;
    if (WatermarkWidget.IsValid())
    {
        WatermarkWidget.Pin()->SetFontSize(FontSize);
    }
}

/**
 * @brief 显示过期弹窗
 *
 * 根据设置选择UMG或Slate实现，避免重复弹出。
 */
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
					bExpiredDialogShown = true;
					
					// 添加到视口
					ExpiredWidget->AddToViewport(1000); // 高层级确保显示在最前面
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

    // 创建弹窗窗口（无图标、无标题栏，完全自定义）
	TSharedRef<SWindow> ExpiredWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("试用许可证提示 | Trial License Notice")))
        .ClientSize(FVector2D(800.f, 600.f))
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .IsTopmostWindow(true)
        .SizingRule(ESizingRule::UserSized)
        .HasCloseButton(false)
        .CreateTitleBar(false)
		[
			ExpiredWidget.ToSharedRef()
		];

	bExpiredDialogShown = true;

	// 显示为模态窗口
	FSlateApplication::Get().AddModalWindow(ExpiredWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
	bExpiredDialogShown = false;
	UE_LOG(LogTemp, Log, TEXT("[TrialLicense] Default Slate expired dialog shown. Reason: %s"), *Reason);
}

void FTrialWatermarkOverlay::OnLicenseValidated(const FString& NewLicenseKey)
{
    CachedLicenseKey = NewLicenseKey;
    bExpiredDialogShown = false;
    LastExpireCheckTime = 0.0;
    UpdateWatermarkText();
	UE_LOG(LogTemp, Log, TEXT("[TrialLicense] Overlay cache refreshed after license validation. KeyLen=%d"), CachedLicenseKey.Len());
}

void FTrialWatermarkOverlay::ResetExpiredDialogState()
{
	bExpiredDialogShown = false;
}
