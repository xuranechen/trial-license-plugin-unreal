// Copyright 2025, Rune. All Rights Reserved.

/**
 * @file TrialLicenseSettingsCustomization.cpp
 * @brief 试用许可证设置界面定制实现
 *
 * 该文件实现了FTrialLicenseSettingsCustomization类，用于定制编辑器中
 * 试用许可证设置页面的外观和交互。通过继承IDetailCustomization接口，
 * 可以自定义设置面板的布局和控件，提供更好的用户体验和功能定制。
 */

#include "TrialLicenseSettingsCustomization.h"
#if WITH_EDITOR
#include "TrialLicenseSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

/**
 * @brief 创建设置界面定制实例
 *
 * 静态工厂方法，用于创建并返回试用许可证设置的界面定制实例。
 * 该实例会被编辑器用来定制设置面板的外观和交互行为。
 *
 * @return 返回设置界面定制实例的共享引用
 */
TSharedRef<IDetailCustomization> FTrialLicenseSettingsCustomization::MakeInstance()
{
    return MakeShared<FTrialLicenseSettingsCustomization>();
}

/**
 * @brief 定制设置详情界面
 *
 * 实现IDetailCustomization接口的核心方法，用于定制设置详情面板的布局和控件。
 * 该方法会在设置面板显示时被调用，可以添加自定义控件、修改现有控件或调整面板布局。
 *
 * @param DetailBuilder 详情布局构建器，用于构建和定制设置面板的布局
 */
void FTrialLicenseSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // 移除了预览按钮,使用编辑器预览勾选框代替
    // 该方法当前保持为空实现，保留了未来扩展定制功能的可能性
}
#endif // WITH_EDITOR
