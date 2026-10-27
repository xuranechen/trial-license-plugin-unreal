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
#include "FTrialLicenseInfo.h"
#include "TrialWatermarkOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/App.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

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

static FString FormatRemainingTimeSegmentedCN_UMG(float RemainingSeconds)
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

static FString FormatRemainingTimeSegmentedEN_UMG(float RemainingSeconds)
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
 * @brief 初始化过期Widget
 *
 * 保存过期原因、剩余时间及是否永久授权，并触发蓝图事件。
 */
void UTrialExpiredWidgetBase::InitializeExpiredWidget(const FString& InReason, float InRemainingTime, bool bInIsPerpetual)
{
    ExpireReason = InReason;
    RemainingTime = InRemainingTime;
    bIsPerpetual = bInIsPerpetual;
    bValidationSuccess = false;
    ValidationStatusMessage = TEXT("");

    // 直接生成并缓存试用请求信息，避免每次 UI 刷新时重新生成
    const FString MachineID = GetMachineID();
    const FString ProjectID = GetProjectID();
    const FString TrialRequest = GenerateTrialRequestString();

    FString InfoText = FString::Printf(
        TEXT("项目ID | Project ID:\n%s\n\n机器ID | Machine ID:\n%s\n\n试用请求 | Trial Request:\n%s\n\n"),
        *ProjectID,
        *MachineID,
        *TrialRequest
    );

    InfoText += TEXT("说明：请将上述\"试用请求\"发送给开发者以获取许可证密钥。\n");
    InfoText += TEXT("Note: Send the above \"Trial Request\" to the developer to obtain a license key.");

    CachedTrialRequestInfo = FText::FromString(InfoText);

    // 触发蓝图事件
    OnExpiredWidgetInitialized();
}

/**
 * @brief 关闭弹窗
 */
void UTrialExpiredWidgetBase::CloseDialog()
{
	FTrialWatermarkOverlay::Get().ResetExpiredDialogState();
    // 从父窗口移除
    RemoveFromParent();
}

/**
 * @brief 退出应用
 */
void UTrialExpiredWidgetBase::QuitApplication()
{
	FTrialWatermarkOverlay::Get().ResetExpiredDialogState();
    // 退出应用
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void UTrialExpiredWidgetBase::NativeDestruct()
{
	FTrialWatermarkOverlay::Get().ResetExpiredDialogState();
	Super::NativeDestruct();
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

bool UTrialExpiredWidgetBase::ExportTrialRequestToFile(const FString& FilePath)
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        const bool bSuccess = Subsystem->ExportTrialRequestToFile(FilePath);
        FileOperationStatusMessage = Subsystem->GetLastFileOperationMessage();
        return bSuccess;
    }
    FileOperationStatusMessage = TEXT("无法访问许可证子系统 | Cannot access license subsystem");
    return false;
}

bool UTrialExpiredWidgetBase::ExportTrialRequestWithDialog()
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        const bool bSuccess = Subsystem->ExportTrialRequestWithDialog();
        FileOperationStatusMessage = Subsystem->GetLastFileOperationMessage();
        return bSuccess;
    }
    FileOperationStatusMessage = TEXT("无法访问许可证子系统 | Cannot access license subsystem");
    return false;
}

bool UTrialExpiredWidgetBase::ImportLicenseFromFile(const FString& FilePath)
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        const bool bSuccess = Subsystem->ImportLicenseFromFile(FilePath);
        FileOperationStatusMessage = Subsystem->GetLastFileOperationMessage();
        if (bSuccess)
        {
            ValidationStatusMessage = FileOperationStatusMessage;
            bValidationSuccess = true;
            OnLicenseValidated();
        }
        return bSuccess;
    }
    FileOperationStatusMessage = TEXT("无法访问许可证子系统 | Cannot access license subsystem");
    return false;
}

bool UTrialExpiredWidgetBase::ImportLicenseWithDialog()
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        const bool bSuccess = Subsystem->ImportLicenseWithDialog();
        FileOperationStatusMessage = Subsystem->GetLastFileOperationMessage();
        if (bSuccess)
        {
            ValidationStatusMessage = FileOperationStatusMessage;
            bValidationSuccess = true;
            OnLicenseValidated();
        }
        return bSuccess;
    }
    FileOperationStatusMessage = TEXT("无法访问许可证子系统 | Cannot access license subsystem");
    return false;
}

FText UTrialExpiredWidgetBase::GetFileOperationStatusText() const
{
    return FText::FromString(FileOperationStatusMessage);
}

FText UTrialExpiredWidgetBase::GetLicenseWatermarkStatusText() const
{
    if (UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem())
    {
        const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
        const FString LicenseKey = Settings ? Settings->LicenseKey : FString();
        if (LicenseKey.IsEmpty())
        {
            return FText::FromString(TEXT("当前未加载许可证，水印显示使用项目默认策略。"));
        }

        const FTrialLicenseInfo Info = Subsystem->ParseLicenseKey(LicenseKey);
        return FText::FromString(
            Info.bShowWatermark
                ? TEXT("当前许可证要求显示水印。")
                : TEXT("当前许可证要求隐藏水印。"));
    }

    return FText::FromString(TEXT("无法读取当前许可证的水印状态。"));
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
 * @brief 获取验证按钮文本（可在蓝图重写）
 */
FText UTrialExpiredWidgetBase::GetValidateButtonText_Implementation() const
{
    return FText::FromString(TEXT("验证许可证 | Validate"));
}

/**
 * @brief 验证许可证密钥
 */
bool UTrialExpiredWidgetBase::ValidateLicenseKey(const FString& LicenseKey)
{
    bValidationSuccess = false;
    ValidationStatusMessage = TEXT("");

    // 检查输入是否为空
    if (LicenseKey.IsEmpty())
    {
        ValidationStatusMessage = TEXT("❌ 请输入许可证密钥 | Please enter a license key");
        return false;
    }

    // 获取子系统进行验证
    UTrialLicenseSubsystem* Subsystem = GetTrialLicenseSubsystem();
    if (!Subsystem)
    {
        ValidationStatusMessage = TEXT("❌ 无法访问许可证子系统 | Cannot access license subsystem");
        return false;
    }

    // 验证许可证
    if (Subsystem->ValidateLicense(LicenseKey))
    {
        // 解析许可证信息
        FTrialLicenseInfo Info = Subsystem->ParseLicenseKey(LicenseKey);
        
        if (Info.bPerpetual)
        {
            ValidationStatusMessage = TEXT("✓ 永久许可证验证成功！| Perpetual license validated successfully!");
        }
        else
        {
            const float RemainingTimeValue = Subsystem->GetTrialRemainingTime(LicenseKey);
            const FString RemainingCN = FormatRemainingTimeSegmentedCN_UMG(RemainingTimeValue);
            const FString RemainingEN = FormatRemainingTimeSegmentedEN_UMG(RemainingTimeValue);
            
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
            Settings->LicenseKey = LicenseKey;
            Settings->SaveConfig();
            UE_LOG(LogTemp, Log, TEXT("[TrialLicense] License key updated in settings"));
        }

        // 触发蓝图事件
        OnLicenseValidated();

        // 延迟关闭弹窗，让用户看到成功消息
        FTimerHandle TimerHandle;
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                CloseDialog();
            }, 2.0f, false);
        }

        return true;
    }
    else
    {
        ValidationStatusMessage = TEXT("❌ 许可证验证失败！请检查密钥是否正确 | License validation failed! Please check the key");
        bValidationSuccess = false;
        return false;
    }
}

/**
 * @brief 获取验证状态消息文本
 */
FText UTrialExpiredWidgetBase::GetValidationStatusText() const
{
    return FText::FromString(ValidationStatusMessage);
}

/**
 * @brief 获取验证是否成功
 */
bool UTrialExpiredWidgetBase::IsValidationSuccessful() const
{
    return bValidationSuccess;
}

/**
 * @brief 组合试用请求信息文本
 *
 * 返回缓存的试用请求信息，包含项目ID、机器ID、试用请求加密串及说明。
 */
FText UTrialExpiredWidgetBase::GetTrialRequestInfo() const
{
    return CachedTrialRequestInfo;
}
