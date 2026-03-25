"""
管理员API路由
"""

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session, joinedload
from typing import List, Optional
from datetime import datetime, timedelta
from app.database import get_db
from app.schemas.application import (
    ApplicationCreate,
    ApplicationUpdate,
    ApplicationResponse,
)
from app.models.application import Application
from app.models.license import License, LicenseType
from app.models.order import Order, OrderStatus
from app.models.system_config import SystemConfig
from app.models.user import User
from app.models.user_application_benefit import UserApplicationBenefit
from app.schemas.admin_user import (
    AdminAdjustBenefitRequest,
    AdminIssueLicenseRequest,
    AdminUserDetailResponse,
)
from app.schemas.system_config import SystemConfigResponse, SystemConfigBase
from app.services.config_service import get_or_create_system_config
from app.utils.dependencies import get_current_admin_user
from app.services.license_service import (
    generate_paid_license_for_order,
    get_effective_passphrase,
    get_effective_trial_policy,
)
from app.services.license_reminder_service import sync_license_expired_status
from app.services.encryption import EncryptionService
import uuid

router = APIRouter(prefix="/admin", tags=["Admin"])


def _normalize_passphrase(value: "Optional[str]", project_id: str) -> str:
    """规范化口令，避免保存空值、undefined、null。"""
    cleaned = (value or "").strip()
    if not cleaned or cleaned.lower() in {"undefined", "null"}:
        return project_id
    return cleaned


def _serialize_license(license_obj: License) -> dict:
    return {
        "id": license_obj.id,
        "license_key": license_obj.license_key,
        "project_id": license_obj.project_id,
        "machine_id": license_obj.machine_id,
        "user_id": license_obj.user_id,
        "email": license_obj.email,
        "company": license_obj.company,
        "license_type": getattr(
            license_obj.license_type, "value", license_obj.license_type
        ),
        "duration_hours": license_obj.duration_hours,
        "generate_time": license_obj.generate_time,
        "expiry_time": license_obj.expiry_time,
        "is_perpetual": license_obj.is_perpetual,
        "is_expired": getattr(license_obj, "is_expired", False),
        "is_revoked": license_obj.is_revoked,
        "revoke_reason": license_obj.revoke_reason,
        "revoked_at": license_obj.revoked_at,
        "order_id": license_obj.order_id,
        "created_at": license_obj.created_at,
        "updated_at": license_obj.updated_at,
    }


def _serialize_order(order: Order) -> dict:
    return {
        "id": order.id,
        "order_no": order.order_no,
        "user_id": order.user_id,
        "application_id": order.application_id,
        "application_name": order.application.project_name
        if order.application
        else None,
        "amount": order.amount,
        "currency": order.currency,
        "status": getattr(order.status, "value", order.status),
        "product_name": order.product_name,
        "license_type": order.license_type,
        "license_duration": order.license_duration,
        "quantity": order.quantity,
        "created_at": order.created_at,
        "paid_at": order.paid_at,
        "expired_at": order.expired_at,
    }


def _serialize_user(user: User) -> dict:
    return {
        "id": user.id,
        "username": user.username,
        "email": user.email,
        "full_name": user.full_name,
        "company": user.company,
        "is_active": user.is_active,
        "is_admin": user.is_admin,
        "created_at": user.created_at,
        "updated_at": user.updated_at,
        "last_login": user.last_login,
    }


def _license_duration_for_type(license_type: str) -> tuple[float, bool]:
    mapping = {
        "trial": (72.0, False),
        "daily": (24.0, False),
        "weekly": (24.0 * 7, False),
        "monthly": (24.0 * 30, False),
        "quarterly": (24.0 * 90, False),
        "yearly": (24.0 * 365, False),
        "perpetual": (0.0, True),
    }
    if license_type not in mapping:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="无效的许可证类型"
        )
    return mapping[license_type]


@router.get("/applications", response_model=List[ApplicationResponse])
async def list_applications(
    skip: int = 0, limit: int = 100, db: Session = Depends(get_db)
):
    """
    查询所有应用列表
    """
    applications = db.query(Application).offset(skip).limit(limit).all()
    return applications


@router.post(
    "/applications",
    response_model=ApplicationResponse,
    status_code=status.HTTP_201_CREATED,
)
async def create_application(
    app_data: ApplicationCreate, db: Session = Depends(get_db)
):
    """
    创建新应用
    """
    # 检查项目ID是否已存在
    existing = (
        db.query(Application)
        .filter(Application.project_id == app_data.project_id)
        .first()
    )

    if existing:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="项目ID已存在"
        )

    # 创建应用
    app = Application(
        id=str(uuid.uuid4()),
        project_id=app_data.project_id,
        project_name=app_data.project_name,
        description=app_data.description,
        passphrase=_normalize_passphrase(
            app_data.passphrase, app_data.project_id
        ),  # TODO: 使用主密钥加密
        trial_duration_hours=app_data.trial_duration_hours,
        trial_request_limit=app_data.trial_request_limit,
        daily_price=app_data.daily_price,
        weekly_price=app_data.weekly_price,
        monthly_price=app_data.monthly_price,
        quarterly_price=app_data.quarterly_price,
        yearly_price=app_data.yearly_price,
        perpetual_price=app_data.perpetual_price,
        is_active=True,
    )

    db.add(app)
    db.commit()
    db.refresh(app)

    return app


@router.get("/applications/{app_id}", response_model=ApplicationResponse)
async def get_application(app_id: str, db: Session = Depends(get_db)):
    """
    查询应用详情
    """
    app = db.query(Application).filter(Application.id == app_id).first()

    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    return app


@router.put("/applications/{app_id}", response_model=ApplicationResponse)
async def update_application(
    app_id: str, app_data: ApplicationUpdate, db: Session = Depends(get_db)
):
    """
    更新应用信息
    """
    app = db.query(Application).filter(Application.id == app_id).first()

    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    # 更新字段
    if app_data.project_name is not None:
        app.project_name = app_data.project_name
    if app_data.description is not None:
        app.description = app_data.description
    if app_data.passphrase is not None:
        app.passphrase = _normalize_passphrase(
            app_data.passphrase, app.project_id
        )  # TODO: 使用主密钥加密
    if app_data.trial_duration_hours is not None:
        app.trial_duration_hours = app_data.trial_duration_hours
    if app_data.trial_request_limit is not None:
        app.trial_request_limit = app_data.trial_request_limit
    if app_data.daily_price is not None:
        app.daily_price = app_data.daily_price
    if app_data.weekly_price is not None:
        app.weekly_price = app_data.weekly_price
    if app_data.monthly_price is not None:
        app.monthly_price = app_data.monthly_price
    if app_data.quarterly_price is not None:
        app.quarterly_price = app_data.quarterly_price
    if app_data.yearly_price is not None:
        app.yearly_price = app_data.yearly_price
    if app_data.perpetual_price is not None:
        app.perpetual_price = app_data.perpetual_price
    if app_data.is_active is not None:
        app.is_active = app_data.is_active

    db.commit()
    db.refresh(app)

    return app


@router.delete("/applications/{app_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_application(app_id: str, db: Session = Depends(get_db)):
    """
    删除应用
    """
    app = db.query(Application).filter(Application.id == app_id).first()

    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    # 检查是否有关联的许可证
    license_count = (
        db.query(License).filter(License.project_id == app.project_id).count()
    )

    if license_count > 0:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"无法删除，该应用下有 {license_count} 个许可证",
        )

    db.delete(app)
    db.commit()

    return None


@router.get("/licenses")
async def list_licenses(
    skip: int = 0,
    limit: int = 100,
    project_id: str = None,
    license_type: str = None,
    is_revoked: bool = None,
    db: Session = Depends(get_db),
):
    """
    获取所有许可证列表（管理员）

    支持筛选参数：
    - project_id: 按项目ID筛选
    - license_type: 按许可证类型筛选
    - is_revoked: 按撤销状态筛选
    """
    sync_license_expired_status(db)

    query = db.query(License)

    # 应用筛选条件
    if project_id:
        query = query.filter(License.project_id == project_id)
    if license_type:
        query = query.filter(License.license_type == license_type)
    if is_revoked is not None:
        query = query.filter(License.is_revoked == is_revoked)

    # 按生成时间倒序排列
    licenses = (
        query.order_by(License.generate_time.desc()).offset(skip).limit(limit).all()
    )

    return licenses


@router.get("/licenses/{license_id}")
async def get_license_detail(
    license_id: int,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    license_obj = db.query(License).filter(License.id == license_id).first()
    if not license_obj:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="许可证不存在"
        )
    return _serialize_license(license_obj)


@router.post("/licenses/{license_id}/revoke")
async def revoke_license(
    license_id: int,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    license_obj = db.query(License).filter(License.id == license_id).first()
    if not license_obj:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="许可证不存在"
        )
    if license_obj.is_revoked:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="许可证已撤销"
        )

    license_obj.is_revoked = True
    license_obj.revoke_reason = f"管理员 {current_user.username} 手动撤销"
    license_obj.revoked_at = datetime.now()
    db.commit()
    db.refresh(license_obj)
    return {"message": "许可证已撤销", "license": _serialize_license(license_obj)}


@router.get("/orders")
async def list_orders(
    skip: int = 0,
    limit: int = 100,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    orders = (
        db.query(Order)
        .options(joinedload(Order.application))
        .order_by(Order.created_at.desc())
        .offset(skip)
        .limit(limit)
        .all()
    )
    return [_serialize_order(order) for order in orders]


@router.get("/orders/{order_id}")
async def get_order_detail(
    order_id: str,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    order = (
        db.query(Order)
        .options(joinedload(Order.application))
        .filter(Order.id == order_id)
        .first()
    )
    if not order:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="订单不存在")
    return _serialize_order(order)


@router.post("/orders/{order_id}/complete")
async def complete_order_by_admin(
    order_id: str,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    order = db.query(Order).filter(Order.id == order_id).first()
    if not order:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="订单不存在")
    if order.status == OrderStatus.PAID:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="订单已支付"
        )

    order.status = OrderStatus.PAID
    order.paid_at = datetime.now()
    db.commit()
    db.refresh(order)
    generate_paid_license_for_order(db, order)
    return {"message": "订单已标记为已支付", "order": _serialize_order(order)}


@router.get("/users")
async def list_users(
    skip: int = 0,
    limit: int = 100,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    users = (
        db.query(User).order_by(User.created_at.desc()).offset(skip).limit(limit).all()
    )
    return [_serialize_user(user) for user in users]


@router.get("/users/{user_id}")
async def get_user_detail(
    user_id: str,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    user = db.query(User).filter(User.id == user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")
    return _serialize_user(user)


@router.patch("/users/{user_id}/status")
async def update_user_status(
    user_id: str,
    payload: dict,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    user = db.query(User).filter(User.id == user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")
    if user.is_admin and payload.get("is_active") is False:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="管理员账号不能被禁用"
        )
    if user.id == current_user.id and payload.get("is_active") is False:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="不能禁用当前管理员账号"
        )

    user.is_active = bool(payload.get("is_active", user.is_active))
    db.commit()
    db.refresh(user)
    return {"message": "用户状态已更新", "user": _serialize_user(user)}


@router.delete("/users/{user_id}")
async def delete_user(
    user_id: str,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    user = db.query(User).filter(User.id == user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")
    if user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="不能删除管理员账号"
        )

    db.delete(user)
    db.commit()
    return None


@router.get("/users/{user_id}/detail", response_model=AdminUserDetailResponse)
async def get_user_detail_dashboard(
    user_id: str,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    user = db.query(User).filter(User.id == user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")

    sync_license_expired_status(db)
    licenses = (
        db.query(License)
        .filter(License.user_id == user_id)
        .order_by(License.generate_time.desc())
        .all()
    )
    benefits = (
        db.query(UserApplicationBenefit)
        .filter(UserApplicationBenefit.user_id == user_id)
        .all()
    )
    benefit_map = {benefit.project_id: benefit for benefit in benefits}

    project_ids = sorted(
        {license.project_id for license in licenses if license.project_id}
    )
    applications = (
        db.query(Application).filter(Application.project_id.in_(project_ids)).all()
        if project_ids
        else []
    )
    app_map = {app.project_id: app for app in applications}

    app_summaries = []
    for project_id in project_ids:
        project_licenses = [item for item in licenses if item.project_id == project_id]
        benefit = benefit_map.get(project_id)
        app_summaries.append(
            {
                "project_id": project_id,
                "project_name": app_map.get(project_id).project_name
                if app_map.get(project_id)
                else project_id,
                "license_count": len(project_licenses),
                "device_count": len(
                    {item.machine_id for item in project_licenses if item.machine_id}
                ),
                "active_license_count": len(
                    [
                        item
                        for item in project_licenses
                        if not item.is_revoked
                        and not getattr(item, "is_expired", False)
                    ]
                ),
                "extra_trial_requests": int(
                    getattr(benefit, "extra_trial_requests", 0) or 0
                ),
                "extra_trial_duration_hours": float(
                    getattr(benefit, "extra_trial_duration_hours", 0) or 0
                ),
            }
        )

    device_map = {}
    for item in licenses:
        if not item.machine_id:
            continue
        summary = device_map.setdefault(
            item.machine_id, {"projects": set(), "count": 0, "latest": None}
        )
        summary["projects"].add(item.project_id)
        summary["count"] += 1
        if not summary["latest"] or (
            item.generate_time and item.generate_time > summary["latest"]
        ):
            summary["latest"] = item.generate_time

    return {
        "id": user.id,
        "username": user.username,
        "email": user.email,
        "full_name": user.full_name,
        "company": user.company,
        "is_active": user.is_active,
        "is_admin": user.is_admin,
        "created_at": user.created_at,
        "last_login": user.last_login,
        "total_licenses": len(licenses),
        "active_licenses": len(
            [
                item
                for item in licenses
                if not item.is_revoked and not getattr(item, "is_expired", False)
            ]
        ),
        "revoked_licenses": len([item for item in licenses if item.is_revoked]),
        "expired_licenses": len(
            [item for item in licenses if getattr(item, "is_expired", False)]
        ),
        "total_devices": len(device_map),
        "total_applications": len(project_ids),
        "applications": app_summaries,
        "devices": [
            {
                "machine_id": machine_id,
                "project_count": len(data["projects"]),
                "license_count": data["count"],
                "latest_license_time": data["latest"],
            }
            for machine_id, data in device_map.items()
        ],
        "recent_licenses": [
            {
                "id": item.id,
                "project_id": item.project_id,
                "machine_id": item.machine_id,
                "license_type": getattr(item.license_type, "value", item.license_type),
                "expiry_time": item.expiry_time,
                "is_perpetual": item.is_perpetual,
                "is_expired": getattr(item, "is_expired", False),
                "is_revoked": item.is_revoked,
                "generate_time": item.generate_time,
            }
            for item in licenses[:20]
        ],
    }


@router.post("/users/{user_id}/issue-license")
async def issue_license_for_user(
    user_id: str,
    payload: AdminIssueLicenseRequest,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    user = db.query(User).filter(User.id == user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")
    app = (
        db.query(Application)
        .filter(Application.project_id == payload.project_id)
        .first()
    )
    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    duration_hours, is_perpetual = _license_duration_for_type(payload.license_type)
    if payload.license_type == "trial":
        duration_hours = float(
            payload.duration_hours
            if payload.duration_hours is not None
            else get_effective_trial_policy(db, user_id, app)["trial_duration_hours"]
        )

    generate_time = datetime.now()
    expiry_time = (
        datetime(2099, 12, 31, 23, 59, 59)
        if is_perpetual
        else generate_time + timedelta(hours=duration_hours)
    )

    license_key = EncryptionService.generate_license_key(
        duration_hours=duration_hours,
        expiry_date=expiry_time,
        passphrase=get_effective_passphrase(app, app.project_id),
        project_id=app.project_id,
        machine_id=payload.machine_id,
        is_perpetual=is_perpetual,
    )

    license_record = License(
        license_key=license_key,
        project_id=app.project_id,
        machine_id=payload.machine_id,
        user_id=user.id,
        email=user.email,
        company=user.company,
        license_type=LicenseType.PERPETUAL if is_perpetual else payload.license_type,
        duration_hours=duration_hours,
        generate_time=generate_time,
        expiry_time=expiry_time,
        is_perpetual=is_perpetual,
    )
    db.add(license_record)
    db.commit()
    db.refresh(license_record)
    return {"message": "许可证已下发", "license": _serialize_license(license_record)}


@router.post("/users/{user_id}/trial-benefits")
async def update_user_trial_benefits(
    user_id: str,
    payload: AdminAdjustBenefitRequest,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    user = db.query(User).filter(User.id == user_id).first()
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="用户不存在")
    app = (
        db.query(Application)
        .filter(Application.project_id == payload.project_id)
        .first()
    )
    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    benefit = (
        db.query(UserApplicationBenefit)
        .filter(
            UserApplicationBenefit.user_id == user_id,
            UserApplicationBenefit.project_id == payload.project_id,
        )
        .first()
    )
    if not benefit:
        benefit = UserApplicationBenefit(
            id=str(uuid.uuid4()),
            user_id=user_id,
            project_id=payload.project_id,
        )
        db.add(benefit)

    benefit.extra_trial_requests = payload.extra_trial_requests
    benefit.extra_trial_duration_hours = payload.extra_trial_duration_hours
    db.commit()
    db.refresh(benefit)
    return {
        "message": "用户试用权益已更新",
        "benefit": {
            "project_id": benefit.project_id,
            "project_name": app.project_name,
            "extra_trial_requests": int(benefit.extra_trial_requests or 0),
            "extra_trial_duration_hours": float(
                benefit.extra_trial_duration_hours or 0
            ),
        },
    }


@router.get("/statistics")
async def get_statistics(db: Session = Depends(get_db)):
    """
    获取统计数据
    """
    total_applications = db.query(Application).count()
    active_applications = (
        db.query(Application).filter(Application.is_active == True).count()
    )

    total_licenses = db.query(License).count()
    trial_licenses = (
        db.query(License).filter(License.license_type == LicenseType.TRIAL).count()
    )
    perpetual_licenses = (
        db.query(License).filter(License.license_type == LicenseType.PERPETUAL).count()
    )
    revoked_licenses = db.query(License).filter(License.is_revoked == True).count()

    return {
        "applications": {
            "total": total_applications,
            "active": active_applications,
            "inactive": total_applications - active_applications,
        },
        "licenses": {
            "total": total_licenses,
            "trial": trial_licenses,
            "perpetual": perpetual_licenses,
            "revoked": revoked_licenses,
            "active": total_licenses - revoked_licenses,
        },
    }


@router.get("/system-config", response_model=SystemConfigResponse)
async def get_system_config(
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    return get_or_create_system_config(db)


@router.put("/system-config", response_model=SystemConfigResponse)
async def update_system_config(
    payload: SystemConfigBase,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    config = get_or_create_system_config(db)
    for key, value in payload.model_dump().items():
        setattr(config, key, value)
    db.commit()
    db.refresh(config)
    return config
