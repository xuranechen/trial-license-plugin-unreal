// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#if WITH_EDITOR
#include "IDetailCustomization.h"

/**
 * @brief 试用许可证设置的自定义详情面板类
 *
 * 该类实现了在编辑器属性面板中对试用许可证设置的自定义显示和布局。
 * 用于优化设置界面，提供更好的用户体验和更清晰的配置选项组织。
 * 仅在编辑器模式下有效。
 */
class FTrialLicenseSettingsCustomization : public IDetailCustomization
{
public:
    /**
     * @brief 创建自定义详情面板实例
     *
     * 创建并返回一个自定义详情面板的共享引用实例。
     *
     * @return 自定义详情面板的共享引用
     */
    static TSharedRef<IDetailCustomization> MakeInstance();

    /**
     * @brief 自定义详情布局
     *
     * 自定义属性面板的布局和显示方式，可以重新组织属性、添加说明文本等。
     *
     * @param DetailBuilder 详情布局构建器，用于构建自定义布局
     */
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
#endif // WITH_EDITOR
