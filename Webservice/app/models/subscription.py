"""
订阅模型
"""
from sqlalchemy import Column, String, Float, Integer, DateTime, ForeignKey, Boolean, Enum as SQLEnum
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from app.database import Base
import enum


class SubscriptionStatus(str, enum.Enum):
    """订阅状态"""
    ACTIVE = "active"  # 激活
    CANCELLED = "cancelled"  # 已取消
    EXPIRED = "expired"  # 已过期
    SUSPENDED = "suspended"  # 已暂停


class SubscriptionPlan(Base):
    """订阅计划表"""
    __tablename__ = "subscription_plans"
    
    id = Column(String(50), primary_key=True)
    application_id = Column(String(50), ForeignKey("applications.id"), nullable=False, comment="应用ID")
    
    # 计划信息
    name = Column(String(255), nullable=False, comment="计划名称")
    description = Column(String(500), comment="计划描述")
    
    # 定价信息
    price = Column(Float, nullable=False, comment="价格")
    currency = Column(String(10), default="CNY", comment="货币类型")
    billing_cycle = Column(String(50), nullable=False, comment="计费周期: monthly, quarterly, yearly")
    
    # 许可证限制
    max_licenses = Column(Integer, comment="最大许可证数量")
    max_devices = Column(Integer, comment="最大设备数")
    features = Column(String(1000), comment="功能列表(JSON)")
    
    # 状态
    is_active = Column(Boolean, default=True, comment="是否启用")
    
    # 时间戳
    created_at = Column(DateTime(timezone=True), server_default=func.now(), comment="创建时间")
    updated_at = Column(DateTime(timezone=True), onupdate=func.now(), comment="更新时间")
    
    # 关系
    application = relationship("Application", backref="subscription_plans")
    subscriptions = relationship("Subscription", back_populates="plan")
    
    def __repr__(self):
        return f"<SubscriptionPlan(name='{self.name}', price={self.price}, cycle='{self.billing_cycle}')>"


class Subscription(Base):
    """用户订阅表"""
    __tablename__ = "subscriptions"
    
    id = Column(String(50), primary_key=True)
    user_id = Column(String(50), ForeignKey("users.id"), nullable=False, index=True, comment="用户ID")
    plan_id = Column(String(50), ForeignKey("subscription_plans.id"), nullable=False, comment="订阅计划ID")
    
    # 订阅信息
    status = Column(SQLEnum(SubscriptionStatus), default=SubscriptionStatus.ACTIVE, index=True, comment="订阅状态")
    
    # 时间信息
    start_date = Column(DateTime(timezone=True), nullable=False, comment="开始时间")
    end_date = Column(DateTime(timezone=True), nullable=False, comment="结束时间")
    next_billing_date = Column(DateTime(timezone=True), comment="下次扣费时间")
    cancelled_at = Column(DateTime(timezone=True), comment="取消时间")
    
    # 自动续费
    auto_renew = Column(Boolean, default=True, comment="是否自动续费")
    
    # 使用统计
    licenses_used = Column(Integer, default=0, comment="已使用许可证数")
    devices_used = Column(Integer, default=0, comment="已使用设备数")
    
    # 时间戳
    created_at = Column(DateTime(timezone=True), server_default=func.now(), comment="创建时间")
    updated_at = Column(DateTime(timezone=True), onupdate=func.now(), comment="更新时间")
    
    # 关系
    user = relationship("User", backref="subscriptions")
    plan = relationship("SubscriptionPlan", back_populates="subscriptions")
    
    def __repr__(self):
        return f"<Subscription(user_id='{self.user_id}', status='{self.status}')>"
