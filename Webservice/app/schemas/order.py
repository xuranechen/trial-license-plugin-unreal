"""
订单相关Schema
"""

from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime
from app.models.order import OrderStatus
from app.models.payment import PaymentMethod


class OrderCreate(BaseModel):
    """创建订单"""

    application_id: str = Field(..., description="应用ID")
    product_name: str = Field(..., description="产品名称")
    license_type: str = Field(..., description="许可证类型")
    license_duration: Optional[str] = Field(None, description="许可证时长")
    amount: float = Field(..., gt=0, description="金额")
    currency: str = Field(default="CNY", description="货币类型")
    quantity: int = Field(default=1, gt=0, description="数量")
    machine_id: Optional[str] = Field(None, description="设备ID")


class OrderResponse(BaseModel):
    """订单响应"""

    id: str
    user_id: str
    application_id: str
    order_no: str
    amount: float
    currency: str
    status: OrderStatus
    product_name: Optional[str] = None
    license_type: Optional[str] = None
    license_duration: Optional[str] = None
    machine_id: Optional[str] = None
    quantity: float
    created_at: datetime
    paid_at: Optional[datetime] = None

    class Config:
        from_attributes = True


class PaymentCreate(BaseModel):
    """创建支付"""

    order_id: str = Field(..., description="订单ID")
    payment_method: PaymentMethod = Field(..., description="支付方式")
    return_url: Optional[str] = Field(None, description="同步回调地址")
    notify_url: Optional[str] = Field(None, description="异步回调地址")


class PaymentResponse(BaseModel):
    """支付响应"""

    id: str
    payment_no: str
    payment_method: PaymentMethod
    amount: float
    currency: str
    status: str
    payment_url: Optional[str] = None
    qr_code: Optional[str] = None
    transaction_id: Optional[str] = None
    created_at: datetime

    class Config:
        from_attributes = True


class BatchLicenseCreate(BaseModel):
    """批量创建许可证"""

    application_id: str = Field(..., description="应用ID")
    license_type: str = Field(..., description="许可证类型: trial/permanent")
    count: int = Field(..., gt=0, le=1000, description="生成数量（最多1000个）")
    duration_days: Optional[int] = Field(None, description="试用天数（仅试用版）")
    machine_id: Optional[str] = Field(None, description="机器ID（可选，留空则不绑定）")
    remark: Optional[str] = Field(None, description="备注")


class BatchLicenseResponse(BaseModel):
    """批量许可证响应"""

    total: int = Field(..., description="总数")
    success: int = Field(..., description="成功数量")
    failed: int = Field(..., description="失败数量")
    licenses: list = Field(..., description="许可证列表")
    download_url: Optional[str] = Field(None, description="Excel下载地址")
