// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrialLicenseSettings.generated.h"

/**
 * @brief 试用许可证插件设置类
 *
 * 该类提供了试用许可证插件的所有可配置选项，包括水印显示设置、
 * 许可证配置、过期提示设置等。所有设置都会保存到项目配置文件中，
 * 可以在编辑器的项目设置界面中进行配置。
 */
UCLASS(Config=Game, DefaultConfig)
class TRIALLICENSEPLUGIN_API UTrialLicenseSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    /**
     * @brief 构造函数，初始化默认设置值
     *
     * 设置所有配置项的默认值，确保插件在首次使用时具有合理的默认配置。
     */
    UTrialLicenseSettings();

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="启用水印 | Enable Watermark", ToolTip="是否在视口显示试用水印 | Whether to display the trial watermark in the viewport"))
    bool WatermarkEnabled;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="编辑器预览 | Editor Preview", ToolTip="在编辑器模式下预览水印效果(需重启编辑器) | Preview watermark in editor mode (requires editor restart)"))
    bool bEnableEditorPreview;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="水印文本 | Watermark Text", ToolTip="显示在屏幕上的水印内容 | The text content shown as the watermark"))
    FString WatermarkText;

    UPROPERTY(EditAnywhere, Config, Category="License", meta=(DisplayName="许可证密钥 | License Key", ToolTip="用于解密和验证的许可证密钥 | The license key used for decryption/validation"))
    FString LicenseKey;

    UPROPERTY(EditAnywhere, Config, Category="License", meta=(DisplayName="口令 | Passphrase", ToolTip="与许可证配套的口令/密钥短语 | Passphrase paired with the license"))
    FString PassPhrase;

    UPROPERTY(EditAnywhere, Config, Category="License", meta=(DisplayName="使用UMG弹窗 | Use UMG Widget", ToolTip="启用后使用UMG自定义弹窗，否则使用默认Slate弹窗 | Enable to use custom UMG widget, otherwise use default Slate dialog"))
    bool bUseUMGWidget;

    UPROPERTY(EditAnywhere, Config, Category="License", meta=(DisplayName="UMG弹窗类 | UMG Widget Class", ToolTip="自定义UMG弹窗类（需继承自TrialExpiredWidgetBase），仅在启用UMG弹窗时有效 | Custom UMG widget class (must inherit from TrialExpiredWidgetBase), only works when UMG widget is enabled", EditCondition="bUseUMGWidget"))
    FSoftClassPath ExpiredWidgetClass;

    UPROPERTY(EditAnywhere, Config, Category="License", meta=(DisplayName="自动弹出过期提示 | Auto Show Expired Dialog", ToolTip="证书过期时自动弹出提示窗口 | Automatically show dialog when license expires"))
    bool bAutoShowExpiredDialog;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(ClampMin="0.0", ClampMax="1.0", DisplayName="水印不透明度 | Opacity", ToolTip="水印透明度(0-1) | Watermark opacity (0-1)"))
    float WatermarkOpacity;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(ClampMin="8", DisplayName="水印字号 | Font Size", ToolTip="水印字体大小(像素) | Font size in pixels"))
    int32 WatermarkFontSize;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(ClampMin="-90.0", ClampMax="90.0", DisplayName="水印角度(度) | Angle (deg)", ToolTip="水印旋转角度(度) | Watermark rotation angle in degrees"))
    float WatermarkAngle;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(ClampMin="0.0", DisplayName="水印水平间距 | Tile Spacing X", ToolTip="相邻水印之间的水平距离(像素)，设为0则自动调整 | Horizontal distance between adjacent watermarks in pixels, set to 0 for auto"))
    float WatermarkTileSpacingX;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(ClampMin="0.0", DisplayName="水印垂直间距 | Tile Spacing Y", ToolTip="相邻水印之间的垂直距离(像素)，设为0则自动调整 | Vertical distance between adjacent watermarks in pixels, set to 0 for auto"))
    float WatermarkTileSpacingY;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="垂直偏移 | Vertical Offset", ToolTip="水印整体在垂直方向的偏移量(像素)，正值向下，负值向上 | Overall vertical offset of the watermark in pixels, positive moves down, negative moves up"))
    float WatermarkVerticalOffset;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="水印图片路径 | Image Path (PNG/JPG)", ToolTip="可选的水印图片文件路径(绝对路径或相对项目Content路径)，留空则只显示文本 | Optional watermark image file path, leave empty to show text only", FilePathFilter="Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg"))
    FFilePath WatermarkImagePath;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(ClampMin="0.1", ClampMax="5.0", DisplayName="图片缩放 | Image Scale", ToolTip="水印图片的缩放比例 | Scale factor for the watermark image"))
    float WatermarkImageScale;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="水印字体路径 | Font Path (TTF/OTF/TTC)", ToolTip="自定义字体文件路径(绝对路径或相对项目Content路径), 用于显示中文等多语言字符", FilePathFilter="Font files (*.ttf;*.otf;*.ttc)|*.ttf;*.otf;*.ttc"))
    FFilePath WatermarkFontPath;

    UPROPERTY(EditAnywhere, Config, Category="Watermark", meta=(DisplayName="仅显示图片 | Image Only", ToolTip="仅显示图片水印，不显示文本 | Show only image watermark without text"))
    bool bWatermarkImageOnly;

    /**
     * @brief 获取设置分类名称
     *
     * 返回在项目设置界面中显示的设置分类名称。
     *
     * @return 设置分类名称
     */
    virtual FName GetCategoryName() const override;

    /**
     * @brief 获取设置节标题文本
     *
     * 返回在项目设置界面中显示的节标题文本。
     *
     * @return 节标题文本
     */
    virtual FText GetSectionText() const override;

    /**
     * @brief 获取设置节描述文本
     *
     * 返回在项目设置界面中显示的节描述文本，用于说明该设置节的功能。
     *
     * @return 节描述文本
     */
    virtual FText GetSectionDescription() const override;

#if WITH_EDITOR
    /**
     * @brief 属性编辑后回调
     *
     * 当用户在编辑器中修改设置属性时，此方法会被调用。
     * 可以在此方法中实现属性变更后的响应逻辑，如实时更新水印显示等。
     *
     * @param PropertyChangedEvent 属性变更事件信息
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
