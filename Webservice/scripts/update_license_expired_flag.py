"""
为 licenses 表添加已过期状态字段
"""

import os
import sys

from sqlalchemy import text

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import engine


def main():
    try:
        with engine.connect() as conn:
            result = conn.execute(
                text(
                    "SELECT COUNT(*) FROM pragma_table_info('licenses') WHERE name='is_expired'"
                )
            )
            exists = result.scalar() > 0
            if exists:
                print("ℹ️  is_expired 字段已存在")
            else:
                conn.execute(
                    text("ALTER TABLE licenses ADD COLUMN is_expired BOOLEAN DEFAULT 0")
                )
                print("✅ 已添加字段: is_expired")
            conn.commit()
        print("✅ licenses 表过期状态字段更新完成")
    except Exception as exc:
        print(f"❌ 更新失败: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
