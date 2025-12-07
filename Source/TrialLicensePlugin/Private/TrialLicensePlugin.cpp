// Copyright 2025, Rune. All Rights Reserved.

#include "TrialLicensePlugin.h"
#include "UTrialLicenseLibrary.h"
#include "TimeGuard.h"
#include "TrialWatermarkOverlay.h"
#include "TrialLicenseSettings.h"
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

void FTrialLicensePluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Bind pre-exit to persist session and record time guard
	if (!PreExitHandle.IsValid())
	{
		PreExitHandle = FCoreDelegates::OnPreExit.AddLambda([]()
		{
			UTrialLicenseLibrary::EndTrialSessionAndPersist();
			FTimeGuard::RecordSessionEndTime();
		});
	}

#if WITH_EDITOR
	// Bind EndPIE to persist session and record time guard when exiting PIE
	if (!EndPIEHandle.IsValid())
	{
		EndPIEHandle = FEditorDelegates::EndPIE.AddLambda([](bool)
		{
			UTrialLicenseLibrary::EndTrialSessionAndPersist();
			FTimeGuard::RecordSessionEndTime();
			// 清理PIE模式下的水印
			FTrialWatermarkOverlay::Get().DetachFromViewport();
		});
	}

    // Register settings customization
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.RegisterCustomClassLayout(UTrialLicenseSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTrialLicenseSettingsCustomization::MakeInstance));
        PropertyModule.NotifyCustomizationModuleChanged();
    }

    // Initialize watermark overlay in editor if preview is enabled
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
    // Initialize global trial watermark overlay (runtime only)
    FTrialWatermarkOverlay::Get().Initialize();
#endif

}

void FTrialLicensePluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Persist trial session usage on exit
	UTrialLicenseLibrary::EndTrialSessionAndPersist();

	// Record time guard checkpoint on exit
	FTimeGuard::RecordSessionEndTime();

	// Unbind delegates
	if (PreExitHandle.IsValid())
	{
		FCoreDelegates::OnPreExit.Remove(PreExitHandle);
		PreExitHandle.Reset();
	}
#if WITH_EDITOR
	if (EndPIEHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);
		EndPIEHandle.Reset();
	}

    // Unregister settings customization
    {
        if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
        {
            FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
            PropertyModule.UnregisterCustomClassLayout(UTrialLicenseSettings::StaticClass()->GetFName());
            PropertyModule.NotifyCustomizationModuleChanged();
        }
    }
#endif

    // Shutdown watermark overlay
    FTrialWatermarkOverlay::Get().Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTrialLicensePluginModule, TrialLicensePlugin)