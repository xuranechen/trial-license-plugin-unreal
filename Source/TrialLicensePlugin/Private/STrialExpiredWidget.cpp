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
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Misc/App.h"
#include "Framework/Application/SlateApplication.h"
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
	// 回退方案：尝试从GEngine获取
	if (GEngine && GEngine->GameViewport)
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
 * @brief 构建过期弹窗Widget
 *
 * 保存入参并设置根部件。
 */
void STrialExpiredWidget::Construct(const FArguments& InArgs)
{
    ExpireReason = InArgs._ExpireReason;
    RemainingTime = InArgs._RemainingTime;
    bIsPerpetual = InArgs._bIsPerpetual;

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
    // 默认UI实现：包含试用请求信息的弹窗布局
    return SNew(SBox)
        .MinDesiredWidth(600.f)
        .MinDesiredHeight(400.f)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBorder)
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
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                    .Justification(ETextJustify::Center)
                ]

                // 消息内容
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 10)
                [
                    SNew(STextBlock)
                    .Text(this, &STrialExpiredWidget::GetMessageText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                    .AutoWrapText(true)
                    .Justification(ETextJustify::Center)
                ]

                // 试用请求信息区域
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
                            .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
                        ]
                    ]
                ]

                // 按钮区域
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SUniformGridPanel)
                    .SlotPadding(FMargin(5.f, 0.f))

                    // 复制试用请求按钮
                    + SUniformGridPanel::Slot(0, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnCopyTrialRequestClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetCopyButtonText)
                        ]
                    ]

                    // 关闭按钮
                    + SUniformGridPanel::Slot(1, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnCloseClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetCloseButtonText)
                        ]
                    ]

                    // 退出应用按钮
                    + SUniformGridPanel::Slot(2, 0)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center)
                        .OnClicked(this, &STrialExpiredWidget::OnQuitClicked)
                        [
                            SNew(STextBlock)
                            .Text(this, &STrialExpiredWidget::GetQuitButtonText)
                        ]
                    ]
                ]
            ]
        ];
}

/**
 * @brief 关闭按钮回调
 *
 * 默认行为：销毁所属窗口，子类可重写。
 */
FReply STrialExpiredWidget::OnCloseClicked()
{
    // 默认实现：关闭弹窗
    // 子类可以重写此方法实现自定义逻辑
    TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
    if (ParentWindow.IsValid())
    {
        ParentWindow->RequestDestroyWindow();
    }
    return FReply::Handled();
}

/**
 * @brief 退出按钮回调
 *
 * 默认行为：请求退出应用，子类可重写。
 */
FReply STrialExpiredWidget::OnQuitClicked()
{
    // 默认实现：退出应用
    // 子类可以重写此方法实现自定义逻辑
    FPlatformMisc::RequestExit(false);
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
 * @brief 获取关闭按钮文本
 */
FText STrialExpiredWidget::GetCloseButtonText() const
{
    return FText::FromString(TEXT("关闭 | Close"));
}

/**
 * @brief 获取退出按钮文本
 */
FText STrialExpiredWidget::GetQuitButtonText() const
{
    return FText::FromString(TEXT("退出应用 | Quit"));
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

/**
 * @brief 获取“复制请求”按钮文本
 */
FText STrialExpiredWidget::GetCopyButtonText() const
{
    return FText::FromString(TEXT("复制请求 | Copy Request"));
}

/**
 * @brief 获取试用请求信息文本
 *
 * 包含项目ID、机器ID、试用请求加密串及说明。
 */
FText STrialExpiredWidget::GetTrialRequestInfo() const
{
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

    return FText::FromString(InfoText);
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
