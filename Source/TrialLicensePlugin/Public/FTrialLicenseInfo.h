// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrialLicenseInfo.generated.h"

/**
 * 试用许可证信息结构体
 */
USTRUCT(BlueprintType)
struct TRIALLICENSEPLUGIN_API FTrialLicenseInfo
{
	GENERATED_BODY()

	// 试用时长（小时）
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	float DurationHours = 0.0f;

	// 生成时间
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FDateTime GenerateTime;

	// 过期时间
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FDateTime ExpireTime;

	// 设备ID
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString MachineID;

	// 签名
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString Signature;

	// 许可证密钥
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString LicenseKey;

	// 项目标识（用于防止跨项目复用）
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString ProjectId;

	// 是否为永久授权
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	bool bPerpetual = false;

	FTrialLicenseInfo()
	{
		GenerateTime = FDateTime::Now();
		ExpireTime = FDateTime::Now();
	}

	// 构造函数
	FTrialLicenseInfo(float InDurationHours, const FDateTime& InGenerateTime, const FDateTime& InExpireTime, const FString& InMachineID, const FString& InSignature)
		: DurationHours(InDurationHours), GenerateTime(InGenerateTime), ExpireTime(InExpireTime), MachineID(InMachineID), Signature(InSignature)
	{
	}
};