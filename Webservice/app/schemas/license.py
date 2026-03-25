"""
许可证相关的Pydantic schemas
"""

from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime


class LicenseRequest(BaseModel):
    """许可证申请请求"""

    project_id: str = Field(..., description="项目ID")
    encrypted_token: str = Field(..., description="加密口令（包含设备ID等信息）")
    email: Optional[str] = Field(None, description="用户邮箱（可选）")
    user_id: Optional[str] = Field(None, description="用户ID")
    payment_id: Optional[str] = Field(None, description="支付ID（购买时需要）")
    license_type: Optional[str] = Field(
        "trial", description="许可证类型：trial或perpetual"
    )


class TrialAvailabilityResponse(BaseModel):
    """试用申请次数可用性响应"""

    allowed: bool = Field(..., description="是否还能申请试用")
    remaining_trial_requests: int = Field(..., description="剩余试用申请次数")
    trial_request_limit: int = Field(..., description="试用申请次数上限")
    used_trial_requests: int = Field(..., description="已使用试用申请次数")
    message: str = Field(..., description="提示消息")


class LicenseResponse(BaseModel):
    """许可证生成响应"""

    success: bool = Field(..., description="是否成功")
    license_key: str = Field("", description="许可证密钥")
    license_type: str = Field(..., description="许可证类型")
    expiry_date: Optional[datetime] = Field(None, description="过期时间")
    duration_hours: float = Field(0, description="时长（小时）")
    is_perpetual: bool = Field(..., description="是否永久许可证")
    message: str = Field(..., description="消息")
    order_id: Optional[str] = Field(None, description="订单ID（需要支付时返回）")
    order_no: Optional[str] = Field(None, description="订单号")
    amount: Optional[float] = Field(None, description="支付金额")
    payment_url: Optional[str] = Field(None, description="支付页面URL")
    remaining_trial_requests: Optional[int] = Field(
        None, description="剩余试用申请次数"
    )
    trial_request_limit: Optional[int] = Field(None, description="试用申请次数上限")


class ValidateLicenseRequest(BaseModel):
    """许可证验证请求"""

    license_key: str = Field(..., description="许可证密钥")
    project_id: str = Field(..., description="项目ID")
    machine_id: str = Field(..., description="设备ID")


class ValidateLicenseResponse(BaseModel):
    """许可证验证响应"""

    valid: bool = Field(..., description="是否有效")
    remaining_time: Optional[float] = Field(
        None, description="剩余时间（秒），-1表示永久"
    )
    is_perpetual: bool = Field(False, description="是否永久许可证")
    expiry_date: Optional[datetime] = Field(None, description="过期时间")
    message: str = Field(..., description="消息")


class LicenseDetail(BaseModel):
    """许可证详情"""

    id: int
    license_key: str
    project_id: str
    machine_id: str
    user_id: Optional[str]
    email: Optional[str]
    company: Optional[str]
    license_type: str
    duration_hours: Optional[float]
    generate_time: datetime
    expiry_time: Optional[datetime]
    is_perpetual: bool
    is_expired: bool = False
    is_revoked: bool
    revoke_reason: Optional[str]
    created_at: datetime

    class Config:
        from_attributes = True
