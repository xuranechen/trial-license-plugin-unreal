// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用过期弹窗基类（UMG）
 *
 * 提供UMG版本的默认过期弹窗逻辑：标题、提示信息、剩余时间格式化、
 * 复制试用请求等，可在蓝图或C++中派生自定义UI。
 */
#include "TrialExpiredWidgetBase.h"
#include "UTrialLicenseSubsystem.h"
#include "TrialLicenseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/App.h"
#include "Engine/Engine.h"

// 辅助函数：获取试用许可证子系统
/**
 * @brief 获取试用许可证子系统实例
 *
 * 优先从当前播放世界的GameInstance获取，若不可用则尝试从GEngine->GameViewport获取。
 *
 * @return 子系统指针，失败返回nullptr
 */
static UTrialLicenseSubsystem* GetTrialLicenseSubsystem()
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
 * @brief 初始化过期Widget
 *
 * 保存过期原因、剩余时间及是否永久授权，并触发蓝图事件。
 */
void UTrialExpiredWidgetBase::InitializeExpiredWidget(const FString& InReason, float InRemainingTime, bool bInIsPerpetual)
{
    ExpireReason = InReason;
    RemainingTime = InRemainingTime;
    bIsPerpetual = bInIsPerpetual;

    // 触发蓝图事件
    OnExpiredWidgetInitialized();
}

/**
 * @brief 关闭弹窗
 */
void UTrialExpiredWidgetBase::CloseDialog()
{
    // 从父窗口移除
    RemoveFromParent();
}

/**
 * @brief 退出应用
 */
void UTrialExpiredWidgetBase::QuitApplication()
{
    // 退出应用
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

/**
 * @brief 获取标题文本（可在蓝图重写）
 */
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

/**
 * @brief 获取提示信息文本（可在蓝图重写）
 */
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

/**
 * @brief 获取关闭按钮文本（可在蓝图重写）
 */
FText UTrialExpiredWidgetBase::GetCloseButtonText_Implementation() const
{
    return FText::FromString(TEXT("关闭 | Close"));
}

/**
 * @brief 获取退出按钮文本（可在蓝图重写）
 */
FText UTrialExpiredWidgetBase::GetQuitButtonText_Implementation() const
{
    return FText::FromString(TEXT("退出应用 | Quit"));
}

/**
 * @brief 格式化剩余时间文本
 */
FText UTrialExpiredWidgetBase::GetFormattedRemainingTime() const
{
    const int32 Hours = FMath::FloorToInt(RemainingTime / 3600.f);
    const int32 Minutes = FMath::FloorToInt(FMath::Fmod(RemainingTime, 3600.f) / 60.f);
    
    return FText::FromString(FString::Printf(
        TEXT("剩余试用时间：%d 小时 %d 分钟\n\nRemaining trial time: %d hours %d minutes"),
        Hours, Minutes, Hours, Minutes
    ));
}

/**
 * @brief 复制试用请求到剪贴板
 */
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

/**
 * @brief 生成试用请求加密字符串
 *
 * 从子系统获取，失败返回空字符串。
 */
FString UTrialExpiredWidgetBase::GenerateTrialRequestString()
{
    // GenerateTrialRequest 不需要参数，它会自动从设置中读取
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        return Subsystem->GenerateTrialRequest();
    }
    return FString();
}

/**
 * @brief 获取当前机器ID
 */
FString UTrialExpiredWidgetBase::GetMachineID() const
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        return Subsystem->GetMachineID();
    }
    return FString();
}

/**
 * @brief 获取项目ID（项目名）
 */
FString UTrialExpiredWidgetBase::GetProjectID() const
{
    return FApp::GetProjectName();
}

/**
 * @brief 组合试用请求信息文本
 *
 * 包含项目ID、机器ID、试用请求加密串及说明。
 */
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
