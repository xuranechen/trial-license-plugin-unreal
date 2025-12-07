// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 设备ID获取辅助类
 */
class TRIALLICENSEPLUGIN_API FMachineIDHelper
{
public:
	// 获取设备唯一标识符
	static FString GetMachineID();

private:
	// 获取CPU序列号
	static FString GetCPUSerial();

	// 获取主板序列号
	static FString GetBaseboardSerial();

	// 获取硬盘序列号
	static FString GetHardDiskSerial();
};