"""
为 licenses 表添加到期提醒状态字段
"""

import os
import sys

from sqlalchemy import text

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import engine


REMINDER_COLUMNS = {
    "reminder_7d_sent": 0,
    "reminder_3d_sent": 0,
    "reminder_1d_sent": 0,
    "expired_notice_sent": 0,
}


def main():
    try:
        with engine.connect() as conn:
            for column_name, default_value in REMINDER_COLUMNS.items():
                result = conn.execute(
                    text(
                        "SELECT COUNT(*) FROM pragma_table_info('licenses') WHERE name=:name"
                    ),
                    {"name": column_name},
                )
                exists = result.scalar() > 0
                if exists:
                    print(f"ℹ️  {column_name} 字段已存在")
                    continue
                conn.execute(
                    text(
                        f"ALTER TABLE licenses ADD COLUMN {column_name} BOOLEAN DEFAULT {default_value}"
                    )
                )
                print(f"✅ 已添加字段: {column_name}")
            conn.commit()
        print("✅ licenses 表提醒字段更新完成")
    except Exception as exc:
        print(f"❌ 更新失败: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
