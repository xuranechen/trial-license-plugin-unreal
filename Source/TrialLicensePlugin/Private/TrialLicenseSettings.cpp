// Copyright 2025, Rune. All Rights Reserved.

#include "TrialLicenseSettings.h"
#include "TrialWatermarkOverlay.h"

#define LOCTEXT_NAMESPACE "UTrialLicenseSettings"

UTrialLicenseSettings::UTrialLicenseSettings()
{
    WatermarkEnabled = true;
    bEnableEditorPreview = false;
    WatermarkText = TEXT("Trial Version");
    LicenseKey = TEXT("");
    PassPhrase = TEXT("");
    WatermarkOpacity = 0.12f;
    WatermarkFontSize = 48;
    WatermarkAngle = -30.f;
    WatermarkTileSpacingX = 0.f; // 0 表示自动调整
    WatermarkTileSpacingY = 300.f;
    WatermarkVerticalOffset = 0.f;
    WatermarkImagePath.FilePath = TEXT("");
    WatermarkImageScale = 1.0f;
    WatermarkFontPath.FilePath = TEXT("../../../trial-license-plugin-unreal/Content/SourceHanSansSC-Medium-2.otf");
    bWatermarkImageOnly = false;
    ExpiredWidgetClass = FSoftClassPath(); // 留空使用默认Slate弹窗
    bUseUMGWidget = false; // 默认使用Slate版本
    bAutoShowExpiredDialog = true;
}

FName UTrialLicenseSettings::GetCategoryName() const
{
    return FName(TEXT("Plugins"));
}

FText UTrialLicenseSettings::GetSectionText() const
{
    return LOCTEXT("SectionText", "试用许可证插件");
}

FText UTrialLicenseSettings::GetSectionDescription() const
{
    return LOCTEXT("SectionDesc", "配置试用版水印与许可证参数。");
}

#if WITH_EDITOR
void UTrialLicenseSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (!PropertyChangedEvent.Property)
    {
        return;
    }

    const FName PropertyName = PropertyChangedEvent.Property->GetFName();

    // 立即应用水印相关配置到覆盖层
    FTrialWatermarkOverlay& Overlay = FTrialWatermarkOverlay::Get();

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkEnabled))
    {
        Overlay.SetEnabled(WatermarkEnabled);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkText))
    {
        Overlay.SetBaseText(WatermarkText);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkOpacity))
    {
        Overlay.SetOpacity(WatermarkOpacity);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkFontSize))
    {
        Overlay.SetFontSize(WatermarkFontSize);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkAngle))
    {
        Overlay.SetAngle(WatermarkAngle);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkTileSpacingX) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkTileSpacingY))
    {
        Overlay.SetTileSpacing(WatermarkTileSpacingX, WatermarkTileSpacingY);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkVerticalOffset))
    {
        Overlay.SetVerticalOffset(WatermarkVerticalOffset);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkImagePath) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkImageScale) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, bWatermarkImageOnly))
    {
        // 图片相关配置变更需要重建 Widget
        Overlay.DetachFromViewport();
        Overlay.Initialize();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkFontPath))
    {
        // 字体路径变更需要重新创建Widget
        Overlay.DetachFromViewport();
        Overlay.Initialize();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, LicenseKey) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, PassPhrase))
    {
        // 许可证信息变更，需要重新初始化以更新缓存
        Overlay.Shutdown();
        Overlay.Initialize();
    }
}
#endif

#undef LOCTEXT_NAMESPACE
