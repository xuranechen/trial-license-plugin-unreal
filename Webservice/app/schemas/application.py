"""
应用相关的Pydantic schemas
"""

from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime


class ApplicationCreate(BaseModel):
    """创建应用请求"""

    project_id: str = Field(..., description="项目ID")
    project_name: str = Field(..., description="项目名称")
    description: Optional[str] = Field(None, description="项目描述")
    passphrase: str = Field(..., description="加密口令")
    trial_duration_hours: float = Field(72.0, description="默认试用时长（小时）")
    trial_request_limit: int = Field(1, ge=1, description="每设备试用申请次数限制")
    daily_price: float = Field(9.9, ge=0, description="天卡价格")
    weekly_price: float = Field(49.9, ge=0, description="周卡价格")
    monthly_price: float = Field(149.9, ge=0, description="月卡价格")
    quarterly_price: float = Field(399.9, ge=0, description="季卡价格")
    yearly_price: float = Field(999.9, ge=0, description="年卡价格")
    perpetual_price: float = Field(2999.0, ge=0, description="永久授权价格")


class ApplicationUpdate(BaseModel):
    """更新应用请求"""

    project_name: Optional[str] = Field(None, description="项目名称")
    description: Optional[str] = Field(None, description="项目描述")
    passphrase: Optional[str] = Field(None, description="加密口令")
    trial_duration_hours: Optional[float] = Field(
        None, description="默认试用时长（小时）"
    )
    trial_request_limit: Optional[int] = Field(
        None, ge=1, description="每设备试用申请次数限制"
    )
    daily_price: Optional[float] = Field(None, ge=0, description="天卡价格")
    weekly_price: Optional[float] = Field(None, ge=0, description="周卡价格")
    monthly_price: Optional[float] = Field(None, ge=0, description="月卡价格")
    quarterly_price: Optional[float] = Field(None, ge=0, description="季卡价格")
    yearly_price: Optional[float] = Field(None, ge=0, description="年卡价格")
    perpetual_price: Optional[float] = Field(None, ge=0, description="永久授权价格")
    is_active: Optional[bool] = Field(None, description="是否启用")


class ApplicationResponse(BaseModel):
    """应用响应"""

    id: str
    project_id: str
    project_name: str
    description: Optional[str]
    passphrase: str
    trial_duration_hours: float
    trial_request_limit: int
    daily_price: float
    weekly_price: float
    monthly_price: float
    quarterly_price: float
    yearly_price: float
    perpetual_price: float
    is_active: bool
    created_at: datetime
    updated_at: Optional[datetime]

    class Config:
        from_attributes = True
