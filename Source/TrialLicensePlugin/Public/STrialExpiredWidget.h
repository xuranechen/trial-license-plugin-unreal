// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * @brief 试用证书失效提示弹窗（Slate版本）
 *
 * 这是默认的、功能完整的弹窗实现，无需重写即可使用。
 * 当证书过期或试用时间耗尽时自动弹出，提供完整的用户交互功能。
 *
 * 功能特性：
 * - 显示过期原因和剩余时间信息
 * - 显示项目ID、机器ID和试用请求信息
 * - 一键复制试用请求到剪贴板
 * - 支持关闭弹窗或退出应用操作
 *
 * 如需自定义UI，请使用UMG版本（UTrialExpiredWidgetBase）。
 */
class TRIALLICENSEPLUGIN_API STrialExpiredWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STrialExpiredWidget)
        : _ExpireReason(TEXT("Trial period has expired"))
        , _RemainingTime(0.f)
        , _bIsPerpetual(false)
    {}
        /** @brief 过期原因文本 */
        SLATE_ARGUMENT(FString, ExpireReason)
        /** @brief 剩余时间（秒），0表示已过期 */
        SLATE_ARGUMENT(float, RemainingTime)
        /** @brief 是否为永久授权 */
        SLATE_ARGUMENT(bool, bIsPerpetual)
    SLATE_END_ARGS()

    /**
     * @brief 构造函数，初始化弹窗内容
     *
     * 根据传入的参数构建弹窗UI，包括标题、消息、试用请求信息和操作按钮。
     *
     * @param InArgs Slate构造参数，包含过期原因、剩余时间和永久授权标志
     */
    void Construct(const FArguments& InArgs);

    /**
     * @brief 设置过期原因文本
     *
     * 更新弹窗显示的过期原因信息。
     *
     * @param InReason 新的过期原因文本
     */
    void SetExpireReason(const FString& InReason);

    /**
     * @brief 设置剩余时间
     *
     * 更新弹窗显示的剩余试用时间信息。
     *
     * @param InSeconds 剩余时间（秒），0表示已过期
     */
    void SetRemainingTime(float InSeconds);

private:
    /**
     * @brief 获取弹窗内容Widget
     *
     * 构建并返回完整的弹窗UI内容，包括标题、消息、试用请求信息和按钮区域。
     *
     * @return 弹窗内容的Widget引用
     */
    TSharedRef<SWidget> GetContent();

    /**
     * @brief 关闭按钮点击事件处理
     *
     * 关闭当前弹窗窗口，不退出应用程序。
     *
     * @return 事件处理结果
     */
    FReply OnCloseClicked();

    /**
     * @brief 退出应用按钮点击事件处理
     *
     * 退出整个应用程序。
     *
     * @return 事件处理结果
     */
    FReply OnQuitClicked();

    /**
     * @brief 复制试用请求按钮点击事件处理
     *
     * 将试用请求信息复制到系统剪贴板，方便用户发送给开发者。
     *
     * @return 事件处理结果
     */
    FReply OnCopyTrialRequestClicked();

    /**
     * @brief 获取标题文本
     *
     * 根据过期状态和永久授权标志返回相应的标题文本。
     *
     * @return 标题文本
     */
    FText GetTitleText() const;

    /**
     * @brief 获取消息文本
     *
     * 根据过期原因、剩余时间和永久授权标志返回相应的消息文本。
     *
     * @return 消息文本
     */
    FText GetMessageText() const;

    /**
     * @brief 获取关闭按钮文本
     *
     * 返回关闭按钮的显示文本。
     *
     * @return 关闭按钮文本
     */
    FText GetCloseButtonText() const;

    /**
     * @brief 获取退出按钮文本
     *
     * 返回退出应用按钮的显示文本。
     *
     * @return 退出按钮文本
     */
    FText GetQuitButtonText() const;

    /**
     * @brief 获取复制按钮文本
     *
     * 返回复制试用请求按钮的显示文本。
     *
     * @return 复制按钮文本
     */
    FText GetCopyButtonText() const;

    /**
     * @brief 获取试用请求信息文本
     *
     * 生成包含项目ID、机器ID和试用请求的完整信息文本，用于显示在弹窗中。
     *
     * @return 试用请求信息文本
     */
    FText GetTrialRequestInfo() const;

    /**
     * @brief 生成试用请求字符串
     *
     * 从子系统获取并生成加密的试用请求字符串，用于发送给开发者获取许可证。
     *
     * @return 加密的试用请求字符串，如果获取失败则返回空字符串
     */
    FString GenerateTrialRequestString() const;

private:
    /** @brief 过期原因文本 */
    FString ExpireReason;

    /** @brief 剩余时间（秒），0表示已过期 */
    float RemainingTime;

    /** @brief 是否为永久授权 */
    bool bIsPerpetual;
};
