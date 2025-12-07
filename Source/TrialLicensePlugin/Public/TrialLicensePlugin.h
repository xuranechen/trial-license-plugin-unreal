// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTrialLicensePluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FDelegateHandle PreExitHandle;
#if WITH_EDITOR
	FDelegateHandle EndPIEHandle;
#endif
};