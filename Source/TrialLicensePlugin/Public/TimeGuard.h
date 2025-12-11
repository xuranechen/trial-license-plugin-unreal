// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief 时间保护机制类，提供系统时间篡改检测功能
 *
 * 该类实现了防止用户通过修改系统时间来延长试用期的保护机制。
 * 通过记录和比较会话结束时间，检测系统时间是否被向后调整。
 * 所有方法都是静态的，无需创建实例即可使用。
 */
class TRIALLICENSEPLUGIN_API FTimeGuard
{
public:
	/**
	 * @brief 检测系统时间是否被篡改
	 *
	 * 通过比较当前系统时间和上次记录的会话结束时间，判断系统时间是否被向后调整。
	 * 如果当前时间早于上次会话结束时间，则认为系统时间被篡改。
	 *
	 * @return true如果检测到系统时间被篡改，false如果时间正常
	 */
	static bool HasSystemTimeTampered();

	/**
	 * @brief 记录当前会话结束时间
	 *
	 * 将当前系统时间保存为会话结束时间，用于下次启动时进行时间篡改检测。
	 * 应在应用程序关闭或会话结束时调用。
	 */
	static void RecordSessionEndTime();

	/**
	 * @brief 获取上次会话结束时间
	 *
	 * 从持久化存储中读取上次记录的会话结束时间。
	 *
	 * @return 上次会话结束的时间戳，如果从未记录过则返回默认时间
	 */
	static FDateTime GetLastSessionEndTime();
};