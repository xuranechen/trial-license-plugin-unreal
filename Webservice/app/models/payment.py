"""
支付模型
"""
from sqlalchemy import Column, String, Float, DateTime, ForeignKey, Enum as SQLEnum, Text
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from app.database import Base
import enum


class PaymentMethod(str, enum.Enum):
    """支付方式"""
    STRIPE = "stripe"
    ALIPAY = "alipay"
    WECHAT = "wechat"
    BANK_TRANSFER = "bank_transfer"


class PaymentStatus(str, enum.Enum):
    """支付状态"""
    PENDING = "pending"  # 待支付
    PROCESSING = "processing"  # 处理中
    SUCCESS = "success"  # 成功
    FAILED = "failed"  # 失败
    REFUNDED = "refunded"  # 已退款


class Payment(Base):
    """支付记录表"""
    __tablename__ = "payments"
    
    id = Column(String(50), primary_key=True)
    order_id = Column(String(50), ForeignKey("orders.id"), nullable=False, unique=True, index=True, comment="订单ID")
    
    # 支付信息
    payment_no = Column(String(100), unique=True, nullable=False, index=True, comment="支付流水号")
    payment_method = Column(SQLEnum(PaymentMethod), nullable=False, comment="支付方式")
    status = Column(SQLEnum(PaymentStatus), default=PaymentStatus.PENDING, index=True, comment="支付状态")
    
    # 金额信息
    amount = Column(Float, nullable=False, comment="支付金额")
    currency = Column(String(10), default="CNY", comment="货币类型")
    
    # 第三方支付信息
    transaction_id = Column(String(255), index=True, comment="第三方交易ID")
    payment_url = Column(Text, comment="支付链接")
    qr_code = Column(Text, comment="支付二维码")
    
    # 回调信息
    callback_data = Column(Text, comment="回调原始数据")
    callback_time = Column(DateTime(timezone=True), comment="回调时间")
    
    # 退款信息
    refund_amount = Column(Float, default=0, comment="退款金额")
    refund_reason = Column(Text, comment="退款原因")
    refund_time = Column(DateTime(timezone=True), comment="退款时间")
    
    # 备注
    remark = Column(Text, comment="备注")
    
    # 时间戳
    created_at = Column(DateTime(timezone=True), server_default=func.now(), comment="创建时间")
    paid_at = Column(DateTime(timezone=True), comment="支付完成时间")
    
    # 关系
    order = relationship("Order", back_populates="payment")
    
    def __repr__(self):
        return f"<Payment(payment_no='{self.payment_no}', method='{self.payment_method}', status='{self.status}')>"
