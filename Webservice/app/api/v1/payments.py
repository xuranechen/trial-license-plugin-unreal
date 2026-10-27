"""
支付回调 API

各支付渠道的异步通知入口。职责只有两件事：

1. 验证回调来源可信（Stripe 签名 / 支付宝 RSA 签名 / 微信签名）
2. 把「哪一笔支付成功了」交给 payment_callback_service 完成状态流转

所有渠道最终都会调用 ``apply_payment_success``，保证订单与许可证逻辑一致。
"""

import json
import logging
import xml.etree.ElementTree as ET
from typing import Any, Dict

import stripe
from fastapi import APIRouter, Depends, HTTPException, Request, status
from fastapi.responses import PlainTextResponse
from sqlalchemy.orm import Session

from app.config import settings
from app.database import get_db
from app.models.payment import Payment
from app.services.payment_callback_service import (
    apply_payment_success,
    find_payment,
    find_payment_by_order_no,
)
from app.services.payment_service import AlipayPaymentService, WechatPaymentService

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/payments", tags=["Payments"])


def _obj_get(obj: Any, key: str, default: Any = None) -> Any:
    """兼容 stripe.StripeObject（dict 子类）与普通属性的取值。"""
    try:
        return obj[key]
    except (KeyError, TypeError, AttributeError):
        return getattr(obj, key, default)


@router.post("/webhook/stripe")
async def stripe_webhook(request: Request, db: Session = Depends(get_db)):
    """
    Stripe 异步通知。

    需要在环境变量 / 配置中提供 ``STRIPE_WEBHOOK_SECRET``（Stripe Dashboard
    里生成的 ``whsec_...``），否则拒绝处理。
    """
    secret = getattr(settings, "STRIPE_WEBHOOK_SECRET", "")
    if not secret:
        logger.error("Stripe webhook 未配置 STRIPE_WEBHOOK_SECRET")
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail="Stripe webhook 未配置签名密钥",
        )

    payload = await request.body()
    signature = request.headers.get("stripe-signature")
    if not signature:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="缺少 stripe-signature"
        )

    try:
        event = stripe.Webhook.construct_event(payload, signature, secret)
    except Exception as exc:  # noqa: BLE001 - 签名/JSON 解析失败统一拒绝
        logger.warning("Stripe webhook 签名校验失败: %s", exc)
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="签名校验失败"
        )

    event_type = _obj_get(event, "type", "")
    if event_type not in ("payment_intent.succeeded", "checkout.session.completed"):
        return {"received": True, "ignored": event_type}

    payment_obj = _obj_get(_obj_get(event, "data", {}), "object", {})
    reference = _obj_get(payment_obj, "id", "")
    metadata = _obj_get(payment_obj, "metadata", {}) or {}
    order_id = _obj_get(metadata, "order_id", "")

    # checkout.session.completed 的 id 是 session id，真正的支付意图在其字段里
    payment_intent = _obj_get(payment_obj, "payment_intent", "")
    if payment_intent:
        reference = payment_intent

    payment = find_payment(db, reference)
    if not payment and order_id:
        payment = db.query(Payment).filter(Payment.order_id == order_id).first()

    if not payment:
        logger.warning("Stripe 回调找不到对应支付记录: ref=%s order_id=%s", reference, order_id)
        return {"received": True, "matched": False}

    _, order, license_record = apply_payment_success(
        db, payment, callback_data=json.dumps(event, ensure_ascii=False, default=str)
    )
    return {
        "received": True,
        "matched": True,
        "order_no": getattr(order, "order_no", None),
        "license_issued": license_record is not None,
    }


@router.post("/webhook/alipay")
async def alipay_webhook(request: Request, db: Session = Depends(get_db)):
    """
    支付宝异步通知（notify_url）。

    必须返回纯文本 ``success`` 才会停止重试。
    """
    form = await request.form()
    data: Dict[str, Any] = {key: value for key, value in form.items()}

    service = AlipayPaymentService()
    # verify_payment 会 pop sign，传副本避免污染原始数据
    if not service.verify_payment(dict(data)):
        logger.warning("支付宝回调验签失败: %s", data.get("out_trade_no"))
        return PlainTextResponse("fail", status_code=status.HTTP_400_BAD_REQUEST)

    payment = find_payment_by_order_no(db, data.get("out_trade_no", ""))
    if not payment:
        logger.warning("支付宝回调找不到对应支付记录: %s", data.get("out_trade_no"))
        return PlainTextResponse("fail", status_code=status.HTTP_404_NOT_FOUND)

    apply_payment_success(
        db, payment, callback_data=json.dumps(data, ensure_ascii=False, default=str)
    )
    return PlainTextResponse("success")


def _parse_wechat_xml(body: str) -> Dict[str, str]:
    """把微信支付回调 XML 拍平成 dict。"""
    root = ET.fromstring(body)
    return {
        child.tag: (child.text or "").strip()
        for child in root
        if isinstance(child.tag, str)
    }


@router.post("/webhook/wechat")
async def wechat_webhook(request: Request, db: Session = Depends(get_db)):
    """
    微信支付异步通知（notify_url）。

    必须返回 ``return_code=SUCCESS`` 的 XML 才会停止重试。
    """
    body = (await request.body()).decode("utf-8")
    try:
        data = _parse_wechat_xml(body)
    except ET.ParseError:
        return PlainTextResponse(
            "<xml><return_code><![CDATA[FAIL]]></return_code></xml>",
            media_type="application/xml",
            status_code=status.HTTP_400_BAD_REQUEST,
        )

    service = WechatPaymentService()
    if not service.verify_payment(data):
        logger.warning("微信回调验签失败: %s", data.get("out_trade_no"))
        return PlainTextResponse(
            "<xml><return_code><![CDATA[FAIL]]></return_code></xml>",
            media_type="application/xml",
            status_code=status.HTTP_400_BAD_REQUEST,
        )

    payment = find_payment_by_order_no(db, data.get("out_trade_no", ""))
    if not payment:
        logger.warning("微信回调找不到对应支付记录: %s", data.get("out_trade_no"))
        return PlainTextResponse(
            "<xml><return_code><![CDATA[FAIL]]></return_code></xml>",
            media_type="application/xml",
            status_code=status.HTTP_404_NOT_FOUND,
        )

    apply_payment_success(
        db, payment, callback_data=json.dumps(data, ensure_ascii=False, default=str)
    )
    return PlainTextResponse(
        "<xml><return_code><![CDATA[SUCCESS]]></return_code></xml>",
        media_type="application/xml",
    )
