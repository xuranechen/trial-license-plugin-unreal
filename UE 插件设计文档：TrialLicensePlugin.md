# UE 插件设计文档：TrialLicensePlugin
（基于UE5子系统架构的试用授权管理系统）

---

## 一、设计目标

本插件基于UE5子系统架构，为 Unreal Engine 项目提供一种 **安全、灵活且可控** 的试用授权机制，支持通过密钥管理试用期、检测系统时间篡改、绑定设备、记录使用时长、显示试用水印。插件支持 **纯蓝图调用** 与 **C++ 扩展**，无需外部依赖。

核心目标：

- ✅ 基于UGameInstanceSubsystem的现代化架构
- ✅ 生成与解析加密授权字符串（Trial Key）
- ✅ 支持设置试用时长、到期日期、生成时间、有效期、密钥口令等参数
- ✅ 检测系统时间篡改
- ✅ 记录累计使用时长（跨会话）
- ✅ 绑定设备（MachineID）和项目（ProjectID）
- ✅ 支持 AES + SHA256 内置加密（无外部库）
- ✅ 支持蓝图调用生成、验证与状态读取
- ✅ 数据本地加密存储（JSON + AES）
- ✅ 内置试用水印显示功能
- ✅ 许可证过期提醒对话框

---

## 二、总体设计思路

### 1. 架构设计

插件采用UE5子系统架构，主要组件：

- **UTrialLicenseSubsystem**：核心子系统，继承自UGameInstanceSubsystem，管理所有功能（包括许可证管理和水印控制）
- **FTrialLicenseInfo**：许可证信息数据结构
- **加密模块**：LicenseEncryptor、MachineIDHelper、TimeGuard、TrialDataStorage
- **UI组件**：STrialWatermarkWidget、TrialWatermarkOverlay、过期对话框

### 2. 核心流程

```plaintext
[生成阶段]
输入参数（时长 / 有效期 / 密钥口令 / 项目ID / 设备ID / 永久授权）
   ↓
生成加密数据体（包含时间戳、签名、绑定信息）
   ↓
使用 FAES + SHA256 进行加密
   ↓
输出字符串型 LicenseKey

[验证阶段]
解析输入 LicenseKey
   ↓
解密并校验签名、项目ID、设备ID、一致性
   ↓
检查系统时间与记录文件
   ↓
计算剩余时间 / 是否过期
   ↓
返回验证结果（成功 / 过期 / 时间被篡改 / 设备不符 / 项目不符）

[运行时监控]
定时更新使用时长 → 加密写入本地 JSON
检测时间异常 → 标记密钥失效
许可证状态变化 → 自动控制水印显示

[水印显示]
许可证验证失败 → 显示水印
许可证验证成功 → 隐藏水印
支持自定义样式和布局
```

## 三、技术方案

### 1. 加密算法

- **AES (FAES)**：用于数据加密存储与 LicenseKey 生成。
- **SHA256**：用于口令生成签名与完整性校验。
- 口令密钥作为加解密种子，可配置或固定在代码中。

### 2. MachineID 设备绑定

- 使用系统硬件特征生成唯一 MachineID：
  - Windows 示例：
    - CPU 序列号（WMI: `Win32_Processor`）
    - 主板序列号（WMI: `Win32_BaseBoard`）
    - 硬盘序列号（`Win32_PhysicalMedia`）
  - 使用 SHA256(拼接字符串) → 固定长度 ID
- 提供 Blueprint 节点：
  - `GetMachineID()`
  - `IsLicenseMatchedDevice()`

### 3. 时间防篡改检测

- 在本地保存：
  - 上次退出时间（加密）
  - 当前系统时间（解密验证）
- 检测逻辑：
  - 若系统时间早于上次退出时间 → 判定“系统时间异常”
  - 若异常超过阈值 → 锁定 LicenseKey
- 可选：定期从在线 NTP 校时或服务器时间同步（预留接口）

### 4. 使用时长记录

- 本地文件（加密 JSON）记录：

  ```json
  {
    "LastSessionEnd": "2025-11-12T10:00:00Z",
    "UsedSeconds": 3600,
    "LicenseKeyHash": "xxxxx",
    "MachineID": "xxxxx"
  }
  ```

- 每次启动时：

  - 校验 LicenseKeyHash 与当前一致
  - 若不一致 → 重置计时
  - 每隔固定时间自动更新已用时长

- 文件加密方式：FAES + Base64

### 5. 子系统架构设计

- **UTrialLicenseSubsystem**：继承自UGameInstanceSubsystem的核心管理类
  - 自动生命周期管理（Initialize/Deinitialize）
  - 状态封装（成员变量替代全局静态变量）
  - 线程安全的会话管理
  - 集成所有许可证相关功能

- **兼容层设计**：
  - 保留UTrialLicenseLibrary作为向后兼容层
  - 内部调用子系统实例实现功能
  - 保持现有API不变

### 6. 水印显示系统

- **UTrialWatermarkOverlay**：水印显示的主要Actor类
  - 自动添加到视口
  - 支持平铺显示
  - 响应许可证状态变化

- **STrialWatermarkWidget**：UMG水印组件
  - Slate实现的高性能渲染
  - 支持旋转、透明度、字体大小自定义
  - 平铺布局算法

- **水印控制逻辑**：
  - 许可证验证成功 → 自动隐藏水印
  - 许可证验证失败 → 自动显示水印
  - 支持手动控制显示状态
  - 项目设置集成

### 7. 过期提醒系统

- **STrialExpiredWidget**：过期提醒对话框
  - 显示过期原因和剩余时间
  - 支持永久许可证状态
  - 用户友好的界面设计

### 8. 蓝图功能接口（UFUNCTION）

#### 许可证管理接口
| 函数名 | 说明 |
|--------|------|
| `GenerateLicenseKey(DurationHours, ExpiryDate, PassPhrase, ProjectId, TargetMachineID, bPerpetual)` | 生成加密密钥字符串（支持项目和设备绑定） |
| `ParseLicenseKey(LicenseKey, PassPhrase)` | 解密并验证密钥内容 |
| `ValidateLicense(LicenseKey, PassPhrase)` | 校验密钥有效性（项目、设备、时间、签名） |
| `GetTrialRemainingTime(LicenseKey, PassPhrase)` | 获取剩余可用时长 |
| `GetMachineID()` | 返回设备唯一标识符 |
| `GetUsedTime()` | 返回累计使用时间 |
| `ResetTrialData()` | 清空本地试用数据 |
| `HasSystemTimeTampered()` | 检测是否存在系统时间篡改 |
| `GenerateTrialRequest()` | 生成试用请求字符串 |
| `ParseTrialRequest()` | 解析试用请求 |
| `EndTrialSessionAndPersist()` | 结束会话并保存数据 |

#### 水印控制接口
| 函数名 | 说明 |
|--------|------|
| `SetWatermarkEnabled(bEnabled)` | 启用/禁用水印显示 |
| `GetWatermarkEnabled()` | 获取水印启用状态 |
| `SetWatermarkText(Text)` | 设置水印文本内容 |
| `GetWatermarkText()` | 获取水印文本内容 |
| `SetWatermarkAngle(Angle)` | 设置水印旋转角度 |
| `GetWatermarkAngle()` | 获取水印旋转角度 |
| `SetWatermarkOpacity(Opacity)` | 设置水印透明度 |
| `GetWatermarkOpacity()` | 获取水印透明度 |
| `SetWatermarkFontSize(Size)` | 设置水印字体大小 |
| `GetWatermarkFontSize()` | 获取水印字体大小 |
| `SetWatermarkTileSpacing(Spacing)` | 设置水印平铺间距（统一） |
| `SetWatermarkTileSpacingXY(X, Y)` | 设置水印平铺间距（分别） |
| `GetWatermarkTileSpacing()` | 获取水印统一间距 |
| `GetWatermarkTileSpacingX()` | 获取水印X方向间距 |
| `GetWatermarkTileSpacingY()` | 获取水印Y方向间距 |
| `ApplyWatermarkFromProjectSettings()` | 从项目设置应用水印配置 |

#### 对话框接口
| 函数名 | 说明 |
|--------|------|
| `ShowExpiredDialog(Reason, RemainingTime, bIsPerpetual)` | 显示过期提醒对话框 |

------

## 四、实现路径

### 阶段一：基础结构

- 创建插件模块：`TrialLicensePlugin`
- 新增 `UTrialLicenseLibrary`（BlueprintFunctionLibrary）
- 实现：
  - AES + SHA256 加密封装
  - MachineID 生成模块
  - License 数据结构定义（FTrialLicenseInfo）

### 阶段二：加密逻辑实现

- 实现 LicenseKey 生成与解析

- 设计加密数据结构体（JSON → AES → Base64）

  ```json
  {
    "DurationHours": 72,
    "GenerateTime": "2025-11-12T09:00:00Z",
    "ExpireTime": "2025-11-15T09:00:00Z",
    "MachineID": "xxxxx",
    "Signature": "SHA256(PassPhrase + MachineID + Time)"
  }
  ```

### 阶段三：防篡改与使用记录

- 实现系统时间记录与对比机制
- 加密保存 `TrialData.json`
- 持续更新使用时长（Tick 或 Timer）

### 阶段四：蓝图接口与测试

- 添加全部 Blueprint 可调用函数
- 在编辑器中测试生成/验证流程
- 测试异常情况：
  - 不同设备
  - 时间倒退
  - 密钥错误
  - 超期使用

------

## 五、安全注意事项

1. **口令密钥**
   - 不应硬编码在公开源码中，建议通过编译宏或外部配置提供。
2. **时间篡改检测**
   - 仅依赖系统时间不可完全防御，可考虑混合：
     - 文件创建时间校验
     - 多次取平均值校验
3. **JSON 加密存储**
   - 加密 JSON 文件本身安全性取决于 AES 口令强度。
   - 建议结合 SHA256 校验完整性。
4. **设备绑定**
   - MachineID 尽量使用稳定硬件特征，避免用户更换硬盘导致授权失效。
5. **反调试保护（可选）**
   - 插件可添加简单反调试检测或加壳以防逆向。

------

## 六、扩展方向（未来版本）

- 🔐 支持 LicenseKey 在线验证（REST / SignalR）
- 🌐 支持 NTP 时间同步校验
- 📦 支持 License 文件导入（.lic 格式）
- ⚙️ 支持多级授权（Trial / Standard / Enterprise）
- 🧩 支持内置序列号黑名单系统

------

## 七、目录结构

```
TrialLicensePlugin/
├── Source/
│   ├── TrialLicensePlugin/
│   │   ├── Public/
│   │   │   ├── FTrialLicenseInfo.h                    # 许可证信息结构体
│   │   │   ├── LicenseEncryptor.h                     # 加密工具类
│   │   │   ├── MachineIDHelper.h                      # 设备ID生成工具
│   │   │   ├── STrialExpiredWidget.h                  # 过期对话框组件
│   │   │   ├── STrialWatermarkWidget.h                # 水印组件
│   │   │   ├── TimeGuard.h                            # 时间守护工具
│   │   │   ├── TrialDataStorage.h                     # 数据存储工具
│   │   │   ├── TrialExpiredWidgetBase.h               # 过期对话框基类
│   │   │   ├── TrialLicensePlugin.h                   # 插件模块头文件
│   │   │   ├── TrialLicenseSettings.h                 # 项目设置类
│   │   │   ├── TrialLicenseSettingsCustomization.h    # 设置自定义UI
│   │   │   ├── TrialWatermarkOverlay.h                # 水印覆盖层
│   │   │   ├── UTrialLicenseSubsystem.h               # 核心子系统
│   │   └── Private/
│   │       ├── LicenseEncryptor.cpp                   # 加密实现
│   │       ├── MachineIDHelper.cpp                    # 设备ID实现
│   │       ├── STrialExpiredWidget.cpp                # 过期对话框实现
│   │       ├── STrialWatermarkWidget.cpp              # 水印组件实现
│   │       ├── TimeGuard.cpp                          # 时间守护实现
│   │       ├── TrialDataStorage.cpp                   # 数据存储实现
│   │       ├── TrialExpiredWidgetBase.cpp             # 过期对话框基类实现
│   │       ├── TrialLicensePlugin.cpp                 # 插件模块实现
│   │       ├── TrialLicenseSettings.cpp               # 项目设置实现
│   │       ├── TrialLicenseSettingsCustomization.cpp  # 设置UI实现
│   │       ├── TrialWatermarkOverlay.cpp              # 水印覆盖层实现
│   │       └── UTrialLicenseSubsystem.cpp             # 子系统实现
├── Content/
│   ├── SourceHanSansSC-Medium-2.otf                    # 水印字体文件
├── Binaries/                                           # 编译输出目录
├── Intermediate/                                       # 中间文件目录
├── TrialLicensePlugin.uplugin                          # 插件描述文件
├── README.md                                           # 使用说明
├── UE 插件设计文档：TrialLicensePlugin.md              # 设计文档
├── UE 插件设计文档：Subsystem迁移指南.md               # 迁移指南
├── LICENSE                                             # 许可证文件
├── .gitignore                                          # Git忽略文件
└── .gitattributes                                      # Git属性文件
```

------

## 八、总结

本插件设计通过 UE 内置加密与 JSON 存储机制，构建一个轻量但可靠的试用授权系统。
 它既能离线工作，又可扩展为联网验证版本，满足多数商业 UE 项目的安全需求。
