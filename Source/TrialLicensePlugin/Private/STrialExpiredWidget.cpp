// Copyright 2025, Rune. All Rights Reserved.

#include "STrialExpiredWidget.h"
#include "UTrialLicenseLibrary.h"
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

FReply STrialExpiredWidget::OnQuitClicked()
{
    // 默认实现：退出应用
    // 子类可以重写此方法实现自定义逻辑
    FPlatformMisc::RequestExit(false);
    return FReply::Handled();
}

void STrialExpiredWidget::SetExpireReason(const FString& InReason)
{
    ExpireReason = InReason;
}

void STrialExpiredWidget::SetRemainingTime(float InSeconds)
{
    RemainingTime = InSeconds;
}

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

FText STrialExpiredWidget::GetCloseButtonText() const
{
    return FText::FromString(TEXT("关闭 | Close"));
}

FText STrialExpiredWidget::GetQuitButtonText() const
{
    return FText::FromString(TEXT("退出应用 | Quit"));
}

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

FText STrialExpiredWidget::GetCopyButtonText() const
{
    return FText::FromString(TEXT("复制请求 | Copy Request"));
}

FText STrialExpiredWidget::GetTrialRequestInfo() const
{
    const FString MachineID = UTrialLicenseLibrary::GetMachineID();
    const FString ProjectID = FApp::GetProjectName();
    const FString TrialRequest = GenerateTrialRequestString();

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

FString STrialExpiredWidget::GenerateTrialRequestString() const
{
    // GenerateTrialRequest 不需要参数，它会自动从设置中读取
    return UTrialLicenseLibrary::GenerateTrialRequest();
}
