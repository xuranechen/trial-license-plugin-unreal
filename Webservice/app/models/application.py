"""
应用模型
"""

from sqlalchemy import Column, String, Float, Boolean, DateTime, Text
from sqlalchemy.sql import func
from app.database import Base


class Application(Base):
    """应用表 - 管理不同的UE项目"""

    __tablename__ = "applications"

    id = Column(String(50), primary_key=True)
    project_id = Column(
        String(255), unique=True, nullable=False, index=True, comment="项目标识"
    )
    project_name = Column(String(255), nullable=False, comment="项目名称")
    description = Column(Text, comment="项目描述")
    passphrase = Column(String(255), nullable=False, comment="加密口令（加密存储）")
    trial_duration_hours = Column(Float, default=72.0, comment="默认试用时长（小时）")
    trial_request_limit = Column(Float, default=1, comment="每设备试用申请次数限制")
    daily_price = Column(Float, default=9.9, comment="天卡价格")
    weekly_price = Column(Float, default=49.9, comment="周卡价格")
    monthly_price = Column(Float, default=149.9, comment="月卡价格")
    quarterly_price = Column(Float, default=399.9, comment="季卡价格")
    yearly_price = Column(Float, default=999.9, comment="年卡价格")
    perpetual_price = Column(Float, default=2999.0, comment="永久授权价格")
    is_active = Column(Boolean, default=True, comment="是否启用")
    created_at = Column(
        DateTime(timezone=True), server_default=func.now(), comment="创建时间"
    )
    updated_at = Column(
        DateTime(timezone=True), onupdate=func.now(), comment="更新时间"
    )

    def __repr__(self):
        return f"<Application(project_id='{self.project_id}', project_name='{self.project_name}')>"
