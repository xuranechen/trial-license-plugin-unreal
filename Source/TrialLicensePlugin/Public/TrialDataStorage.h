// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrialLicenseInfo.h"

/**
 * 试用数据存储管理
 */
class TRIALLICENSEPLUGIN_API FTrialDataStorage
{
public:
	// 保存试用数据
	static void SaveTrialData(const FString& LicenseKeyHash, const FString& MachineID, float UsedSeconds);

	// 加载试用数据
	static bool LoadTrialData(FString& OutLicenseKeyHash, FString& OutMachineID, float& OutUsedSeconds);

	// 获取已使用时间
	static float GetUsedTime();

	// 重置试用数据
	static void ResetTrialData();

	// 获取试用数据文件路径
	static FString GetTrialDataFilePath();

private:
	// 加密保存数据
	static void EncryptAndSaveData(const FString& Data);

	// 加载并解密数据
	static FString LoadAndDecryptData();
};