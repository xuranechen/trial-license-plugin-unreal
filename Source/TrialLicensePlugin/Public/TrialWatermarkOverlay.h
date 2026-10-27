// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Containers/Ticker.h"

class STrialWatermarkWidget;

/**
 * @brief 试用水印覆盖层管理器
 *
 * 该类负责管理试用版水印的显示和配置，包括水印文本、角度、间距、不透明度等设置。
 * 实现了单例模式，确保全局只有一个水印覆盖层实例。
 * 支持动态显示过期提醒对话框。
 */
class TRIALLICENSEPLUGIN_API FTrialWatermarkOverlay
{
public:
	/**
	 * @brief 获取单例实例
	 *
	 * 返回全局唯一的FTrialWatermarkOverlay实例。
	 *
	 * @return 水印覆盖层管理器的引用
	 */
    static FTrialWatermarkOverlay& Get();

	/**
	 * @brief 初始化水印覆盖层
	 *
	 * 设置水印的初始状态并准备显示。需要在引擎启动时调用。
	 */
    void Initialize();

	/**
	 * @brief 关闭水印覆盖层
	 *
	 * 清理资源并停止水印显示。需要在引擎关闭时调用。
	 */
    void Shutdown();

	/**
	 * @brief 从视口分离水印
	 *
	 * 将水印从当前视口中移除，但不销毁实例。
	 */
    void DetachFromViewport();

	/**
	 * @brief 设置水印是否启用
	 *
	 * 控制水印的显示和隐藏状态。
	 *
	 * @param bInEnabled true启用水印显示，false隐藏水印
	 */
    void SetEnabled(bool bInEnabled);

	/**
	 * @brief 设置水印的基础文本
	 *
	 * 设置水印显示的文本内容。
	 *
	 * @param InText 水印文本内容
	 */
    void SetBaseText(const FString& InText);

	/**
	 * @brief 设置水印的旋转角度
	 *
	 * 设置水印文本的旋转角度，以度为单位。
	 *
	 * @param InAngleDegrees 旋转角度（度）
	 */
    void SetAngle(float InAngleDegrees);

	/**
	 * @brief 设置水印的平铺间距
	 *
	 * 设置水印在X和Y方向上的重复间距。
	 *
	 * @param InTileSpacingX X方向间距，0表示自动调整
	 * @param InTileSpacingY Y方向间距
	 */
    void SetTileSpacing(float InTileSpacingX, float InTileSpacingY);

	/**
	 * @brief 设置水印的垂直偏移
	 *
	 * 设置水印相对于屏幕顶部的垂直偏移量。
	 *
	 * @param InVerticalOffset 垂直偏移量（像素）
	 */
    void SetVerticalOffset(float InVerticalOffset);

	/**
	 * @brief 设置水印的不透明度
	 *
	 * 设置水印的透明度级别。
	 *
	 * @param InOpacity 不透明度值（0.0-1.0）
	 */
    void SetOpacity(float InOpacity);

	/**
	 * @brief 设置水印的字体大小
	 *
	 * 设置水印文本的字体大小。
	 *
	 * @param InFontSize 字体大小（像素）
	 */
    void SetFontSize(int32 InFontSize);

	/**
	 * @brief 显示证书过期弹窗
	 *
	 * 显示许可证过期的提醒对话框，包含过期原因和剩余时间信息。
	 *
	 * @param Reason 过期原因描述
	 * @param RemainingTime 剩余时间（秒）
	 * @param bIsPerpetual 是否为永久许可证
	 */
    void ShowExpiredDialog(const FString& Reason = TEXT(""), float RemainingTime = 0.f, bool bIsPerpetual = false);

	/**
	 * @brief 许可证验证成功后刷新覆盖层缓存
	 *
	 * 用于在运行时输入新许可证后立即生效，避免继续使用旧缓存密钥。
	 */
	void OnLicenseValidated(const FString& NewLicenseKey);
	void ResetExpiredDialogState();

	/**
	 * @brief 获取水印是否启用
	 *
	 * @return true如果水印已启用，false如果已禁用
	 */
    bool IsEnabled() const { return bEnabled; }

	/**
	 * @brief 获取水印的基础文本
	 *
	 * @return 水印文本内容
	 */
    FString GetBaseText() const { return BaseText; }

	/**
	 * @brief 获取水印的旋转角度
	 *
	 * @return 旋转角度（度）
	 */
    float GetAngle() const { return AngleDegrees; }

	/**
	 * @brief 获取水印X方向的平铺间距
	 *
	 * @return X方向间距
	 */
    float GetTileSpacingX() const { return TileSpacingX; }

	/**
	 * @brief 获取水印Y方向的平铺间距
	 *
	 * @return Y方向间距
	 */
    float GetTileSpacingY() const { return TileSpacingY; }

	/**
	 * @brief 获取水印的垂直偏移
	 *
	 * @return 垂直偏移量
	 */
    float GetVerticalOffset() const { return VerticalOffset; }

	/**
	 * @brief 获取水印的不透明度
	 *
	 * @return 不透明度值
	 */
    float GetOpacity() const { return Opacity; }

	/**
	 * @brief 获取水印的字体大小
	 *
	 * @return 字体大小
	 */
    int32 GetFontSize() const { return FontSize; }

private:
    FTrialWatermarkOverlay() = default;

    void TryAttachToViewport();
    void UpdateWatermarkText();

    bool Tick(float DeltaTime);

private:
    TWeakPtr<SWidget> RootOverlayWidget;
    TWeakPtr<STrialWatermarkWidget> WatermarkWidget;
    FTSTicker::FDelegateHandle TickerHandle;
    bool bAttached = false;
    FString CachedLicenseKey;
    FString BaseText;
    bool bEnabled = true;
    float AngleDegrees = -30.f;
    float TileSpacingX = 0.f; // 0 = 自动调整
    float TileSpacingY = 300.f;
    float VerticalOffset = 0.f;
    float Opacity = 0.12f;
    int32 FontSize = 48;

    /** 过期弹窗是否已显示 */
    bool bExpiredDialogShown = false;

    /** 上次检查过期的时间戳 */
    double LastExpireCheckTime = 0.0;
};
