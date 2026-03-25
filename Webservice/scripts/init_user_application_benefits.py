"""初始化用户应用权益表"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import Base, engine
from app.models.user_application_benefit import UserApplicationBenefit


def main():
    Base.metadata.create_all(bind=engine, tables=[UserApplicationBenefit.__table__])
    print("✅ user_application_benefits 表已初始化")


if __name__ == "__main__":
    main()
