// Copyright 2025, Rune. All Rights Reserved.

#include "TimeGuard.h"
#include "HAL/PlatformFilemanager.h"
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
#include "Async/Async.h"
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
			return true; // 无法解密视为异常
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
	return false;
}

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
	
	// 加密并保存到文件
	FString FilePath = GetTimeGuardFilePath();
	const FString Encrypted = FLicenseEncryptor::AESEncrypt(JsonString, GetTimeGuardEncryptKey());
	Async(EAsyncExecution::ThreadPool, [FilePath, DataToSave = Encrypted]()
	{
		FFileHelper::SaveStringToFile(DataToSave, *FilePath);
	});
}

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
		const FString JsonString = FLicenseEncryptor::AESDecrypt(FileContent, TEXT("TrialTimeGuardKey"));
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