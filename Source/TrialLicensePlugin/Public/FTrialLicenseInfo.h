// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrialLicenseInfo.generated.h"

/**
 * @brief 试用许可证信息结构体
 *
 * 该结构体包含了试用许可证的所有必要信息，包括时长、时间戳、设备绑定信息等。
 * 用于在许可证生成、验证和存储过程中传递许可证数据。
 * 支持蓝图使用，可以在编辑器中直接编辑属性。
 */
USTRUCT(BlueprintType)
struct TRIALLICENSEPLUGIN_API FTrialLicenseInfo
{
	GENERATED_BODY()

	/**
	 * @brief 试用时长（小时）
	 *
	 * 指定许可证的有效试用时长，以小时为单位。
	 * 对于永久许可证，该值可能为0或被忽略。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	float DurationHours = 0.0f;

	/**
	 * @brief 生成时间
	 *
	 * 许可证的生成时间戳，用于计算过期时间和验证许可证时效性。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FDateTime GenerateTime;

	/**
	 * @brief 过期时间
	 *
	 * 许可证的到期时间戳。对于永久许可证，该时间可能设置为一个遥远的未来时间。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FDateTime ExpireTime;

	/**
	 * @brief 设备ID
	 *
	 * 绑定许可证的机器唯一标识符，用于防止许可证在不同设备间共享。
	 * 通过收集硬件信息生成，确保设备唯一性。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString MachineID;

	/**
	 * @brief 签名
	 *
	 * 许可证数据的数字签名，用于验证许可证的完整性和真实性。
	 * 通过SHA256哈希关键信息生成。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString Signature;

	/**
	 * @brief 许可证密钥
	 *
	 * 加密后的许可证密钥字符串，包含所有许可证信息的加密版本。
	 * 用于许可证的分发和验证。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString LicenseKey;

	/**
	 * @brief 项目标识（用于防止跨项目复用）
	 *
	 * 项目唯一标识符，用于防止许可证在不同项目间复用。
	 * 确保每个许可证只能在特定项目中使用。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	FString ProjectId;

	/**
	 * @brief 是否为永久授权
	 *
	 * 指示该许可证是否为永久授权。如果为true，则忽略时间限制。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	bool bPerpetual = false;

	/**
	 * @brief 是否显示水印
	 *
	 * 允许许可证自身控制水印是否显示。缺失该字段的旧许可证将回退到默认策略：
	 * 试用许可证显示水印，永久许可证隐藏水印。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trial License")
	bool bShowWatermark = true;

	/**
	 * @brief 默认构造函数
	 *
	 * 初始化结构体，将生成时间和过期时间设置为当前时间。
	 */
	FTrialLicenseInfo()
	{
		GenerateTime = FDateTime::Now();
		ExpireTime = FDateTime::Now();
	}

	/**
	 * @brief 参数化构造函数
	 *
	 * 使用指定的参数初始化许可证信息结构体。
	 *
	 * @param InDurationHours 试用时长（小时）
	 * @param InGenerateTime 生成时间
	 * @param InExpireTime 过期时间
	 * @param InMachineID 机器ID
	 * @param InSignature 签名
	 */
	FTrialLicenseInfo(float InDurationHours, const FDateTime& InGenerateTime, const FDateTime& InExpireTime, const FString& InMachineID, const FString& InSignature)
		: DurationHours(InDurationHours), GenerateTime(InGenerateTime), ExpireTime(InExpireTime), MachineID(InMachineID), Signature(InSignature)
	{
	}
};
