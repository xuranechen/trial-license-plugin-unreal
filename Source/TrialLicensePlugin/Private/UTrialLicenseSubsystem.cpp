// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用许可证子系统实现
 *
 * 该文件实现了UTrialLicenseSubsystem类，负责试用许可证系统的核心业务逻辑。
 * 包括许可证验证、会话管理、时间跟踪、水印控制等功能。
 * 作为UGameInstanceSubsystem的子类，确保在游戏实例生命周期内正常工作。
 */
#include "UTrialLicenseSubsystem.h"
#include "MachineIDHelper.h"
#include "TimeGuard.h"
#include "LicenseEncryptor.h"
#include "TrialDataStorage.h"
#include "TrialLicenseSettings.h"
#include "TrialWatermarkOverlay.h"
#include "Engine/Engine.h"
#include "Misc/EngineVersion.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/App.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Containers/Ticker.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"

/**
 * @brief 构造函数：初始化默认状态
 */
UTrialLicenseSubsystem::UTrialLicenseSubsystem()
	: bTrialSessionActive(false)
	, TrialLastFlushedSessionSeconds(0.0)
	, TrialFlushIntervalSeconds(300.0)
	, bTrialPeriodicEnabled(true)
	, LastLicenseMonitorCheckTime(0.0)
{
}

/**
 * @brief 子系统初始化
 *
 * 重置会话状态并从配置读取周期性写盘开关与间隔。
 */
void UTrialLicenseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 初始化会话状态
	bTrialSessionActive = false;
	TrialSessionStart = FDateTime();
	TrialSessionHash.Empty();
	TrialSessionMachineID.Empty();
	TrialLastFlushedSessionSeconds = 0.0;

	// 从配置读取周期性保存开关与间隔
	{
		bool bEnable = true;
		float IntervalSec = 300.0f;
		if (GConfig)
		{
			GConfig->GetBool(TEXT("TrialLicensePlugin"), TEXT("TrialPeriodicSaveEnabled"), bEnable, GGameIni);
			GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("TrialPeriodicSaveIntervalSeconds"), IntervalSec, GGameIni);
		}
		bTrialPeriodicEnabled = bEnable;
		TrialFlushIntervalSeconds = FMath::Max(30.0, (double)IntervalSec);
	}

	if (!LicenseMonitorTickerHandle.IsValid())
	{
		LicenseMonitorTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateStatic(&UTrialLicenseSubsystem::LicenseMonitorTick, this),
			1.0f);
	}

	RefreshLicenseRuntimeState();
}

/**
 * @brief 子系统反初始化
 *
 * 结束会话并持久化试用数据。
 */
void UTrialLicenseSubsystem::Deinitialize()
{
	// 结束会话并持久化数据
	EndTrialSessionAndPersist();

	if (LicenseMonitorTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(LicenseMonitorTickerHandle);
		LicenseMonitorTickerHandle = FTSTicker::FDelegateHandle();
	}

	Super::Deinitialize();
}

/**
 * @brief 生成许可证密钥
 *
 * @param DurationHours 时长（小时）
 * @param ExpiryDate 绝对过期时间
 * @param PassPhrase 加密口令
 * @param ProjectId 项目ID
 * @param TargetMachineID 目标机器ID，空则使用当前机器
 * @param bPerpetual 是否为永久授权
 * @return 生成的许可证密钥字符串
 */
FString UTrialLicenseSubsystem::GenerateLicenseKey(float DurationHours, const FDateTime& ExpiryDate, const FString& ProjectId, const FString& TargetMachineID, bool bPerpetual)
{
	const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
	const bool bDefaultShowWatermark = bPerpetual ? false : (Settings ? Settings->bDefaultTrialLicenseShowWatermark : true);
	return GenerateLicenseKeyAdvanced(DurationHours, ExpiryDate, ProjectId, TargetMachineID, bPerpetual, bDefaultShowWatermark);
}

FString UTrialLicenseSubsystem::GenerateLicenseKeyAdvanced(float DurationHours, const FDateTime& ExpiryDate, const FString& ProjectId, const FString& TargetMachineID, bool bPerpetual, bool bShowWatermark)
{
	// 创建许可证信息
	FTrialLicenseInfo LicenseInfo;
	LicenseInfo.DurationHours = DurationHours;
	LicenseInfo.GenerateTime = FDateTime::Now();
	LicenseInfo.ExpireTime = ExpiryDate;
	LicenseInfo.MachineID = TargetMachineID.IsEmpty() ? GetMachineID() : TargetMachineID;
	LicenseInfo.ProjectId = ProjectId;
	LicenseInfo.bPerpetual = bPerpetual;
	LicenseInfo.bShowWatermark = bShowWatermark;
	LicenseInfo.Signature = FLicenseEncryptor::GenerateSignature(LicenseInfo.MachineID, LicenseInfo.GenerateTime, LicenseInfo.ProjectId, LicenseInfo.bPerpetual);

	// 生成许可证密钥
	return FLicenseEncryptor::GenerateLicenseKey(LicenseInfo);
}

/**
 * @brief 解析许可证密钥
 */
FTrialLicenseInfo UTrialLicenseSubsystem::ParseLicenseKey(const FString& LicenseKey)
{
	return FLicenseEncryptor::ParseLicenseKey(LicenseKey);
}

FTrialLicenseInfo UTrialLicenseSubsystem::ParseCurrentLicenseKey()
{
	if (const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>())
	{
		if (!Settings->LicenseKey.IsEmpty())
		{
			return ParseLicenseKey(Settings->LicenseKey);
		}
	}

	return FTrialLicenseInfo();
}

/**
 * @brief 获取当前机器ID
 */
FString UTrialLicenseSubsystem::GetMachineID()
{
	return FMachineIDHelper::GetMachineID();
}

/**
 * @brief 验证许可证合法性
 *
 * 执行全面的许可证验证流程，包括：
 * 1. 许可证解析和完整性检查
 * 2. 设备绑定验证（机器ID匹配）
 * 3. 项目绑定验证（项目ID匹配）
 * 4. 过期时间检查（非永久许可证）
 * 5. 系统时间篡改检测
 * 6. 数字签名验证
 * 7. 本地数据更新和会话初始化
 *
 * @param LicenseKey 要验证的许可证密钥字符串
 * @param PassPhrase 用于解密和验证的密码短语
 * @return true如果所有验证都通过，许可证有效；false如果任何验证失败
 */
bool UTrialLicenseSubsystem::ValidateLicense(const FString& LicenseKey)
{
	LastValidationError.Empty();

	// 解析许可证
	FTrialLicenseInfo LicenseInfo = ParseLicenseKey(LicenseKey);

	// 验证是否解析成功
	if (LicenseInfo.MachineID.IsEmpty() || LicenseInfo.Signature.IsEmpty())
	{
		const FString ProjectName = FApp::GetProjectName();
		const FString DecryptedCandidate = FLicenseEncryptor::AESDecrypt(LicenseKey, ProjectName);
		if (!DecryptedCandidate.IsEmpty())
		{
			if (DecryptedCandidate.Contains(TEXT("\"ProjectId\""))
				&& DecryptedCandidate.Contains(TEXT("\"MachineID\""))
				&& DecryptedCandidate.Contains(TEXT("\"Time\""))
				&& !DecryptedCandidate.Contains(TEXT("\"Signature\"")))
			{
				LastValidationError = TEXT("输入的是试用请求（Trial Request），不是许可证（License Key）");
			}
			else if (DecryptedCandidate.Contains(TEXT("\"Signature\"")))
			{
				LastValidationError = TEXT("已解密但许可证字段不完整，请确认后端输出的是完整 License Key");
			}
			else
			{
				LastValidationError = FString::Printf(
					TEXT("解密成功但内容不是许可证格式。当前项目名=%s"),
					*ProjectName
				);
			}
		}
		else
		{
			LastValidationError = FString::Printf(
				TEXT("许可证解析失败（密钥无效或加密口令不匹配）。当前项目名=%s，密钥长度=%d"),
				*ProjectName,
				LicenseKey.Len()
			);
		}
		UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed: parse/decrypt failed or missing fields. Project=%s"), FApp::GetProjectName());
		return false;
	}

	// 验证设备ID是否匹配
	if (!IsLicenseMatchedDevice(LicenseInfo.MachineID))
	{
		LastValidationError = FString::Printf(TEXT("设备不匹配：当前=%s，许可证=%s"), *GetMachineID(), *LicenseInfo.MachineID);
		UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed: machine mismatch. Current=%s, License=%s"), *GetMachineID(), *LicenseInfo.MachineID);
		return false;
	}

	// 验证项目是否匹配
	const FString CurrentProjectId = FApp::GetProjectName();
	if (!LicenseInfo.ProjectId.IsEmpty() && !LicenseInfo.ProjectId.Equals(CurrentProjectId))
	{
		LastValidationError = FString::Printf(TEXT("项目不匹配：当前=%s，许可证=%s"), *CurrentProjectId, *LicenseInfo.ProjectId);
		UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed: project mismatch. Current=%s, License=%s"), *CurrentProjectId, *LicenseInfo.ProjectId);
		return false;
	}

	// 验证是否过期（永久授权跳过）
	if (!LicenseInfo.bPerpetual)
	{
		FDateTime CurrentTime = FDateTime::Now();
		if (CurrentTime > LicenseInfo.ExpireTime)
		{
			LastValidationError = FString::Printf(TEXT("许可证已过期：过期时间=%s"), *LicenseInfo.ExpireTime.ToString());
			UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed: expired. Now=%s, Expire=%s"), *CurrentTime.ToString(), *LicenseInfo.ExpireTime.ToString());
			return false;
		}
	}

	// 验证系统时间是否被篡改
	if (HasSystemTimeTampered())
	{
		LastValidationError = TEXT("检测到系统时间异常（可能存在回拨）");
		UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed: system time tampering detected."));
		return false;
	}

	// 验证签名（可选）
	FString ExpectedSignature = FLicenseEncryptor::GenerateSignature(LicenseInfo.MachineID, LicenseInfo.GenerateTime, LicenseInfo.ProjectId, LicenseInfo.bPerpetual);
	if (ExpectedSignature != LicenseInfo.Signature)
	{
		LastValidationError = TEXT("许可证签名不匹配（数据可能被篡改或时间格式不一致）");
		UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Validate failed: signature mismatch. Expected=%s, Actual=%s"), *ExpectedSignature, *LicenseInfo.Signature);
		UE_LOG(LogTemp, Warning, TEXT("[TrialLicense] Signature inputs: Project=%s, Machine=%s, GenerateTime=%s, Perpetual=%s"), *LicenseInfo.ProjectId, *LicenseInfo.MachineID, *LicenseInfo.GenerateTime.ToString(), LicenseInfo.bPerpetual ? TEXT("true") : TEXT("false"));
		return false;
	}

	// 刷新本地试用数据（写入当前 LicenseKeyHash 与 MachineID）
	{
		float UsedSecondsStored = 0.0f;
		FString StoredHash;
		FString StoredMachineID;
		const FString CurrentHash = FLicenseEncryptor::SHA256Hash(LicenseKey);
		if (FTrialDataStorage::LoadTrialData(StoredHash, StoredMachineID, UsedSecondsStored))
		{
			if (StoredHash != CurrentHash)
			{
				UsedSecondsStored = 0.0f; // 不同密钥，重置累计用时
			}
		}
		FTrialDataStorage::SaveTrialData(CurrentHash, LicenseInfo.MachineID, UsedSecondsStored);

		// 初始化会话（仅首次成功验证时）
		if (!bTrialSessionActive)
		{
			bTrialSessionActive = true;
			TrialSessionStart = FDateTime::Now();
			TrialSessionHash = CurrentHash;
			TrialSessionMachineID = LicenseInfo.MachineID;
			TrialLastFlushedSessionSeconds = 0.0;
			// 启动定时器
			if (bTrialPeriodicEnabled)
			{
				if (!TrialTickerHandle.IsValid())
				{
					TrialTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&UTrialLicenseSubsystem::TrialPeriodicFlushTick, this), (float)TrialFlushIntervalSeconds);
				}
			}
		}
	}

	return true;
}

bool UTrialLicenseSubsystem::PersistValidatedLicenseKey(const FString& LicenseKey, const FTrialLicenseInfo& LicenseInfo)
{
	if (UTrialLicenseSettings* Settings = GetMutableDefault<UTrialLicenseSettings>())
	{
		Settings->LicenseKey = LicenseKey;
		Settings->SaveConfig();
	}

	FTrialWatermarkOverlay::Get().OnLicenseValidated(LicenseKey);
	UpdateWatermarkVisibilityForLicense(LicenseKey);
	LastFileOperationMessage = LicenseInfo.bPerpetual
		? TEXT("许可证导入成功：永久授权已生效")
		: TEXT("许可证导入成功：试用授权已生效");
	return true;
}

FString UTrialLicenseSubsystem::GetLastValidationError() const
{
	return LastValidationError;
}

/**
 * @brief 获取试用剩余时间（秒）
 *
 * 永久授权返回-1；过期返回0；结合绝对到期与时长上限取最小值。
 */
float UTrialLicenseSubsystem::GetTrialRemainingTime(const FString& LicenseKey)
{
	FTrialLicenseInfo LicenseInfo = ParseLicenseKey(LicenseKey);

	// 永久授权：返回 -1 表示无限制
	if (LicenseInfo.bPerpetual)
	{
		return -1.0f;
	}

	FDateTime Now = FDateTime::Now();
	if (Now > LicenseInfo.ExpireTime)
	{
		return 0.0f; // 已过期
	}

	// 读取本地使用时长（仅当 LicenseKeyHash 一致时才使用）
	float UsedSecondsStored = 0.0f;
	FString StoredHash, StoredMachineID;
	const FString CurrentHash = FLicenseEncryptor::SHA256Hash(LicenseKey);
	if (FTrialDataStorage::LoadTrialData(StoredHash, StoredMachineID, UsedSecondsStored))
	{
		if (StoredHash != CurrentHash)
		{
			UsedSecondsStored = 0.0f; // 不同密钥，忽略历史使用时长
		}
	}

	// 基于有效期与时长上限共同裁剪
	const double SecondsUntilExpiry = (LicenseInfo.ExpireTime - Now).GetTotalSeconds();
	const double SecondsByDuration = FMath::Max(0.0, LicenseInfo.DurationHours * 3600.0 - UsedSecondsStored);
	return (float)FMath::Max(0.0, FMath::Min(SecondsUntilExpiry, SecondsByDuration));
}

/**
 * @brief 获取累计使用时长（秒）
 */
float UTrialLicenseSubsystem::GetUsedTime()
{
	return FTrialDataStorage::GetUsedTime();
}

/**
 * @brief 重置本地试用数据
 */
void UTrialLicenseSubsystem::ResetTrialData()
{
	FTrialDataStorage::ResetTrialData();
}

/**
 * @brief 结束当前试用会话并持久化
 *
 * 将本次会话的用时增量写入存储，停止定时器。
 */
void UTrialLicenseSubsystem::EndTrialSessionAndPersist()
{
	if (!bTrialSessionActive)
	{
		return;
	}

	const FDateTime Now = FDateTime::Now();
	const double SessionSeconds = FMath::Max(0.0, (Now - TrialSessionStart).GetTotalSeconds());

	float UsedSecondsStored = 0.0f;
	FString StoredHash;
	FString StoredMachineID;
	FTrialDataStorage::LoadTrialData(StoredHash, StoredMachineID, UsedSecondsStored);

	if (!TrialSessionHash.IsEmpty())
	{
		if (StoredHash != TrialSessionHash)
		{
			UsedSecondsStored = 0.0f;
		}

		const double DeltaSinceLastFlush = SessionSeconds - TrialLastFlushedSessionSeconds;
		UsedSecondsStored += (float)FMath::Max(0.0, DeltaSinceLastFlush);
		FTrialDataStorage::SaveTrialData(TrialSessionHash, TrialSessionMachineID, UsedSecondsStored);
		TrialLastFlushedSessionSeconds = SessionSeconds;
	}

	if (TrialTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TrialTickerHandle);
		TrialTickerHandle = FTSTicker::FDelegateHandle();
	}

	bTrialSessionActive = false;
}

/**
 * @brief 检测系统时间是否被篡改
 */
bool UTrialLicenseSubsystem::HasSystemTimeTampered()
{
	return FTimeGuard::HasSystemTimeTampered();
}

/**
 * @brief 计算SHA256哈希
 */
FString UTrialLicenseSubsystem::SHA256Hash(const FString& Input)
{
	return FLicenseEncryptor::SHA256Hash(Input);
}

/**
 * @brief AES加密
 */
FString UTrialLicenseSubsystem::AESEncrypt(const FString& Input, const FString& Key)
{
	return FLicenseEncryptor::AESEncrypt(Input, Key);
}

/**
 * @brief AES解密
 */
FString UTrialLicenseSubsystem::AESDecrypt(const FString& Input, const FString& Key)
{
	return FLicenseEncryptor::AESDecrypt(Input, Key);
}

/**
 * @brief 检查许可证绑定的机器ID是否与当前设备匹配
 */
bool UTrialLicenseSubsystem::IsLicenseMatchedDevice(const FString& LicenseMachineID)
{
	FString CurrentMachineID = GetMachineID();
	return CurrentMachineID == LicenseMachineID;
}

/**
 * @brief 获取试用数据存储路径
 */
FString UTrialLicenseSubsystem::GetTrialDataFilePath()
{
	return FTrialDataStorage::GetTrialDataFilePath();
}

/**
 * @brief 生成试用请求加密串
 *
 * 包含项目ID、机器ID和时间，使用项目名作为内部口令加密。
 */
FString UTrialLicenseSubsystem::GenerateTrialRequest()
{
	// 直接读取所需信息
	const FString ProjectId = FApp::GetProjectName();
	const FString Machine = GetMachineID();

	// 组装 JSON
	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
	Json->SetStringField(TEXT("ProjectId"), ProjectId);
	Json->SetStringField(TEXT("MachineID"), Machine);
	Json->SetStringField(TEXT("Time"), FDateTime::UtcNow().ToString());

	FString JsonStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	// 使用项目名作为内部加密口令
	const FString InternalKey = FApp::GetProjectName();
	return FLicenseEncryptor::AESEncrypt(JsonStr, InternalKey);
}

bool UTrialLicenseSubsystem::ExportTrialRequestToFile(const FString& FilePath)
{
	LastFileOperationMessage.Empty();
	FString NormalizedPath = FilePath;
	NormalizedPath.TrimStartAndEndInline();
	if (NormalizedPath.IsEmpty())
	{
		LastFileOperationMessage = TEXT("导出失败：文件路径为空");
		return false;
	}

	const FString TrialRequest = GenerateTrialRequest();
	if (TrialRequest.IsEmpty())
	{
		LastFileOperationMessage = TEXT("导出失败：未能生成 request 内容");
		return false;
	}

	if (!NormalizedPath.EndsWith(TEXT(".request")))
	{
		NormalizedPath += TEXT(".request");
	}

	if (!FFileHelper::SaveStringToFile(TrialRequest, *NormalizedPath))
	{
		LastFileOperationMessage = FString::Printf(TEXT("导出失败：无法写入文件 %s"), *NormalizedPath);
		return false;
	}

	LastFileOperationMessage = FString::Printf(TEXT("已导出 request 文件：%s"), *NormalizedPath);
	return true;
}

bool UTrialLicenseSubsystem::ExportTrialRequestWithDialog()
{
	LastFileOperationMessage.Empty();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		LastFileOperationMessage = TEXT("导出失败：当前平台不支持文件对话框");
		return false;
	}

	const void* ParentWindowHandle = nullptr;
	if (FSlateApplication::IsInitialized())
	{
		ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	}

	const FString DefaultPath = FPaths::ProjectSavedDir();
	const FString DefaultFile = FString::Printf(TEXT("%s.request"), FApp::GetProjectName());
	TArray<FString> OutFiles;
	const bool bPicked = DesktopPlatform->SaveFileDialog(
		ParentWindowHandle,
		TEXT("导出 Trial Request"),
		DefaultPath,
		DefaultFile,
		TEXT("Trial Request (*.request)|*.request"),
		EFileDialogFlags::None,
		OutFiles);

	if (!bPicked || OutFiles.Num() == 0)
	{
		LastFileOperationMessage = TEXT("已取消导出 request 文件");
		return false;
	}

	return ExportTrialRequestToFile(OutFiles[0]);
}

bool UTrialLicenseSubsystem::ImportLicenseFromFile(const FString& FilePath)
{
	LastFileOperationMessage.Empty();
	FString LicenseKey;
	if (!FFileHelper::LoadFileToString(LicenseKey, *FilePath))
	{
		LastFileOperationMessage = FString::Printf(TEXT("导入失败：无法读取文件 %s"), *FilePath);
		return false;
	}

	LicenseKey.TrimStartAndEndInline();
	LicenseKey.ReplaceInline(TEXT("\r"), TEXT(""));
	LicenseKey.ReplaceInline(TEXT("\n"), TEXT(""));
	LicenseKey.ReplaceInline(TEXT("\t"), TEXT(""));
	LicenseKey.ReplaceInline(TEXT(" "), TEXT(""));

	if (LicenseKey.IsEmpty())
	{
		LastFileOperationMessage = TEXT("导入失败：lic 文件内容为空");
		return false;
	}

	if (!ValidateLicense(LicenseKey))
	{
		LastFileOperationMessage = LastValidationError.IsEmpty()
			? TEXT("导入失败：许可证验证未通过")
			: FString::Printf(TEXT("导入失败：%s"), *LastValidationError);
		return false;
	}

	const FTrialLicenseInfo LicenseInfo = ParseLicenseKey(LicenseKey);
	return PersistValidatedLicenseKey(LicenseKey, LicenseInfo);
}

bool UTrialLicenseSubsystem::ImportLicenseWithDialog()
{
	LastFileOperationMessage.Empty();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		LastFileOperationMessage = TEXT("导入失败：当前平台不支持文件对话框");
		return false;
	}

	const void* ParentWindowHandle = nullptr;
	if (FSlateApplication::IsInitialized())
	{
		ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	}

	TArray<FString> OutFiles;
	const bool bPicked = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("导入许可证文件"),
		FPaths::ProjectSavedDir(),
		TEXT(""),
		TEXT("License File (*.lic)|*.lic|Text File (*.txt)|*.txt"),
		EFileDialogFlags::None,
		OutFiles);

	if (!bPicked || OutFiles.Num() == 0)
	{
		LastFileOperationMessage = TEXT("已取消导入许可证文件");
		return false;
	}

	return ImportLicenseFromFile(OutFiles[0]);
}

FString UTrialLicenseSubsystem::GetLastFileOperationMessage() const
{
	return LastFileOperationMessage;
}

/**
 * @brief 解析试用请求
 *
 * @param EncryptedRequest 加密请求字符串
 * @param PassPhrase 解密口令
 * @param OutProjectId 输出项目ID
 * @param OutMachineID 输出机器ID
 * @return 解析成功返回true
 */
bool UTrialLicenseSubsystem::ParseTrialRequest(const FString& EncryptedRequest, FString& OutProjectId, FString& OutMachineID)
{
	OutProjectId.Empty();
	OutMachineID.Empty();

	const FString InternalKey = FApp::GetProjectName();

	const FString JsonStr = FLicenseEncryptor::AESDecrypt(EncryptedRequest, InternalKey);
	if (JsonStr.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		return false;
	}

	if (Json->HasTypedField<EJson::String>(TEXT("ProjectId")))
	{
		OutProjectId = Json->GetStringField(TEXT("ProjectId"));
	}
	if (Json->HasTypedField<EJson::String>(TEXT("MachineID")))
	{
		OutMachineID = Json->GetStringField(TEXT("MachineID"));
	}

	return !OutProjectId.IsEmpty() && !OutMachineID.IsEmpty();
}

/**
 * @brief 周期性写盘定时器
 *
 * 按配置间隔累积并保存会话用时。
 */
bool UTrialLicenseSubsystem::TrialPeriodicFlushTick(float DeltaTime, UTrialLicenseSubsystem* Subsystem)
{
	if (!Subsystem || !Subsystem->bTrialSessionActive || !Subsystem->bTrialPeriodicEnabled)
	{
		return false;
	}

	const FDateTime Now = FDateTime::Now();
	const double SessionSeconds = FMath::Max(0.0, (Now - Subsystem->TrialSessionStart).GetTotalSeconds());
	if (SessionSeconds - Subsystem->TrialLastFlushedSessionSeconds < Subsystem->TrialFlushIntervalSeconds)
	{
		return true;
	}

	float UsedSecondsStored = 0.0f;
	FString StoredHash;
	FString StoredMachineID;
	FTrialDataStorage::LoadTrialData(StoredHash, StoredMachineID, UsedSecondsStored);

	if (!Subsystem->TrialSessionHash.IsEmpty())
	{
		if (StoredHash != Subsystem->TrialSessionHash)
		{
			UsedSecondsStored = 0.0f;
		}

		const double DeltaSinceLastFlush = SessionSeconds - Subsystem->TrialLastFlushedSessionSeconds;
		UsedSecondsStored += (float)FMath::Max(0.0, DeltaSinceLastFlush);
		FTrialDataStorage::SaveTrialData(Subsystem->TrialSessionHash, Subsystem->TrialSessionMachineID, UsedSecondsStored);
		Subsystem->TrialLastFlushedSessionSeconds = SessionSeconds;
	}

	return true;
}

bool UTrialLicenseSubsystem::LicenseMonitorTick(float DeltaTime, UTrialLicenseSubsystem* Subsystem)
{
	if (!Subsystem)
	{
		return false;
	}

	const double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - Subsystem->LastLicenseMonitorCheckTime < 10.0)
	{
		return true;
	}

	Subsystem->LastLicenseMonitorCheckTime = CurrentTime;
	Subsystem->RefreshLicenseRuntimeState();
	return true;
}

void UTrialLicenseSubsystem::UpdateWatermarkVisibilityForLicense(const FString& LicenseKey)
{
	const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
	const bool bFallbackEnabled = Settings ? Settings->WatermarkEnabled : true;

	if (LicenseKey.IsEmpty())
	{
		FTrialWatermarkOverlay::Get().SetEnabled(bFallbackEnabled);
		return;
	}

	const FTrialLicenseInfo Info = ParseLicenseKey(LicenseKey);
	if (Info.MachineID.IsEmpty() || Info.Signature.IsEmpty())
	{
		FTrialWatermarkOverlay::Get().SetEnabled(bFallbackEnabled);
		return;
	}

	FTrialWatermarkOverlay::Get().SetEnabled(Info.bShowWatermark);
}

void UTrialLicenseSubsystem::CheckAndHandleLicenseExpiry(const FString& LicenseKey)
{
	const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
	if (!Settings || !Settings->bAutoShowExpiredDialog || LicenseKey.IsEmpty())
	{
		return;
	}

	const FTrialLicenseInfo Info = ParseLicenseKey(LicenseKey);
	if (Info.MachineID.IsEmpty() || Info.Signature.IsEmpty())
	{
		return;
	}

	const float RemainingTime = GetTrialRemainingTime(LicenseKey);
	if (!Info.bPerpetual && RemainingTime <= 0.f)
	{
		ShowExpiredDialog(
			TEXT("您的试用期已到期，请联系开发者获取正式许可证。\n\nYour trial period has expired. Please contact the developer for a full license."),
			RemainingTime,
			Info.bPerpetual);
	}
	else if (!ValidateLicense(LicenseKey))
	{
		ShowExpiredDialog(
			TEXT("许可证验证失败，可能是设备不匹配或证书已被篡改。\n\nLicense validation failed. Device mismatch or certificate tampering detected."),
			RemainingTime,
			Info.bPerpetual);
	}
}

void UTrialLicenseSubsystem::RefreshLicenseRuntimeState()
{
	FString RuntimeLicenseKey;
	if (const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>())
	{
		RuntimeLicenseKey = Settings->LicenseKey;
	}

	UpdateWatermarkVisibilityForLicense(RuntimeLicenseKey);
	CheckAndHandleLicenseExpiry(RuntimeLicenseKey);
}

// 水印管理方法实现
/**
 * @brief 设置水印启用状态
 */
void UTrialLicenseSubsystem::SetWatermarkEnabled(bool bEnabled)
{
	FTrialWatermarkOverlay::Get().SetEnabled(bEnabled);
}

/**
 * @brief 设置水印文本
 */
void UTrialLicenseSubsystem::SetWatermarkText(const FString& NewText)
{
	FTrialWatermarkOverlay::Get().SetBaseText(NewText);
}

/**
 * @brief 设置水印角度
 */
void UTrialLicenseSubsystem::SetWatermarkAngle(float AngleDegrees)
{
	FTrialWatermarkOverlay::Get().SetAngle(AngleDegrees);
}

/**
 * @brief 设置水印平铺间距（XY相同）
 */
void UTrialLicenseSubsystem::SetWatermarkTileSpacing(float TileSpacing)
{
	FTrialWatermarkOverlay::Get().SetTileSpacing(TileSpacing, TileSpacing);
}

/**
 * @brief 设置水印平铺间距（独立XY）
 */
void UTrialLicenseSubsystem::SetWatermarkTileSpacingXY(float TileSpacingX, float TileSpacingY)
{
	FTrialWatermarkOverlay::Get().SetTileSpacing(TileSpacingX, TileSpacingY);
}

/**
 * @brief 设置水印不透明度
 */
void UTrialLicenseSubsystem::SetWatermarkOpacity(float Opacity)
{
	FTrialWatermarkOverlay::Get().SetOpacity(Opacity);
}

/**
 * @brief 设置水印字体大小
 */
void UTrialLicenseSubsystem::SetWatermarkFontSize(int32 FontSize)
{
	FTrialWatermarkOverlay::Get().SetFontSize(FontSize);
}

/**
 * @brief 获取水印启用状态
 */
bool UTrialLicenseSubsystem::GetWatermarkEnabled()
{
	return FTrialWatermarkOverlay::Get().IsEnabled();
}

/**
 * @brief 获取水印文本
 */
FString UTrialLicenseSubsystem::GetWatermarkText()
{
	return FTrialWatermarkOverlay::Get().GetBaseText();
}

/**
 * @brief 获取水印角度
 */
float UTrialLicenseSubsystem::GetWatermarkAngle()
{
	return FTrialWatermarkOverlay::Get().GetAngle();
}

/**
 * @brief 获取水印平铺间距（X）
 */
float UTrialLicenseSubsystem::GetWatermarkTileSpacing()
{
	return FTrialWatermarkOverlay::Get().GetTileSpacingX();
}

/**
 * @brief 获取水印平铺间距X
 */
float UTrialLicenseSubsystem::GetWatermarkTileSpacingX()
{
	return FTrialWatermarkOverlay::Get().GetTileSpacingX();
}

/**
 * @brief 获取水印平铺间距Y
 */
float UTrialLicenseSubsystem::GetWatermarkTileSpacingY()
{
	return FTrialWatermarkOverlay::Get().GetTileSpacingY();
}

/**
 * @brief 获取水印不透明度
 */
float UTrialLicenseSubsystem::GetWatermarkOpacity()
{
	return FTrialWatermarkOverlay::Get().GetOpacity();
}

/**
 * @brief 获取水印字体大小
 */
int32 UTrialLicenseSubsystem::GetWatermarkFontSize()
{
	return FTrialWatermarkOverlay::Get().GetFontSize();
}

/**
 * @brief 从项目设置应用水印配置
 */
void UTrialLicenseSubsystem::ApplyWatermarkFromProjectSettings()
{
	// 从项目设置应用水印配置
	const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
	if (Settings)
	{
		SetWatermarkEnabled(Settings->WatermarkEnabled);
		SetWatermarkText(Settings->WatermarkText);
		SetWatermarkOpacity(Settings->WatermarkOpacity);
		SetWatermarkAngle(Settings->WatermarkAngle);
		SetWatermarkFontSize(Settings->WatermarkFontSize);
		SetWatermarkTileSpacingXY(Settings->WatermarkTileSpacingX, Settings->WatermarkTileSpacingY);
	}
}

/**
 * @brief 显示许可证过期/验证失败对话框
 */
void UTrialLicenseSubsystem::ShowExpiredDialog(const FString& Reason, float RemainingTime, bool bIsPerpetual)
{
	FTrialWatermarkOverlay::Get().ShowExpiredDialog(Reason, RemainingTime, bIsPerpetual);
}

/**
 * @brief 验证本地保存的证书是否有效（蓝图可调用）
 *
 * 该函数直接调用FTrialDataStorage::IsLocalLicenseValid()进行验证。
 * 验证内容包括：
 * 1. 证书文件是否存在
 * 2. 证书数据是否可以正确解密和解析
 * 3. 机器ID是否匹配当前设备
 * 4. 证书哈希是否存在
 *
 * @return true 如果本地证书存在且有效，false 如果证书不存在、损坏或机器ID不匹配
 */
bool UTrialLicenseSubsystem::IsLocalLicenseValid()
{
	return FTrialDataStorage::IsLocalLicenseValid();
}
