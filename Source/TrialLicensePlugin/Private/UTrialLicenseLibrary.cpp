// Copyright 2025, Rune. All Rights Reserved.

#include "UTrialLicenseLibrary.h"
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

// 会话状态（仅在打开与退出时使用，不用周期性定时器）
static bool GTrialSessionActive = false;
static FDateTime GTrialSessionStart;
static FString GTrialSessionHash;
static FString GTrialSessionMachineID;
static FTSTicker::FDelegateHandle GTrialTickerHandle;
static double GTrialLastFlushedSessionSeconds = 0.0;
static double GTrialFlushIntervalSeconds = 300.0; // 可配置（秒）
static bool GTrialPeriodicEnabled = true; // 可配置

static bool TrialPeriodicFlushTick(float DeltaTime)
{
	if (!GTrialSessionActive || !GTrialPeriodicEnabled)
	{
		return false;
	}

	const FDateTime Now = FDateTime::Now();
	const double SessionSeconds = FMath::Max(0.0, (Now - GTrialSessionStart).GetTotalSeconds());
	if (SessionSeconds - GTrialLastFlushedSessionSeconds < GTrialFlushIntervalSeconds)
	{
		return true;
	}

	float UsedSecondsStored = 0.0f;
	FString StoredHash;
	FString StoredMachineID;
	FTrialDataStorage::LoadTrialData(StoredHash, StoredMachineID, UsedSecondsStored);

	if (!GTrialSessionHash.IsEmpty())
	{
		if (StoredHash != GTrialSessionHash)
		{
			UsedSecondsStored = 0.0f;
		}

		const double DeltaSinceLastFlush = SessionSeconds - GTrialLastFlushedSessionSeconds;
		UsedSecondsStored += (float)FMath::Max(0.0, DeltaSinceLastFlush);
		FTrialDataStorage::SaveTrialData(GTrialSessionHash, GTrialSessionMachineID, UsedSecondsStored);
		GTrialLastFlushedSessionSeconds = SessionSeconds;
	}

	return true;
}

FString UTrialLicenseLibrary::GenerateLicenseKey(float DurationHours, const FDateTime& ExpiryDate, const FString& PassPhrase, const FString& ProjectId, const FString& TargetMachineID, bool bPerpetual)
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

FTrialLicenseInfo UTrialLicenseLibrary::ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase)
{
	return FLicenseEncryptor::ParseLicenseKey(LicenseKey, PassPhrase);
}

FString UTrialLicenseLibrary::GetMachineID()
{
	return FMachineIDHelper::GetMachineID();
}

bool UTrialLicenseLibrary::ValidateLicense(const FString& LicenseKey, const FString& PassPhrase)
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
		FString StoredHash, StoredMachineID;
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
		if (!GTrialSessionActive)
		{
			GTrialSessionActive = true;
			GTrialSessionStart = FDateTime::Now();
			GTrialSessionHash = CurrentHash;
			GTrialSessionMachineID = LicenseInfo.MachineID;
			GTrialLastFlushedSessionSeconds = 0.0;
			// 从配置读取周期性保存开关与间隔（可选）
			{
				bool bEnable = true;
				float IntervalSec = 300.0f;
				if (GConfig)
				{
					GConfig->GetBool(TEXT("TrialLicensePlugin"), TEXT("TrialPeriodicSaveEnabled"), bEnable, GGameIni);
					GConfig->GetFloat(TEXT("TrialLicensePlugin"), TEXT("TrialPeriodicSaveIntervalSeconds"), IntervalSec, GGameIni);
				}
				GTrialPeriodicEnabled = bEnable;
				GTrialFlushIntervalSeconds = FMath::Max(30.0, (double)IntervalSec); // 给个最小 30s 保护
			}
			if (GTrialPeriodicEnabled)
			{
				if (!GTrialTickerHandle.IsValid())
				{
					GTrialTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TrialPeriodicFlushTick), (float)GTrialFlushIntervalSeconds);
				}
			}
		}
	}

	return true;
}

float UTrialLicenseLibrary::GetTrialRemainingTime(const FString& LicenseKey, const FString& PassPhrase)
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

float UTrialLicenseLibrary::GetUsedTime()
{
	return FTrialDataStorage::GetUsedTime();
}

void UTrialLicenseLibrary::ResetTrialData()
{
	FTrialDataStorage::ResetTrialData();
}

void UTrialLicenseLibrary::EndTrialSessionAndPersist()
{
    if (!GTrialSessionActive)
    {
        return;
    }

    const FDateTime Now = FDateTime::Now();
    const double SessionSeconds = FMath::Max(0.0, (Now - GTrialSessionStart).GetTotalSeconds());

    float UsedSecondsStored = 0.0f;
    FString StoredHash;
    FString StoredMachineID;
    FTrialDataStorage::LoadTrialData(StoredHash, StoredMachineID, UsedSecondsStored);

    if (!GTrialSessionHash.IsEmpty())
    {
        if (StoredHash != GTrialSessionHash)
        {
            UsedSecondsStored = 0.0f;
        }

        const double DeltaSinceLastFlush = SessionSeconds - GTrialLastFlushedSessionSeconds;
        UsedSecondsStored += (float)FMath::Max(0.0, DeltaSinceLastFlush);
        FTrialDataStorage::SaveTrialData(GTrialSessionHash, GTrialSessionMachineID, UsedSecondsStored);
        GTrialLastFlushedSessionSeconds = SessionSeconds;
    }

    if (GTrialTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(GTrialTickerHandle);
        GTrialTickerHandle = FTSTicker::FDelegateHandle();
    }

    GTrialSessionActive = false;
}

bool UTrialLicenseLibrary::HasSystemTimeTampered()
{
    return FTimeGuard::HasSystemTimeTampered();
}

FString UTrialLicenseLibrary::SHA256Hash(const FString& Input)
{
	return FLicenseEncryptor::SHA256Hash(Input);
}

FString UTrialLicenseLibrary::AESEncrypt(const FString& Input, const FString& Key)
{
	return FLicenseEncryptor::AESEncrypt(Input, Key);
}

FString UTrialLicenseLibrary::AESDecrypt(const FString& Input, const FString& Key)
{
	return FLicenseEncryptor::AESDecrypt(Input, Key);
}

bool UTrialLicenseLibrary::IsLicenseMatchedDevice(const FString& LicenseMachineID)
{
	FString CurrentMachineID = GetMachineID();
	return CurrentMachineID == LicenseMachineID;
}

FString UTrialLicenseLibrary::GetTrialDataFilePath()
{
	return FTrialDataStorage::GetTrialDataFilePath();
}

FString UTrialLicenseLibrary::GenerateTrialRequest()
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

bool UTrialLicenseLibrary::ParseTrialRequest(const FString& EncryptedRequest, const FString& PassPhrase, FString& OutProjectId, FString& OutMachineID)
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