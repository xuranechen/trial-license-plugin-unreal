# 许可证授权Web服务

基于FastAPI的许可证生成和管理服务，与UE插件加密算法保持完全一致。

## 功能特性

- ✅ 许可证生成（试用版/永久版）
- ✅ 许可证验证
- ✅ 应用管理
- ✅ 用户管理
- ✅ 在线验证API
- ✅ RESTful API接口
- ✅ 自动API文档（Swagger UI）
- ✅ 数据库持久化

## 技术栈

- **Python**: 3.9+
- **FastAPI**: Web框架
- **SQLAlchemy**: ORM
- **SQLite**: 数据库（默认，可切换PostgreSQL）
- **Cryptography**: 加密库
- **Uvicorn**: ASGI服务器

## 快速开始

### 1. 安装依赖

```bash
cd Webservice
pip install -r requirements.txt
```

### 2. 配置环境变量

复制 `.env.example` 为 `.env` 并配置：

```bash
cp .env.example .env
```

### 3. 初始化数据库

```bash
# 使用初始化脚本（自动创建SQLite数据库）
python scripts/init_db.py
```

### 4. 创建初始应用

```bash
python scripts/create_app.py
```

### 5. 启动服务

```bash
# 开发模式（自动创建数据库文件）
python run.py

# 或使用uvicorn直接启动
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

**注意**: 首次启动会自动创建 `license.db` SQLite数据库文件。

### 6. 访问API文档

- Swagger UI: http://localhost:8000/docs
- ReDoc: http://localhost:8000/redoc

## API接口

### 许可证管理

- `POST /api/v1/license/request-trial` - 申请试用许可证
- `POST /api/v1/license/purchase` - 购买正式许可证
- `POST /api/v1/license/validate` - 在线验证许可证
- `GET /api/v1/license/{license_id}` - 查询许可证详情

### 应用管理

- `GET /api/v1/admin/applications` - 查询所有应用
- `POST /api/v1/admin/applications` - 创建应用
- `PUT /api/v1/admin/applications/{app_id}` - 更新应用
- `DELETE /api/v1/admin/applications/{app_id}` - 删除应用

### 统计信息

- `GET /api/v1/admin/statistics` - 获取统计数据

## 数据库配置

### 默认：SQLite（推荐用于开发和中小规模部署）

无需额外配置，开箱即用：

```ini
DATABASE_URL=sqlite:///./license.db
```

**优点**：
- ✅ 零配置，自动创建数据库文件
- ✅ 无需安装数据库服务器
- ✅ 适合单机部署
- ✅ 性能足够应对中小规模应用

### 可选：PostgreSQL（大规模生产环境）

在 `.env` 文件中修改：

```ini
DATABASE_URL=postgresql://user:password@localhost:5432/license_db
```

**切换步骤**：
1. 安装PostgreSQL服务器
2. 创建数据库和用户
3. 修改 `.env` 中的 `DATABASE_URL`
4. 运行 `python scripts/init_db.py` 初始化表结构

## Docker部署（可选，包含PostgreSQL）

```bash
# 启动服务（PostgreSQL + API）
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down
```

**注意**: Docker部署默认使用PostgreSQL数据库。

## 开发指南

### 项目结构

```
Webservice/
├── app/                    # 应用主目录
│   ├── api/               # API路由
│   ├── models/            # 数据库模型
│   ├── schemas/           # Pydantic模型
│   ├── services/          # 业务逻辑
│   ├── middleware/        # 中间件
│   └── utils/             # 工具函数
├── alembic/               # 数据库迁移
├── scripts/               # 脚本工具
├── tests/                 # 测试
└── docker-compose.yml     # Docker配置
```

### 测试

```bash
# 运行所有测试
pytest

# 运行单个测试
pytest tests/test_encryption.py

# 查看覆盖率
pytest --cov=app tests/
```

## 许可证

MIT License
