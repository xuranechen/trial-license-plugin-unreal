// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 时间保护机制（防篡改）
 */
class TRIALLICENSEPLUGIN_API FTimeGuard
{
public:
	// 检测系统时间是否被篡改
	static bool HasSystemTimeTampered();

	// 记录当前会话结束时间
	static void RecordSessionEndTime();

	// 获取上次会话结束时间
	static FDateTime GetLastSessionEndTime();
};