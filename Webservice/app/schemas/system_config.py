"""
系统配置 Schema
"""

from pydantic import BaseModel, Field


class SystemConfigBase(BaseModel):
    smtp_enabled: bool = False
    smtp_server: str = Field(default="smtp.gmail.com")
    smtp_port: int = Field(default=587, ge=1)
    smtp_username: str = ""
    smtp_password: str = ""
    smtp_use_tls: bool = True
    from_email: str = ""
    from_name: str = "License Service"

    stripe_enabled: bool = False
    stripe_secret_key: str = ""
    stripe_publishable_key: str = ""

    alipay_enabled: bool = False
    alipay_app_id: str = ""
    alipay_private_key: str = ""
    alipay_public_key: str = ""

    wechat_enabled: bool = False
    wechat_app_id: str = ""
    wechat_mch_id: str = ""
    wechat_api_key: str = ""


class SystemConfigResponse(SystemConfigBase):
    id: int

    class Config:
        from_attributes = True
