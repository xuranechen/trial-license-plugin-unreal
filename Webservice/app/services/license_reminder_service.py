"""
许可证到期提醒服务
"""

from datetime import datetime, timedelta

from sqlalchemy.orm import Session

from app.models.application import Application
from app.models.license import License
from app.services.email_service import email_service


REMINDER_WINDOWS = [7, 3, 1]


def _attr_for_days(days_left: int) -> str:
    return f"reminder_{days_left}d_sent"


def send_license_expiry_notifications(db: Session) -> dict:
    now = datetime.now()
    licenses = (
        db.query(License)
        .filter(
            License.is_perpetual == False,
            License.is_revoked == False,
            License.expiry_time.isnot(None),
        )
        .all()
    )

    sent_upcoming = 0
    sent_expired = 0

    for license_record in licenses:
        if not license_record.email or not license_record.expiry_time:
            continue

        app = (
            db.query(Application)
            .filter(Application.project_id == license_record.project_id)
            .first()
        )
        app_name = app.project_name if app else license_record.project_id

        if license_record.expiry_time <= now:
            license_record.is_expired = True
            if not bool(getattr(license_record, "expired_notice_sent", False)):
                if email_service.send_license_expired_notice(
                    to_email=license_record.email,
                    user_name=license_record.email,
                    app_name=app_name,
                    expire_date=license_record.expiry_time,
                ):
                    license_record.expired_notice_sent = True
                    sent_expired += 1
            continue

        if bool(getattr(license_record, "is_expired", False)):
            license_record.is_expired = False

        time_left = license_record.expiry_time - now
        days_left = time_left.total_seconds() / 86400

        for window in REMINDER_WINDOWS:
            attr = _attr_for_days(window)
            if (
                days_left <= window
                and days_left > max(window - 1, 0)
                and not bool(getattr(license_record, attr, False))
            ):
                if email_service.send_license_expiry_reminder(
                    to_email=license_record.email,
                    user_name=license_record.email,
                    app_name=app_name,
                    expire_date=license_record.expiry_time,
                    days_left=window,
                ):
                    setattr(license_record, attr, True)
                    sent_upcoming += 1

    db.commit()
    return {
        "checked": len(licenses),
        "upcoming_sent": sent_upcoming,
        "expired_sent": sent_expired,
    }


def sync_license_expired_status(db: Session) -> int:
    now = datetime.now()
    licenses = (
        db.query(License)
        .filter(
            License.is_perpetual == False,
            License.expiry_time.isnot(None),
            License.is_revoked == False,
        )
        .all()
    )

    changed = 0
    for license_record in licenses:
        should_expire = bool(
            license_record.expiry_time and license_record.expiry_time <= now
        )
        if bool(getattr(license_record, "is_expired", False)) != should_expire:
            license_record.is_expired = should_expire
            changed += 1

    if changed:
        db.commit()
    return changed
