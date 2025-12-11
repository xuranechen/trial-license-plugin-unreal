// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @brief 试用许可证插件模块类
 *
 * 该类是试用许可证插件的入口模块，负责插件的初始化和清理工作。
 * 在模块启动时初始化水印覆盖层，在模块关闭时清理资源。
 * 支持编辑器和运行时两种模式，确保在不同环境下正常工作。
 */
class FTrialLicensePluginModule : public IModuleInterface
{
public:

	/**
	 * @brief 模块启动时调用
	 *
	 * 初始化水印覆盖层，注册引擎退出和PIE结束的回调函数。
	 * 确保在游戏启动时水印系统能够正常工作。
	 */
	virtual void StartupModule() override;

	/**
	 * @brief 模块关闭时调用
	 *
	 * 清理水印覆盖层资源，取消注册的回调函数。
	 * 确保在插件卸载时所有资源都被正确释放。
	 */
	virtual void ShutdownModule() override;

private:
	/** @brief 引擎退出前的回调句柄 */
	FDelegateHandle PreExitHandle;
#if WITH_EDITOR
	/** @brief PIE结束时的回调句柄 */
	FDelegateHandle EndPIEHandle;
#endif
};