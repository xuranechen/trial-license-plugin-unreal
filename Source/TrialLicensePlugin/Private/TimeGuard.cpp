// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 时间保护类，用于检测和防止系统时间篡改
 *
 * 该类通过记录上次会话结束时间来检测用户是否将系统时间向前调整，
 * 从而防止用户通过修改系统时间来绕过试用期限制。数据通过AES加密存储在文件中。
 */
#include "TimeGuard.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "LicenseEncryptor.h"
#include "Engine/Engine.h"
#include "Misc/EngineVersion.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/ConfigCacheIni.h"

static FString GetTimeGuardFilePath()
{
	return FPaths::ProjectSavedDir() + TEXT("TimeGuardData.json");
}

static FString GetTimeGuardEncryptKey()
{
	FString Key;
	if (GConfig && GConfig->GetString(TEXT("TrialLicensePlugin"), TEXT("TimeGuardKey"), Key, GGameIni) && !Key.IsEmpty())
	{
		return Key;
	}
	return TEXT("TrialTimeGuardKey");
}

/**
 * @brief 检测系统时间是否被篡改
 *
 * 通过比较当前系统时间与上次记录的会话结束时间来判断用户是否将系统时间向前调整。
 * 如果当前时间早于上次会话结束时间，则认为时间被篡改。
 *
 * @return true如果检测到时间篡改，false如果时间正常
 */
bool FTimeGuard::HasSystemTimeTampered()
{
	FString FilePath = GetTimeGuardFilePath();

	// 检查文件是否存在
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		// 如果文件不存在，记录当前时间并返回false（没有篡改）
		RecordSessionEndTime();
		return false;
	}

	// 读取文件内容
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		// 解密并解析JSON
		const FString JsonString = FLicenseEncryptor::AESDecrypt(FileContent, GetTimeGuardEncryptKey());
		if (JsonString.IsEmpty())
		{
			// 历史文件可能由旧密钥或异常退出导致，避免一次损坏直接锁死
			UE_LOG(LogTemp, Warning, TEXT("[TrialLicense][TimeGuard] Failed to decrypt time guard file, reset it."));
			RecordSessionEndTime();
			return false;
		}
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasTypedField<EJson::String>(TEXT("LastSessionEnd")))
			{
				FString LastSessionEndStr = JsonObject->GetStringField(TEXT("LastSessionEnd"));
				FDateTime LastSessionEnd;
				if (FDateTime::Parse(LastSessionEndStr, LastSessionEnd))
				{
					// 获取当前系统时间
					FDateTime CurrentTime = FDateTime::Now();

					// 如果当前时间早于上次会话结束时间，则认为时间被篡改
					if (CurrentTime < LastSessionEnd)
					{
						return true; // 时间被篡改
					}
				}
			}
		}
	}

	// 没有发现时间篡改
	RecordSessionEndTime();
	return false;
}

/**
 * @brief 记录当前会话结束时间
 *
 * 将当前系统时间加密后保存到文件中，用于下次启动时检测时间篡改。
 * 采用同步写入，避免进程退出阶段异步任务未落盘导致数据损坏。
 */
void FTimeGuard::RecordSessionEndTime()
{
	FDateTime CurrentTime = FDateTime::Now();

	// 创建JSON对象
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("LastSessionEnd"), CurrentTime.ToString());

	// 序列化为字符串
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 加密并保存到文件（同步写入，避免进程退出时异步任务未落盘）
	FString FilePath = GetTimeGuardFilePath();
	const FString Encrypted = FLicenseEncryptor::AESEncrypt(JsonString, GetTimeGuardEncryptKey());
	FFileHelper::SaveStringToFile(Encrypted, *FilePath);
}

/**
 * @brief 获取上次记录的会话结束时间
 *
 * 从加密文件中读取并解密上次会话结束时间。如果文件不存在或解析失败，
 * 返回FDateTime::MinValue()表示无效时间。
 *
 * @return 上次会话结束时间，如果无法获取则返回最小时间值
 */
FDateTime FTimeGuard::GetLastSessionEndTime()
{
	FString FilePath = GetTimeGuardFilePath();

	// 检查文件是否存在
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		return FDateTime::MinValue();
	}

	// 读取文件内容
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		// 解密并解析JSON
		const FString JsonString = FLicenseEncryptor::AESDecrypt(FileContent, GetTimeGuardEncryptKey());
		if (JsonString.IsEmpty())
		{
			return FDateTime::MinValue();
		}
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasTypedField<EJson::String>(TEXT("LastSessionEnd")))
			{
				FString LastSessionEndStr = JsonObject->GetStringField(TEXT("LastSessionEnd"));
				FDateTime LastSessionEnd;
				if (FDateTime::Parse(LastSessionEndStr, LastSessionEnd))
				{
					return LastSessionEnd;
				}
			}
		}
	}

	return FDateTime::MinValue();
}
