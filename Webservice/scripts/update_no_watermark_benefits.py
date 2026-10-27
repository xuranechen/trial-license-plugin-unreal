"""为试用免水印权益补充数据库字段"""

import os
import sys

from sqlalchemy import text

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import engine


def _ensure_column(conn, table_name: str, column_name: str, ddl: str) -> None:
    exists_result = conn.execute(
        text(
            f"SELECT COUNT(*) FROM pragma_table_info('{table_name}') WHERE name=:name"
        ),
        {"name": column_name},
    )
    exists = exists_result.scalar() > 0
    if exists:
        print(f"ℹ️  {table_name}.{column_name} 已存在")
        return
    conn.execute(text(f"ALTER TABLE {table_name} ADD COLUMN {ddl}"))
    print(f"✅ 已添加字段: {table_name}.{column_name}")


def main():
    try:
        with engine.connect() as conn:
            _ensure_column(
                conn,
                "user_application_benefits",
                "extra_no_watermark_trial_requests",
                "extra_no_watermark_trial_requests FLOAT DEFAULT 0",
            )
            _ensure_column(
                conn,
                "user_application_benefits",
                "allow_no_watermark_trial",
                "allow_no_watermark_trial BOOLEAN DEFAULT 0",
            )
            _ensure_column(
                conn,
                "licenses",
                "show_watermark",
                "show_watermark BOOLEAN DEFAULT 1",
            )
            _ensure_column(
                conn,
                "licenses",
                "uses_no_watermark_benefit",
                "uses_no_watermark_benefit BOOLEAN DEFAULT 0",
            )
            conn.commit()
        print("✅ 免水印权益相关字段迁移完成")
    except Exception as exc:
        print(f"❌ 迁移失败: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
