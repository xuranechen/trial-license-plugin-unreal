// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FTrialLicenseInfo.h"
#include "UTrialLicenseLibrary.generated.h"

/**
 * 试用许可证管理蓝图函数库
 */
UCLASS()
class TRIALLICENSEPLUGIN_API UTrialLicenseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 生成许可证密钥
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static FString GenerateLicenseKey(float DurationHours, const FDateTime& ExpiryDate, const FString& PassPhrase, const FString& ProjectId, const FString& TargetMachineID, bool bPerpetual);

	// 解析许可证密钥
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static FTrialLicenseInfo ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase);

	// 获取设备ID
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static FString GetMachineID();

	// 验证许可证
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static bool ValidateLicense(const FString& LicenseKey, const FString& PassPhrase);

	// 获取剩余试用时间（秒）
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static float GetTrialRemainingTime(const FString& LicenseKey, const FString& PassPhrase);

	// 获取已使用时间（秒）
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static float GetUsedTime();

	// 重置试用数据
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static void ResetTrialData();

	// 检测系统时间是否被篡改
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static bool HasSystemTimeTampered();

	// 生成试用请求（包含 ProjectId 与 MachineID 的加密字符串）
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static FString GenerateTrialRequest();

	// 解析试用请求
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static bool ParseTrialRequest(const FString& EncryptedRequest, const FString& PassPhrase, FString& OutProjectId, FString& OutMachineID);

	// 结束本次会话并将使用时长持久化（用于退出时调用）
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	static void EndTrialSessionAndPersist();

	// 内部函数：SHA256哈希
	static FString SHA256Hash(const FString& Input);

	// 内部函数：AES加密
	static FString AESEncrypt(const FString& Input, const FString& Key);

	// 内部函数：AES解密
	static FString AESDecrypt(const FString& Input, const FString& Key);

	// 内部函数：验证设备ID是否匹配
	static bool IsLicenseMatchedDevice(const FString& LicenseMachineID);

	// 内部函数：获取本地存储的试用数据文件路径
	static FString GetTrialDataFilePath();
};