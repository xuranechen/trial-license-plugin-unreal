// Copyright 2025, Rune. All Rights Reserved.

/**
 * @brief 试用许可证插件模块实现
 *
 * 该文件实现了FTrialLicensePluginModule类，负责插件的生命周期管理。
 * 包括模块启动时的初始化、水印覆盖设置、编辑器集成以及关机时的清理工作。
 * 确保试用许可证系统的正常运行和数据持久化。
 */
#include "TrialLicensePlugin.h"
#include "UTrialLicenseSubsystem.h"
#include "TimeGuard.h"
#include "TrialWatermarkOverlay.h"
#include "TrialLicenseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "TrialLicenseSettingsCustomization.h"
#endif

#define LOCTEXT_NAMESPACE "FTrialLicensePluginModule"

/**
 * @brief 辅助函数：通过子系统结束试用会话
 *
 * 在应用程序关闭时安全地访问试用许可证子系统并结束会话。
 * 使用多重回退策略确保在各种关机情况下都能正确访问子系统。
 * 避免在子系统已经自动处理的情况下重复调用。
 */
static void EndTrialSessionViaSubsystem()
{
	// 在关机时，GameInstance可能已经被销毁，所以我们需要更安全的检查
	if (!GEngine)
	{
		UE_LOG(LogTemp, Warning, TEXT("GEngine is null during shutdown, cannot access TrialLicenseSubsystem."));
		return;
	}

	UTrialLicenseSubsystem* Subsystem = nullptr;

	// 首先尝试通过GameInstance获取子系统
	if (UWorld* World = GEngine->GetCurrentPlayWorld())
	{
		if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World))
		{
			Subsystem = GameInstance->GetSubsystem<UTrialLicenseSubsystem>();
		}
	}

	// 回退方案：尝试从GEngine获取
	if (!Subsystem && GEngine->GameViewport)
	{
		if (UGameInstance* GameInstance = GEngine->GameViewport->GetGameInstance())
		{
			Subsystem = GameInstance->GetSubsystem<UTrialLicenseSubsystem>();
		}
	}

	if (Subsystem)
	{
		// 只有在会话仍然活跃时才调用EndTrialSessionAndPersist
		// 子系统的Deinitialize会自动调用，所以这里避免重复调用
		if (Subsystem->IsValidLowLevel() && Subsystem->HasAnyFlags(RF_ClassDefaultObject) == false)
		{
			Subsystem->EndTrialSessionAndPersist();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot access TrialLicenseSubsystem for session end - no valid world or game instance available."));
	}
}

/**
 * @brief 启动插件模块
 *
 * 初始化试用许可证插件模块，设置必要的委托绑定和系统集成。
 * 根据编译配置（编辑器/运行时）执行不同的初始化逻辑。
 * 确保时间守卫记录和水印系统的正确启动。
 */
void FTrialLicensePluginModule::StartupModule()
{
	// 此代码将在模块加载到内存后执行；确切的时间在 .uplugin 文件中按模块指定

	// 绑定预退出委托，确保在应用程序关闭时记录时间守卫
	if (!PreExitHandle.IsValid())
	{
		PreExitHandle = FCoreDelegates::OnPreExit.AddLambda([]()
		{
			// 子系统会在Deinitialize中自动处理会话结束，这里只记录时间守卫
			FTimeGuard::RecordSessionEndTime();
		});
	}

#if WITH_EDITOR
	// 编辑器专用初始化

	// 绑定PIE结束委托，在退出Play In Editor时进行清理
	if (!EndPIEHandle.IsValid())
	{
		EndPIEHandle = FEditorDelegates::EndPIE.AddLambda([](bool)
		{
			// 子系统会在Deinitialize中自动处理会话结束，这里只记录时间守卫
			FTimeGuard::RecordSessionEndTime();
			// 清理PIE模式下的水印，避免在编辑器中残留
			FTrialWatermarkOverlay::Get().DetachFromViewport();
		});
	}

    // 注册属性编辑器自定义，为设置类提供自定义UI
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.RegisterCustomClassLayout(UTrialLicenseSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTrialLicenseSettingsCustomization::MakeInstance));
        PropertyModule.NotifyCustomizationModuleChanged();
    }

    // 如果启用了编辑器预览，则初始化水印并应用设置
    const UTrialLicenseSettings* Settings = GetDefault<UTrialLicenseSettings>();
    if (Settings && Settings->bEnableEditorPreview)
    {
        FTrialWatermarkOverlay::Get().Initialize();
        FTrialWatermarkOverlay& Overlay = FTrialWatermarkOverlay::Get();
        Overlay.SetEnabled(Settings->WatermarkEnabled);
        Overlay.SetBaseText(Settings->WatermarkText);
        Overlay.SetOpacity(Settings->WatermarkOpacity);
        Overlay.SetFontSize(Settings->WatermarkFontSize);
        Overlay.SetAngle(Settings->WatermarkAngle);
        Overlay.SetTileSpacing(Settings->WatermarkTileSpacingX, Settings->WatermarkTileSpacingY);
    }
#endif

#if !WITH_EDITOR
    // 运行时专用初始化：初始化全局水印覆盖
    FTrialWatermarkOverlay::Get().Initialize();
#endif

}

/**
 * @brief 关闭插件模块
 *
 * 清理试用许可证插件模块的所有资源和绑定。
 * 确保数据持久化、委托解绑和系统资源的正确释放。
 * 根据编译配置执行不同的清理逻辑。
 */
void FTrialLicensePluginModule::ShutdownModule()
{
	// 此函数可能在关机期间被调用以清理您的模块。对于支持动态重载的模块，
	// 我们会在卸载模块之前调用此函数。

	// 记录最后的时间守卫，确保时间篡改检测的连续性
	FTimeGuard::RecordSessionEndTime();

	// 解绑所有委托，避免内存泄漏和野指针
	if (PreExitHandle.IsValid())
	{
		FCoreDelegates::OnPreExit.Remove(PreExitHandle);
		PreExitHandle.Reset();
	}

#if WITH_EDITOR
	// 编辑器专用清理

	// 解绑PIE结束委托
	if (EndPIEHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);
		EndPIEHandle.Reset();
	}

    // 注销属性编辑器自定义
    {
        if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
        {
            FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
            PropertyModule.UnregisterCustomClassLayout(UTrialLicenseSettings::StaticClass()->GetFName());
            PropertyModule.NotifyCustomizationModuleChanged();
        }
    }
#endif

    // 关闭水印覆盖系统，释放相关资源
    FTrialWatermarkOverlay::Get().Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTrialLicensePluginModule, TrialLicensePlugin)
