"""兼容入口：请优先使用 scripts/init_db.py。"""

from scripts.init_db import init_database


def main():
    print("⚠️ migrate_all.py 已合并到 init_db.py，正在调用统一入口...")
    init_database(init_admin=False)


if __name__ == "__main__":
    main()
