"""
应用配置管理
"""

from pydantic_settings import BaseSettings
from typing import List


class Settings(BaseSettings):
    """应用配置类"""

    # 应用信息
    APP_NAME: str = "License Authorization Service"
    APP_VERSION: str = "1.0.0"
    DEBUG: bool = True

    # 数据库配置（默认SQLite，可切换为PostgreSQL）
    DATABASE_URL: str = "sqlite:///./license.db"

    # 服务器配置
    HOST: str = "0.0.0.0"
    PORT: int = 8000

    # 安全配置
    SECRET_KEY: str = "your-secret-key-change-this-in-production"
    JWT_SECRET_KEY: str = "your-jwt-secret-key-change-this"
    JWT_ALGORITHM: str = "HS256"
    JWT_EXPIRE_MINUTES: int = 30

    # 主加密密钥（用于加密应用的PassPhrase）
    MASTER_ENCRYPTION_KEY: str = "your-master-key-change-this-in-production"

    # CORS配置
    CORS_ORIGINS: List[str] = ["http://localhost:3000", "http://localhost:8080"]

    # 邮件配置
    SMTP_SERVER: str = "smtp.gmail.com"
    SMTP_PORT: int = 587
    SMTP_USERNAME: str = ""
    SMTP_PASSWORD: str = ""
    FROM_EMAIL: str = ""
    FROM_NAME: str = "License Service"

    # Stripe支付配置
    STRIPE_SECRET_KEY: str = ""
    STRIPE_PUBLISHABLE_KEY: str = ""
    # Stripe 异步回调签名密钥（Dashboard -> Webhooks -> whsec_...）
    STRIPE_WEBHOOK_SECRET: str = ""

    # 支付宝支付配置
    ALIPAY_APP_ID: str = ""
    ALIPAY_PRIVATE_KEY: str = ""
    ALIPAY_PUBLIC_KEY: str = ""

    # 微信支付配置
    WECHAT_APP_ID: str = ""
    WECHAT_MCH_ID: str = ""
    WECHAT_API_KEY: str = ""

    # 日志配置
    LOG_LEVEL: str = "INFO"

    # 许可证到期提醒任务
    LICENSE_REMINDER_ENABLED: bool = True
    LICENSE_REMINDER_DAILY_HOUR: int = 9
    LICENSE_REMINDER_DAILY_MINUTE: int = 0

    class Config:
        env_file = ".env"
        case_sensitive = True


# 创建全局配置实例
settings = Settings()
