// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrialExpiredWidgetBase.generated.h"

/**
 * @brief UMG版本的试用证书失效提示弹窗基类
 *
 * 该类提供了可在蓝图中继承和自定义的过期提示弹窗基类。
 * 开发者可以在蓝图中继承此类，设计自定义的UI界面，实现个性化的过期提示效果。
 *
 * 使用方法：
 * 1. 创建蓝图类，继承自 TrialExpiredWidgetBase
 * 2. 在蓝图中设计UI界面，绑定相关事件和数据
 * 3. 在项目设置中指定你的蓝图类路径
 */
UCLASS(Blueprintable, BlueprintType)
class TRIALLICENSEPLUGIN_API UTrialExpiredWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief 初始化弹窗数据
     *
     * 设置弹窗显示的过期原因、剩余时间和永久授权标志，并触发蓝图初始化事件。
     *
     * @param InReason 过期原因文本
     * @param InRemainingTime 剩余时间（秒），0表示已过期
     * @param bInIsPerpetual 是否为永久授权
     */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void InitializeExpiredWidget(const FString& InReason, float InRemainingTime, bool bInIsPerpetual);

    /**
     * @brief 蓝图事件：弹窗初始化时调用
     *
     * 当弹窗数据初始化完成后，此事件会被触发，用于在蓝图中更新UI显示。
     * 可以在蓝图中重写此事件，实现自定义的UI更新逻辑。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Trial License", meta = (DisplayName = "On Expired Widget Initialized"))
    void OnExpiredWidgetInitialized();

    /**
     * @brief 蓝图事件：关闭按钮点击
     *
     * 当用户点击关闭按钮时，此事件会被触发。
     * 可以在蓝图中重写此事件，实现自定义的关闭逻辑。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Trial License", meta = (DisplayName = "On Close Clicked"))
    void OnCloseClicked();

    /**
     * @brief 蓝图事件：退出应用按钮点击
     *
     * 当用户点击退出应用按钮时，此事件会被触发。
     * 可以在蓝图中重写此事件，实现自定义的退出逻辑。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Trial License", meta = (DisplayName = "On Quit Clicked"))
    void OnQuitClicked();

    /**
     * @brief 关闭弹窗
     *
     * 从父窗口移除当前Widget，关闭弹窗显示。
     * 可以在蓝图中调用此方法手动关闭弹窗。
     */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void CloseDialog();

    /**
     * @brief 退出应用
     *
     * 退出整个应用程序。
     * 可以在蓝图中调用此方法退出应用。
     */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void QuitApplication();

    /**
     * @brief 复制试用请求到剪贴板
     *
     * 生成试用请求字符串并复制到系统剪贴板，方便用户发送给开发者获取许可证。
     * 可以在蓝图中调用此方法实现一键复制功能。
     */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    void CopyTrialRequestToClipboard();

    /**
     * @brief 生成试用请求字符串
     *
     * 从子系统获取并生成加密的试用请求字符串，用于发送给开发者获取许可证。
     *
     * @return 加密的试用请求字符串，如果获取失败则返回空字符串
     */
    UFUNCTION(BlueprintCallable, Category = "Trial License")
    FString GenerateTrialRequestString();

protected:
    /** @brief 过期原因文本 */
    UPROPERTY(BlueprintReadOnly, Category = "Trial License")
    FString ExpireReason;

    /** @brief 剩余时间（秒），0表示已过期 */
    UPROPERTY(BlueprintReadOnly, Category = "Trial License")
    float RemainingTime;

    /** @brief 是否为永久授权 */
    UPROPERTY(BlueprintReadOnly, Category = "Trial License")
    bool bIsPerpetual;

public:
    /**
     * @brief 获取标题文本（可在蓝图中重写）
     *
     * 根据过期状态和永久授权标志返回相应的标题文本。
     * 可以在蓝图中重写此方法，实现自定义的标题显示逻辑。
     *
     * @return 标题文本
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetTitleText() const;

    /**
     * @brief 获取消息文本（可在蓝图中重写）
     *
     * 根据过期原因、剩余时间和永久授权标志返回相应的消息文本。
     * 可以在蓝图中重写此方法，实现自定义的消息显示逻辑。
     *
     * @return 消息文本
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetMessageText() const;

    /**
     * @brief 获取关闭按钮文本（可在蓝图中重写）
     *
     * 返回关闭按钮的显示文本。
     * 可以在蓝图中重写此方法，实现自定义的按钮文本。
     *
     * @return 关闭按钮文本
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetCloseButtonText() const;

    /**
     * @brief 获取退出按钮文本（可在蓝图中重写）
     *
     * 返回退出应用按钮的显示文本。
     * 可以在蓝图中重写此方法，实现自定义的按钮文本。
     *
     * @return 退出按钮文本
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Trial License")
    FText GetQuitButtonText() const;

    /**
     * @brief 获取剩余时间的格式化文本
     *
     * 将剩余时间（秒）格式化为"X小时Y分钟"的文本格式。
     *
     * @return 格式化后的剩余时间文本
     */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FText GetFormattedRemainingTime() const;

    /**
     * @brief 获取机器ID
     *
     * 从子系统获取当前机器的唯一标识符。
     *
     * @return 机器唯一标识符字符串，如果获取失败则返回空字符串
     */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FString GetMachineID() const;

    /**
     * @brief 获取项目ID
     *
     * 获取当前项目的名称作为项目标识符。
     *
     * @return 项目名称字符串
     */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FString GetProjectID() const;

    /**
     * @brief 获取试用请求信息（用于显示）
     *
     * 生成包含项目ID、机器ID和试用请求的完整信息文本，用于在UI中显示。
     *
     * @return 试用请求信息文本
     */
    UFUNCTION(BlueprintPure, Category = "Trial License")
    FText GetTrialRequestInfo() const;
};
