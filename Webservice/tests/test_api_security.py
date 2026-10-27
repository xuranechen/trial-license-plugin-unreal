"""API 安全相关回归测试。

* /license/purchase 必须校验真实支付，不能凭任意 payment_id 发永久证
* /license/debug-decrypt 必须管理员权限，且生产环境（DEBUG=False）不可用
* /payments/webhook/stripe 必须配置签名密钥并校验签名
"""

import json
import uuid
from datetime import datetime, timedelta

from app.config import settings
from app.models.application import Application
from app.models.license import License
from app.models.order import Order, OrderStatus
from app.models.payment import Payment, PaymentMethod, PaymentStatus
from app.models.user import User
from app.services.encryption import EncryptionService
from app.utils.jwt import create_access_token

MACHINE_ID = "MACHINE-ABC"


def _seed(db, *, is_admin=False, order_status=OrderStatus.PENDING,
          payment_status=PaymentStatus.PENDING, transaction_id="pi_test_123"):
    suffix = uuid.uuid4().hex[:8]
    user = User(
        id=str(uuid.uuid4()),
        username=f"user_{suffix}",
        email=f"user_{suffix}@example.com",
        hashed_password="x",
        is_active=True,
        is_admin=is_admin,
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
        machine_id=MACHINE_ID,
    )
    payment = Payment(
        id=str(uuid.uuid4()),
        order_id=order.id,
        payment_no=f"PAY{suffix}",
        payment_method=PaymentMethod.STRIPE,
        status=payment_status,
        amount=9.9,
        currency="CNY",
        transaction_id=transaction_id,
    )
    db.add_all([user, app, order, payment])
    db.commit()
    return user, app, order, payment


def _headers(user):
    return {"Authorization": f"Bearer {create_access_token({'sub': user.id})}"}


def _token(project_id, machine_id=MACHINE_ID):
    return EncryptionService.aes_encrypt(
        json.dumps(
            {
                "ProjectId": project_id,
                "MachineID": machine_id,
                "Time": "2026.10.27-09.00.00",
            }
        ),
        project_id,
    )


def _purchase_body(project_id):
    return {"project_id": project_id, "encrypted_token": _token(project_id)}


# ---------------------------------------------------------------------------
# /license/purchase
# ---------------------------------------------------------------------------


def test_purchase_requires_payment_id(client, db_session):
    _, app, _, _ = _seed(db_session)
    body = _purchase_body(app.project_id)  # 含 encrypted_token，但没有 payment_id

    resp = client.post("/api/v1/license/purchase", json=body)

    assert resp.status_code == 400
    assert "支付凭证" in resp.json()["detail"]


def test_purchase_rejects_unknown_payment(client, db_session):
    _, app, _, _ = _seed(db_session)
    body = _purchase_body(app.project_id)
    body["payment_id"] = "pi_does_not_exist"

    resp = client.post("/api/v1/license/purchase", json=body)

    assert resp.status_code == 402
    assert "无效" in resp.json()["detail"] or "未完成" in resp.json()["detail"]


def test_purchase_rejects_pending_payment(client, db_session):
    _, app, _, payment = _seed(db_session, payment_status=PaymentStatus.PENDING)
    body = _purchase_body(app.project_id)
    body["payment_id"] = payment.transaction_id

    resp = client.post("/api/v1/license/purchase", json=body)

    assert resp.status_code == 402


def test_purchase_rejects_success_payment_with_unpaid_order(client, db_session):
    _, app, _, payment = _seed(
        db_session,
        payment_status=PaymentStatus.SUCCESS,
        order_status=OrderStatus.PENDING,
    )
    body = _purchase_body(app.project_id)
    body["payment_id"] = payment.transaction_id

    resp = client.post("/api/v1/license/purchase", json=body)

    assert resp.status_code == 400
    assert "订单" in resp.json()["detail"]


def test_purchase_issues_license_once(client, db_session):
    _, app, order, payment = _seed(
        db_session,
        payment_status=PaymentStatus.SUCCESS,
        order_status=OrderStatus.PAID,
    )
    body = _purchase_body(app.project_id)
    body["payment_id"] = payment.transaction_id

    resp = client.post("/api/v1/license/purchase", json=body)

    assert resp.status_code == 200, resp.text
    payload = resp.json()
    assert payload["success"] is True
    assert payload["license_key"]
    assert payload["is_perpetual"] is True

    license_record = (
        db_session.query(License).filter(License.order_id == order.id).one()
    )
    assert license_record.machine_id == MACHINE_ID
    assert license_record.project_id == app.project_id

    # 同一订单不能重复领取
    resp2 = client.post("/api/v1/license/purchase", json=body)
    assert resp2.status_code == 400
    assert "已发放" in resp2.json()["detail"]


# ---------------------------------------------------------------------------
# /license/debug-decrypt
# ---------------------------------------------------------------------------


def test_debug_decrypt_requires_auth(client, db_session):
    _, app, _, _ = _seed(db_session)
    body = {
        "license_key": "whatever",
        "project_id": app.project_id,
        "machine_id": MACHINE_ID,
    }

    resp = client.post("/api/v1/license/debug-decrypt", json=body)

    assert resp.status_code in (401, 403)


def test_debug_decrypt_hidden_in_production(client, db_session, monkeypatch):
    user, app, _, _ = _seed(db_session, is_admin=True)
    monkeypatch.setattr(settings, "DEBUG", False)
    body = {
        "license_key": "whatever",
        "project_id": app.project_id,
        "machine_id": MACHINE_ID,
    }

    resp = client.post(
        "/api/v1/license/debug-decrypt", json=body, headers=_headers(user)
    )

    assert resp.status_code == 404


def test_debug_decrypt_works_for_admin_in_debug(client, db_session, monkeypatch):
    user, app, _, _ = _seed(db_session, is_admin=True)
    monkeypatch.setattr(settings, "DEBUG", True)
    license_key = EncryptionService.generate_license_key(
        duration_hours=24,
        expiry_date=datetime.now() + timedelta(days=1),
        passphrase=app.project_id,
        project_id=app.project_id,
        machine_id=MACHINE_ID,
        is_perpetual=False,
    )
    body = {
        "license_key": license_key,
        "project_id": app.project_id,
        "machine_id": MACHINE_ID,
    }

    resp = client.post(
        "/api/v1/license/debug-decrypt", json=body, headers=_headers(user)
    )

    assert resp.status_code == 200, resp.text
    payload = resp.json()
    assert payload["ok"] is True
    assert payload["checks"]["machine_match"] is True
    assert payload["checks"]["project_match"] is True


# ---------------------------------------------------------------------------
# /payments/webhook/stripe
# ---------------------------------------------------------------------------


def test_stripe_webhook_requires_configured_secret(client, monkeypatch):
    monkeypatch.setattr(settings, "STRIPE_WEBHOOK_SECRET", "")

    resp = client.post(
        "/api/v1/payments/webhook/stripe",
        content=b"{}",
        headers={"stripe-signature": "t=1,v1=deadbeef"},
    )

    assert resp.status_code == 503


def test_stripe_webhook_rejects_bad_signature(client, monkeypatch):
    monkeypatch.setattr(settings, "STRIPE_WEBHOOK_SECRET", "whsec_test_secret")

    resp = client.post(
        "/api/v1/payments/webhook/stripe",
        content=b'{"type":"payment_intent.succeeded"}',
        headers={"stripe-signature": "t=1,v1=deadbeef"},
    )

    assert resp.status_code == 400
