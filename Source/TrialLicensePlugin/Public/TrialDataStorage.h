// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrialLicenseInfo.h"

/**
 * @brief 试用数据存储管理类，提供试用数据的持久化功能
 *
 * 该类负责管理试用许可证的使用时间数据，包括保存、加载、重置等操作。
 * 数据以加密形式存储在本地文件中，确保数据安全性和完整性。
 * 所有方法都是静态的，无需创建实例即可使用。
 */
class TRIALLICENSEPLUGIN_API FTrialDataStorage
{
public:
	/**
	 * @brief 保存试用数据到本地文件
	 *
	 * 将许可证密钥哈希、机器ID和已使用时间保存到持久化存储中。
	 * 数据会先进行加密处理，然后写入配置文件。
	 *
	 * @param LicenseKeyHash 许可证密钥的SHA256哈希值
	 * @param MachineID 机器唯一标识符
	 * @param UsedSeconds 已使用的试用时间（秒）
	 */
	static void SaveTrialData(const FString& LicenseKeyHash, const FString& MachineID, float UsedSeconds);

	/**
	 * @brief 从本地文件加载试用数据
	 *
	 * 从持久化存储中读取并解密试用数据，包括许可证密钥哈希、机器ID和已使用时间。
	 *
	 * @param OutLicenseKeyHash 输出的许可证密钥哈希值
	 * @param OutMachineID 输出的机器唯一标识符
	 * @param OutUsedSeconds 输出的已使用时间（秒）
	 * @return true如果成功加载数据，false如果文件不存在或加载失败
	 */
	static bool LoadTrialData(FString& OutLicenseKeyHash, FString& OutMachineID, float& OutUsedSeconds);

	/**
	 * @brief 获取已使用的试用时间
	 *
	 * 从持久化存储中读取已使用的试用时间总和。
	 *
	 * @return 已使用的试用时间（秒），如果无法读取则返回0.0
	 */
	static float GetUsedTime();

	/**
	 * @brief 重置试用数据
	 *
	 * 清除所有已保存的试用数据，包括许可证密钥哈希、机器ID和已使用时间。
	 * 用于重置试用期或清除历史数据。
	 */
	static void ResetTrialData();

	/**
	 * @brief 获取试用数据文件路径
	 *
	 * 返回存储试用数据的本地文件完整路径。
	 *
	 * @return 试用数据文件的完整路径字符串
	 */
	static FString GetTrialDataFilePath();

	/**
	 * @brief 验证本地保存的证书是否有效
	 *
	 * 检查本地保存的证书数据是否存在且有效。验证内容包括：
	 * 1. 证书文件是否存在
	 * 2. 证书数据是否可以正确解密和解析
	 * 3. 机器ID是否匹配当前设备
	 * 4. 证书哈希是否存在
	 *
	 * @return true 如果本地证书存在且有效，false 如果证书不存在、损坏或机器ID不匹配
	 */
	static bool IsLocalLicenseValid();

private:
	/**
	 * @brief 加密并保存数据到文件
	 *
	 * 使用内部加密算法对数据进行加密，然后写入配置文件。
	 *
	 * @param Data 要保存的原始数据字符串
	 */
	static void EncryptAndSaveData(const FString& Data);

	/**
	 * @brief 从文件加载并解密数据
	 *
	 * 从配置文件中读取加密数据，然后进行解密处理。
	 *
	 * @return 解密后的原始数据字符串，如果文件不存在或解密失败则返回空字符串
	 */
	static FString LoadAndDecryptData();
};
