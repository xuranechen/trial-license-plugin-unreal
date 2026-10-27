// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用过期弹窗（Slate版）
 *
 * 默认的试用过期提示界面，提供标题、信息、试用请求展示与复制，
 * 以及关闭/退出按钮。可在项目中直接使用或派生自定义样式。
 */
#include "STrialExpiredWidget.h"
#include "UTrialLicenseSubsystem.h"
#include "TrialLicenseSettings.h"
#include "TrialWatermarkOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Misc/App.h"
#include "Framework/Application/SlateApplication.h"
#include "Containers/Ticker.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "HAL/PlatformApplicationMisc.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

// 辅助函数：获取试用许可证子系统 (Slate版本)
/**
 * @brief 获取试用许可证子系统（Slate版本）
 *
 * 优先从当前播放世界的GameInstance中获取子系统，
 * 若不可用则退回从GEngine的GameViewport获取。失败则返回nullptr。
 *
 * @return 子系统指针，获取失败返回nullptr
 */
static UTrialLicenseSubsystem* GetTrialLicenseSubsystemForSlate()
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

static FString FormatRemainingTimeSegmentedCN(float RemainingSeconds)
{
	int64 TotalMinutes = FMath::Max<int64>(0, FMath::FloorToInt64(RemainingSeconds / 60.0f));

	const int64 MinutesPerHour = 60;
	const int64 MinutesPerDay = 24 * MinutesPerHour;
	const int64 MinutesPerMonth = 30 * MinutesPerDay;
	const int64 MinutesPerYear = 365 * MinutesPerDay;

	const int64 Years = TotalMinutes / MinutesPerYear;
	TotalMinutes %= MinutesPerYear;
	const int64 Months = TotalMinutes / MinutesPerMonth;
	TotalMinutes %= MinutesPerMonth;
	const int64 Days = TotalMinutes / MinutesPerDay;
	TotalMinutes %= MinutesPerDay;
	const int64 Hours = TotalMinutes / MinutesPerHour;
	const int64 Minutes = TotalMinutes % MinutesPerHour;

	FString Result;
	if (Years > 0)
	{
		Result += FString::Printf(TEXT("%lld年"), Years);
	}
	if (Months > 0)
	{
		Result += FString::Printf(TEXT("%lld月"), Months);
	}
	if (Days > 0)
	{
		Result += FString::Printf(TEXT("%lld天"), Days);
	}
	if (Hours > 0)
	{
		Result += FString::Printf(TEXT("%lld小时"), Hours);
	}
	if (Minutes > 0 || Result.IsEmpty())
	{
		Result += FString::Printf(TEXT("%lld分钟"), Minutes);
	}

	return Result;
}

static FString FormatRemainingTimeSegmentedEN(float RemainingSeconds)
{
	int64 TotalMinutes = FMath::Max<int64>(0, FMath::FloorToInt64(RemainingSeconds / 60.0f));

	const int64 MinutesPerHour = 60;
	const int64 MinutesPerDay = 24 * MinutesPerHour;
	const int64 MinutesPerMonth = 30 * MinutesPerDay;
	const int64 MinutesPerYear = 365 * MinutesPerDay;

	const int64 Years = TotalMinutes / MinutesPerYear;
	TotalMinutes %= MinutesPerYear;
	const int64 Months = TotalMinutes / MinutesPerMonth;
	TotalMinutes %= MinutesPerMonth;
	const int64 Days = TotalMinutes / MinutesPerDay;
	TotalMinutes %= MinutesPerDay;
	const int64 Hours = TotalMinutes / MinutesPerHour;
	const int64 Minutes = TotalMinutes % MinutesPerHour;

	FString Result;
	if (Years > 0)
	{
		Result += FString::Printf(TEXT("%lldy "), Years);
	}
	if (Months > 0)
	{
		Result += FString::Printf(TEXT("%lldmo "), Months);
	}
	if (Days > 0)
	{
		Result += FString::Printf(TEXT("%lldd "), Days);
	}
	if (Hours > 0)
	{
		Result += FString::Printf(TEXT("%lldh "), Hours);
	}
	if (Minutes > 0 || Result.IsEmpty())
	{
		Result += FString::Printf(TEXT("%lldm"), Minutes);
	}

	Result.TrimEndInline();
	return Result;
}

/**
 * @brief 构建过期弹窗Widget
 *
 * 保存入参并设置根部件。
 */
void STrialExpiredWidget::Construct(const FArguments& InArgs)
{
    ExpireReason = InArgs._ExpireReason;
    RemainingTime = InArgs._RemainingTime;
    bIsPerpetual = InArgs._bIsPerpetual;
    bValidationSuccess = false;
    ValidationStatusMessage = TEXT("");

    // 直接生成并缓存试用请求信息，避免每次 UI 刷新时重新生成
    FString MachineID;
    FString TrialRequest;

    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForSlate())
    {
        MachineID = Subsystem->GetMachineID();
        TrialRequest = Subsystem->GenerateTrialRequest();
    }

    const FString ProjectID = FApp::GetProjectName();

    FString InfoText = FString::Printf(
        TEXT("项目ID | Project ID:\n%s\n\n机器ID | Machine ID:\n%s\n\n试用请求 | Trial Request:\n%s\n\n"),
        *ProjectID,
        *MachineID,
        *TrialRequest
    );

    InfoText += TEXT("说明：请将上述\"试用请求\"发送给开发者以获取许可证密钥。\n");
    InfoText += TEXT("Note: Send the above \"Trial Request\" to the developer to obtain a license key.");

    CachedTrialRequestInfo = FText::FromString(InfoText);

    ChildSlot
    [
        GetContent()
    ];
}

/**
 * @brief 构建弹窗主体内容
 *
 * 默认实现：包含标题、信息区域、试用请求信息和按钮栏。
 */
TSharedRef<SWidget> STrialExpiredWidget::GetContent()
{
    // 默认UI实现：包含试用请求信息的弹窗布局（优化为自适应布局）
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
        .Padding(20.f)
        [
            SNew(SVerticalBox)

            // 标题
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(STextBlock)
                .Text(this, &STrialExpiredWidget::GetTitleText)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
                .Justification(ETextJustify::Center)
            ]

            // 消息内容
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 15)
            [
                SNew(STextBlock)
                .Text(this, &STrialExpiredWidget::GetMessageText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                .AutoWrapText(true)
                .Justification(ETextJustify::Center)
            ]

            // 试用请求信息区域（自适应填充剩余空间）
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 0, 0, 10)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                .Padding(10.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SNew(SMultiLineEditableTextBox)
                        .Text(this, &STrialExpiredWidget::GetTrialRequestInfo)
                        .IsReadOnly(true)
                        .Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
                    ]
                ]
            ]

            // 许可证输入区域
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 5)
            [
                SNew(STextBlock)
                .Text(this, &STrialExpiredWidget::GetLicenseInputHintText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 5)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                .Padding(5.f)
                [
                    SNew(SEditableTextBox)
                    .HintText(FText::FromString(TEXT("请输入许可证密钥 | Enter license key here")))
                    .OnTextChanged(this, &STrialExpiredWidget::OnLicenseInputTextChanged)
                    .Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
                ]
            ]

            // 验证状态消息
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 15)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(this, &STrialExpiredWidget::GetValidationStatusText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                    .AutoWrapText(true)
                    .WrapTextAt(560.0f)
                    .ColorAndOpacity_Lambda([this]() -> FSlateColor
                    {
                        return bValidationSuccess ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Red);
                    })
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 6, 0, 0)
                [
                    SNew(STextBlock)
                    .Text(this, &STrialExpiredWidget::GetLicenseWatermarkStatusText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                    .AutoWrapText(true)
                    .WrapTextAt(560.0f)
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.74f, 0.84f, 0.96f, 1.0f)))
                ]
            ]

            // 按钮区域
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 6)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("试用申请文件 | Trial Request File")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 10)
                [
                    SNew(SUniformGridPanel)
                    .SlotPadding(FMargin(5.f, 0.f))

                    + SUniformGridPanel::Slot(0, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnCopyTrialRequestClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetCopyButtonText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                        ]
                    ]

                    + SUniformGridPanel::Slot(1, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnExportTrialRequestClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetExportButtonText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                        ]
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 6)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("许可证导入与验证 | License Import & Validation")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SUniformGridPanel)
                    .SlotPadding(FMargin(5.f, 0.f))

                    + SUniformGridPanel::Slot(0, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnImportLicenseClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetImportButtonText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                        ]
                    ]

                    + SUniformGridPanel::Slot(1, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnValidateLicenseClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetValidateButtonText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                        ]
                    ]

                    + SUniformGridPanel::Slot(2, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnQuitClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetQuitButtonText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                        ]
                    ]

                    + SUniformGridPanel::Slot(3, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .IsEnabled_Lambda([this]() { return bValidationSuccess; })
                        .OnClicked(this, &STrialExpiredWidget::OnEnterAppClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetEnterButtonText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                        ]
                    ]
                ]
            ]
        ];
}

/**
 * @brief 验证许可证按钮回调
 *
 * 解析并验证用户输入的许可证密钥。
 */
FReply STrialExpiredWidget::OnValidateLicenseClicked()
{
    bValidationSuccess = false;
    ValidationStatusMessage = TEXT("");

	// 检查输入是否为空
	if (LicenseInputText.IsEmpty())
	{
		ValidationStatusMessage = TEXT("❌ 请输入许可证密钥 | Please enter a license key");
		return FReply::Handled();
	}

	FString NormalizedLicenseKey = LicenseInputText;
	NormalizedLicenseKey.TrimStartAndEndInline();
	NormalizedLicenseKey.ReplaceInline(TEXT("\r"), TEXT(""));
	NormalizedLicenseKey.ReplaceInline(TEXT("\n"), TEXT(""));
	NormalizedLicenseKey.ReplaceInline(TEXT("\t"), TEXT(""));
	NormalizedLicenseKey.ReplaceInline(TEXT(" "), TEXT(""));
	if (NormalizedLicenseKey.IsEmpty())
	{
		ValidationStatusMessage = TEXT("❌ 许可证密钥为空 | License key is empty");
		return FReply::Handled();
	}

	UE_LOG(LogTemp, Log, TEXT("[TrialLicense] Validate input normalized. Length=%d Prefix=%s"), NormalizedLicenseKey.Len(), *NormalizedLicenseKey.Left(16));

    // 获取子系统进行验证
    UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForSlate();
    if (!Subsystem)
    {
        ValidationStatusMessage = TEXT("❌ 无法访问许可证子系统 | Cannot access license subsystem");
        return FReply::Handled();
    }

    // 验证许可证
	if (Subsystem->ValidateLicense(NormalizedLicenseKey))
	{
		// 解析许可证信息
		FTrialLicenseInfo Info = Subsystem->ParseLicenseKey(NormalizedLicenseKey);
        
        if (Info.bPerpetual)
        {
            ValidationStatusMessage = TEXT("✓ 永久许可证验证成功！| Perpetual license validated successfully!");
        }
        else
        {
			const float RemainingTimeSeconds = Subsystem->GetTrialRemainingTime(NormalizedLicenseKey);
			const FString RemainingCN = FormatRemainingTimeSegmentedCN(RemainingTimeSeconds);
			const FString RemainingEN = FormatRemainingTimeSegmentedEN(RemainingTimeSeconds);
            
            ValidationStatusMessage = FString::Printf(
				TEXT("✓ 许可证验证成功！剩余 %s | License validated! %s remaining"),
				*RemainingCN,
				*RemainingEN
            );
        }
        
        bValidationSuccess = true;
        
        // 更新设置中的许可证密钥
		if (UTrialLicenseSettings* Settings = GetMutableDefault<UTrialLicenseSettings>())
		{
			Settings->LicenseKey = NormalizedLicenseKey;
			Settings->SaveConfig();
			UE_LOG(LogTemp, Log, TEXT("[TrialLicense] License key updated in settings"));
		}

		FTrialWatermarkOverlay::Get().OnLicenseValidated(NormalizedLicenseKey);

        ValidationStatusMessage += TEXT("\n点击“进入应用”继续。| Click 'Enter App' to continue.");
    }
	else
	{
		const FString DetailedError = Subsystem->GetLastValidationError();
		if (DetailedError.IsEmpty())
		{
			ValidationStatusMessage = TEXT("❌ 许可证验证失败！请检查密钥是否正确 | License validation failed! Please check the key");
			UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed with empty detailed error. InputLen=%d"), NormalizedLicenseKey.Len());
		}
		else
		{
			ValidationStatusMessage = FString::Printf(
				TEXT("❌ %s\n(详见日志关键字: [TrialLicense])"),
				*DetailedError
			);
			UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Detailed validation error: %s"), *DetailedError);
			UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validation context: InputLen=%d, Prefix=%s"), NormalizedLicenseKey.Len(), *NormalizedLicenseKey.Left(32));
		}
		bValidationSuccess = false;
	}

    return FReply::Handled();
}

/**
 * @brief 退出按钮回调
 *
 * 默认行为：先关闭模态窗口，然后请求退出应用，子类可重写。
 */
FReply STrialExpiredWidget::OnQuitClicked()
{
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	FTrialWatermarkOverlay::Get().ResetExpiredDialogState();
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
    }

#if WITH_EDITOR
    if (GIsEditor && GEditor && GEditor->PlayWorld)
    {
        FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([](float)
            {
                if (GEditor && GEditor->PlayWorld)
                {
                    GEditor->RequestEndPlayMap();
                }
                return false;
            }),
            0.0f);
        return FReply::Handled();
    }
#endif

    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([](float)
        {
            FPlatformMisc::RequestExit(false);
            return false;
        }),
        0.0f);

    return FReply::Handled();
}

FReply STrialExpiredWidget::OnEnterAppClicked()
{
    if (!bValidationSuccess)
    {
        return FReply::Handled();
    }

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	FTrialWatermarkOverlay::Get().ResetExpiredDialogState();
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
    }

    return FReply::Handled();
}

/**
 * @brief 更新过期原因
 */
void STrialExpiredWidget::SetExpireReason(const FString& InReason)
{
    ExpireReason = InReason;
}

/**
 * @brief 更新剩余时间（秒）
 */
void STrialExpiredWidget::SetRemainingTime(float InSeconds)
{
    RemainingTime = InSeconds;
}

/**
 * @brief 获取弹窗标题文本
 */
FText STrialExpiredWidget::GetTitleText() const
{
    if (bIsPerpetual)
    {
        return FText::FromString(TEXT("许可证验证失败 | License Validation Failed"));
    }
    else if (RemainingTime <= 0.f)
    {
        return FText::FromString(TEXT("试用期已过期 | Trial Period Expired"));
    }
    else
    {
        return FText::FromString(TEXT("试用许可证提示 | Trial License Notice"));
    }
}

/**
 * @brief 获取主要提示信息
 */
FText STrialExpiredWidget::GetMessageText() const
{
    if (!ExpireReason.IsEmpty())
    {
        return FText::FromString(ExpireReason);
    }

    if (bIsPerpetual)
    {
        return FText::FromString(TEXT("许可证验证失败，请联系开发者获取有效许可证。\n\nLicense validation failed. Please contact the developer for a valid license."));
    }
    else if (RemainingTime <= 0.f)
    {
        return FText::FromString(TEXT("您的试用期已到期。\n请联系开发者获取正式许可证以继续使用。\n\nYour trial period has expired.\nPlease contact the developer for a full license."));
    }
    else
    {
        const int32 Hours = FMath::FloorToInt(RemainingTime / 3600.f);
        const int32 Minutes = FMath::FloorToInt(FMath::Fmod(RemainingTime, 3600.f) / 60.f);
        
        return FText::FromString(FString::Printf(
            TEXT("剩余试用时间：%d 小时 %d 分钟\n\nRemaining trial time: %d hours %d minutes"),
            Hours, Minutes, Hours, Minutes
        ));
    }
}

/**
 * @brief 获取退出按钮文本
 */
FText STrialExpiredWidget::GetQuitButtonText() const
{
    return FText::FromString(TEXT("退出应用 | Quit"));
}

FText STrialExpiredWidget::GetEnterButtonText() const
{
    if (bValidationSuccess)
    {
        return FText::FromString(TEXT("进入应用 | Enter App"));
    }

    return FText::FromString(TEXT("验证后可进入 | Validate First"));
}

/**
 * @brief 复制试用请求按钮回调
 *
 * 生成试用请求并写入系统剪贴板。
 */
FReply STrialExpiredWidget::OnCopyTrialRequestClicked()
{
    // 复制试用请求到剪贴板
    const FString TrialRequest = GenerateTrialRequestString();
    if (!TrialRequest.IsEmpty() && FSlateApplication::IsInitialized())
    {
        // 使用Slate的剪贴板功能（跨平台）
        FPlatformApplicationMisc::ClipboardCopy(*TrialRequest);
        UE_LOG(LogTemp, Log, TEXT("[TrialLicense] Trial request copied to clipboard"));
    }
    return FReply::Handled();
}

FReply STrialExpiredWidget::OnExportTrialRequestClicked()
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForSlate())
    {
        ValidationStatusMessage = Subsystem->ExportTrialRequestWithDialog()
            ? FString::Printf(TEXT("✓ %s"), *Subsystem->GetLastFileOperationMessage())
            : FString::Printf(TEXT("❌ %s"), *Subsystem->GetLastFileOperationMessage());
    }
    else
    {
        ValidationStatusMessage = TEXT("❌ 无法访问许可证子系统 | Cannot access license subsystem");
    }
    return FReply::Handled();
}

FReply STrialExpiredWidget::OnImportLicenseClicked()
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForSlate())
    {
        const bool bSuccess = Subsystem->ImportLicenseWithDialog();
        ValidationStatusMessage = bSuccess
            ? FString::Printf(TEXT("✓ %s"), *Subsystem->GetLastFileOperationMessage())
            : FString::Printf(TEXT("❌ %s"), *Subsystem->GetLastFileOperationMessage());
        bValidationSuccess = bSuccess;
    }
    else
    {
        ValidationStatusMessage = TEXT("❌ 无法访问许可证子系统 | Cannot access license subsystem");
        bValidationSuccess = false;
    }
    return FReply::Handled();
}

/**
 * @brief 获取“复制请求”按钮文本
 */
FText STrialExpiredWidget::GetCopyButtonText() const
{
    return FText::FromString(TEXT("复制请求串 | Copy Request"));
}

FText STrialExpiredWidget::GetExportButtonText() const
{
    return FText::FromString(TEXT("导出 .request 文件 | Export .request"));
}

FText STrialExpiredWidget::GetImportButtonText() const
{
    return FText::FromString(TEXT("导入 .lic 文件 | Import .lic"));
}

/**
 * @brief 获取试用请求信息文本
 *
 * 返回缓存的试用请求信息，包含项目ID、机器ID、试用请求加密串及说明。
 */
FText STrialExpiredWidget::GetTrialRequestInfo() const
{
    return CachedTrialRequestInfo;
}

/**
 * @brief 获取验证按钮文本
 */
FText STrialExpiredWidget::GetValidateButtonText() const
{
    return FText::FromString(TEXT("验证输入密钥 | Validate Key"));
}

/**
 * @brief 获取许可证输入提示文本
 */
FText STrialExpiredWidget::GetLicenseInputHintText() const
{
    return FText::FromString(TEXT("输入许可证密钥 | Enter License Key:"));
}

/**
 * @brief 获取验证状态消息文本
 */
FText STrialExpiredWidget::GetValidationStatusText() const
{
    return FText::FromString(ValidationStatusMessage);
}

FText STrialExpiredWidget::GetLicenseWatermarkStatusText() const
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForSlate())
    {
        if (const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>())
        {
            if (Settings->LicenseKey.IsEmpty())
            {
                return FText::FromString(TEXT("当前未加载许可证，水印显示使用项目默认策略。"));
            }

            const FTrialLicenseInfo Info = Subsystem->ParseLicenseKey(Settings->LicenseKey);
            return FText::FromString(
                Info.bShowWatermark
                    ? TEXT("当前许可证要求显示水印。")
                    : TEXT("当前许可证要求隐藏水印。"));
        }
    }

    return FText::FromString(TEXT("无法读取当前许可证的水印状态。"));
}

/**
 * @brief 许可证输入文本变更回调
 */
void STrialExpiredWidget::OnLicenseInputTextChanged(const FText& InText)
{
    LicenseInputText = InText.ToString();
    // 清除之前的验证状态
    ValidationStatusMessage = TEXT("");
    bValidationSuccess = false;
}

/**
 * @brief 生成试用请求加密字符串
 *
 * 调用子系统生成，失败返回空字符串。
 */
FString STrialExpiredWidget::GenerateTrialRequestString() const
{
    // GenerateTrialRequest 不需要参数，它会自动从设置中读取
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystemForSlate())
    {
        return Subsystem->GenerateTrialRequest();
    }
    return FString();
}
