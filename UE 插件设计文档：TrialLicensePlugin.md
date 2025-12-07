# UE 插件设计文档：TrialLicensePlugin  
（基于时长、有效期、口令密钥、设备绑定的试用授权系统）

---

## 一、设计目标

本插件旨在为 Unreal Engine 项目提供一种 **安全、灵活且可控** 的试用授权机制，支持通过密钥管理试用期、检测系统时间篡改、绑定设备并记录使用时长。插件支持 **纯蓝图调用** 与 **C++ 扩展**，无需外部依赖。

核心目标：

- ✅ 生成与解析加密授权字符串（Trial Key）  
- ✅ 支持设置试用时长、到期日期、生成时间、有效期、密钥口令等参数  
- ✅ 检测系统时间篡改  
- ✅ 记录累计使用时长（跨会话）  
- ✅ 绑定设备（MachineID）  
- ✅ 支持 AES + SHA256 内置加密（无外部库）  
- ✅ 支持蓝图调用生成、验证与状态读取  
- ✅ 数据本地加密存储（JSON + AES）

---

## 二、总体设计思路

### 1. 核心流程

```plaintext
[生成阶段]
输入参数（时长 / 有效期 / 密钥口令 / 设备ID）
   ↓
生成加密数据体（包含时间戳、签名、绑定信息）
   ↓
使用 FAES + SHA256 进行加密
   ↓
输出字符串型 LicenseKey

[验证阶段]
解析输入 LicenseKey
   ↓
解密并校验签名、设备ID、一致性
   ↓
检查系统时间与记录文件
   ↓
计算剩余时间 / 是否过期
   ↓
返回验证结果（成功 / 过期 / 时间被篡改 / 设备不符）

[运行时监控]
定时更新使用时长 → 加密写入本地 JSON
检测时间异常 → 标记密钥失效
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

### 5. 蓝图功能接口（UFUNCTION）

| 函数名                                                      | 说明                               |
| ----------------------------------------------------------- | ---------------------------------- |
| `GenerateLicenseKey(DurationHours, ExpiryDate, PassPhrase)` | 生成加密密钥字符串                 |
| `ParseLicenseKey(LicenseKey, PassPhrase)`                   | 解密并验证密钥内容                 |
| `GetMachineID()`                                            | 返回设备唯一标识符                 |
| `ValidateLicense(LicenseKey)`                               | 校验密钥有效性（设备、时间、签名） |
| `GetTrialRemainingTime()`                                   | 获取剩余可用时长                   |
| `GetUsedTime()`                                             | 返回累计使用时间                   |
| `ResetTrialData()`                                          | 清空本地试用数据                   |
| `HasSystemTimeTampered()`                                   | 检测是否存在系统时间篡改           |

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

## 七、目录结构建议

```
TrialLicensePlugin/
├── Source/
│   ├── TrialLicensePlugin/
│   │   ├── Public/
│   │   │   ├── UTrialLicenseLibrary.h
│   │   │   ├── FTrialLicenseInfo.h
│   │   └── Private/
│   │       ├── UTrialLicenseLibrary.cpp
│   │       ├── MachineIDHelper.cpp
│   │       ├── TimeGuard.cpp
│   │       ├── LicenseEncryptor.cpp
│   │       └── TrialDataStorage.cpp
└── Resources/
    ├── Icon128.png
    └── LicenseConfig.json (可选)
```

------

## 八、总结

本插件设计通过 UE 内置加密与 JSON 存储机制，构建一个轻量但可靠的试用授权系统。
 它既能离线工作，又可扩展为联网验证版本，满足多数商业 UE 项目的安全需求。