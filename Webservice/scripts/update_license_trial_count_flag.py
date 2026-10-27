"""为 licenses 表添加试用次数占用标记字段"""

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
                    "SELECT COUNT(*) FROM pragma_table_info('licenses') WHERE name='counts_against_trial_limit'"
                )
            )
            exists = result.scalar() > 0
            if exists:
                print("ℹ️  counts_against_trial_limit 字段已存在")
            else:
                conn.execute(
                    text(
                        "ALTER TABLE licenses ADD COLUMN counts_against_trial_limit BOOLEAN DEFAULT 1"
                    )
                )
                print("✅ 已添加字段: counts_against_trial_limit")
            conn.commit()
        print("✅ licenses 表试用次数标记字段更新完成")
    except Exception as exc:
        print(f"❌ 更新失败: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
