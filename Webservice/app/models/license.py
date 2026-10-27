"""
许可证模型
"""

from sqlalchemy import (
    BigInteger,
    Boolean,
    Column,
    DateTime,
    Enum,
    Float,
    Integer,
    String,
    Text,
)
from sqlalchemy.sql import func
import enum
from app.database import Base


class LicenseType(str, enum.Enum):
    """许可证类型枚举"""

    TRIAL = "trial"
    DAILY = "daily"
    WEEKLY = "weekly"
    MONTHLY = "monthly"
    QUARTERLY = "quarterly"
    YEARLY = "yearly"
    PERPETUAL = "perpetual"
    SUBSCRIPTION = "subscription"

    # 兼容历史数据（旧版本使用大写值）
    LEGACY_TRIAL = "TRIAL"
    LEGACY_DAILY = "DAILY"
    LEGACY_WEEKLY = "WEEKLY"
    LEGACY_MONTHLY = "MONTHLY"
    LEGACY_QUARTERLY = "QUARTERLY"
    LEGACY_YEARLY = "YEARLY"
    LEGACY_PERPETUAL = "PERPETUAL"
    LEGACY_SUBSCRIPTION = "SUBSCRIPTION"


class License(Base):
    """许可证表 - 存储所有生成的许可证"""

    __tablename__ = "licenses"

    # SQLite 只有 INTEGER PRIMARY KEY 才是 rowid 别名并能自增；
    # 直接用 BIGINT 建表会导致插入时报 NOT NULL constraint failed: licenses.id。
    id = Column(
        BigInteger().with_variant(Integer, "sqlite"),
        primary_key=True,
        autoincrement=True,
    )
    license_key = Column(Text, unique=True, nullable=False, comment="许可证密钥")
    project_id = Column(String(255), nullable=False, index=True, comment="项目标识")
    machine_id = Column(String(255), nullable=False, index=True, comment="设备ID")
    user_id = Column(String(255), index=True, comment="用户ID")
    email = Column(String(255), comment="用户邮箱")
    company = Column(String(255), comment="公司名称")
    license_type = Column(
        Enum(
            LicenseType,
            values_callable=lambda enum_cls: [item.value for item in enum_cls],
            native_enum=False,
        ),
        nullable=False,
        comment="许可证类型",
    )
    duration_hours = Column(Float, comment="时长（小时）")
    generate_time = Column(DateTime(timezone=True), nullable=False, comment="生成时间")
    expiry_time = Column(DateTime(timezone=True), comment="过期时间")
    is_perpetual = Column(Boolean, default=False, comment="是否永久许可证")
    show_watermark = Column(Boolean, default=True, comment="是否显示水印")
    uses_no_watermark_benefit = Column(
        Boolean, default=False, comment="是否占用免水印试用权益"
    )
    counts_against_trial_limit = Column(
        Boolean, default=True, comment="是否占用试用次数"
    )
    is_expired = Column(Boolean, default=False, comment="是否已过期")
    is_revoked = Column(Boolean, default=False, comment="是否已撤销")
    revoke_reason = Column(Text, comment="撤销原因")
    revoked_at = Column(DateTime(timezone=True), comment="撤销时间")
    reminder_7d_sent = Column(Boolean, default=False, comment="是否已发送7天提醒")
    reminder_3d_sent = Column(Boolean, default=False, comment="是否已发送3天提醒")
    reminder_1d_sent = Column(Boolean, default=False, comment="是否已发送1天提醒")
    expired_notice_sent = Column(Boolean, default=False, comment="是否已发送到期通知")
    order_id = Column(String(255), index=True, comment="关联订单ID")
    created_at = Column(
        DateTime(timezone=True), server_default=func.now(), comment="创建时间"
    )
    updated_at = Column(
        DateTime(timezone=True), onupdate=func.now(), comment="更新时间"
    )

    def __repr__(self):
        return f"<License(id={self.id}, project_id='{self.project_id}', type={self.license_type})>"
