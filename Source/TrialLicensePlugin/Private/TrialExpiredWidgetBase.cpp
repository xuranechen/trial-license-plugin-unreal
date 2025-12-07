// Copyright 2025, Rune. All Rights Reserved.

#include "TrialExpiredWidgetBase.h"
#include "UTrialLicenseLibrary.h"
#include "TrialLicenseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/App.h"

void UTrialExpiredWidgetBase::InitializeExpiredWidget(const FString& InReason, float InRemainingTime, bool bInIsPerpetual)
{
    ExpireReason = InReason;
    RemainingTime = InRemainingTime;
    bIsPerpetual = bInIsPerpetual;

    // 触发蓝图事件
    OnExpiredWidgetInitialized();
}

void UTrialExpiredWidgetBase::CloseDialog()
{
    // 从父窗口移除
    RemoveFromParent();
}

void UTrialExpiredWidgetBase::QuitApplication()
{
    // 退出应用
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

FText UTrialExpiredWidgetBase::GetTitleText_Implementation() const
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

FText UTrialExpiredWidgetBase::GetMessageText_Implementation() const
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
        return GetFormattedRemainingTime();
    }
}

FText UTrialExpiredWidgetBase::GetCloseButtonText_Implementation() const
{
    return FText::FromString(TEXT("关闭 | Close"));
}

FText UTrialExpiredWidgetBase::GetQuitButtonText_Implementation() const
{
    return FText::FromString(TEXT("退出应用 | Quit"));
}

FText UTrialExpiredWidgetBase::GetFormattedRemainingTime() const
{
    const int32 Hours = FMath::FloorToInt(RemainingTime / 3600.f);
    const int32 Minutes = FMath::FloorToInt(FMath::Fmod(RemainingTime, 3600.f) / 60.f);
    
    return FText::FromString(FString::Printf(
        TEXT("剩余试用时间：%d 小时 %d 分钟\n\nRemaining trial time: %d hours %d minutes"),
        Hours, Minutes, Hours, Minutes
    ));
}

void UTrialExpiredWidgetBase::CopyTrialRequestToClipboard()
{
    const FString TrialRequest = GenerateTrialRequestString();
    if (!TrialRequest.IsEmpty())
    {
        // 使用跨平台剪贴板API
        FPlatformApplicationMisc::ClipboardCopy(*TrialRequest);
        UE_LOG(LogTemp, Log, TEXT("[TrialLicense] Trial request copied to clipboard"));
    }
}

FString UTrialExpiredWidgetBase::GenerateTrialRequestString()
{
    // GenerateTrialRequest 不需要参数，它会自动从设置中读取
    return UTrialLicenseLibrary::GenerateTrialRequest();
}

FString UTrialExpiredWidgetBase::GetMachineID() const
{
    return UTrialLicenseLibrary::GetMachineID();
}

FString UTrialExpiredWidgetBase::GetProjectID() const
{
    return FApp::GetProjectName();
}

FText UTrialExpiredWidgetBase::GetTrialRequestInfo() const
{
    const FString MachineID = GetMachineID();
    const FString ProjectID = GetProjectID();
    const FString TrialRequest = const_cast<UTrialExpiredWidgetBase*>(this)->GenerateTrialRequestString();

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
