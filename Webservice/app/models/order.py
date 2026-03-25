"""
订单模型
"""

from sqlalchemy import Column, String, Float, DateTime, ForeignKey, Enum as SQLEnum
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from app.database import Base
import enum


class OrderStatus(str, enum.Enum):
    """订单状态"""

    PENDING = "pending"  # 待支付
    PAID = "paid"  # 已支付
    EXPIRED = "expired"  # 已过期
    CANCELLED = "cancelled"  # 已取消
    REFUNDED = "refunded"  # 已退款


class Order(Base):
    """订单表"""

    __tablename__ = "orders"

    id = Column(String(50), primary_key=True)
    user_id = Column(
        String(50), ForeignKey("users.id"), nullable=False, index=True, comment="用户ID"
    )
    application_id = Column(
        String(50), ForeignKey("applications.id"), nullable=False, comment="应用ID"
    )

    # 订单信息
    order_no = Column(
        String(100), unique=True, nullable=False, index=True, comment="订单号"
    )
    amount = Column(Float, nullable=False, comment="订单金额")
    currency = Column(String(10), default="CNY", comment="货币类型")
    status = Column(
        SQLEnum(OrderStatus),
        default=OrderStatus.PENDING,
        index=True,
        comment="订单状态",
    )

    # 产品信息
    product_name = Column(String(255), comment="产品名称")
    license_type = Column(String(50), comment="许可证类型")
    license_duration = Column(String(50), comment="许可证时长")
    machine_id = Column(String(255), comment="设备ID")
    quantity = Column(Float, default=1, comment="数量")

    # 时间戳
    created_at = Column(
        DateTime(timezone=True), server_default=func.now(), comment="创建时间"
    )
    paid_at = Column(DateTime(timezone=True), comment="支付时间")
    expired_at = Column(DateTime(timezone=True), comment="过期时间")

    # 关系
    user = relationship("User", backref="orders")
    application = relationship("Application", backref="orders")
    payment = relationship("Payment", back_populates="order", uselist=False)

    def __repr__(self):
        return f"<Order(order_no='{self.order_no}', status='{self.status}')>"
