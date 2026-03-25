"""
统计分析API
"""

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from sqlalchemy import func, and_
from datetime import datetime, timedelta
from typing import Dict, Any

from app.database import get_db
from app.models.user import User
from app.models.license import License
from app.models.order import Order, OrderStatus
from app.models.payment import Payment, PaymentStatus
from app.utils.dependencies import get_current_admin_user

router = APIRouter(prefix="/statistics", tags=["统计分析"])


@router.get("/overview")
def get_overview_statistics(
    current_user: User = Depends(get_current_admin_user), db: Session = Depends(get_db)
) -> Dict[str, Any]:
    """
    获取概览统计数据（管理员）
    """
    # 用户统计
    total_users = db.query(func.count(User.id)).scalar()
    active_users = db.query(func.count(User.id)).filter(User.is_active == True).scalar()

    # 许可证统计
    total_licenses = db.query(func.count(License.id)).scalar()
    activated_licenses = (
        db.query(func.count(License.id)).filter(License.is_revoked == False).scalar()
    )
    trial_licenses = (
        db.query(func.count(License.id))
        .filter(License.license_type == "trial")
        .scalar()
    )
    permanent_licenses = (
        db.query(func.count(License.id))
        .filter(License.license_type == "permanent")
        .scalar()
    )

    # 订单统计
    total_orders = db.query(func.count(Order.id)).scalar()
    paid_orders = (
        db.query(func.count(Order.id)).filter(Order.status == OrderStatus.PAID).scalar()
    )
    pending_orders = (
        db.query(func.count(Order.id))
        .filter(Order.status == OrderStatus.PENDING)
        .scalar()
    )

    # 收入统计
    total_revenue = (
        db.query(func.sum(Order.amount))
        .filter(Order.status == OrderStatus.PAID)
        .scalar()
        or 0
    )

    # 今日统计
    today_start = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    today_orders = (
        db.query(func.count(Order.id)).filter(Order.created_at >= today_start).scalar()
    )
    today_revenue = (
        db.query(func.sum(Order.amount))
        .filter(and_(Order.created_at >= today_start, Order.status == OrderStatus.PAID))
        .scalar()
        or 0
    )

    return {
        "users": {"total": total_users, "active": active_users},
        "licenses": {
            "total": total_licenses,
            "activated": activated_licenses,
            "trial": trial_licenses,
            "permanent": permanent_licenses,
        },
        "orders": {
            "total": total_orders,
            "paid": paid_orders,
            "pending": pending_orders,
        },
        "revenue": {"total": total_revenue, "today": today_revenue},
        "today": {"orders": today_orders, "revenue": today_revenue},
    }


@router.get("/revenue")
def get_revenue_statistics(
    days: int = 30,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
) -> Dict[str, Any]:
    """
    获取收入统计（按日期）
    """
    start_date = datetime.now() - timedelta(days=days)

    # 按日期分组统计订单和收入
    daily_stats = (
        db.query(
            func.date(Order.created_at).label("date"),
            func.count(Order.id).label("order_count"),
            func.sum(Order.amount).label("revenue"),
        )
        .filter(and_(Order.created_at >= start_date, Order.status == OrderStatus.PAID))
        .group_by(func.date(Order.created_at))
        .all()
    )

    result = []
    for stat in daily_stats:
        result.append(
            {
                "date": stat.date.strftime("%Y-%m-%d"),
                "order_count": stat.order_count,
                "revenue": float(stat.revenue or 0),
            }
        )

    return {"period": f"最近{days}天", "data": result}


@router.get("/licenses")
def get_license_statistics(
    current_user: User = Depends(get_current_admin_user), db: Session = Depends(get_db)
) -> Dict[str, Any]:
    """
    获取许可证统计
    """
    # 按应用统计许可证
    from app.models.application import Application

    app_stats = (
        db.query(
            Application.name,
            func.count(License.id).label("total"),
            func.sum(func.case((License.is_revoked == False, 1), else_=0)).label(
                "activated"
            ),
            func.sum(func.case((License.license_type == "trial", 1), else_=0)).label(
                "trial"
            ),
            func.sum(
                func.case((License.license_type == "permanent", 1), else_=0)
            ).label("permanent"),
        )
        .join(License, Application.id == License.application_id)
        .group_by(Application.name)
        .all()
    )

    result = []
    for stat in app_stats:
        result.append(
            {
                "application": stat.name,
                "total": stat.total,
                "activated": stat.activated,
                "trial": stat.trial,
                "permanent": stat.permanent,
            }
        )

    return {"by_application": result}


@router.get("/users")
def get_user_statistics(
    current_user: User = Depends(get_current_admin_user), db: Session = Depends(get_db)
) -> Dict[str, Any]:
    """
    获取用户统计
    """
    # 按日期统计新增用户
    last_30_days = datetime.now() - timedelta(days=30)

    daily_users = (
        db.query(
            func.date(User.created_at).label("date"), func.count(User.id).label("count")
        )
        .filter(User.created_at >= last_30_days)
        .group_by(func.date(User.created_at))
        .all()
    )

    new_users = []
    for stat in daily_users:
        new_users.append({"date": stat.date.strftime("%Y-%m-%d"), "count": stat.count})

    # 用户活跃度
    last_login_7_days = (
        db.query(func.count(User.id))
        .filter(User.last_login >= datetime.now() - timedelta(days=7))
        .scalar()
    )

    return {"new_users_30_days": new_users, "active_users_7_days": last_login_7_days}


@router.get("/payments")
def get_payment_statistics(
    current_user: User = Depends(get_current_admin_user), db: Session = Depends(get_db)
) -> Dict[str, Any]:
    """
    获取支付统计
    """
    # 按支付方式统计
    payment_method_stats = (
        db.query(
            Payment.payment_method,
            func.count(Payment.id).label("count"),
            func.sum(Payment.amount).label("amount"),
        )
        .filter(Payment.status == PaymentStatus.SUCCESS)
        .group_by(Payment.payment_method)
        .all()
    )

    by_method = []
    for stat in payment_method_stats:
        by_method.append(
            {
                "method": stat.payment_method,
                "count": stat.count,
                "amount": float(stat.amount or 0),
            }
        )

    # 支付成功率
    total_payments = db.query(func.count(Payment.id)).scalar()
    success_payments = (
        db.query(func.count(Payment.id))
        .filter(Payment.status == PaymentStatus.SUCCESS)
        .scalar()
    )

    success_rate = (
        (success_payments / total_payments * 100) if total_payments > 0 else 0
    )

    return {
        "by_method": by_method,
        "success_rate": round(success_rate, 2),
        "total": total_payments,
        "success": success_payments,
    }
