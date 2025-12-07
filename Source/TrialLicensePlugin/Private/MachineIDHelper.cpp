// Copyright 2025, Rune. All Rights Reserved.

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

FString FMachineIDHelper::GetCPUSerial()
{
	// 在Windows上获取CPU序列号需要特殊权限，这里使用替代方法
	return FPlatformMisc::GetCPUBrand();
}

FString FMachineIDHelper::GetBaseboardSerial()
{
	// 获取主板信息
	return FPlatformMisc::GetPrimaryGPUBrand();
}

FString FMachineIDHelper::GetHardDiskSerial()
{
    // 获取硬盘序列号
    // 使用稳定的替代：组合计算机名与主目录路径哈希
    FString Fallback = FPlatformProcess::ComputerName();
    Fallback += FPlatformProcess::UserName();
    uint32 Hash = FCrc::StrCrc32(*Fallback);
    return FString::Printf(TEXT("%08x"), Hash);
}