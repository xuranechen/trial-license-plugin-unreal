"""
系统配置模型
"""

from sqlalchemy import Boolean, Column, Integer, String, Text, DateTime
from sqlalchemy.sql import func

from app.database import Base


class SystemConfig(Base):
    """系统配置表 - 存储后台可配置项"""

    __tablename__ = "system_configs"

    id = Column(Integer, primary_key=True, autoincrement=True)

    smtp_enabled = Column(Boolean, default=False, comment="是否启用 SMTP")
    smtp_server = Column(String(255), default="smtp.gmail.com", comment="SMTP 服务器")
    smtp_port = Column(Integer, default=587, comment="SMTP 端口")
    smtp_username = Column(String(255), default="", comment="SMTP 用户名")
    smtp_password = Column(Text, default="", comment="SMTP 密码")
    smtp_use_tls = Column(Boolean, default=True, comment="是否使用 TLS")
    from_email = Column(String(255), default="", comment="发件邮箱")
    from_name = Column(String(255), default="License Service", comment="发件人名称")

    stripe_enabled = Column(Boolean, default=False, comment="是否启用 Stripe")
    stripe_secret_key = Column(Text, default="", comment="Stripe Secret Key")
    stripe_publishable_key = Column(Text, default="", comment="Stripe Publishable Key")

    alipay_enabled = Column(Boolean, default=False, comment="是否启用支付宝")
    alipay_app_id = Column(String(255), default="", comment="支付宝 App ID")
    alipay_private_key = Column(Text, default="", comment="支付宝私钥")
    alipay_public_key = Column(Text, default="", comment="支付宝公钥")

    wechat_enabled = Column(Boolean, default=False, comment="是否启用微信支付")
    wechat_app_id = Column(String(255), default="", comment="微信 App ID")
    wechat_mch_id = Column(String(255), default="", comment="微信商户号")
    wechat_api_key = Column(Text, default="", comment="微信 API Key")

    created_at = Column(
        DateTime(timezone=True), server_default=func.now(), comment="创建时间"
    )
    updated_at = Column(
        DateTime(timezone=True), onupdate=func.now(), comment="更新时间"
    )
