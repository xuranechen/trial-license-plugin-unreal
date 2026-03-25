"""
初始化系统配置表
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import Base, engine, SessionLocal
from app.models.system_config import SystemConfig
from app.services.config_service import get_or_create_system_config


def main():
    Base.metadata.create_all(bind=engine, tables=[SystemConfig.__table__])
    db = SessionLocal()
    try:
        get_or_create_system_config(db)
        print("✅ 系统配置表已初始化")
    finally:
        db.close()


if __name__ == "__main__":
    main()
