// Copyright 2025, Rune. All Rights Reserved.

/**
 * @file TrialLicenseSettings.cpp
 * @brief 试用许可证设置类实现
 *
 * 该文件实现了UTrialLicenseSettings类，定义了试用版许可证插件的配置参数，
 * 包括水印显示参数、许可证验证参数等。这些设置可在编辑器的插件设置中进行配置，
 * 并在编辑器中修改时实时应用到水印显示效果中。
 */

#include "TrialLicenseSettings.h"
#include "TrialWatermarkOverlay.h"

#define LOCTEXT_NAMESPACE "UTrialLicenseSettings"

/**
 * @brief 构造函数
 *
 * 初始化试用许可证插件的各项默认设置参数，包括水印显示参数、许可证验证参数等。
 * 这些参数定义了试用版水印的默认外观和行为特征，以及许可证验证的相关配置。
 */
UTrialLicenseSettings::UTrialLicenseSettings()
{
    WatermarkEnabled = true;
    bEnableEditorPreview = false;
    WatermarkText = TEXT("Trial Version");
    LicenseKey = TEXT("");
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
    bDefaultTrialLicenseShowWatermark = true;
}

/**
 * @brief 获取设置页面的类别名称
 *
 * 返回该设置页面在编辑器中所属的类别名称，用于在插件设置中进行分类显示。
 *
 * @return 类别名称，这里返回"Plugins"表示属于插件类别
 */
FName UTrialLicenseSettings::GetCategoryName() const
{
    return FName(TEXT("Plugins"));
}

/**
 * @brief 获取设置页面的标题文本
 *
 * 返回该设置页面在编辑器中显示的标题文本，用于标识该插件的设置页面。
 *
 * @return 标题文本，显示为"试用许可证插件"
 */
FText UTrialLicenseSettings::GetSectionText() const
{
    return LOCTEXT("SectionText", "试用许可证插件");
}

/**
 * @brief 获取设置页面的描述文本
 *
 * 返回该设置页面在编辑器中显示的描述文本，用于说明该设置页面的功能用途。
 *
 * @return 描述文本，说明该页面用于配置试用版水印与许可证参数
 */
FText UTrialLicenseSettings::GetSectionDescription() const
{
    return LOCTEXT("SectionDesc", "配置试用版水印与许可证参数。");
}

#if WITH_EDITOR
/**
 * @brief 编辑器属性变更回调函数
 *
 * 当在编辑器中修改设置属性时触发此回调函数，根据被修改的属性实时更新水印显示效果，
 * 以提供即时的视觉反馈。不同的属性变更会触发不同的处理逻辑，某些属性需要重建水印组件才能生效。
 *
 * @param PropertyChangedEvent 属性变更事件，包含被修改的属性信息
 */
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
        // 水印启用状态变更，更新显示状态
        Overlay.SetEnabled(WatermarkEnabled);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkText))
    {
        // 水印文本变更，更新显示文本
        Overlay.SetBaseText(WatermarkText);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkOpacity))
    {
        // 水印透明度变更，更新透明度级别
        Overlay.SetOpacity(WatermarkOpacity);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkFontSize))
    {
        // 水印字体大小变更，更新字体大小
        Overlay.SetFontSize(WatermarkFontSize);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkAngle))
    {
        // 水印角度变更，更新旋转角度
        Overlay.SetAngle(WatermarkAngle);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkTileSpacingX) ||
             PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkTileSpacingY))
    {
        // 水印平铺间距变更，更新X和Y方向的间距设置
        Overlay.SetTileSpacing(WatermarkTileSpacingX, WatermarkTileSpacingY);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, WatermarkVerticalOffset))
    {
        // 水印垂直偏移量变更，更新偏移量设置
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
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTrialLicenseSettings, LicenseKey))
    {
        // 许可证信息变更，需要重新初始化以更新缓存
        Overlay.Shutdown();
        Overlay.Initialize();
    }
}
#endif

#undef LOCTEXT_NAMESPACE
