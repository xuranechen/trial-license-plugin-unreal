# 许可证管理系统 API 文档

## 📚 目录

- [概述](#概述)
- [认证系统](#认证系统)
- [订单和支付](#订单和支付)
- [统计分析](#统计分析)

---

## 概述

本系统提供完整的许可证管理功能，包括：

- ✅ **用户认证系统（JWT）**
- ✅ **支付集成（Stripe/支付宝/微信）**
- ✅ **邮件通知**
- ✅ **许可证统计和分析**
- ✅ **批量许可证生成**

### 基础信息

- **Base URL**: `http://localhost:8000/api/v1`
- **认证方式**: Bearer Token (JWT)
- **Content-Type**: `application/json`

---

## 认证系统

### 1. 用户注册

```http
POST /auth/register
```

**请求体：**
```json
{
  "username": "testuser",
  "email": "test@example.com",
  "password": "password123",
  "full_name": "测试用户",
  "company": "测试公司"
}
```

**响应：**
```json
{
  "id": "uuid",
  "username": "testuser",
  "email": "test@example.com",
  "full_name": "测试用户",
  "company": "测试公司",
  "is_active": true,
  "is_admin": false
}
```

### 2. 用户登录

```http
POST /auth/login
```

**请求体：**
```json
{
  "username": "testuser",
  "password": "password123"
}
```

**响应：**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "token_type": "bearer",
  "expires_in": 86400
}
```

### 3. 获取当前用户信息

```http
GET /auth/me
Authorization: Bearer {token}
```

### 4. 修改密码

```http
POST /auth/change-password
Authorization: Bearer {token}
```

**请求体：**
```json
{
  "old_password": "oldpass123",
  "new_password": "newpass123"
}
```

---

## 订单和支付

### 1. 创建订单

```http
POST /orders
Authorization: Bearer {token}
```

**请求体：**
```json
{
  "application_id": "app-uuid",
  "product_name": "专业版许可证",
  "license_type": "permanent",
  "license_duration": null,
  "amount": 99.00,
  "currency": "CNY",
  "quantity": 1
}
```

**响应：**
```json
{
  "id": "order-uuid",
  "order_no": "ORD20240101120000abcd1234",
  "amount": 99.00,
  "currency": "CNY",
  "status": "pending",
  "created_at": "2024-01-01T12:00:00Z"
}
```

### 2. 获取订单列表

```http
GET /orders?skip=0&limit=100
Authorization: Bearer {token}
```

### 3. 创建支付

```http
POST /orders/{order_id}/pay
Authorization: Bearer {token}
```

**请求体：**
```json
{
  "order_id": "order-uuid",
  "payment_method": "stripe",
  "return_url": "https://yoursite.com/payment/return",
  "notify_url": "https://yoursite.com/payment/notify"
}
```

**支付方式选项：**
- `stripe` - Stripe支付
- `alipay` - 支付宝
- `wechat` - 微信支付
- `bank_transfer` - 银行转账

**响应（Stripe）：**
```json
{
  "id": "payment-uuid",
  "payment_no": "PAY20240101120000abcd1234",
  "payment_method": "stripe",
  "amount": 99.00,
  "currency": "CNY",
  "status": "pending",
  "payment_url": "pi_xxx_secret_yyy",
  "transaction_id": "pi_xxx",
  "created_at": "2024-01-01T12:00:00Z"
}
```

**响应（支付宝）：**
```json
{
  "id": "payment-uuid",
  "payment_no": "PAY20240101120000abcd1234",
  "payment_method": "alipay",
  "amount": 99.00,
  "payment_url": "https://openapi.alipay.com/gateway.do?...",
  "created_at": "2024-01-01T12:00:00Z"
}
```

**响应（微信）：**
```json
{
  "id": "payment-uuid",
  "payment_no": "PAY20240101120000abcd1234",
  "payment_method": "wechat",
  "amount": 99.00,
  "qr_code": "weixin://wxpay/bizpayurl?pr=xxx",
  "created_at": "2024-01-01T12:00:00Z"
}
```

### 4. 完成订单（管理员）

```http
POST /orders/{order_id}/complete
Authorization: Bearer {admin_token}
```

---

## 统计分析

### 1. 概览统计（管理员）

```http
GET /statistics/overview
Authorization: Bearer {admin_token}
```

**响应：**
```json
{
  "users": {
    "total": 1000,
    "active": 850
  },
  "licenses": {
    "total": 5000,
    "activated": 4200,
    "trial": 1500,
    "permanent": 3500
  },
  "orders": {
    "total": 3000,
    "paid": 2800,
    "pending": 200
  },
  "revenue": {
    "total": 150000.00,
    "today": 5000.00
  },
  "today": {
    "orders": 50,
    "revenue": 5000.00
  }
}
```

### 2. 收入统计

```http
GET /statistics/revenue?days=30
Authorization: Bearer {admin_token}
```

### 3. 许可证统计

```http
GET /statistics/licenses
Authorization: Bearer {admin_token}
```

### 4. 用户统计

```http
GET /statistics/users
Authorization: Bearer {admin_token}
```

### 5. 支付统计

```http
GET /statistics/payments
Authorization: Bearer {admin_token}
```

---

---

## 错误响应

所有API错误都遵循以下格式：

```json
{
  "detail": "错误描述信息"
}
```

**常见HTTP状态码：**

- `200` - 成功
- `201` - 创建成功
- `400` - 请求参数错误
- `401` - 未授权（未登录或token无效）
- `403` - 禁止访问（权限不足）
- `404` - 资源不存在
- `500` - 服务器内部错误

---

## 使用示例

### Python 示例

```python
import requests

# 1. 登录获取token
login_response = requests.post(
    "http://localhost:8000/api/v1/auth/login",
    json={
        "username": "admin",
        "password": "password123"
    }
)
token = login_response.json()["access_token"]

# 2. 创建订单
headers = {"Authorization": f"Bearer {token}"}
order_response = requests.post(
    "http://localhost:8000/api/v1/orders",
    headers=headers,
    json={
        "application_id": "app-uuid",
        "product_name": "专业版许可证",
        "license_type": "permanent",
        "amount": 99.00,
        "currency": "CNY",
        "quantity": 1
    }
)
order_id = order_response.json()["id"]

# 3. 创建支付
payment_response = requests.post(
    f"http://localhost:8000/api/v1/orders/{order_id}/pay",
    headers=headers,
    json={
        "order_id": order_id,
        "payment_method": "stripe"
    }
)
print(payment_response.json())
```

### JavaScript 示例

```javascript
// 1. 登录获取token
const loginResponse = await fetch('http://localhost:8000/api/v1/auth/login', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    username: 'admin',
    password: 'password123'
  })
});
const { access_token } = await loginResponse.json();

// 2. 获取统计数据
const statsResponse = await fetch('http://localhost:8000/api/v1/statistics/overview', {
  headers: { 'Authorization': `Bearer ${access_token}` }
});
const stats = await statsResponse.json();
console.log(stats);
```

### cURL 示例

```bash
# 1. 注册用户
curl -X POST http://localhost:8000/api/v1/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "testuser",
    "email": "test@example.com",
    "password": "password123"
  }'

# 2. 登录
curl -X POST http://localhost:8000/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "testuser",
    "password": "password123"
  }'

# 3. 获取订单列表（需要替换TOKEN）
curl -X GET http://localhost:8000/api/v1/orders \
  -H "Authorization: Bearer YOUR_TOKEN_HERE"
```

---

## 邮件通知

系统会自动发送以下邮件通知：

1. **许可证激活成功** - 用户激活许可证后
2. **许可证到期提醒** - 临近到期的许可证自动提醒
3. **支付成功通知** - 支付完成后

---

## 注意事项

1. **安全性**
   - 生产环境必须使用HTTPS
   - 定期更换JWT密钥
   - 妥善保管支付配置密钥

2. **性能优化**
   - 建议使用PostgreSQL作为生产数据库
   - 添加Redis缓存提升性能
   - 使用CDN加速静态资源

3. **备份**
   - 定期备份数据库
   - 保存支付回调日志

4. **监控**
   - 监控API响应时间
   - 监控支付成功率
   - 监控错误日志

---

## 技术支持

如有问题，请访问：
- API文档：http://localhost:8000/docs
- ReDoc文档：http://localhost:8000/redoc
- GitHub Issues：https://github.com/your-repo/issues
