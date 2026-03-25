"""
更新 orders 表结构，添加 machine_id 字段
"""

import os
import sys

from sqlalchemy import text

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import engine


def update_orders_machine_id():
    try:
        with engine.connect() as conn:
            result = conn.execute(
                text(
                    "SELECT COUNT(*) FROM pragma_table_info('orders') WHERE name='machine_id'"
                )
            )
            exists = result.scalar() > 0

            if exists:
                print("ℹ️  machine_id 字段已存在，无需添加")
            else:
                conn.execute(
                    text("ALTER TABLE orders ADD COLUMN machine_id VARCHAR(255)")
                )
                print("✅ 成功添加 machine_id 字段到 orders 表")

            conn.commit()
    except Exception as e:
        print(f"❌ 更新 orders 表结构失败: {e}")
        return False

    return True


if __name__ == "__main__":
    print("正在更新 orders 表结构...")
    success = update_orders_machine_id()
    if success:
        print("✅ 数据库表结构更新完成")
    else:
        print("❌ 数据库表结构更新失败")
        sys.exit(1)
