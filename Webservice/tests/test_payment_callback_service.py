"""支付回调状态流转测试。

覆盖 /payments/webhook/* 三个渠道共用的核心逻辑：
支付成功 -> 订单已支付 -> 签发许可证，且整个过程幂等。
"""

import uuid

from app.models.application import Application
from app.models.license import License
from app.models.order import Order, OrderStatus
from app.models.payment import Payment, PaymentMethod, PaymentStatus
from app.models.user import User
from app.services.payment_callback_service import (
    apply_payment_success,
    find_payment,
    find_payment_by_order_no,
)


def _seed(db, *, order_status=OrderStatus.PENDING, payment_status=PaymentStatus.PENDING):
    suffix = uuid.uuid4().hex[:8]
    user = User(
        id=str(uuid.uuid4()),
        username=f"user_{suffix}",
        email=f"user_{suffix}@example.com",
        hashed_password="x",
        is_active=True,
        is_admin=False,
    )
    app = Application(
        id=str(uuid.uuid4()),
        project_id=f"Proj_{suffix}",
        project_name="Demo",
        passphrase=f"Proj_{suffix}",
        is_active=True,
    )
    order = Order(
        id=str(uuid.uuid4()),
        user_id=user.id,
        application_id=app.id,
        order_no=f"ORD{suffix}",
        amount=9.9,
        status=order_status,
        license_type="perpetual",
        machine_id="MACHINE-ABC",
    )
    payment = Payment(
        id=str(uuid.uuid4()),
        order_id=order.id,
        payment_no=f"PAY{suffix}",
        payment_method=PaymentMethod.STRIPE,
        status=payment_status,
        amount=9.9,
        currency="CNY",
    )
    db.add_all([user, app, order, payment])
    db.commit()
    return app, order, payment


def test_apply_payment_success_marks_order_and_issues_license(db_session):
    app, order, payment = _seed(db_session)

    result_payment, result_order, license_record = apply_payment_success(
        db_session, payment, callback_data='{"ok":true}'
    )

    assert result_payment.status == PaymentStatus.SUCCESS
    assert result_payment.paid_at is not None
    assert result_payment.callback_data == '{"ok":true}'

    assert result_order.status == OrderStatus.PAID
    assert result_order.paid_at is not None

    assert license_record is not None
    assert license_record.order_id == order.id
    assert license_record.machine_id == "MACHINE-ABC"
    assert license_record.project_id == app.project_id
    assert license_record.is_perpetual is True
    assert license_record.user_id == order.user_id


def test_apply_payment_success_is_idempotent(db_session):
    _, order, payment = _seed(db_session)

    apply_payment_success(db_session, payment)
    apply_payment_success(db_session, payment)  # 重复回调

    assert db_session.query(License).filter(License.order_id == order.id).count() == 1


def test_refunded_payment_ignores_late_success(db_session):
    _, order, payment = _seed(db_session, payment_status=PaymentStatus.REFUNDED)

    result_payment, result_order, license_record = apply_payment_success(
        db_session, payment
    )

    assert result_payment.status == PaymentStatus.REFUNDED
    assert result_order.status == OrderStatus.PENDING
    assert license_record is None
    assert db_session.query(License).count() == 0


def test_missing_machine_id_does_not_crash_callback(db_session):
    _, order, payment = _seed(db_session)
    order.machine_id = None
    db_session.commit()

    result_payment, result_order, license_record = apply_payment_success(
        db_session, payment
    )

    # 钱已到账：支付/订单状态仍要落库，只是发证失败
    assert result_payment.status == PaymentStatus.SUCCESS
    assert result_order.status == OrderStatus.PAID
    assert license_record is None
    assert db_session.query(License).count() == 0


def test_find_helpers(db_session):
    _, order, payment = _seed(db_session)

    assert find_payment(db_session, payment.payment_no).id == payment.id
    assert find_payment(db_session, payment.id).id == payment.id
    assert find_payment(db_session, "does-not-exist") is None
    assert find_payment_by_order_no(db_session, order.order_no).id == payment.id
    assert find_payment_by_order_no(db_session, "ORD-NOPE") is None
