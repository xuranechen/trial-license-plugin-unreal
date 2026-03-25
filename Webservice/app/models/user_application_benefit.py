"""用户应用权益模型"""

from sqlalchemy import Column, String, Float, DateTime, UniqueConstraint
from sqlalchemy.sql import func

from app.database import Base


class UserApplicationBenefit(Base):
    """用户在指定应用下的附加试用权益"""

    __tablename__ = "user_application_benefits"
    __table_args__ = (
        UniqueConstraint("user_id", "project_id", name="uq_user_project_benefit"),
    )

    id = Column(String(50), primary_key=True)
    user_id = Column(String(50), nullable=False, index=True, comment="用户ID")
    project_id = Column(String(255), nullable=False, index=True, comment="项目ID")
    extra_trial_requests = Column(Float, default=0, comment="额外试用次数")
    extra_trial_duration_hours = Column(
        Float, default=0, comment="额外试用时长（小时）"
    )
    created_at = Column(
        DateTime(timezone=True), server_default=func.now(), comment="创建时间"
    )
    updated_at = Column(
        DateTime(timezone=True), onupdate=func.now(), comment="更新时间"
    )
