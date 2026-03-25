"""
发送许可证即将到期/已到期提醒
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import SessionLocal
from app.services.license_reminder_service import send_license_expiry_notifications


def main():
    db = SessionLocal()
    try:
        result = send_license_expiry_notifications(db)
        print(
            f"✅ 检查 {result['checked']} 个许可证，发送即将到期提醒 {result['upcoming_sent']} 封，发送已到期通知 {result['expired_sent']} 封"
        )
    finally:
        db.close()


if __name__ == "__main__":
    main()
