"""
更新 License 表结构，添加 order_id 字段
"""
import sys
import os

# 添加父目录到路径
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from sqlalchemy import text
from app.database import engine

def update_license_table():
    """更新 License 表，添加 order_id 字段"""
    try:
        with engine.connect() as conn:
            # 检查字段是否已存在
            result = conn.execute(text(
                "SELECT COUNT(*) FROM pragma_table_info('licenses') WHERE name='order_id'"
            ))
            exists = result.scalar() > 0
            
            if not exists:
                # 添加 order_id 字段
                conn.execute(text(
                    "ALTER TABLE licenses ADD COLUMN order_id VARCHAR(255)"
                ))
                conn.commit()
                print("✅ 成功添加 order_id 字段到 licenses 表")
            else:
                print("ℹ️  order_id 字段已存在，无需添加")
                
    except Exception as e:
        print(f"❌ 更新表结构失败: {e}")
        return False
    
    return True

if __name__ == "__main__":
    print("正在更新 License 表结构...")
    success = update_license_table()
    if success:
        print("✅ 数据库表结构更新完成")
    else:
        print("❌ 数据库表结构更新失败")
        sys.exit(1)
