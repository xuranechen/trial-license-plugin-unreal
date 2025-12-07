// Copyright 2025, Rune. All Rights Reserved.

#include "TrialDataStorage.h"
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

static FString GetTrialDataEncryptKey()
{
	FString Key;
	if (GConfig && GConfig->GetString(TEXT("TrialLicensePlugin"), TEXT("TrialDataKey"), Key, GGameIni) && !Key.IsEmpty())
	{
		return Key;
	}
	return TEXT("TrialLicenseKey");
}

FString FTrialDataStorage::GetTrialDataFilePath()
{
	return FPaths::ProjectSavedDir() + TEXT("TrialData.json");
}

void FTrialDataStorage::SaveTrialData(const FString& LicenseKeyHash, const FString& MachineID, float UsedSeconds)
{
	// 创建JSON对象
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("LicenseKeyHash"), LicenseKeyHash);
	JsonObject->SetStringField(TEXT("MachineID"), MachineID);
	JsonObject->SetNumberField(TEXT("UsedSeconds"), UsedSeconds);
	JsonObject->SetStringField(TEXT("LastSessionEnd"), FDateTime::Now().ToString());
	
	// 序列化为字符串
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	// 加密数据
	FString EncryptedData = FLicenseEncryptor::AESEncrypt(JsonString, GetTrialDataEncryptKey());
	
	// 保存到文件
	FString FilePath = GetTrialDataFilePath();
	Async(EAsyncExecution::ThreadPool, [FilePath, DataToSave = MoveTemp(EncryptedData)]()
	{
		FFileHelper::SaveStringToFile(DataToSave, *FilePath);
	});
}

bool FTrialDataStorage::LoadTrialData(FString& OutLicenseKeyHash, FString& OutMachineID, float& OutUsedSeconds)
{
	FString FilePath = GetTrialDataFilePath();
	
	// 检查文件是否存在
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		return false;
	}

	// 读取文件内容
	FString EncryptedData;
	if (FFileHelper::LoadFileToString(EncryptedData, *FilePath))
	{
		// 解密数据
		FString JsonString = FLicenseEncryptor::AESDecrypt(EncryptedData, GetTrialDataEncryptKey());
		
		if (!JsonString.IsEmpty())
		{
			// 解析JSON
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				if (JsonObject->HasTypedField<EJson::String>(TEXT("LicenseKeyHash")))
				{
					OutLicenseKeyHash = JsonObject->GetStringField(TEXT("LicenseKeyHash"));
				}
				
				if (JsonObject->HasTypedField<EJson::String>(TEXT("MachineID")))
				{
					OutMachineID = JsonObject->GetStringField(TEXT("MachineID"));
				}
				
				if (JsonObject->HasTypedField<EJson::Number>(TEXT("UsedSeconds")))
				{
					OutUsedSeconds = JsonObject->GetNumberField(TEXT("UsedSeconds"));
				}
				
				return true;
			}
		}
	}
	
	return false;
}

float FTrialDataStorage::GetUsedTime()
{
	FString LicenseKeyHash, MachineID;
	float UsedSeconds = 0.0f;
	
	if (LoadTrialData(LicenseKeyHash, MachineID, UsedSeconds))
	{
		return UsedSeconds;
	}
	
	return 0.0f;
}

void FTrialDataStorage::ResetTrialData()
{
	FString FilePath = GetTrialDataFilePath();
	
	// 删除文件
	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*FilePath);
	}
}