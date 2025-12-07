// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrialExpiredWidgetBase.generated.h"

/**
 * UMG版本的试用证书失效提示弹窗基类
 * 可在蓝图中继承此类进行UI设计
 * 
 * 使用方法：
 * 1. 创建蓝图类，继承自 TrialExpiredWidgetBase
 * 2. 在蓝图中设计UI界面
 * 3. 在项目设置中指定你的蓝图类路径
 */
UCLASS(Blueprintable, BlueprintType)
class TRIALLICENSEPLUGIN_API UTrialExpiredWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 初始化弹窗数据 */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void InitializeExpiredWidget(const FString& InReason, float InRemainingTime, bool bInIsPerpetual);

    /** 蓝图事件：弹窗初始化时调用，用于更新UI */
    UFUNCTION(BlueprintImplementableEvent, Category = "Trial License", meta = (DisplayName = "On Expired Widget Initialized"))
    void OnExpiredWidgetInitialized();

    /** 蓝图事件：关闭按钮点击 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Trial License", meta = (DisplayName = "On Close Clicked"))
    void OnCloseClicked();

    /** 蓝图事件：退出应用按钮点击 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Trial License", meta = (DisplayName = "On Quit Clicked"))
    void OnQuitClicked();

    /** C++调用：关闭弹窗 */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void CloseDialog();

    /** C++调用：退出应用 */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void QuitApplication();

    /** 复制试用请求到剪贴板 */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void CopyTrialRequestToClipboard();

    /** 生成试用请求字符串 */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    FString GenerateTrialRequestString();

protected:
    /** 过期原因文本 */
    UPROPERTY(BlueprintReadOnly, Category = "Trial License")
    FString ExpireReason;

    /** 剩余时间（秒），0表示已过期 */
    UPROPERTY(BlueprintReadOnly, Category = "Trial License")
    float RemainingTime;

    /** 是否为永久授权 */
    UPROPERTY(BlueprintReadOnly, Category = "Trial License")
    bool bIsPerpetual;

public:
    /** 获取标题文本（可在蓝图中重写） */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetTitleText() const;

    /** 获取消息文本（可在蓝图中重写） */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetMessageText() const;

    /** 获取关闭按钮文本（可在蓝图中重写） */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetCloseButtonText() const;

    /** 获取退出按钮文本（可在蓝图中重写） */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetQuitButtonText() const;

    /** 获取剩余时间的格式化文本 */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FText GetFormattedRemainingTime() const;

    /** 获取机器ID */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FString GetMachineID() const;

    /** 获取项目ID */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FString GetProjectID() const;

    /** 获取试用请求信息（用于显示） */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FText GetTrialRequestInfo() const;
};
