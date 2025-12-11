# Trial License Plugin 使用说明

轻量、可扩展的试用/授权管理插件。基于UE5子系统架构，支持项目绑定、设备绑定、永久授权、时间篡改检测、本地加密存储、试用水印显示，提供完善的蓝图与 C++ 接口。

---

## 安装与设置

### 插件安装
1. 将 `TrialLicensePlugin` 文件夹复制到您的 Unreal Engine 项目的 `Plugins` 目录下。
   - 如果 `Plugins` 目录不存在，请创建它。
2. 重新启动 Unreal Engine 编辑器。
3. 在编辑器中，导航到 `Edit > Plugins`，搜索 "Trial License Plugin" 并启用它。
4. 重启编辑器以应用更改。

### 依赖项
- Unreal Engine 4.27 或更高版本（推荐 5.x）。
- C++ 项目支持（插件包含 C++ 代码）。

### 配置设置
在 `DefaultGame.ini` 或项目配置文件中添加以下设置（可选，但推荐用于生产环境）：

```ini
[TrialLicensePlugin]
; 周期性保存开关（默认 true）
TrialPeriodicSaveEnabled=true
; 周期性保存间隔（秒，最小30s，默认300s）
TrialPeriodicSaveIntervalSeconds=300
; 本地试用数据加密口令（用于 TrialData.json）
TrialDataKey=YourStrongTrialDataKeyHere
; 时间守护加密口令（用于 TimeGuardData.json）
TimeGuardKey=YourStrongTimeGuardKeyHere
```

### 构建插件
如果您需要修改插件代码：
1. 确保您的项目是 C++ 项目。
2. 在 Visual Studio 中打开 `.uproject` 文件。
3. 构建项目，这将自动编译插件。
4. 对于打包：确保插件已启用，并在打包设置中包含它。

---

## 功能特性

- 试用限制：按到期日与累计时长双重限制（取最小值）
- 设备绑定：可指定目标机器的 MachineID 生成证书
- 项目绑定：证书与 `ProjectId`（项目名）强绑定，跨项目不可用
- 永久授权：支持生成永久授权（不受到期时间与时长限制）
- 防篡改检测：记录上次会话结束时间，发现系统时间倒退即告警
- 本地加密存储：试用数据、时间防篡改数据均以 AES 封装并带 MAC 校验
- 试用水印：支持在游戏界面显示试用版水印，可自定义文本、样式和布局
- 过期提醒：自动显示许可证过期对话框，提供用户友好的提醒

---

## 核心数据结构

### FTrialLicenseInfo
- `DurationHours` 试用时长（小时）
- `GenerateTime` 生成时间
- `ExpireTime` 过期时间
- `MachineID` 设备ID（用于设备绑定）
- `ProjectId` 项目标识（当前项目名）
- `bPerpetual` 是否为永久授权
- `Signature` 签名（绑定关键字段）
- `LicenseKey` 许可证密钥（回填原始入参）

---

## 接口总览（C++）

所在类：`UTrialLicenseLibrary`

- 生成证书（需显式指定 ProjectId，支持目标机器与永久授权）
  ```cpp
  static FString GenerateLicenseKey(
      float DurationHours,
      const FDateTime& ExpiryDate,
      const FString& PassPhrase,
      const FString& ProjectId,      // 显式传入项目标识（如 MyGame 或 UniqueID）
      const FString& TargetMachineID, // 为空则使用本机 MachineID
      bool bPerpetual                  // true = 永久授权
  );
  ```

- 解析证书
  ```cpp
  static FTrialLicenseInfo ParseLicenseKey(const FString& LicenseKey, const FString& PassPhrase);
  ```

- 获取设备 ID（在目标机器上调用获取）
  ```cpp
  static FString GetMachineID();
  ```

- 验证证书有效性（项目/设备/时间/签名/篡改）
  ```cpp
  static bool ValidateLicense(const FString& LicenseKey, const FString& PassPhrase);
  ```

- 获取剩余试用时间（秒）
  ```cpp
  // 返回值：
  //  >0  剩余秒数
  //  =0  已过期
  //  -1  永久授权（无限制）
  static float GetTrialRemainingTime(const FString& LicenseKey, const FString& PassPhrase);
  ```

- 读取/清空本地试用数据、时间篡改检测
  ```cpp
  static float GetUsedTime();
  static void ResetTrialData();
  static bool HasSystemTimeTampered();
  ```

---

## 快速上手（C++）

```cpp
// 在目标机器上：
const FString TargetID = UTrialLicenseLibrary::GetMachineID();

// 在证书生成端：
const FString Pass = TEXT("StrongPassPhrase");
const FDateTime Expiry = FDateTime(2026,1,1);

// 生成普通试用证书（72小时、到期日裁剪、绑定目标机器与当前项目）
const FString TrialKey = UTrialLicenseLibrary::GenerateLicenseKey(72.f, Expiry, Pass, TargetID, /*bPerpetual=*/false);

// 生成永久授权证书（不受时间限制，仍绑定项目与设备）
const FString PerpKey = UTrialLicenseLibrary::GenerateLicenseKey(0.f, FDateTime::MaxValue(), Pass, TargetID, /*bPerpetual=*/true);

// 在运行端验证
bool bOk = UTrialLicenseLibrary::ValidateLicense(TrialKey, Pass);
float SecondsLeft = UTrialLicenseLibrary::GetTrialRemainingTime(TrialKey, Pass); // 永久授权返回 -1
```

---

## 子系统架构

插件基于UE5子系统架构设计，主要通过 `UTrialLicenseSubsystem` 管理所有功能。子系统自动管理生命周期，提供更好的状态封装和测试友好性。

### 获取子系统实例（蓝图）

在蓝图中使用 `Get Game Instance Subsystem` 节点获取 `Trial License Subsystem` 实例：

1. 右键搜索 "Get Game Instance Subsystem"
2. 选择 "Trial License Subsystem"
3. 连接到后续操作

### 获取子系统实例（C++）

```cpp
#include "UTrialLicenseSubsystem.h"

// 获取子系统实例
UTrialLicenseSubsystem* TrialSubsystem = UGameInstance::GetSubsystem<UTrialLicenseSubsystem>(GetGameInstance());
```

## 蓝图使用

### 许可证管理节点

- **许可证生成与验证**：
  - Generate License Key（包含 TargetMachineID 与 bPerpetual）
  - Parse License Key / Validate License / Get Trial Remaining Time
  - Generate Trial Request / Parse Trial Request

- **设备与数据管理**：
  - Get Machine ID / Get Used Time / Reset Trial Data / Has System Time Tampered
  - End Trial Session And Persist

### 水印显示节点

- **水印控制**：
  - Set Watermark Enabled / Get Watermark Enabled
  - Set Watermark Text / Get Watermark Text
  - Set Watermark Angle / Get Watermark Angle
  - Set Watermark Opacity / Get Watermark Opacity
  - Set Watermark Font Size / Get Watermark Font Size
  - Set Watermark Tile Spacing / Set Watermark Tile Spacing XY
  - Get Watermark Tile Spacing / Get Watermark Tile Spacing X / Get Watermark Tile Spacing Y

- **配置应用**：
  - Apply Watermark From Project Settings

### 对话框节点

- Show Expired Dialog（显示许可证过期提醒对话框）

注意：更新版本后，生成节点签名已变化，旧蓝图需重新连线。

---

## 安全与存储

- 证书 JSON 经过 AES 封装并附带 MAC 校验，防篡改
- 试用数据与时间守护数据存放于项目 `Saved` 目录下，加密保存
- 为安全起见：
  - 使用强口令（PassPhrase）
  - 口令可外置到配置文件/环境变量（避免硬编码）

---

## 常见问题

---

## 配置（DefaultGame.ini 示例）

```ini
[TrialLicensePlugin]
; 周期性保存开关（默认 true）
TrialPeriodicSaveEnabled=true
; 周期性保存间隔（秒，最小30s，默认300s）
TrialPeriodicSaveIntervalSeconds=300
; 本地试用数据加密口令（用于 TrialData.json）
TrialDataKey=YourStrongTrialDataKeyHere
; 时间守护加密口令（用于 TimeGuardData.json）
TimeGuardKey=YourStrongTimeGuardKeyHere
```

说明：
- 若未配置，将回退至内置默认口令（不建议在生产中使用）。
- 最小周期保存间隔为 30 秒；可关闭周期保存，仅依赖退出保存。

---

## 退出保存与非常规退出覆盖

插件会在以下时机保存使用时长并记录时间守护点：
- 模块卸载：`ShutdownModule()`
- 引擎退出前：`FCoreDelegates::OnPreExit`
- 编辑器结束 PIE：`FEditorDelegates::EndPIE`（仅编辑器）

保存行为包括：
- `EndTrialSessionAndPersist()` 将本次会话自上次落盘后的增量时间累加并异步写盘
- `TimeGuard::RecordSessionEndTime()` 记录最后会话结束时间（异步）

---

## 周期性保存（异步、低频）

- 首次 `ValidateLicense()` 成功后启动周期性落盘（默认每 300 秒）。
- 仅在会话有效时运行；会话结束时自动注销定时器。
- 每次仅追加“自上次落盘以来的增量”，避免重复计数。
- 所有写盘操作通过线程池后台执行，避免阻塞主线程。

可通过配置开关与调整间隔，参见“配置”章节。

---

## 安全建议

- 强烈建议使用项目级配置为 `TrialDataKey` 与 `TimeGuardKey` 提供强口令。
- 生产环境中避免使用默认口令；可考虑结合环境变量或平台密钥库。
- 口令更新将导致旧数据无法解密，更新前请做好数据迁移或兼容策略。

---

## 蓝图接口（节点）

- Generate License Key（含 TargetMachineID 与 bPerpetual）
- Parse License Key
- Validate License
- Get Trial Remaining Time
- Get Machine ID
- Get Used Time
- Reset Trial Data
- Has System Time Tampered
- Generate Trial Request（包含 ProjectId 与 MachineID 的加密请求）
- Parse Trial Request（解密出 ProjectId 与 MachineID）

说明：更新版本后节点签名可能变化，旧蓝图需重新连线。

---

## C++ 接口补充

- `UTrialLicenseLibrary::GenerateTrialRequest()` → 返回加密的请求字符串
- `UTrialLicenseLibrary::ParseTrialRequest(Encrypted, Pass, OutProjectId, OutMachineID)`

---

## 数据文件与路径

- 试用数据：`Saved/TrialData.json`（AES 加密，含 `UsedSeconds`、`LicenseKeyHash`、`MachineID`、`LastSessionEnd`）
- 时间守护：`Saved/TimeGuardData.json`（AES 加密，含 `LastSessionEnd`）
- 口令来源：优先读取配置（见“配置”），否则使用默认内置口令

---

## 设备标识（MachineID）说明

- 默认组合计算机名、用户名、CPU 品牌、内存、OS 版本后 SHA256 固定长度哈希（跨平台稳定性较好）。
- Windows 可扩展为使用 WMI（如 CPU/主板/硬盘序列号），但需考虑权限与稳定性差异；当前实现采用稳健替代方案。

---

## 时间篡改检测说明

- 启动时读取上次会话结束时间 `LastSessionEnd` 与当前系统时间对比。
- 如当前时间早于 `LastSessionEnd` → 判定为“时间被篡改”，`ValidateLicense()` 将返回失败。
- 首次运行无文件时会写入初始守护点，后续参与对比。

---

## 试用水印功能说明

插件提供内置的试用水印显示功能，用于在试用版中显示水印提醒用户当前为试用状态。

### 水印特性

- **自动显示**：许可证验证失败时自动显示水印
- **自定义样式**：支持自定义水印文本、角度、不透明度、字体大小
- **平铺布局**：水印可平铺显示在整个界面上
- **性能优化**：使用UMG实现，渲染效率高
- **配置驱动**：支持从项目设置自动应用配置

### 水印参数说明

- **文本 (Text)**：显示的水印内容，默认 "TRIAL VERSION"
- **角度 (Angle)**：水印旋转角度（度），默认 45°
- **不透明度 (Opacity)**：水印透明度，0.0-1.0，默认 0.3
- **字体大小 (Font Size)**：水印字体大小（像素），默认 24
- **平铺间距 (Tile Spacing)**：水印之间的间距，可分别设置X/Y方向

### 使用方法

#### 蓝图中控制水印

```blueprint
// 启用水印
Set Watermark Enabled (true)

// 设置水印文本
Set Watermark Text ("试用版 - 请购买正版")

// 设置水印样式
Set Watermark Angle (30.0)
Set Watermark Opacity (0.5)
Set Watermark Font Size (32)

// 设置平铺间距
Set Watermark Tile Spacing XY (200.0, 150.0)
```

#### C++中控制水印

```cpp
// 获取子系统
UTrialLicenseSubsystem* Subsystem = UGameInstance::GetSubsystem<UTrialLicenseSubsystem>(GetGameInstance());

// 控制水印
Subsystem->SetWatermarkEnabled(true);
Subsystem->SetWatermarkText(TEXT("试用版"));
Subsystem->SetWatermarkOpacity(0.4f);
Subsystem->SetWatermarkFontSize(28);
```

### 水印显示逻辑

- 许可证验证成功时，水印自动隐藏
- 许可证验证失败时，水印自动显示
- 可通过API手动控制水印显示状态
- 水印显示不影响游戏交互

### 项目设置集成

插件提供项目设置页面，可在编辑器中配置水印默认参数：

1. 打开 `Edit > Project Settings`
2. 搜索 "Trial License"
3. 配置水印相关设置
4. 调用 `Apply Watermark From Project Settings` 应用配置

---

## 性能与线程模型

- 所有写盘均通过线程池后台执行（异步），避免阻塞游戏线程。
- 周期性保存默认 300s 触发一次，可关闭或调节（最小 30s）。
- 退出时保存与守护点记录也为异步，但在引擎关闭阶段仍建议避免过密写入（已节流）。

---

## 许可校验语义

- 项目绑定：证书中的 `ProjectId` 与当前项目名需一致。
- 设备绑定：`MachineID` 必须与当前设备一致。
- 有效期：若非永久授权，`Now > ExpireTime` 则过期。
- 时长限制：剩余时间取 `到期剩余` 与 `时长上限-已用` 之最小值。
- 永久授权：`GetTrialRemainingTime()` 返回 `-1` 表示无限制。
- 时间篡改：检测为真时，`ValidateLicense()` 返回失败。

---

## 故障排查（Troubleshooting）

- 无法通过校验：
  - 检查 `PassPhrase` 是否一致
  - 检查 `ProjectId` 是否与当前项目名一致
  - 检查 `MachineID` 是否与目标设备一致
  - 检查是否被判定为时间篡改（删除 TimeGuard 文件会重置，但不建议在生产中使用）
- 已用时长未更新：
  - 确认周期性保存是否开启，间隔是否过长
  - 确认是否处于有效会话（需要先 `ValidateLicense()`）
- 读取配置失败：
  - 确认 `DefaultGame.ini` 中已添加 `[TrialLicensePlugin]` 段
  - 确认打包设置是否将 ini 合并入构建

---

## 已知限制

- 仅依赖本地系统时间的防篡改具有限度，可结合在线时间源增强。
- 加密强度取决于口令质量，建议强口令并避免泄露。
- 更换关键硬件或系统环境可能影响 MachineID；请做好授权迁移流程。

---

## 测试清单（建议）

- 生成/解析/验证各类证书（普通/永久、不同项目/设备）。
- 剩余时间与已用时长的边界值（到期边缘、零/负、跨会话累计）。
- 周期性保存与退出保存是否正确追加增量且无重复计数。
- PIE 结束、引擎 PreExit、模块卸载触发保存与守护点记录。
- 时间回拨被识别并拒绝验证。
- 配置项读取与默认回退是否符合预期。

---
