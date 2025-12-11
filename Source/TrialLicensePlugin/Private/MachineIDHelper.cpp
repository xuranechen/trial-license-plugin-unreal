// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 机器ID助手类，提供获取设备唯一标识符的功能
 *
 * 该类通过收集系统硬件信息（如CPU、内存、操作系统等）来生成稳定的设备标识符，
 * 用于许可证绑定到特定机器，确保许可证不会被复制到其他设备使用。
 */
#include "MachineIDHelper.h"
#include "HAL/PlatformMisc.h"
#include "Misc/EngineVersion.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "LicenseEncryptor.h"
#include "Templates/UnrealTemplate.h"
#include "HAL/CriticalSection.h"
#include "Misc/Crc.h"

/**
 * @brief 获取机器唯一标识符
 *
 * 通过收集多种系统信息（计算机名、用户名、CPU品牌、内存大小、操作系统版本）
 * 并使用SHA256哈希生成稳定的设备标识符，用于许可证绑定。
 *
 * @return 64字符的十六进制SHA256哈希字符串，作为机器唯一ID
 */
FString FMachineIDHelper::GetMachineID()
{
	// 获取系统信息作为设备标识符
	FString MachineID = "";

	// 获取计算机名称
	MachineID += FPlatformProcess::ComputerName();

	// 获取用户名
	MachineID += FPlatformProcess::UserName();

	// 获取CPU信息
	MachineID += FPlatformMisc::GetCPUBrand();

	// 获取内存大小
	MachineID += FString::FromInt(FPlatformMemory::GetPhysicalGBRam());

	// 获取操作系统版本
	MachineID += FPlatformMisc::GetOSVersion();

	// 使用SHA256哈希生成固定长度的设备ID
	return FLicenseEncryptor::SHA256Hash(MachineID);
}

/**
 * @brief 获取CPU序列号
 *
 * 由于在Windows平台上获取真实的CPU序列号需要特殊权限，这里使用CPU品牌信息作为替代。
 * 这提供了一个稳定的标识符，虽然不如真正的序列号唯一，但对于许可证验证已经足够。
 *
 * @return CPU品牌字符串，作为CPU的标识符
 */
FString FMachineIDHelper::GetCPUSerial()
{
	// 在Windows上获取CPU序列号需要特殊权限，这里使用替代方法
	return FPlatformMisc::GetCPUBrand();
}

/**
 * @brief 获取主板序列号
 *
 * 获取主板相关信息作为硬件标识符。这里使用GPU品牌信息作为主板的替代标识符，
 * 因为直接获取主板序列号在跨平台环境下比较复杂。
 *
 * @return GPU品牌字符串，作为主板的标识符
 */
FString FMachineIDHelper::GetBaseboardSerial()
{
	// 获取主板信息
	return FPlatformMisc::GetPrimaryGPUBrand();
}

/**
 * @brief 获取硬盘序列号
 *
 * 获取硬盘相关信息作为硬件标识符。由于直接获取硬盘序列号需要系统权限且跨平台复杂，
 * 这里使用计算机名和用户名的组合哈希作为稳定的替代标识符。
 *
 * @return 8字符的十六进制CRC32哈希字符串，作为硬盘的标识符
 */
FString FMachineIDHelper::GetHardDiskSerial()
{
    // 获取硬盘序列号
    // 使用稳定的替代：组合计算机名与主目录路径哈希
    FString Fallback = FPlatformProcess::ComputerName();
    Fallback += FPlatformProcess::UserName();
    uint32 Hash = FCrc::StrCrc32(*Fallback);
    return FString::Printf(TEXT("%08x"), Hash);
}
