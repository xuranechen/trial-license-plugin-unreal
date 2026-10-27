// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用数据存储类，负责管理试用许可证相关数据的持久化存储
 *
 * 该类提供加密的数据存储功能，用于保存和加载许可证哈希、机器ID、使用时间等信息。
 * 数据通过AES加密存储在JSON文件中，确保数据的安全性和完整性。
 */
#include "TrialDataStorage.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "LicenseEncryptor.h"
#include "MachineIDHelper.h"
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

/**
 * @brief 获取试用数据文件的存储路径
 *
 * 返回项目保存目录下的TrialData.json文件路径，用于存储加密的试用数据。
 *
 * @return 试用数据文件的完整路径
 */
FString FTrialDataStorage::GetTrialDataFilePath()
{
	return FPaths::ProjectSavedDir() + TEXT("TrialData.json");
}

/**
 * @brief 保存试用数据到加密文件
 *
 * 将许可证哈希、机器ID、使用时间和当前会话结束时间等信息序列化为JSON，
 * 然后进行AES加密并异步保存到文件中。
 *
 * @param LicenseKeyHash 许可证密钥的哈希值
 * @param MachineID 机器唯一标识符
 * @param UsedSeconds 已使用的试用时间（秒）
 */
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

/**
 * @brief 从加密文件加载试用数据
 *
 * 读取并解密存储的试用数据文件，解析其中的许可证哈希、机器ID和使用时间信息。
 *
 * @param OutLicenseKeyHash 输出参数：许可证密钥哈希
 * @param OutMachineID 输出参数：机器ID
 * @param OutUsedSeconds 输出参数：已使用的试用时间（秒）
 * @return true如果成功加载数据，false如果文件不存在或解析失败
 */
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

/**
 * @brief 获取已使用的试用时间
 *
 * 从存储的数据中读取并返回已使用的试用时间（秒）。如果无法加载数据，返回0.0。
 *
 * @return 已使用的试用时间（秒），如果无法获取则返回0.0
 */
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

/**
 * @brief 重置试用数据
 *
 * 删除存储的试用数据文件，清除所有已保存的许可证信息和使用记录。
 * 这通常在需要重新开始试用或清除所有数据时使用。
 */
void FTrialDataStorage::ResetTrialData()
{
	FString FilePath = GetTrialDataFilePath();

	// 删除文件
	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*FilePath);
	}
}

/**
 * @brief 验证本地保存的证书是否有效
 *
 * 检查本地保存的证书数据是否存在且有效。验证内容包括：
 * 1. 证书文件是否存在
 * 2. 证书数据是否可以正确解密和解析
 * 3. 机器ID是否匹配当前设备
 * 4. 证书哈希是否存在且非空
 *
 * @return true 如果本地证书存在且有效，false 如果证书不存在、损坏或机器ID不匹配
 */
bool FTrialDataStorage::IsLocalLicenseValid()
{
	FString LicenseKeyHash, MachineID;
	float UsedSeconds = 0.0f;

	// 尝试加载本地证书数据
	if (!LoadTrialData(LicenseKeyHash, MachineID, UsedSeconds))
	{
		// 无法加载数据（文件不存在或解密失败）
		return false;
	}

	// 验证证书哈希是否存在
	if (LicenseKeyHash.IsEmpty())
	{
		return false;
	}

	// 验证机器ID是否存在
	if (MachineID.IsEmpty())
	{
		return false;
	}

	// 获取当前机器ID
	FString CurrentMachineID = FMachineIDHelper::GetMachineID();
	if (CurrentMachineID.IsEmpty())
	{
		// 无法获取当前机器ID
		return false;
	}

	// 验证机器ID是否匹配
	if (MachineID != CurrentMachineID)
	{
		// 机器ID不匹配，证书可能被转移到其他设备
		return false;
	}

	// 所有验证通过
	return true;
}
