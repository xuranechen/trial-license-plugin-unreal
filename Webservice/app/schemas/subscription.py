"""
订阅相关Schema
"""

from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime
from app.models.subscription import SubscriptionStatus


class SubscriptionPlanCreate(BaseModel):
    """创建订阅计划"""

    application_id: str = Field(..., description="应用ID")
    name: str = Field(..., description="计划名称")
    description: Optional[str] = Field(None, description="计划描述")
    price: float = Field(..., gt=0, description="价格")
    currency: str = Field(default="CNY", description="货币类型")
    billing_cycle: str = Field(..., description="计费周期: monthly/quarterly/yearly")
    max_licenses: Optional[int] = Field(None, description="最大许可证数量")
    max_devices: Optional[int] = Field(None, description="最大设备数")
    features: Optional[str] = Field(None, description="功能列表(JSON)")


class SubscriptionPlanResponse(BaseModel):
    """订阅计划响应"""

    id: str
    application_id: str
    name: str
    description: Optional[str] = None
    price: float
    currency: str
    billing_cycle: str
    max_licenses: Optional[int] = None
    max_devices: Optional[int] = None
    features: Optional[str] = None
    is_active: bool
    created_at: datetime

    class Config:
        from_attributes = True


class SubscriptionCreate(BaseModel):
    """创建订阅"""

    plan_id: str = Field(..., description="订阅计划ID")
    auto_renew: bool = Field(default=True, description="是否自动续费")


class SubscriptionResponse(BaseModel):
    """订阅响应"""

    id: str
    user_id: str
    plan_id: str
    status: SubscriptionStatus
    start_date: datetime
    end_date: datetime
    next_billing_date: Optional[datetime] = None
    auto_renew: bool
    licenses_used: int
    devices_used: int
    created_at: datetime

    class Config:
        from_attributes = True


class SubscriptionUpdate(BaseModel):
    """更新订阅"""

    auto_renew: Optional[bool] = Field(None, description="是否自动续费")
    status: Optional[SubscriptionStatus] = Field(None, description="订阅状态")
