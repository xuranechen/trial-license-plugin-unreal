// Copyright 2025, Rune. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 试用证书失效提示弹窗（Slate版本）
 * 这是默认的、功能完整的弹窗实现，无需重写
 * 当证书过期或试用时间耗尽时自动弹出
 * 
 * 功能特性：
 * - 显示过期原因和剩余时间
 * - 显示项目ID、机器ID和试用请求信息
 * - 一键复制试用请求到剪贴板
 * - 关闭弹窗或退出应用
 * 
 * 如需自定义UI，请使用UMG版本（UTrialExpiredWidgetBase）
 */
class TRIALLICENSEPLUGIN_API STrialExpiredWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STrialExpiredWidget)
        : _ExpireReason(TEXT("Trial period has expired"))
        , _RemainingTime(0.f)
        , _bIsPerpetual(false)
    {}
        /** 过期原因文本 */
        SLATE_ARGUMENT(FString, ExpireReason)
        /** 剩余时间（秒），0表示已过期 */
        SLATE_ARGUMENT(float, RemainingTime)
        /** 是否为永久授权 */
        SLATE_ARGUMENT(bool, bIsPerpetual)
    SLATE_END_ARGS()

    /** 构造函数 */
    void Construct(const FArguments& InArgs);

    /** 设置过期原因 */
    void SetExpireReason(const FString& InReason);

    /** 设置剩余时间 */
    void SetRemainingTime(float InSeconds);

private:
    /** 获取弹窗内容 */
    TSharedRef<SWidget> GetContent();

    /** 关闭按钮点击事件 */
    FReply OnCloseClicked();

    /** 退出应用按钮点击事件 */
    FReply OnQuitClicked();

    /** 复制试用请求按钮点击事件 */
    FReply OnCopyTrialRequestClicked();

    /** 获取标题文本 */
    FText GetTitleText() const;

    /** 获取消息文本 */
    FText GetMessageText() const;

    /** 获取关闭按钮文本 */
    FText GetCloseButtonText() const;

    /** 获取退出按钮文本 */
    FText GetQuitButtonText() const;

    /** 获取复制按钮文本 */
    FText GetCopyButtonText() const;

    /** 获取试用请求信息 */
    FText GetTrialRequestInfo() const;

    /** 生成试用请求字符串 */
    FString GenerateTrialRequestString() const;

private:
    /** 过期原因 */
    FString ExpireReason;

    /** 剩余时间（秒） */
    float RemainingTime;

    /** 是否为永久授权 */
    bool bIsPerpetual;
};
