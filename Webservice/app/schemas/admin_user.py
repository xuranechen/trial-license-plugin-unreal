"""管理员用户详情与下发相关 schema"""

from datetime import datetime
from typing import List, Optional

from pydantic import BaseModel, Field


class UserBenefitResponse(BaseModel):
    project_id: str
    project_name: Optional[str] = None
    extra_trial_requests: int
    extra_trial_duration_hours: float
    extra_no_watermark_trial_requests: int = 0
    allow_no_watermark_trial: bool = False


class UserAppSummary(BaseModel):
    project_id: str
    project_name: str
    license_count: int
    device_count: int
    active_license_count: int
    extra_trial_requests: int = 0
    extra_trial_duration_hours: float = 0
    extra_no_watermark_trial_requests: int = 0
    allow_no_watermark_trial: bool = False


class UserDeviceSummary(BaseModel):
    machine_id: str
    project_count: int
    license_count: int
    latest_license_time: Optional[datetime] = None


class UserLicenseSummary(BaseModel):
    id: int
    project_id: str
    machine_id: str
    license_type: str
    expiry_time: Optional[datetime] = None
    is_perpetual: bool
    show_watermark: bool = True
    uses_no_watermark_benefit: bool = False
    is_expired: bool
    is_revoked: bool
    generate_time: datetime


class AdminUserDetailResponse(BaseModel):
    id: str
    username: str
    email: str
    full_name: Optional[str] = None
    company: Optional[str] = None
    is_active: bool
    is_admin: bool
    created_at: datetime
    last_login: Optional[datetime] = None
    total_licenses: int
    active_licenses: int
    revoked_licenses: int
    expired_licenses: int
    total_devices: int
    total_applications: int
    applications: List[UserAppSummary]
    devices: List[UserDeviceSummary]
    recent_licenses: List[UserLicenseSummary]


class AdminIssueLicenseRequest(BaseModel):
    project_id: str = Field(..., description="项目ID")
    machine_id: str = Field(..., description="设备ID")
    license_type: str = Field(..., description="许可证类型")
    show_watermark: bool = Field(True, description="是否显示水印")
    duration_hours: Optional[float] = Field(
        None, gt=0, description="试用版许可证时长（小时）"
    )


class AdminAdjustBenefitRequest(BaseModel):
    project_id: str = Field(..., description="项目ID")
    extra_trial_requests: int = Field(0, description="额外试用次数调整值（可正可负）")
    extra_trial_duration_hours: float = Field(
        0, description="单次试用时长加成调整值（可正可负）"
    )
    extra_no_watermark_trial_requests: int = Field(
        0, description="额外免水印试用次数调整值（可正可负）"
    )
    allow_no_watermark_trial: bool = Field(False, description="是否允许申请免水印试用")
