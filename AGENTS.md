# TrialLicensePlugin - 项目上下文文档

## 项目概述

TrialLicensePlugin 是一个完整的试用授权管理系统，包含两个主要部分：

1. **Unreal Engine 5 插件** - 嵌入到 UE 项目中的客户端授权管理组件
2. **Python/FastAPI 后端服务** - 许可证生成、管理和在线验证的 Web 服务

该系统为 Unreal Engine 项目提供安全、灵活的试用授权机制，支持设备绑定、项目绑定、时间防篡改、使用时长记录、试用水印显示等功能。

### 核心特性

- ✅ 基于 UE5 子系统架构（UGameInstanceSubsystem）
- ✅ AES-256-CBC + SHA256 加密
- ✅ 设备绑定（MachineID）
- ✅ 项目绑定（ProjectId）
- ✅ 时间防篡改检测
- ✅ 本地加密存储
- ✅ 试用水印显示
- ✅ 过期提醒对话框
- ✅ 永久授权支持
- ✅ 在线验证 API
- ✅ 支付集成（Stripe/支付宝/微信）
- ✅ 订阅制许可证管理

---

## 项目结构

```
trial-license-plugin-unreal/
├── Source/TrialLicensePlugin/          # UE 插件源代码
│   ├── Public/                         # 公共头文件
│   │   ├── FTrialLicenseInfo.h        # 许可证信息结构体
│   │   ├── LicenseEncryptor.h         # 加密工具类
│   │   ├── MachineIDHelper.h          # 设备 ID 生成工具
│   │   ├── TimeGuard.h                # 时间防篡改工具
│   │   ├── TrialDataStorage.h         # 数据存储工具
│   │   ├── UTrialLicenseSubsystem.h   # 核心子系统（主要 API）
│   │   ├── STrialWatermarkWidget.h    # 水印组件
│   │   ├── TrialWatermarkOverlay.h    # 水印覆盖层
│   │   ├── STrialExpiredWidget.h      # 过期对话框
│   │   ├── TrialLicenseSettings.h     # 项目设置类
│   │   └── TrialLicensePlugin.h       # 插件模块头文件
│   └── Private/                        # 私有实现文件
│       ├── LicenseEncryptor.cpp       # 加密实现
│       ├── MachineIDHelper.cpp        # 设备 ID 实现
│       ├── TimeGuard.cpp              # 时间防篡改实现
│       ├── TrialDataStorage.cpp       # 数据存储实现
│       ├── UTrialLicenseSubsystem.cpp # 子系统实现
│       ├── STrialWatermarkWidget.cpp  # 水印组件实现
│       ├── TrialWatermarkOverlay.cpp  # 水印覆盖层实现
│       ├── STrialExpiredWidget.cpp    # 过期对话框实现
│       ├── TrialLicenseSettings.cpp   # 项目设置实现
│       └── TrialLicensePlugin.cpp     # 插件模块实现
│
├── Content/                            # UE 内容资源
│   ├── SourceHanSansSC-Medium-2.otf   # 水印字体文件
│   └── WBP_TrialLicenseMain.uasset    # 主 UI 蓝图
│
├── Webservice/                         # Python 后端服务
│   ├── app/
│   │   ├── main.py                    # FastAPI 主应用
│   │   ├── config.py                  # 配置管理
│   │   ├── database.py                # 数据库连接
│   │   ├── api/v1/                    # API 路由
│   │   │   ├── auth.py                # 认证 API
│   │   │   ├── license.py             # 许可证 API
│   │   │   ├── orders.py              # 订单 API
│   │   │   ├── subscriptions.py       # 订阅 API
│   │   │   ├── statistics.py          # 统计 API
│   │   │   └── admin.py               # 管理员 API
│   │   ├── models/                    # 数据库模型
│   │   ├── schemas/                   # Pydantic 模型
│   │   ├── services/                  # 业务逻辑服务
│   │   │   ├── encryption.py          # 加密服务（与 UE 插件兼容）
│   │   │   ├── payment_service.py     # 支付服务
│   │   │   └── email_service.py       # 邮件服务
│   │   └── utils/                     # 工具函数
│   ├── static/                        # 静态文件（Web 管理界面）
│   │   ├── admin.html                 # 管理员界面
│   │   ├── portal.html                # 用户门户
│   │   ├── css/                       # 样式文件
│   │   └── js/                        # JavaScript 文件
│   ├── scripts/                       # 工具脚本
│   │   ├── init_db.py                 # 数据库初始化
│   │   ├── init_admin.py              # 创建管理员
│   │   ├── create_app.py              # 创建应用
│   │   └── update_license_table.py    # 更新许可证表
│   ├── requirements.txt               # Python 依赖
│   ├── run.py                         # 启动脚本
│   ├── docker-compose.yml             # Docker 配置
│   ├── .env                           # 环境变量
│   └── API_DOCUMENTATION.md           # API 文档
│
├── Resources/                          # 资源文件
│   └── Icon128.png                    # 插件图标
│
├── TrialLicensePlugin.uplugin         # 插件描述文件
├── TrialLicensePlugin.Build.cs        # 构建配置
├── README.md                          # 插件使用说明
├── UE 插件设计文档：TrialLicensePlugin.md  # 插件设计文档
└── LICENSE                            # 许可证文件
```

---

## 技术栈

### Unreal Engine 插件

- **引擎版本**: Unreal Engine 4.27+（推荐 5.x）
- **编程语言**: C++
- **核心依赖**:
  - Core, CoreUObject
  - Engine
  - UMG（UI 组件）
  - Slate（渲染）
  - Json, JsonUtilities
  - InputCore, ApplicationCore
- **加密**: FAES（UE 内置 AES）+ SHA256
- **架构**: UGameInstanceSubsystem（UE5 子系统）

### Python 后端服务

- **语言**: Python 3.9+
- **Web 框架**: FastAPI 0.104.1
- **ASGI 服务器**: Uvicorn
- **数据库**: SQLAlchemy 2.0 + SQLite（默认）/ PostgreSQL（可选）
- **ORM**: SQLAlchemy
- **认证**: JWT (python-jose)
- **加密**: cryptography 41.0.7
- **支付集成**:
  - Stripe 11.2.0
  - 支付宝 SDK Python 3.7.868
  - WeChatPy 1.8.18
- **邮件**: FastAPI-Mail
- **数据处理**: Pandas, OpenPyXL
- **测试**: Pytest, httpx

---

## 构建和运行

### Unreal Engine 插件

#### 安装插件

1. 将 `TrialLicensePlugin` 文件夹复制到 UE 项目的 `Plugins` 目录
2. 重启 Unreal Engine 编辑器
3. 在 `Edit > Plugins` 中启用 "Trial License Plugin"
4. 重启编辑器

#### 配置（推荐）

在项目的 `DefaultGame.ini` 中添加：

```ini
[TrialLicensePlugin]
TrialPeriodicSaveEnabled=true
TrialPeriodicSaveIntervalSeconds=300
TrialDataKey=YourStrongTrialDataKeyHere
TimeGuardKey=YourStrongTimeGuardKeyHere
```

#### 编译插件

如果是 C++ 项目：
1. 在 Visual Studio 中打开 `.uproject` 文件
2. 构建项目（插件会自动编译）
3. 打包时确保插件已启用

#### 使用方法（C++）

```cpp
#include "UTrialLicenseSubsystem.h"

// 获取子系统
UTrialLicenseSubsystem* Subsystem = UGameInstance::GetSubsystem<UTrialLicenseSubsystem>(GetGameInstance());

// 生成许可证
FString LicenseKey = Subsystem->GenerateLicenseKey(
    72.0f,                              // 72 小时
    FDateTime::MaxValue(),              // 永不过期
    FApp::GetProjectName(),             // 当前项目名
    Subsystem->GetMachineID(),          // 本机 MachineID
    false                               // 非永久授权
);

// 验证许可证
bool bValid = Subsystem->ValidateLicense(LicenseKey);

// 获取剩余时间
float RemainingTime = Subsystem->GetTrialRemainingTime(LicenseKey);

// 控制水印
Subsystem->SetWatermarkEnabled(true);
Subsystem->SetWatermarkText(TEXT("试用版"));
```

#### 使用方法（蓝图）

在蓝图中：
1. 使用 `Get Game Instance Subsystem` 节点获取 `Trial License Subsystem`
2. 调用相关节点：
   - `Generate License Key`
   - `Validate License`
   - `Get Trial Remaining Time`
   - `Set Watermark Enabled`
   - `Show Expired Dialog`

### Python 后端服务

#### 安装依赖

```bash
cd Webservice
pip install -r requirements.txt
```

#### 配置环境变量

```bash
cp .env.example .env
# 编辑 .env 文件配置参数
```

#### 初始化数据库

```bash
python scripts/init_db.py
python scripts/create_app.py
```

#### 启动服务

```bash
# 开发模式
python run.py

# 或使用 uvicorn
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

#### 访问文档

- Swagger UI: http://localhost:8000/docs
- ReDoc: http://localhost:8000/redoc
- 管理界面: http://localhost:8000/static/admin.html
- 用户门户: http://localhost:8000/static/portal.html

#### Docker 部署

```bash
# 启动服务（包含 PostgreSQL）
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down
```

---

## 开发规范

### C++ 代码规范

1. **命名约定**:
   - 类名：`U` 或 `A` 前缀（UE 约定）
   - 函数名：PascalCase
   - 变量名：PascalCase（成员变量）
   - 局部变量：camelCase

2. **注释规范**:
   - 公共 API 必须包含 Doxygen 风格注释
   - 复杂逻辑需要内联注释说明

3. **蓝图暴露**:
   - 蓝图可调用的函数使用 `UFUNCTION(BlueprintCallable)` 标记
   - 纯函数使用 `UFUNCTION(BlueprintPure)` 标记
   - Category 使用 "Trial License" 或 "Trial License|Watermark"

4. **错误处理**:
   - 使用 UE 的日志系统（`UE_LOG`）
   - 返回布尔值表示成功/失败
   - 使用可选参数返回详细错误信息

### Python 代码规范

1. **PEP 8**: 遵循 Python 官方代码风格指南
2. **类型提示**: 使用 `typing` 模块添加类型提示
3. **异步编程**: 使用 `async/await` 处理 I/O 操作
4. **依赖注入**: 使用 FastAPI 的依赖注入系统
5. **错误处理**: 使用 HTTPException 返回标准化错误

### Git 工作流

1. **分支命名**:
   - 功能分支: `feature/功能名`
   - 修复分支: `fix/问题描述`
   - 文档分支: `docs/文档内容`

2. **提交消息**:
   - 使用中文提交消息
   - 格式：`类型: 简短描述`
   - 类型：`feat`（新功能）、`fix`（修复）、`docs`（文档）、`refactor`（重构）、`test`（测试）

3. **代码审查**:
   - 所有代码变更需要审查
   - 确保测试通过
   - 更新相关文档

---

## 核心组件说明

### UTrialLicenseSubsystem（核心子系统）

插件的核心管理类，继承自 `UGimeInstanceSubsystem`，负责：
- 许可证生成和验证
- 使用时长记录
- 时间防篡改检测
- 水印显示控制
- 数据持久化

**主要方法**:
- `GenerateLicenseKey()` - 生成许可证密钥
- `ValidateLicense()` - 验证许可证
- `GetTrialRemainingTime()` - 获取剩余时间
- `SetWatermarkEnabled()` - 控制水印
- `ShowExpiredDialog()` - 显示过期对话框

### LicenseEncryptor（加密工具）

处理所有加密和解密操作：
- AES-256-CBC 加密/解密
- SHA256 哈希计算
- 与 Python 后端保持算法一致

### MachineIDHelper（设备标识）

生成设备唯一标识符：
- 组合系统硬件特征
- SHA256 哈希生成固定长度 ID
- 跨平台支持

### TimeGuard（时间守护）

检测系统时间篡改：
- 记录上次会话结束时间
- 检测时间倒退
- 异步保存数据

### TrialDataStorage（数据存储）

管理本地试用数据：
- 加密存储 JSON 文件
- 记录使用时长
- 支持周期性保存

### STrialWatermarkWidget（水印组件）

UMG 水印显示组件：
- 高性能 Slate 渲染
- 支持旋转、透明度、字体大小
- 平铺布局算法

### TrialWatermarkOverlay（水印覆盖层）

水印显示的 Actor 类：
- 自动添加到视口
- 响应许可证状态变化
- 支持自定义样式

---

## API 接口

### 认证 API

- `POST /api/v1/auth/register` - 用户注册
- `POST /api/v1/auth/login` - 用户登录
- `GET /api/v1/auth/me` - 获取当前用户信息
- `POST /api/v1/auth/change-password` - 修改密码

### 许可证 API

- `POST /api/v1/license/request-trial` - 申请试用许可证
- `POST /api/v1/license/purchase` - 购买正式许可证
- `POST /api/v1/license/validate` - 在线验证许可证
- `GET /api/v1/license/{license_id}` - 查询许可证详情

### 订单 API

- `POST /api/v1/orders` - 创建订单
- `GET /api/v1/orders` - 获取订单列表
- `POST /api/v1/orders/{order_id}/pay` - 创建支付
- `POST /api/v1/orders/{order_id}/complete` - 完成订单（管理员）

### 订阅 API

- `GET /api/v1/subscriptions/plans` - 获取订阅计划
- `POST /api/v1/subscriptions` - 创建订阅
- `GET /api/v1/subscriptions` - 获取我的订阅
- `POST /api/v1/subscriptions/{id}/cancel` - 取消订阅

### 统计 API

- `GET /api/v1/statistics/overview` - 概览统计
- `GET /api/v1/statistics/revenue` - 收入统计
- `GET /api/v1/statistics/licenses` - 许可证统计
- `GET /api/v1/statistics/users` - 用户统计

详细 API 文档请参考 `Webservice/API_DOCUMENTATION.md`

---

## 安全注意事项

1. **加密密钥**:
   - 不要硬编码在生产代码中
   - 使用环境变量或配置文件
   - 定期轮换密钥

2. **时间防篡改**:
   - 仅依赖系统时间有限制
   - 可考虑集成在线时间源（NTP）

3. **设备绑定**:
   - MachineID 可能因硬件变化而改变
   - 需要授权迁移流程

4. **数据存储**:
   - 所有数据都经过 AES 加密
   - 定期备份加密数据

5. **API 安全**:
   - 使用 HTTPS（生产环境）
   - JWT token 有效期限制
   - 敏感操作需要二次验证

---

## 测试

### 插件测试

- 生成/解析/验证各类许可证（普通/永久、不同项目/设备）
- 剩余时间与已用时长的边界值测试
- 时间回拨检测测试
- 水印显示和样式测试
- 蓝图接口测试

### 后端测试

```bash
# 运行所有测试
cd Webservice
pytest

# 运行单个测试
pytest tests/test_encryption.py

# 查看覆盖率
pytest --cov=app tests/
```

---

## 故障排查

### 插件常见问题

1. **无法通过校验**:
   - 检查 PassPhrase 是否一致
   - 检查 ProjectId 是否与当前项目名一致
   - 检查 MachineID 是否与目标设备一致
   - 检查是否被判定为时间篡改

2. **已用时长未更新**:
   - 确认周期性保存是否开启
   - 确认是否处于有效会话
   - 检查配置文件设置

3. **水印不显示**:
   - 确认许可证验证失败
   - 检查水印是否已启用
   - 确认水印覆盖层已添加到视口

### 后端常见问题

1. **数据库连接失败**:
   - 检查 DATABASE_URL 配置
   - 确认数据库服务是否运行
   - 检查数据库权限

2. **API 响应慢**:
   - 使用 PostgreSQL 替代 SQLite
   - 添加 Redis 缓存
   - 优化数据库查询

3. **支付回调失败**:
   - 检查支付平台配置
   - 确认回调 URL 可访问
   - 查看服务器日志

---

## 扩展方向

- 🔐 支持更多在线验证方式（REST/SignalR）
- 🌐 集成 NTP 时间同步校验
- 📦 支持许可证文件导入（.lic 格式）
- ⚙️ 支持多级授权（Trial/Standard/Enterprise）
- 🧩 内置序列号黑名单系统
- 📊 更丰富的统计和分析功能
- 🌍 国际化支持（i18n）
- 📱 移动端管理界面

---

## 许可证

MIT License

---

## 联系方式

- GitHub: https://github.com/xuranechen/trial-license-plugin-unreal
- Issues: https://github.com/xuranechen/trial-license-plugin-unreal/issues