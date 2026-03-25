"""
支付集成服务 - 统一支付接口
"""

from typing import Optional, Dict, Any
from datetime import datetime
import uuid
import stripe
import logging
from app.config import settings
from app.services.config_service import get_runtime_system_config
from app.models.payment import Payment, PaymentMethod, PaymentStatus
from app.models.order import Order, OrderStatus

logger = logging.getLogger(__name__)


class StripePaymentService:
    """Stripe支付服务"""

    def __init__(self, runtime_config: Optional[Dict[str, Any]] = None):
        runtime = runtime_config or {}
        self.enabled = bool(runtime.get("stripe_enabled"))
        self.api_key = runtime.get("stripe_secret_key") or getattr(
            settings, "STRIPE_SECRET_KEY", ""
        )
        stripe.api_key = self.api_key

    def create_payment_intent(
        self,
        amount: float,
        currency: str = "usd",
        metadata: Optional[Dict[str, Any]] = None,
    ) -> Optional[Dict[str, Any]]:
        """
        创建支付意图

        Args:
            amount: 金额（单位：元）
            currency: 货币类型
            metadata: 元数据

        Returns:
            支付意图信息
        """
        try:
            if not self.enabled or not self.api_key:
                return None
            # Stripe使用分为单位
            amount_cents = int(amount * 100)

            intent = stripe.PaymentIntent.create(
                amount=amount_cents, currency=currency.lower(), metadata=metadata or {}
            )

            return {
                "payment_intent_id": intent.id,
                "client_secret": intent.client_secret,
                "status": intent.status,
            }
        except Exception as e:
            logger.error(f"Stripe创建支付失败: {str(e)}")
            return None

    def verify_payment(self, payment_intent_id: str) -> bool:
        """验证支付是否成功"""
        try:
            if not self.enabled or not self.api_key:
                return False
            intent = stripe.PaymentIntent.retrieve(payment_intent_id)
            return intent.status == "succeeded"
        except Exception as e:
            logger.error(f"Stripe验证支付失败: {str(e)}")
            return False


class AlipayPaymentService:
    """支付宝支付服务"""

    def __init__(self, runtime_config: Optional[Dict[str, Any]] = None):
        runtime = runtime_config or {}
        try:
            from alipay import AliPay

            self.enabled = bool(runtime.get("alipay_enabled"))
            self.app_id = runtime.get("alipay_app_id") or getattr(
                settings, "ALIPAY_APP_ID", ""
            )
            self.private_key = runtime.get("alipay_private_key") or getattr(
                settings, "ALIPAY_PRIVATE_KEY", ""
            )
            self.public_key = runtime.get("alipay_public_key") or getattr(
                settings, "ALIPAY_PUBLIC_KEY", ""
            )

            if (
                not self.enabled
                or not self.app_id
                or not self.private_key
                or not self.public_key
            ):
                self.alipay = None
                return

            self.alipay = AliPay(
                appid=self.app_id,
                app_notify_url=None,
                app_private_key_string=self.private_key,
                alipay_public_key_string=self.public_key,
                sign_type="RSA2",
                debug=getattr(settings, "DEBUG", False),
            )
        except ImportError:
            logger.warning("支付宝SDK未安装")
            self.alipay = None

    def create_payment(
        self,
        order_no: str,
        amount: float,
        subject: str,
        return_url: Optional[str] = None,
        notify_url: Optional[str] = None,
    ) -> Optional[str]:
        """
        创建支付宝支付

        Args:
            order_no: 订单号
            amount: 金额
            subject: 商品标题
            return_url: 同步回调地址
            notify_url: 异步回调地址

        Returns:
            支付链接
        """
        if not self.alipay:
            return None

        try:
            order_string = self.alipay.api_alipay_trade_page_pay(
                out_trade_no=order_no,
                total_amount=str(amount),
                subject=subject,
                return_url=return_url,
                notify_url=notify_url,
            )

            # 生成支付URL
            base_url = "https://openapi.alipay.com/gateway.do"
            if getattr(settings, "DEBUG", False):
                base_url = "https://openapi.alipaydev.com/gateway.do"

            payment_url = f"{base_url}?{order_string}"
            return payment_url
        except Exception as e:
            logger.error(f"支付宝创建支付失败: {str(e)}")
            return None

    def verify_payment(self, data: Dict[str, Any]) -> bool:
        """验证支付宝回调"""
        if not self.alipay:
            return False

        try:
            signature = data.pop("sign")
            success = self.alipay.verify(data, signature)
            return success and data.get("trade_status") == "TRADE_SUCCESS"
        except Exception as e:
            logger.error(f"支付宝验证失败: {str(e)}")
            return False


class WechatPaymentService:
    """微信支付服务"""

    def __init__(self, runtime_config: Optional[Dict[str, Any]] = None):
        runtime = runtime_config or {}
        try:
            from wechatpy.pay import WeChatPay

            self.enabled = bool(runtime.get("wechat_enabled"))
            self.app_id = runtime.get("wechat_app_id") or getattr(
                settings, "WECHAT_APP_ID", ""
            )
            self.mch_id = runtime.get("wechat_mch_id") or getattr(
                settings, "WECHAT_MCH_ID", ""
            )
            self.api_key = runtime.get("wechat_api_key") or getattr(
                settings, "WECHAT_API_KEY", ""
            )

            if (
                not self.enabled
                or not self.app_id
                or not self.mch_id
                or not self.api_key
            ):
                self.wxpay = None
                return

            self.wxpay = WeChatPay(
                appid=self.app_id, api_key=self.api_key, mch_id=self.mch_id
            )
        except ImportError:
            logger.warning("微信支付SDK未安装")
            self.wxpay = None

    def create_native_payment(
        self, order_no: str, amount: float, body: str, notify_url: str
    ) -> Optional[Dict[str, Any]]:
        """
        创建微信Native支付（扫码支付）

        Args:
            order_no: 订单号
            amount: 金额
            body: 商品描述
            notify_url: 回调地址

        Returns:
            包含code_url的字典
        """
        if not self.wxpay:
            return None

        try:
            # 微信支付金额单位为分
            total_fee = int(amount * 100)

            result = self.wxpay.order.create(
                trade_type="NATIVE",
                body=body,
                out_trade_no=order_no,
                total_fee=total_fee,
                notify_url=notify_url,
            )

            if result.get("return_code") == "SUCCESS":
                return {
                    "code_url": result.get("code_url"),
                    "prepay_id": result.get("prepay_id"),
                }
            return None
        except Exception as e:
            logger.error(f"微信创建支付失败: {str(e)}")
            return None

    def verify_payment(self, data: Dict[str, Any]) -> bool:
        """验证微信支付回调"""
        if not self.wxpay:
            return False

        try:
            result = self.wxpay.parse_payment_result(data)
            return result.get("return_code") == "SUCCESS"
        except Exception as e:
            logger.error(f"微信验证失败: {str(e)}")
            return False


class PaymentService:
    """统一支付服务"""

    def _providers(self):
        runtime = get_runtime_system_config()
        return {
            "stripe": StripePaymentService(runtime),
            "alipay": AlipayPaymentService(runtime),
            "wechat": WechatPaymentService(runtime),
        }

    def create_payment(
        self, order: Order, payment_method: PaymentMethod, **kwargs
    ) -> Optional[Payment]:
        """
        创建支付

        Args:
            order: 订单对象
            payment_method: 支付方式
            **kwargs: 其他参数

        Returns:
            Payment对象
        """
        payment_no = (
            f"PAY{datetime.now().strftime('%Y%m%d%H%M%S')}{uuid.uuid4().hex[:8]}"
        )

        payment = Payment(
            id=str(uuid.uuid4()),
            order_id=order.id,
            payment_no=payment_no,
            payment_method=payment_method,
            amount=order.amount,
            currency=order.currency,
            status=PaymentStatus.PENDING,
        )

        providers = self._providers()

        # 根据支付方式创建支付
        if payment_method == PaymentMethod.STRIPE:
            result = providers["stripe"].create_payment_intent(
                amount=order.amount,
                currency=order.currency,
                metadata={"order_id": order.id, "order_no": order.order_no},
            )
            if result:
                payment.transaction_id = result["payment_intent_id"]
                payment.payment_url = result["client_secret"]

        elif payment_method == PaymentMethod.ALIPAY:
            payment_url = providers["alipay"].create_payment(
                order_no=order.order_no,
                amount=order.amount,
                subject=order.product_name or "许可证购买",
                return_url=kwargs.get("return_url"),
                notify_url=kwargs.get("notify_url"),
            )
            if payment_url:
                payment.payment_url = payment_url

        elif payment_method == PaymentMethod.WECHAT:
            result = providers["wechat"].create_native_payment(
                order_no=order.order_no,
                amount=order.amount,
                body=order.product_name or "许可证购买",
                notify_url=kwargs.get("notify_url", ""),
            )
            if result:
                payment.qr_code = result["code_url"]
                payment.transaction_id = result.get("prepay_id")

        return payment

    def verify_payment(
        self, payment: Payment, callback_data: Optional[Dict[str, Any]] = None
    ) -> bool:
        """
        验证支付

        Args:
            payment: 支付对象
            callback_data: 回调数据

        Returns:
            是否支付成功
        """
        providers = self._providers()

        if payment.payment_method == PaymentMethod.STRIPE:
            return providers["stripe"].verify_payment(payment.transaction_id)

        elif payment.payment_method == PaymentMethod.ALIPAY:
            if callback_data:
                return providers["alipay"].verify_payment(callback_data)

        elif payment.payment_method == PaymentMethod.WECHAT:
            if callback_data:
                return providers["wechat"].verify_payment(callback_data)

        return False


# 单例实例
payment_service = PaymentService()
