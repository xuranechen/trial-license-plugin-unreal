"""将 licenses.license_type 历史大写值归一化为小写"""

import os
import sys

from sqlalchemy import text

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import engine


def main():
    try:
        with engine.begin() as conn:
            conn.execute(
                text(
                    """
                    UPDATE licenses
                    SET license_type = LOWER(license_type)
                    WHERE license_type IN (
                        'TRIAL', 'DAILY', 'WEEKLY', 'MONTHLY',
                        'QUARTERLY', 'YEARLY', 'PERPETUAL', 'SUBSCRIPTION'
                    )
                    """
                )
            )
        print("✅ licenses.license_type 已归一化为小写")
    except Exception as exc:
        print(f"❌ 归一化失败: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
