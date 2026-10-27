# UE5 插件 UMG 导入导出接线指南

本文说明如何在 `WBP_TrialLicenseMain` 中接入以下功能：

- 导出 `.request` 文件
- 导入 `.lic` 文件
- 显示文件操作状态
- 保留手动输入许可证并验证的能力

说明：

- `Slate` 弹窗的导入/导出能力已经在 C++ 中完成
- `UMG` 侧已经提供了可直接调用的基类接口
- 你现在只需要在蓝图里添加控件并绑定事件

---

## 1. 前置确认

打开 `Content/WBP_TrialLicenseMain.uasset`，确认它的父类是：

- `TrialExpiredWidgetBase`

如果不是：

1. 打开蓝图
2. `Class Settings`
3. 找到 `Parent Class`
4. 改成 `TrialExpiredWidgetBase`

---

## 2. 可直接使用的基类函数

在蓝图里可直接调用这些函数：

- `CopyTrialRequestToClipboard`
- `ExportTrialRequestToFile`
- `ExportTrialRequestWithDialog`
- `ImportLicenseFromFile`
- `ImportLicenseWithDialog`
- `GenerateTrialRequestString`
- `ValidateLicenseKey`
- `GetFileOperationStatusText`
- `GetValidationStatusText`
- `IsValidationSuccessful`
- `CloseDialog`
- `QuitApplication`
- `GetTrialRequestInfo`

---

## 3. 推荐控件结构

建议在原有弹窗中按下面结构整理：

### 信息区

- `TextBlock`：标题
- `TextBlock`：过期说明
- `MultiLineEditableTextBox` 或 `MultiLineTextBox`
  - 名称：`Txt_TrialRequestInfo`
  - 用于显示项目 ID、机器 ID、试用请求内容

### Request 文件操作区

- `Button`：`Btn_CopyRequest`
- `Button`：`Btn_ExportRequest`

### 许可证输入区

- `EditableTextBox`：`Input_LicenseKey`
- `Button`：`Btn_ValidateLicense`
- `Button`：`Btn_ImportLic`

### 状态区

- `TextBlock`：`Txt_FileOpStatus`
- `TextBlock`：`Txt_ValidationStatus`

### 底部操作区

- `Button`：`Btn_EnterApp`
- `Button`：`Btn_Quit`

---

## 4. 推荐布局方式

建议做成两组按钮：

### 组 1：试用申请文件

- `复制请求串`
- `导出 .request 文件`

### 组 2：许可证导入与验证

- `导入 .lic 文件`
- `验证输入密钥`

这样和当前 `Slate` 弹窗布局一致，用户更容易理解。

---

## 5. 文本绑定

### 5.1 绑定试用请求信息

选中 `Txt_TrialRequestInfo`：

1. 找到 `Text`
2. 点击 `Bind`
3. 创建绑定函数
4. 在绑定函数中调用：
   - `GetTrialRequestInfo`
5. 将返回值直接作为 Text 输出

### 5.2 绑定文件操作状态

选中 `Txt_FileOpStatus`：

1. 找到 `Text`
2. 点击 `Bind`
3. 创建绑定函数
4. 在绑定函数中调用：
   - `GetFileOperationStatusText`
5. 返回该文本

### 5.3 绑定许可证验证状态

选中 `Txt_ValidationStatus`：

1. 找到 `Text`
2. 点击 `Bind`
3. 创建绑定函数
4. 在绑定函数中调用：
   - `GetValidationStatusText`
5. 返回该文本

---

## 6. 按钮事件绑定

### 6.1 复制请求串按钮

按钮：`Btn_CopyRequest`

`OnClicked` 中调用：

- `CopyTrialRequestToClipboard`

### 6.2 导出 request 文件按钮

按钮：`Btn_ExportRequest`

`OnClicked` 中调用：

- `ExportTrialRequestWithDialog`

说明：

- 会自动弹出系统保存文件对话框
- 用户选择路径后会保存为 `.request`

### 6.3 导入 lic 文件按钮

按钮：`Btn_ImportLic`

`OnClicked` 中调用：

- `ImportLicenseWithDialog`

说明：

- 会自动弹出系统文件选择对话框
- 用户选择 `.lic` 后会立即验证并写入配置

### 6.4 手动验证许可证按钮

按钮：`Btn_ValidateLicense`

`OnClicked` 中这样连：

1. 从 `Input_LicenseKey` 取文本
2. 转成 `String`
3. 调用 `ValidateLicenseKey`

节点逻辑可理解为：

- `Input_LicenseKey -> GetText`
- `Conv_TextToString`
- `ValidateLicenseKey`

---

## 7. 进入应用按钮处理

按钮：`Btn_EnterApp`

推荐两种方式：

### 方式 A：只控制可点击状态

将按钮的 `Is Enabled` 绑定到：

- `IsValidationSuccessful`

然后 `OnClicked` 中调用：

- `CloseDialog`

### 方式 B：点击时再判断

`OnClicked`：

1. 调用 `IsValidationSuccessful`
2. `Branch`
3. 为 `true` 时调用 `CloseDialog`

推荐方式 A，更直观。

---

## 8. 退出应用按钮处理

按钮：`Btn_Quit`

`OnClicked` 中调用：

- `QuitApplication`

---

## 9. 推荐视觉文案

推荐按钮文字：

- `复制请求串`
- `导出 .request 文件`
- `导入 .lic 文件`
- `验证输入密钥`
- `进入应用`
- `退出应用`

推荐状态文本说明：

- `Txt_FileOpStatus`：显示导出/导入结果
- `Txt_ValidationStatus`：显示手动输入密钥验证结果

建议颜色：

- 文件状态：偏蓝/青色
- 验证成功：绿色
- 验证失败：红色

---

## 10. 推荐蓝图连线顺序

建议按下面顺序完成，最不容易漏：

1. 改父类为 `TrialExpiredWidgetBase`
2. 添加 `Txt_TrialRequestInfo`
3. 绑定 `GetTrialRequestInfo`
4. 添加 `Txt_FileOpStatus`
5. 绑定 `GetFileOperationStatusText`
6. 添加 `Txt_ValidationStatus`
7. 绑定 `GetValidationStatusText`
8. 添加 `Btn_CopyRequest`
9. 绑定 `CopyTrialRequestToClipboard`
10. 添加 `Btn_ExportRequest`
11. 绑定 `ExportTrialRequestWithDialog`
12. 添加 `Btn_ImportLic`
13. 绑定 `ImportLicenseWithDialog`
14. 添加 `Input_LicenseKey`
15. 添加 `Btn_ValidateLicense`
16. 绑定 `ValidateLicenseKey`
17. 添加 `Btn_EnterApp`
18. 绑定 `CloseDialog`
19. 给 `Btn_EnterApp` 绑定 `IsValidationSuccessful`
20. 添加 `Btn_Quit`
21. 绑定 `QuitApplication`

---

## 11. 功能验证清单

接完后建议按下面顺序测试：

### 11.1 导出 request

1. 点击 `导出 .request 文件`
2. 选择保存位置
3. 确认生成 `.request` 文件
4. 确认 `Txt_FileOpStatus` 显示成功提示

### 11.2 导入 lic

1. 点击 `导入 .lic 文件`
2. 选择一个有效许可证文件
3. 确认状态文本显示导入成功
4. 确认 `进入应用` 可点击

### 11.3 手动输入验证

1. 在 `Input_LicenseKey` 粘贴许可证串
2. 点击 `验证输入密钥`
3. 确认 `Txt_ValidationStatus` 更新

### 11.4 进入应用

1. 成功导入或验证后
2. 点击 `进入应用`
3. 确认弹窗关闭并可继续进入应用

---

## 12. 重要说明

- `导入 .lic` 和 `手动输入验证` 是两条并行入口
- `导入 .lic` 成功后，也会写入插件设置中的 `LicenseKey`
- `Slate` 弹窗已经完成相同能力，`UMG` 只是蓝图界面层还需要手工接按钮
- 当前方案不直接修改 `.uasset`，所以最安全、最可控

---

## 13. 相关 C++ 文件

如果后续需要对照代码，可查看：

- `Source/TrialLicensePlugin/Public/UTrialLicenseSubsystem.h`
- `Source/TrialLicensePlugin/Private/UTrialLicenseSubsystem.cpp`
- `Source/TrialLicensePlugin/Public/TrialExpiredWidgetBase.h`
- `Source/TrialLicensePlugin/Private/TrialExpiredWidgetBase.cpp`
- `Source/TrialLicensePlugin/Public/STrialExpiredWidget.h`
- `Source/TrialLicensePlugin/Private/STrialExpiredWidget.cpp`
- `Source/TrialLicensePlugin/TrialLicensePlugin.Build.cs`
