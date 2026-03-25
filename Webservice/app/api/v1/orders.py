"""
订单和支付API
"""

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List
from datetime import datetime
import uuid

from app.database import get_db
from app.models.user import User
from app.models.order import Order, OrderStatus
from app.models.payment import Payment, PaymentStatus
from app.models.license import License
from app.schemas.order import (
    OrderCreate,
    OrderResponse,
    PaymentCreate,
    PaymentResponse,
    BatchLicenseCreate,
    BatchLicenseResponse,
)
from app.utils.dependencies import get_current_user, get_current_admin_user
from app.services.payment_service import payment_service
from app.services.encryption import EncryptionService
from app.services.license_service import generate_paid_license_for_order
import pandas as pd
import io

router = APIRouter(prefix="/orders", tags=["订单和支付"])


@router.post("", response_model=OrderResponse, status_code=status.HTTP_201_CREATED)
def create_order(
    order_data: OrderCreate,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    创建订单
    """
    # 生成订单号
    order_no = f"ORD{datetime.now().strftime('%Y%m%d%H%M%S')}{uuid.uuid4().hex[:8]}"

    # 创建订单
    new_order = Order(
        id=str(uuid.uuid4()),
        user_id=current_user.id,
        application_id=order_data.application_id,
        order_no=order_no,
        amount=order_data.amount * order_data.quantity,
        currency=order_data.currency,
        status=OrderStatus.PENDING,
        product_name=order_data.product_name,
        license_type=order_data.license_type,
        license_duration=order_data.license_duration,
        machine_id=order_data.machine_id,
        quantity=order_data.quantity,
    )

    db.add(new_order)
    db.commit()
    db.refresh(new_order)

    return new_order


@router.get("", response_model=List[OrderResponse])
def get_orders(
    skip: int = 0,
    limit: int = 100,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    获取当前用户的订单列表
    """
    orders = (
        db.query(Order)
        .filter(Order.user_id == current_user.id)
        .offset(skip)
        .limit(limit)
        .all()
    )

    return orders


@router.get("/{order_id}", response_model=OrderResponse)
def get_order(
    order_id: str,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    获取订单详情
    """
    order = (
        db.query(Order)
        .filter(Order.id == order_id, Order.user_id == current_user.id)
        .first()
    )

    if not order:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="订单不存在")

    return order


@router.post("/{order_id}/pay", response_model=PaymentResponse)
def create_payment(
    order_id: str,
    payment_data: PaymentCreate,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    创建支付
    """
    # 查找订单
    order = (
        db.query(Order)
        .filter(Order.id == order_id, Order.user_id == current_user.id)
        .first()
    )

    if not order:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="订单不存在")

    if order.status != OrderStatus.PENDING:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="订单状态不允许支付"
        )

    # 检查是否已有支付记录
    existing_payment = db.query(Payment).filter(Payment.order_id == order_id).first()
    if existing_payment and existing_payment.status == PaymentStatus.SUCCESS:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="订单已支付"
        )

    # 创建支付
    payment = payment_service.create_payment(
        order=order,
        payment_method=payment_data.payment_method,
        return_url=payment_data.return_url,
        notify_url=payment_data.notify_url,
    )

    if not payment:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, detail="创建支付失败"
        )

    db.add(payment)
    db.commit()
    db.refresh(payment)

    return payment


@router.post("/{order_id}/complete")
def complete_order(
    order_id: str,
    current_user: User = Depends(get_current_admin_user),
    db: Session = Depends(get_db),
):
    """
    完成订单（管理员手动标记，用于测试或线下支付）
    """
    order = db.query(Order).filter(Order.id == order_id).first()

    if not order:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="订单不存在")

    # 更新订单状态
    order.status = OrderStatus.PAID
    order.paid_at = datetime.now()

    db.commit()
    db.refresh(order)

    generate_paid_license_for_order(db, order)

    return {"message": "订单已完成"}
