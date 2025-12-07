// Copyright 2025, Rune. All Rights Reserved.

#include "TrialLicenseSettingsCustomization.h"
#if WITH_EDITOR
#include "TrialLicenseSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

TSharedRef<IDetailCustomization> FTrialLicenseSettingsCustomization::MakeInstance()
{
    return MakeShared<FTrialLicenseSettingsCustomization>();
}

void FTrialLicenseSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // 移除了预览按钮,使用编辑器预览勾选框代替
}
#endif // WITH_EDITOR
