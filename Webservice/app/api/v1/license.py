"""
许可证API路由
"""

from fastapi import APIRouter, Depends, HTTPException, status, Response, BackgroundTasks
from sqlalchemy.orm import Session
from datetime import datetime, timedelta
from app.database import get_db
from app.schemas.license import (
    LicenseRequest,
    LicenseResponse,
    TrialAvailabilityResponse,
    ValidateLicenseRequest,
    ValidateLicenseResponse,
    LicenseDetail,
)
from app.services.encryption import EncryptionService
from app.services.email_service import email_service
from app.models.application import Application
from app.models.license import License, LicenseType
from app.models.user import User
from app.models.order import Order, OrderStatus
from app.utils.dependencies import get_current_user
from app.services.license_service import (
    generate_paid_license_for_order,
    get_effective_passphrase,
    get_effective_trial_policy,
)
from app.services.license_reminder_service import sync_license_expired_status
import uuid

router = APIRouter(prefix="/license", tags=["License"])


def _serialize_user_license(license_record: License) -> LicenseDetail:
    masked = bool(
        getattr(license_record, "is_revoked", False)
        or getattr(license_record, "is_expired", False)
    )
    license_key = "" if masked else license_record.license_key
    return LicenseDetail(
        id=license_record.id,
        license_key=license_key,
        project_id=license_record.project_id,
        machine_id=license_record.machine_id,
        user_id=license_record.user_id,
        email=license_record.email,
        company=license_record.company,
        license_type=getattr(
            license_record.license_type, "value", license_record.license_type
        ),
        duration_hours=license_record.duration_hours,
        generate_time=license_record.generate_time,
        expiry_time=license_record.expiry_time,
        is_perpetual=license_record.is_perpetual,
        is_expired=getattr(license_record, "is_expired", False),
        is_revoked=license_record.is_revoked,
        revoke_reason=license_record.revoke_reason,
        created_at=license_record.created_at,
    )


def _get_license_pricing(app: Application) -> dict:
    """获取应用对应的许可证价格配置。"""
    return {
        "daily": {
            "price": float(getattr(app, "daily_price", 9.9) or 9.9),
            "name": "天卡",
            "duration": "1天",
        },
        "weekly": {
            "price": float(getattr(app, "weekly_price", 49.9) or 49.9),
            "name": "周卡",
            "duration": "7天",
        },
        "monthly": {
            "price": float(getattr(app, "monthly_price", 149.9) or 149.9),
            "name": "月卡",
            "duration": "30天",
        },
        "quarterly": {
            "price": float(getattr(app, "quarterly_price", 399.9) or 399.9),
            "name": "季卡",
            "duration": "90天",
        },
        "yearly": {
            "price": float(getattr(app, "yearly_price", 999.9) or 999.9),
            "name": "年卡",
            "duration": "365天",
        },
        "perpetual": {
            "price": float(getattr(app, "perpetual_price", 2999.0) or 2999.0),
            "name": "永久授权",
            "duration": "永久",
        },
    }


def _get_trial_usage_info(
    db: Session, user_id: str, project_id: str, machine_id: str, trial_limit: int
) -> tuple[int, int]:
    used_attempts = (
        db.query(License)
        .filter(
            License.user_id == user_id,
            License.project_id == project_id,
            License.machine_id == machine_id,
            License.license_type == LicenseType.TRIAL,
        )
        .count()
    )
    remaining_attempts = max(trial_limit - used_attempts, 0)
    return used_attempts, remaining_attempts


@router.post("/request", response_model=LicenseResponse)
async def request_license(
    request: LicenseRequest,
    background_tasks: BackgroundTasks,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    申请许可证（统一入口）

    支持试用版和永久版：
    - trial: 免费试用，立即生成
    - perpetual: 付费购买，需要完成支付流程

    参数说明：
    - encrypted_token: 加密口令，服务端将从中解析出 machine_id
    - email: 可选参数，如未提供则使用当前用户邮箱
    """
    # 查询应用配置
    app = (
        db.query(Application)
        .filter(Application.project_id == request.project_id)
        .first()
    )

    if not app or not app.is_active:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在或已禁用"
        )

    # 从加密口令中解析 machine_id
    # 注意：UE 客户端使用 project_id 作为加密密钥
    try:
        import traceback

        print(f"[DEBUG] 开始解析加密令牌...")
        print(f"[DEBUG] project_id: {request.project_id}")
        print(f"[DEBUG] encrypted_token (前50字符): {request.encrypted_token[:50]}...")

        machine_id = EncryptionService.parse_encrypted_token(
            request.encrypted_token,
            request.project_id,  # UE 使用 ProjectId 作为加密密钥
        )

        print(f"[DEBUG] 成功解析，machine_id: {machine_id}")

    except ValueError as e:
        print(f"[ERROR] ValueError: {str(e)}")
        print(f"[ERROR] Traceback: {traceback.format_exc()}")
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail=f"加密口令无效: {str(e)}"
        )
    except Exception as e:
        print(f"[ERROR] Exception: {str(e)}")
        print(f"[ERROR] Traceback: {traceback.format_exc()}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"解析加密口令时发生错误: {str(e)}",
        )

    # 根据许可证类型处理
    if request.license_type == "trial":
        return await _generate_trial_license(
            request, current_user, app, db, machine_id, background_tasks
        )
    elif request.license_type in [
        "daily",
        "weekly",
        "monthly",
        "quarterly",
        "yearly",
        "perpetual",
    ]:
        # 所有付费类型都走订单流程
        return await _create_purchase_order(request, current_user, app, db, machine_id)
    else:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"不支持的许可证类型: {request.license_type}",
        )


@router.post("/trial-availability", response_model=TrialAvailabilityResponse)
async def get_trial_availability(
    request: LicenseRequest,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    app = (
        db.query(Application)
        .filter(
            Application.project_id == request.project_id, Application.is_active == True
        )
        .first()
    )
    if not app:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在或已禁用"
        )

    try:
        machine_id = EncryptionService.parse_encrypted_token(
            request.encrypted_token,
            get_effective_passphrase(app, request.project_id),
        )
    except ValueError as exc:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(exc))

    policy = get_effective_trial_policy(db, current_user.id, app)
    trial_limit = policy["trial_request_limit"]
    used_attempts, remaining_attempts = _get_trial_usage_info(
        db, current_user.id, request.project_id, machine_id, trial_limit
    )
    allowed = remaining_attempts > 0
    message = (
        f"当前设备还可申请 {remaining_attempts}/{trial_limit} 次试用许可证"
        if allowed
        else f"当前设备的试用许可证申请次数已用完（0/{trial_limit}）"
    )

    return TrialAvailabilityResponse(
        allowed=allowed,
        remaining_trial_requests=remaining_attempts,
        trial_request_limit=trial_limit,
        used_trial_requests=used_attempts,
        message=message,
    )


async def _generate_trial_license(
    request: LicenseRequest,
    current_user: User,
    app: Application,
    db: Session,
    machine_id: str,
    background_tasks: BackgroundTasks,
) -> LicenseResponse:
    """生成试用许可证"""

    policy = get_effective_trial_policy(db, current_user.id, app)
    trial_limit = policy["trial_request_limit"]
    effective_duration_hours = policy["trial_duration_hours"]
    used_attempts = (
        db.query(License)
        .filter(
            License.user_id == current_user.id,
            License.project_id == request.project_id,
            License.machine_id == machine_id,
            License.license_type == LicenseType.TRIAL,
        )
        .count()
    )
    remaining_before_apply = max(trial_limit - used_attempts, 0)

    if remaining_before_apply <= 0:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"该设备在当前应用下的试用许可证申请次数已用完（剩余 0/{trial_limit} 次）",
        )

    # 检查是否已有试用许可证，如果有则撤销旧的
    existing = (
        db.query(License)
        .filter(
            License.project_id == request.project_id,
            License.machine_id == machine_id,
            License.license_type == LicenseType.TRIAL,
            License.user_id == current_user.id,
            License.is_revoked == False,
        )
        .first()
    )

    if existing:
        # 撤销旧的试用许可证
        existing.is_revoked = True
        existing.revoke_reason = "用户重新申请试用许可证"
        existing.revoked_at = datetime.utcnow()

    # 生成试用许可证
    generate_time = datetime.now()
    expiry_time = generate_time + timedelta(hours=effective_duration_hours)

    try:
        license_key = EncryptionService.generate_license_key(
            duration_hours=effective_duration_hours,
            expiry_date=expiry_time,
            passphrase=get_effective_passphrase(app, request.project_id),
            project_id=request.project_id,
            machine_id=machine_id,
            is_perpetual=False,
        )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"生成许可证失败: {str(e)}",
        )

    # 保存到数据库（email使用请求中的或当前用户的，company字段已废弃设为None）
    license_record = License(
        license_key=license_key,
        project_id=request.project_id,
        machine_id=machine_id,
        email=request.email or current_user.email,
        company=None,
        user_id=current_user.id,
        license_type=LicenseType.TRIAL,
        duration_hours=effective_duration_hours,
        generate_time=generate_time,
        expiry_time=expiry_time,
        is_perpetual=False,
    )

    db.add(license_record)
    db.commit()
    db.refresh(license_record)

    remaining_after_apply = max(trial_limit - used_attempts - 1, 0)

    # 后台异步发送邮件（不阻塞API响应）
    def send_email_task():
        try:
            email_service.send_license_activation_email(
                to_email=request.email or current_user.email,
                user_name=current_user.full_name or current_user.username,
                license_key=license_key,
                app_name=app.project_name,
                expire_date=expiry_time,
            )
            print(f"[INFO] 邮件发送成功: {request.email or current_user.email}")
        except Exception as e:
            print(f"[ERROR] 后台邮件发送失败: {e}")

    background_tasks.add_task(send_email_task)

    return LicenseResponse(
        success=True,
        license_key=license_key,
        license_type="trial",
        expiry_date=expiry_time,
        duration_hours=effective_duration_hours,
        is_perpetual=False,
        message=f"试用许可证已生成，确认邮件正在后台发送，剩余试用申请次数 {remaining_after_apply}/{trial_limit}",
        remaining_trial_requests=remaining_after_apply,
        trial_request_limit=trial_limit,
    )


async def _create_purchase_order(
    request: LicenseRequest,
    current_user: User,
    app: Application,
    db: Session,
    machine_id: str,
) -> LicenseResponse:
    """创建购买订单"""

    # 定义不同许可证类型的价格和名称
    license_pricing = _get_license_pricing(app)

    # 获取许可证类型的配置
    license_config = license_pricing.get(request.license_type)
    if not license_config:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"无效的许可证类型: {request.license_type}",
        )

    # 生成订单号
    order_no = f"ORD{datetime.now().strftime('%Y%m%d%H%M%S')}{uuid.uuid4().hex[:8]}"

    # 创建订单（注意：需要在订单中保存 machine_id 以便后续生成许可证时使用）
    order = Order(
        id=str(uuid.uuid4()),
        user_id=current_user.id,
        application_id=app.id,
        order_no=order_no,
        amount=license_config["price"],
        currency="CNY",
        status=OrderStatus.PENDING,
        product_name=f"{app.project_name} - {license_config['name']}许可证",
        license_type=request.license_type,
        license_duration=license_config["duration"],
        machine_id=machine_id,
        quantity=1,
    )

    db.add(order)
    db.commit()
    db.refresh(order)

    # 返回订单信息，前端需要跳转到支付页面
    return LicenseResponse(
        success=False,
        license_key="",
        license_type=request.license_type,
        expiry_date=None,
        duration_hours=0,
        is_perpetual=(request.license_type == "perpetual"),
        message=f"订单已创建，请完成支付 - {license_config['name']} ¥{license_config['price']}",
        order_id=order.id,
        order_no=order_no,
        amount=license_config["price"],
        payment_url=f"/payment?order_id={order.id}",
    )


@router.post("/complete-purchase", response_model=LicenseResponse)
async def complete_purchase(
    order_id: str,
    payment_proof: str,  # 支付凭证（模拟）
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    完成购买并生成许可证

    在真实场景中，这个接口会在支付回调中被调用
    """
    # 查找订单
    order = (
        db.query(Order)
        .filter(Order.id == order_id, Order.user_id == current_user.id)
        .first()
    )

    if not order:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="订单不存在")

    if order.status == OrderStatus.PAID:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="订单已支付"
        )

    # 查找应用
    app = db.query(Application).filter(Application.id == order.application_id).first()
    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    # 更新订单状态并自动生成许可证
    order.status = OrderStatus.PAID
    order.paid_at = datetime.utcnow()
    db.commit()
    db.refresh(order)

    license_record = generate_paid_license_for_order(db, order)

    is_perpetual = bool(license_record.is_perpetual)
    duration_hours = float(license_record.duration_hours or 0)

    return LicenseResponse(
        success=True,
        license_key=license_record.license_key,
        license_type=order.license_type,
        expiry_date=None if is_perpetual else license_record.expiry_time,
        duration_hours=duration_hours,
        is_perpetual=is_perpetual,
        message="支付成功，许可证已自动生成并发送到您的邮箱",
    )


@router.get("/download/{license_id}")
async def download_license_file(
    license_id: int,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """下载许可证文件(.lic)"""

    license_record = (
        db.query(License)
        .filter(License.id == license_id, License.user_id == current_user.id)
        .first()
    )

    if not license_record:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="许可证不存在"
        )

    if license_record.is_revoked or getattr(license_record, "is_expired", False):
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="已失效的许可证不支持下载"
        )

    # 生成.lic文件内容
    file_content = license_record.license_key

    # 返回文件
    return Response(
        content=file_content,
        media_type="application/octet-stream",
        headers={
            "Content-Disposition": f"attachment; filename=license_{license_id}.lic"
        },
    )


@router.post("/request-trial", response_model=LicenseResponse)
async def request_trial_license(request: LicenseRequest, db: Session = Depends(get_db)):
    """
    申请试用许可证（保留旧接口兼容性）
    """
    # 查询应用配置
    app = (
        db.query(Application)
        .filter(Application.project_id == request.project_id)
        .first()
    )

    if not app or not app.is_active:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在或已禁用"
        )

    # 检查是否已有试用许可证
    existing = (
        db.query(License)
        .filter(
            License.project_id == request.project_id,
            License.machine_id == request.machine_id,
            License.license_type == LicenseType.TRIAL,
            License.is_revoked == False,
        )
        .first()
    )

    if existing:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="该设备已申请过试用许可证"
        )

    # 生成试用许可证
    generate_time = datetime.now()
    expiry_time = generate_time + timedelta(hours=app.trial_duration_hours)

    try:
        license_key = EncryptionService.generate_license_key(
            duration_hours=app.trial_duration_hours,
            expiry_date=expiry_time,
            passphrase=get_effective_passphrase(app, request.project_id),
            project_id=request.project_id,
            machine_id=request.machine_id,
            is_perpetual=False,
        )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"生成许可证失败: {str(e)}",
        )

    # 保存到数据库
    license_record = License(
        license_key=license_key,
        project_id=request.project_id,
        machine_id=request.machine_id,
        email=request.email,
        company=request.company,
        user_id=request.user_id,
        license_type=LicenseType.TRIAL,
        duration_hours=app.trial_duration_hours,
        generate_time=generate_time,
        expiry_time=expiry_time,
        is_perpetual=False,
    )

    db.add(license_record)
    db.commit()
    db.refresh(license_record)

    return LicenseResponse(
        success=True,
        license_key=license_key,
        license_type="trial",
        expiry_date=expiry_time,
        duration_hours=app.trial_duration_hours,
        is_perpetual=False,
        message="试用许可证已生成",
    )


@router.post("/purchase", response_model=LicenseResponse)
async def purchase_license(request: LicenseRequest, db: Session = Depends(get_db)):
    """
    购买正式许可证（永久授权）（保留旧接口兼容性）

    需要验证支付信息
    """
    # 查询应用配置
    app = (
        db.query(Application)
        .filter(Application.project_id == request.project_id)
        .first()
    )

    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    # TODO: 验证支付信息
    if not request.payment_id:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="缺少支付凭证"
        )

    # 生成永久许可证
    generate_time = datetime.now()
    expiry_time = datetime(2099, 12, 31)  # 永久许可证设置为遥远未来

    try:
        license_key = EncryptionService.generate_license_key(
            duration_hours=0,
            expiry_date=expiry_time,
            passphrase=get_effective_passphrase(app, request.project_id),
            project_id=request.project_id,
            machine_id=request.machine_id,
            is_perpetual=True,
        )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"生成许可证失败: {str(e)}",
        )

    # 保存到数据库
    license_record = License(
        license_key=license_key,
        project_id=request.project_id,
        machine_id=request.machine_id,
        email=request.email,
        company=request.company,
        user_id=request.user_id,
        license_type=LicenseType.PERPETUAL,
        duration_hours=0,
        generate_time=generate_time,
        expiry_time=expiry_time,
        is_perpetual=True,
    )

    db.add(license_record)
    db.commit()

    return LicenseResponse(
        success=True,
        license_key=license_key,
        license_type="perpetual",
        expiry_date=expiry_time,
        duration_hours=0,
        is_perpetual=True,
        message="永久许可证已激活",
    )


@router.post("/validate", response_model=ValidateLicenseResponse)
async def validate_license(
    request: ValidateLicenseRequest, db: Session = Depends(get_db)
):
    """
    在线验证许可证

    可选功能，用于额外的服务端验证
    """
    # 查询应用
    app = (
        db.query(Application)
        .filter(Application.project_id == request.project_id)
        .first()
    )

    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    # 使用加密服务验证
    is_valid, message, license_info = EncryptionService.validate_license(
        request.license_key,
        get_effective_passphrase(app, request.project_id),
        request.project_id,
        request.machine_id,
    )

    if not is_valid:
        return ValidateLicenseResponse(valid=False, message=message)

    # 检查是否被撤销
    license_record = (
        db.query(License).filter(License.license_key == request.license_key).first()
    )

    if license_record and license_record.is_revoked:
        return ValidateLicenseResponse(
            valid=False,
            message=f"许可证已被撤销: {license_record.revoke_reason or '未提供原因'}",
        )

    # 计算剩余时间
    remaining_time = EncryptionService.calculate_remaining_time(license_info)

    is_perpetual = bool(
        license_info.get("Perpetual", license_info.get("bPerpetual", False))
    )
    expiry_time = None
    if not is_perpetual:
        expiry_time = EncryptionService._parse_license_datetime(
            license_info["ExpireTime"]
        )

    return ValidateLicenseResponse(
        valid=True,
        remaining_time=remaining_time,
        is_perpetual=is_perpetual,
        expiry_date=expiry_time,
        message="许可证有效",
    )


@router.post("/debug-decrypt")
async def debug_decrypt_license(
    request: ValidateLicenseRequest, db: Session = Depends(get_db)
):
    """
    调试接口：解密并输出许可证 JSON 与关键校验结果。
    """
    app = (
        db.query(Application)
        .filter(Application.project_id == request.project_id)
        .first()
    )

    if not app:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="应用不存在")

    passphrase = get_effective_passphrase(app, request.project_id)

    try:
        decrypted_json = EncryptionService.aes_decrypt(request.license_key, passphrase)
        license_info = EncryptionService.parse_license_key(
            request.license_key, passphrase
        )
    except Exception as e:
        return {
            "ok": False,
            "message": f"解密失败: {str(e)}",
            "project_id": request.project_id,
            "machine_id": request.machine_id,
            "passphrase_used": passphrase,
        }

    perpetual = bool(
        license_info.get("Perpetual", license_info.get("bPerpetual", False))
    )
    machine_match = license_info.get("MachineID") == request.machine_id
    project_match = license_info.get("ProjectId") == request.project_id
    signature_valid = EncryptionService.validate_license_signature(
        license_info, passphrase
    )

    expiry_status = None
    if not perpetual and "ExpireTime" in license_info:
        try:
            expiry_time = EncryptionService._parse_license_datetime(
                license_info["ExpireTime"]
            )
            expiry_status = datetime.now() <= expiry_time
        except Exception as e:
            expiry_status = f"无法解析过期时间: {str(e)}"

    return {
        "ok": True,
        "project_id": request.project_id,
        "machine_id": request.machine_id,
        "passphrase_used": passphrase,
        "decrypted_json": decrypted_json,
        "parsed_license_info": license_info,
        "checks": {
            "project_match": project_match,
            "machine_match": machine_match,
            "signature_valid": signature_valid,
            "perpetual": perpetual,
            "not_expired": expiry_status,
        },
    }


@router.get("/my-licenses", response_model=list[LicenseDetail])
async def get_my_licenses(
    current_user: User = Depends(get_current_user), db: Session = Depends(get_db)
):
    """
    获取当前用户的所有许可证
    """
    sync_license_expired_status(db)

    licenses = (
        db.query(License)
        .filter(License.user_id == current_user.id)
        .order_by(License.generate_time.desc())
        .all()
    )

    return [_serialize_user_license(license) for license in licenses]


@router.get("/{license_id}", response_model=LicenseDetail)
async def get_license_detail(
    license_id: int,
    current_user: User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    """
    查询许可证详情
    """
    license_record = (
        db.query(License)
        .filter(License.id == license_id, License.user_id == current_user.id)
        .first()
    )

    if not license_record:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="许可证不存在"
        )

    return _serialize_user_license(license_record)
