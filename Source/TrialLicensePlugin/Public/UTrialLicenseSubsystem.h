// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTrialLicenseInfo.h"
#include "UTrialLicenseSubsystem.generated.h"

/**
 * @brief 试用许可证子系统
 *
 * 该类是试用许可证系统的核心子系统，继承自UGameInstanceSubsystem。
 * 负责管理许可证验证、试用期控制、数据持久化、水印显示等所有核心功能。
 * 提供蓝图可调用的接口，支持在编辑器和运行时进行许可证管理。
 */
UCLASS()
class TRIALLICENSEPLUGIN_API UTrialLicenseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UTrialLicenseSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * @brief 生成许可证密钥
	 *
	 * 根据指定的参数生成加密的许可证密钥字符串。
	 *
	 * @param DurationHours 试用时长（小时）
	 * @param ExpiryDate 过期日期
	 * @param PassPhrase 密码短语，用于加密
	 * @param ProjectId 项目标识符
	 * @param TargetMachineID 目标机器ID
	 * @param bPerpetual 是否为永久许可证
	 * @return 生成的加密许可证密钥字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	FString GenerateLicenseKey(float DurationHours, const FDateTime& ExpiryDate, const FString& PassPhrase, const FString& ProjectId, const FString& TargetMachineID, bool bPerpetual);

	/**
	 * @brief 解析许可证密钥
	 *
	 * 解密并解析许可证密钥字符串，返回许可证信息结构体。
	 *
	 * @param LicenseKey 要解析的许可证密钥
	 * @param PassPhrase 用于解密的密码短语
	 * @return 解析出的许可证信息结构体
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	FTrialLicenseInfo ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase);

	/**
	 * @brief 验证许可证
	 *
	 * 验证许可证密钥的有效性，包括签名验证、过期检查、设备绑定等。
	 *
	 * @param LicenseKey 要验证的许可证密钥
	 * @param PassPhrase 用于验证的密码短语
	 * @return true如果许可证有效，false如果无效
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	bool ValidateLicense(const FString& LicenseKey, const FString& PassPhrase);

	/**
	 * @brief 获取试用剩余时间
	 *
	 * 计算并返回许可证的剩余试用时间（秒）。
	 *
	 * @param LicenseKey 许可证密钥
	 * @param PassPhrase 密码短语
	 * @return 剩余试用时间（秒），负值表示已过期
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	float GetTrialRemainingTime(const FString& LicenseKey, const FString& PassPhrase);

	/**
	 * @brief 获取机器ID
	 *
	 * 获取当前设备的唯一标识符，用于许可证绑定。
	 *
	 * @return 当前机器的唯一ID字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	FString GetMachineID();

	/**
	 * @brief 获取已使用的试用时间
	 *
	 * 从存储的数据中获取已使用的试用时间。
	 *
	 * @return 已使用的试用时间（秒）
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	float GetUsedTime();

	/**
	 * @brief 重置试用数据
	 *
	 * 清除所有存储的试用数据，包括使用时间和许可证信息。
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	void ResetTrialData();

	/**
	 * @brief 检查系统时间是否被篡改
	 *
	 * 检测用户是否通过修改系统时间来绕过试用期限制。
	 *
	 * @return true如果检测到时间篡改，false如果时间正常
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	bool HasSystemTimeTampered();

	/**
	 * @brief 结束试用会话并持久化数据
	 *
	 * 结束当前的试用会话，并将使用时间等数据保存到存储中。
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	void EndTrialSessionAndPersist();

	/**
	 * @brief 生成试用请求
	 *
	 * 生成包含项目ID和机器ID的试用请求字符串，用于申请许可证。
	 *
	 * @return 加密的试用请求字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	FString GenerateTrialRequest();

	/**
	 * @brief 解析试用请求
	 *
	 * 解析加密的试用请求，提取项目ID和机器ID信息。
	 *
	 * @param EncryptedRequest 加密的试用请求字符串
	 * @param PassPhrase 用于解密的密码短语
	 * @param OutProjectId 输出参数：项目ID
	 * @param OutMachineID 输出参数：机器ID
	 * @return true如果解析成功，false如果失败
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License")
	bool ParseTrialRequest(const FString& EncryptedRequest, const FString& PassPhrase, FString& OutProjectId, FString& OutMachineID);

	/**
	 * @brief 设置水印是否启用
	 *
	 * 控制试用水印的显示状态。
	 *
	 * @param bEnabled true启用水印，false禁用水印
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkEnabled(bool bEnabled);

	/**
	 * @brief 设置水印文本
	 *
	 * 设置水印显示的文本内容。
	 *
	 * @param NewText 新的水印文本
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkText(const FString& NewText);

	/**
	 * @brief 设置水印角度
	 *
	 * 设置水印文本的旋转角度。
	 *
	 * @param AngleDegrees 旋转角度（度）
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkAngle(float AngleDegrees);

	/**
	 * @brief 设置水印平铺间距（统一）
	 *
	 * 设置水印在X和Y方向上的统一间距。
	 *
	 * @param TileSpacing 统一的间距值
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkTileSpacing(float TileSpacing);

	/**
	 * @brief 设置水印平铺间距（分别设置）
	 *
	 * 分别设置水印在X和Y方向上的间距。
	 *
	 * @param TileSpacingX X方向间距
	 * @param TileSpacingY Y方向间距
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkTileSpacingXY(float TileSpacingX, float TileSpacingY);

	/**
	 * @brief 设置水印不透明度
	 *
	 * 设置水印的透明度级别。
	 *
	 * @param Opacity 不透明度值（0.0-1.0）
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkOpacity(float Opacity);

	/**
	 * @brief 设置水印字体大小
	 *
	 * 设置水印文本的字体大小。
	 *
	 * @param FontSize 字体大小（像素）
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void SetWatermarkFontSize(int32 FontSize);

	/**
	 * @brief 获取水印是否启用
	 *
	 * @return true如果水印已启用，false如果已禁用
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	bool GetWatermarkEnabled();

	/**
	 * @brief 获取水印文本
	 *
	 * @return 当前的水印文本内容
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	FString GetWatermarkText();

	/**
	 * @brief 获取水印角度
	 *
	 * @return 水印的旋转角度（度）
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	float GetWatermarkAngle();

	/**
	 * @brief 获取水印平铺间距
	 *
	 * @return 水印的统一间距值
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	float GetWatermarkTileSpacing();

	/**
	 * @brief 获取水印X方向间距
	 *
	 * @return 水印X方向的间距
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	float GetWatermarkTileSpacingX();

	/**
	 * @brief 获取水印Y方向间距
	 *
	 * @return 水印Y方向的间距
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	float GetWatermarkTileSpacingY();

	/**
	 * @brief 获取水印不透明度
	 *
	 * @return 水印的不透明度值
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	float GetWatermarkOpacity();

	/**
	 * @brief 获取水印字体大小
	 *
	 * @return 水印的字体大小
	 */
	UFUNCTION(BlueprintPure, Category = "Trial License|Watermark")
	int32 GetWatermarkFontSize();

	/**
	 * @brief 从项目设置应用水印配置
	 *
	 * 从项目设置中读取水印配置并应用到当前水印显示。
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Watermark")
	void ApplyWatermarkFromProjectSettings();

	/**
	 * @brief 显示过期对话框
	 *
	 * 显示许可证过期的提醒对话框。
	 *
	 * @param Reason 过期原因
	 * @param RemainingTime 剩余时间（秒）
	 * @param bIsPerpetual 是否为永久许可证
	 */
	UFUNCTION(BlueprintCallable, Category = "Trial License|Dialog")
	void ShowExpiredDialog(const FString& Reason = TEXT(""), float RemainingTime = 0.f, bool bIsPerpetual = false);

	// 内部工具方法
	FString SHA256Hash(const FString& Input);
	FString AESEncrypt(const FString& Input, const FString& Key);
	FString AESDecrypt(const FString& Input, const FString& Key);
	bool IsLicenseMatchedDevice(const FString& LicenseMachineID);
	FString GetTrialDataFilePath();

private:
	// 会话状态管理（从全局静态变量迁移过来）
	bool bTrialSessionActive;
	FDateTime TrialSessionStart;
	FString TrialSessionHash;
	FString TrialSessionMachineID;
	FTSTicker::FDelegateHandle TrialTickerHandle;
	double TrialLastFlushedSessionSeconds;
	double TrialFlushIntervalSeconds;
	bool bTrialPeriodicEnabled;

	// 定时器回调函数
	static bool TrialPeriodicFlushTick(float DeltaTime, UTrialLicenseSubsystem* Subsystem);
};
