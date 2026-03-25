"""
系统配置服务
"""

from app.config import settings
from app.database import SessionLocal
from app.models.system_config import SystemConfig


def _defaults() -> dict:
    return {
        "smtp_enabled": False,
        "smtp_server": settings.SMTP_SERVER,
        "smtp_port": settings.SMTP_PORT,
        "smtp_username": settings.SMTP_USERNAME,
        "smtp_password": settings.SMTP_PASSWORD,
        "smtp_use_tls": True,
        "from_email": settings.FROM_EMAIL,
        "from_name": settings.FROM_NAME,
        "stripe_enabled": False,
        "stripe_secret_key": settings.STRIPE_SECRET_KEY,
        "stripe_publishable_key": settings.STRIPE_PUBLISHABLE_KEY,
        "alipay_enabled": False,
        "alipay_app_id": settings.ALIPAY_APP_ID,
        "alipay_private_key": settings.ALIPAY_PRIVATE_KEY,
        "alipay_public_key": settings.ALIPAY_PUBLIC_KEY,
        "wechat_enabled": False,
        "wechat_app_id": settings.WECHAT_APP_ID,
        "wechat_mch_id": settings.WECHAT_MCH_ID,
        "wechat_api_key": settings.WECHAT_API_KEY,
    }


def get_or_create_system_config(db):
    config = db.query(SystemConfig).first()
    if config:
        return config

    config = SystemConfig(**_defaults())
    db.add(config)
    db.commit()
    db.refresh(config)
    return config


def get_runtime_system_config() -> dict:
    db = SessionLocal()
    try:
        config = get_or_create_system_config(db)
        return {
            "smtp_enabled": bool(config.smtp_enabled),
            "smtp_server": config.smtp_server or settings.SMTP_SERVER,
            "smtp_port": int(config.smtp_port or settings.SMTP_PORT),
            "smtp_username": config.smtp_username or "",
            "smtp_password": config.smtp_password or "",
            "smtp_use_tls": bool(config.smtp_use_tls),
            "from_email": config.from_email
            or config.smtp_username
            or settings.FROM_EMAIL,
            "from_name": config.from_name or settings.FROM_NAME,
            "stripe_enabled": bool(config.stripe_enabled),
            "stripe_secret_key": config.stripe_secret_key or "",
            "stripe_publishable_key": config.stripe_publishable_key or "",
            "alipay_enabled": bool(config.alipay_enabled),
            "alipay_app_id": config.alipay_app_id or "",
            "alipay_private_key": config.alipay_private_key or "",
            "alipay_public_key": config.alipay_public_key or "",
            "wechat_enabled": bool(config.wechat_enabled),
            "wechat_app_id": config.wechat_app_id or "",
            "wechat_mch_id": config.wechat_mch_id or "",
            "wechat_api_key": config.wechat_api_key or "",
        }
    finally:
        db.close()
