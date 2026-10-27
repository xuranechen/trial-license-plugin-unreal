"""
支付回调处理服务

把各支付渠道的异步通知统一成一条状态流转：

    支付记录 PENDING -> SUCCESS
    订单 PENDING -> PAID
    签发许可证（幂等）

渠道适配层（Stripe / 支付宝 / 微信）只负责「验证签名 + 解析出哪一笔支付成功了」，
真正的状态变更集中在这里，避免各渠道重复实现导致行为不一致。
"""

import logging
from datetime import datetime
from typing import Optional, Tuple

from sqlalchemy import or_
from sqlalchemy.orm import Session

from app.models.license import License
from app.models.order import Order, OrderStatus
from app.models.payment import Payment, PaymentStatus
from app.services.license_service import generate_paid_license_for_order

logger = logging.getLogger(__name__)


def find_payment(db: Session, reference: str) -> Optional[Payment]:
    """按第三方交易号 / 支付流水号 / 主键查找支付记录。"""
    if not reference:
        return None
    return (
        db.query(Payment)
        .filter(
            or_(
                Payment.transaction_id == reference,
                Payment.payment_no == reference,
                Payment.id == reference,
            )
        )
        .first()
    )


def find_payment_by_order_no(db: Session, order_no: str) -> Optional[Payment]:
    """按商户订单号查找支付记录（支付宝 / 微信的 out_trade_no）。"""
    if not order_no:
        return None
    order = db.query(Order).filter(Order.order_no == order_no).first()
    if not order:
        return None
    return db.query(Payment).filter(Payment.order_id == order.id).first()


def apply_payment_success(
    db: Session,
    payment: Payment,
    callback_data: Optional[str] = None,
) -> Tuple[Payment, Optional[Order], Optional[License]]:
    """
    将支付标记为成功，完成订单并签发许可证。

    幂等：重复回调不会重复签发许可证；已退款的支付不会被迟到的回调覆盖。

    Returns:
        (payment, order, license) —— license 可能为 None（订单缺设备信息等）。
    """
    order = payment.order

    if payment.status == PaymentStatus.REFUNDED:
        logger.warning(
            "忽略已退款支付的迟到成功回调: payment_no=%s", payment.payment_no
        )
        return payment, order, None

    now = datetime.now()
    if payment.status != PaymentStatus.SUCCESS:
        payment.status = PaymentStatus.SUCCESS
        payment.paid_at = now
        payment.callback_time = now
    if callback_data is not None:
        payment.callback_data = callback_data

    if order is not None and order.status != OrderStatus.PAID:
        order.status = OrderStatus.PAID
        order.paid_at = order.paid_at or now

    db.commit()
    db.refresh(payment)

    license_record: Optional[License] = None
    if order is not None:
        try:
            license_record = generate_paid_license_for_order(db, order)
        except Exception as exc:  # noqa: BLE001 - 回调不应因发证失败而无限重试
            logger.error(
                "支付成功但许可证签发失败: order_no=%s, error=%s",
                order.order_no,
                exc,
            )

    return payment, order, license_record
