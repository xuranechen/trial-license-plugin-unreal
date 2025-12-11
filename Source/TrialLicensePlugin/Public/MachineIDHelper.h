// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief 机器ID获取辅助类，提供设备唯一标识符生成功能
 *
 * 该类实现了跨平台的机器唯一标识符获取机制，通过组合CPU序列号、
 * 主板序列号和硬盘序列号生成一个稳定的设备标识符。
 * 所有方法都是静态的，无需创建实例即可使用。
 */
class TRIALLICENSEPLUGIN_API FMachineIDHelper
{
public:
	/**
	 * @brief 获取设备唯一标识符
	 *
	 * 通过组合CPU序列号、主板序列号和硬盘序列号生成一个唯一的机器标识符。
	 * 该标识符用于许可证绑定，确保许可证只能在特定设备上使用。
	 *
	 * @return 设备的唯一标识符字符串，如果无法获取硬件信息则返回空字符串
	 */
	static FString GetMachineID();

private:
	/**
	 * @brief 获取CPU序列号
	 *
	 * 从系统获取CPU的序列号信息，用于生成机器唯一标识符。
	 *
	 * @return CPU序列号字符串，如果无法获取则返回空字符串
	 */
	static FString GetCPUSerial();

	/**
	 * @brief 获取主板序列号
	 *
	 * 从系统获取主板的序列号信息，用于生成机器唯一标识符。
	 *
	 * @return 主板序列号字符串，如果无法获取则返回空字符串
	 */
	static FString GetBaseboardSerial();

	/**
	 * @brief 获取硬盘序列号
	 *
	 * 从系统获取主硬盘的序列号信息，用于生成机器唯一标识符。
	 *
	 * @return 硬盘序列号字符串，如果无法获取则返回空字符串
	 */
	static FString GetHardDiskSerial();
};