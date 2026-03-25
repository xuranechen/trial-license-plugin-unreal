"""为 applications 表添加试用申请次数限制字段"""

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
                    "SELECT COUNT(*) FROM pragma_table_info('applications') WHERE name='trial_request_limit'"
                )
            )
            exists = result.scalar() > 0
            if exists:
                print("ℹ️  trial_request_limit 字段已存在")
            else:
                conn.execute(
                    text(
                        "ALTER TABLE applications ADD COLUMN trial_request_limit INTEGER DEFAULT 1"
                    )
                )
                print("✅ 已添加字段: trial_request_limit")
            conn.commit()
        print("✅ applications 表试用申请限制字段更新完成")
    except Exception as exc:
        print(f"❌ 更新失败: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
