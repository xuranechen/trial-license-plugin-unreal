"""
更新 applications 表结构，添加许可证价格字段
"""

import os
import sys

from sqlalchemy import text

# 添加父目录到路径
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import engine


PRICE_COLUMNS = {
    "daily_price": 9.9,
    "weekly_price": 49.9,
    "monthly_price": 149.9,
    "quarterly_price": 399.9,
    "yearly_price": 999.9,
    "perpetual_price": 2999.0,
}


def update_application_pricing():
    """为 applications 表补充许可证价格字段。"""
    try:
        with engine.connect() as conn:
            for column_name, default_value in PRICE_COLUMNS.items():
                result = conn.execute(
                    text(
                        "SELECT COUNT(*) FROM pragma_table_info('applications') WHERE name=:column_name"
                    ),
                    {"column_name": column_name},
                )
                exists = result.scalar() > 0

                if exists:
                    print(f"ℹ️  {column_name} 字段已存在，无需添加")
                    continue

                conn.execute(
                    text(
                        f"ALTER TABLE applications ADD COLUMN {column_name} FLOAT DEFAULT {default_value}"
                    )
                )
                print(f"✅ 成功添加 {column_name} 字段到 applications 表")

            conn.commit()
    except Exception as e:
        print(f"❌ 更新 applications 表结构失败: {e}")
        return False

    return True


if __name__ == "__main__":
    print("正在更新 applications 表结构...")
    success = update_application_pricing()
    if success:
        print("✅ 数据库表结构更新完成")
    else:
        print("❌ 数据库表结构更新失败")
        sys.exit(1)
