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

/**
 * @brief 构造函数：初始化默认状态
 */
UTrialLicenseSubsystem::UTrialLicenseSubsystem()
	: bTrialSessionActive(false)
	, TrialLastFlushedSessionSeconds(0.0)
	, TrialFlushIntervalSeconds(300.0)
	, bTrialPeriodicEnabled(true)
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
FString UTrialLicenseSubsystem::GenerateLicenseKey(float DurationHours, const FDateTime& ExpiryDate, const FString& PassPhrase, const FString& ProjectId, const FString& TargetMachineID, bool bPerpetual)
{
	// 创建许可证信息
	FTrialLicenseInfo LicenseInfo;
	LicenseInfo.DurationHours = DurationHours;
	LicenseInfo.GenerateTime = FDateTime::Now();
	LicenseInfo.ExpireTime = ExpiryDate;
	LicenseInfo.MachineID = TargetMachineID.IsEmpty() ? GetMachineID() : TargetMachineID;
	LicenseInfo.ProjectId = ProjectId;
	LicenseInfo.bPerpetual = bPerpetual;
	LicenseInfo.Signature = FLicenseEncryptor::GenerateSignature(PassPhrase, LicenseInfo.MachineID, LicenseInfo.GenerateTime, LicenseInfo.ProjectId, LicenseInfo.bPerpetual);

	// 生成许可证密钥
	return FLicenseEncryptor::GenerateLicenseKey(LicenseInfo, PassPhrase);
}

/**
 * @brief 解析许可证密钥
 */
FTrialLicenseInfo UTrialLicenseSubsystem::ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase)
{
	return FLicenseEncryptor::ParseLicenseKey(LicenseKey, PassPhrase);
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
bool UTrialLicenseSubsystem::ValidateLicense(const FString& LicenseKey, const FString& PassPhrase)
{
	// 解析许可证
	FTrialLicenseInfo LicenseInfo = ParseLicenseKey(LicenseKey, PassPhrase);

	// 验证是否解析成功
	if (LicenseInfo.MachineID.IsEmpty() || LicenseInfo.Signature.IsEmpty())
	{
		return false;
	}

	// 验证设备ID是否匹配
	if (!IsLicenseMatchedDevice(LicenseInfo.MachineID))
	{
		return false;
	}

	// 验证项目是否匹配
	const FString CurrentProjectId = FApp::GetProjectName();
	if (!LicenseInfo.ProjectId.IsEmpty() && !LicenseInfo.ProjectId.Equals(CurrentProjectId))
	{
		return false;
	}

	// 验证是否过期（永久授权跳过）
	if (!LicenseInfo.bPerpetual)
	{
		FDateTime CurrentTime = FDateTime::Now();
		if (CurrentTime > LicenseInfo.ExpireTime)
		{
			return false;
		}
	}

	// 验证系统时间是否被篡改
	if (HasSystemTimeTampered())
	{
		return false;
	}

	// 验证签名（可选）
	FString ExpectedSignature = FLicenseEncryptor::GenerateSignature(PassPhrase, LicenseInfo.MachineID, LicenseInfo.GenerateTime, LicenseInfo.ProjectId, LicenseInfo.bPerpetual);
	if (ExpectedSignature != LicenseInfo.Signature)
	{
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

/**
 * @brief 获取试用剩余时间（秒）
 *
 * 永久授权返回-1；过期返回0；结合绝对到期与时长上限取最小值。
 */
float UTrialLicenseSubsystem::GetTrialRemainingTime(const FString& LicenseKey, const FString& PassPhrase)
{
	FTrialLicenseInfo LicenseInfo = ParseLicenseKey(LicenseKey, PassPhrase);

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

/**
 * @brief 解析试用请求
 *
 * @param EncryptedRequest 加密请求字符串
 * @param PassPhrase 解密口令
 * @param OutProjectId 输出项目ID
 * @param OutMachineID 输出机器ID
 * @return 解析成功返回true
 */
bool UTrialLicenseSubsystem::ParseTrialRequest(const FString& EncryptedRequest, const FString& PassPhrase, FString& OutProjectId, FString& OutMachineID)
{
	OutProjectId.Empty();
	OutMachineID.Empty();

	const FString JsonStr = FLicenseEncryptor::AESDecrypt(EncryptedRequest, PassPhrase);
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
