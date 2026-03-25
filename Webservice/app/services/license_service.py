"""
许可证生成服务
"""

from datetime import datetime, timedelta
from typing import Optional

from fastapi import HTTPException, status
from sqlalchemy.orm import Session

from app.models.application import Application
from app.models.license import License, LicenseType
from app.models.order import Order
from app.models.user import User
from app.models.user_application_benefit import UserApplicationBenefit
from app.services.email_service import email_service
from app.services.encryption import EncryptionService


PAID_LICENSE_CONFIG = {
    "daily": {"duration_hours": 24.0, "is_perpetual": False, "response_type": "daily"},
    "weekly": {
        "duration_hours": 24.0 * 7,
        "is_perpetual": False,
        "response_type": "weekly",
    },
    "monthly": {
        "duration_hours": 24.0 * 30,
        "is_perpetual": False,
        "response_type": "monthly",
    },
    "quarterly": {
        "duration_hours": 24.0 * 90,
        "is_perpetual": False,
        "response_type": "quarterly",
    },
    "yearly": {
        "duration_hours": 24.0 * 365,
        "is_perpetual": False,
        "response_type": "yearly",
    },
    "perpetual": {
        "duration_hours": 0.0,
        "is_perpetual": True,
        "response_type": "perpetual",
    },
}


def get_user_trial_benefit(
    db: Session, user_id: str, project_id: str
) -> Optional[UserApplicationBenefit]:
    return (
        db.query(UserApplicationBenefit)
        .filter(
            UserApplicationBenefit.user_id == user_id,
            UserApplicationBenefit.project_id == project_id,
        )
        .first()
    )


def get_effective_trial_policy(db: Session, user_id: str, app: Application) -> dict:
    benefit = get_user_trial_benefit(db, user_id, app.project_id)
    extra_requests = int(getattr(benefit, "extra_trial_requests", 0) or 0)
    extra_hours = float(getattr(benefit, "extra_trial_duration_hours", 0) or 0)
    return {
        "trial_request_limit": max(
            int(getattr(app, "trial_request_limit", 1) or 1) + extra_requests, 1
        ),
        "trial_duration_hours": max(
            float(getattr(app, "trial_duration_hours", 72.0) or 72.0) + extra_hours, 1.0
        ),
        "extra_trial_requests": extra_requests,
        "extra_trial_duration_hours": extra_hours,
    }


def get_effective_passphrase(app: Application, project_id: str) -> str:
    configured = (app.passphrase or "").strip()
    if configured and configured.lower() not in {"undefined", "null"}:
        return configured
    return project_id


def generate_paid_license_for_order(db: Session, order: Order) -> License:
    app = db.query(Application).filter(Application.id == order.application_id).first()
    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    user = db.query(User).filter(User.id == order.user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")

    existing_license = db.query(License).filter(License.order_id == order.id).first()
    if existing_license:
        return existing_license

    config = PAID_LICENSE_CONFIG.get(order.license_type or "")
    if not config:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="订单许可证类型无效"
        )

    machine_id = (order.machine_id or "").strip()
    if not machine_id:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="订单缺少设备信息，无法生成许可证",
        )

    generate_time = datetime.now()
    if config["is_perpetual"]:
        expiry_time = datetime(2099, 12, 31, 23, 59, 59)
        license_type = LicenseType.PERPETUAL
    else:
        expiry_time = generate_time + timedelta(hours=config["duration_hours"])
        license_type = order.license_type

    try:
        license_key = EncryptionService.generate_license_key(
            duration_hours=config["duration_hours"],
            expiry_date=expiry_time,
            passphrase=get_effective_passphrase(app, app.project_id),
            project_id=app.project_id,
            machine_id=machine_id,
            is_perpetual=config["is_perpetual"],
        )
    except Exception as exc:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"生成许可证失败: {str(exc)}",
        )

    license_record = License(
        license_key=license_key,
        project_id=app.project_id,
        machine_id=machine_id,
        email=user.email,
        company=None,
        user_id=user.id,
        license_type=license_type,
        duration_hours=config["duration_hours"],
        generate_time=generate_time,
        expiry_time=expiry_time,
        is_perpetual=config["is_perpetual"],
        order_id=order.id,
    )

    db.add(license_record)
    db.commit()
    db.refresh(license_record)

    try:
        email_service.send_license_activation_email(
            to_email=user.email,
            user_name=user.full_name or user.username,
            license_key=license_key,
            app_name=app.project_name,
            expire_date=None if config["is_perpetual"] else expiry_time,
        )
        email_service.send_payment_success_email(
            to_email=user.email,
            user_name=user.full_name or user.username,
            order_no=order.order_no,
            amount=order.amount,
            product_name=order.product_name or f"{app.project_name} 许可证",
        )
    except Exception:
        pass

    return license_record
