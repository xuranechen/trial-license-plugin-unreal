"""
Pydantic schemas - 数据验证和序列化
"""
from app.schemas.license import (
    LicenseRequest,
    LicenseResponse,
    ValidateLicenseRequest,
    ValidateLicenseResponse,
    LicenseDetail
)
from app.schemas.application import (
    ApplicationCreate,
    ApplicationUpdate,
    ApplicationResponse
)

__all__ = [
    "LicenseRequest",
    "LicenseResponse",
    "ValidateLicenseRequest",
    "ValidateLicenseResponse",
    "LicenseDetail",
    "ApplicationCreate",
    "ApplicationUpdate",
    "ApplicationResponse"
]
