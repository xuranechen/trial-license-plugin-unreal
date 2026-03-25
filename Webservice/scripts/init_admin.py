"""
初始化管理员账号脚本
"""

import sys
import os
from pathlib import Path

# 添加项目根目录到Python路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from sqlalchemy.orm import Session
from app.database import SessionLocal, engine, Base
from app.models.user import User
from app.utils.jwt import get_password_hash
import uuid
from datetime import datetime


def create_admin_user(
    username: str = "admin",
    password: str = "admin123",
    email: str = "admin@example.com",
):
    """
    创建管理员账号

    Args:
        username: 管理员用户名
        password: 管理员密码
        email: 管理员邮箱
    """
    # 创建数据库会话
    db: Session = SessionLocal()

    try:
        # 检查是否已存在管理员账号
        existing_admin = db.query(User).filter(User.is_admin == True).first()

        if existing_admin:
            print(f"⚠️  管理员账号已存在: {existing_admin.username}")
            response = input("是否要创建额外的管理员账号？(y/n): ")
            if response.lower() != "y":
                print("❌ 操作已取消")
                return

        # 检查用户名是否已存在
        existing_user = db.query(User).filter(User.username == username).first()
        if existing_user:
            print(f"❌ 用户名 '{username}' 已被使用")
            return

        # 检查邮箱是否已存在
        existing_email = db.query(User).filter(User.email == email).first()
        if existing_email:
            print(f"❌ 邮箱 '{email}' 已被使用")
            return

        # 创建管理员用户
        admin_user = User(
            id=str(uuid.uuid4()),
            username=username,
            email=email,
            hashed_password=get_password_hash(password),
            full_name="系统管理员",
            company="System",
            is_active=True,
            is_admin=True,  # 设置为管理员
            created_at=datetime.now(),
            updated_at=datetime.now(),
        )

        db.add(admin_user)
        db.commit()
        db.refresh(admin_user)

        print("\n" + "=" * 50)
        print("✅ 管理员账号创建成功！")
        print("=" * 50)
        print(f"用户名: {username}")
        print(f"密码:   {password}")
        print(f"邮箱:   {email}")
        print("=" * 50)
        print("\n⚠️  请立即登录并修改密码！")
        print(f"管理后台: http://localhost:8000/static/admin.html\n")

    except Exception as e:
        db.rollback()
        print(f"❌ 创建管理员失败: {str(e)}")
        raise
    finally:
        db.close()


def interactive_create_admin():
    """
    交互式创建管理员账号
    """
    print("\n" + "=" * 50)
    print("  许可证管理系统 - 管理员账号初始化")
    print("=" * 50 + "\n")

    # 获取用户输入
    username = input("请输入管理员用户名 (默认: admin): ").strip() or "admin"

    while True:
        password = input("请输入管理员密码 (至少6个字符): ").strip()
        if len(password) >= 6:
            password_confirm = input("请再次输入密码: ").strip()
            if password == password_confirm:
                break
            else:
                print("❌ 两次密码输入不一致，请重新输入\n")
        else:
            print("❌ 密码长度至少需要6个字符\n")

    email = (
        input("请输入管理员邮箱 (默认: admin@example.com): ").strip()
        or "admin@example.com"
    )

    # 确认信息
    print("\n" + "-" * 50)
    print("请确认以下信息:")
    print(f"用户名: {username}")
    print(f"密码:   {'*' * len(password)}")
    print(f"邮箱:   {email}")
    print("-" * 50)

    confirm = input("\n确认创建？(y/n): ").strip().lower()
    if confirm == "y":
        create_admin_user(username, password, email)
    else:
        print("❌ 操作已取消")


def check_and_create_default_admin():
    """
    检查是否存在管理员，如果不存在则创建默认管理员
    用于启动时自动初始化
    """
    db: Session = SessionLocal()

    try:
        # 检查是否已存在管理员
        admin_count = db.query(User).filter(User.is_admin == True).count()

        if admin_count == 0:
            print("\n⚠️  未检测到管理员账号，正在创建默认管理员...")

            # 尝试从环境变量读取
            admin_username = os.getenv("ADMIN_USERNAME", "admin")
            admin_password = os.getenv("ADMIN_PASSWORD", "admin123")
            admin_email = os.getenv("ADMIN_EMAIL", "admin@example.com")

            # 创建默认管理员
            admin_user = User(
                id=str(uuid.uuid4()),
                username=admin_username,
                email=admin_email,
                hashed_password=get_password_hash(admin_password),
                full_name="系统管理员",
                company="System",
                is_active=True,
                is_admin=True,
                created_at=datetime.now(),
                updated_at=datetime.now(),
            )

            db.add(admin_user)
            db.commit()

            print("\n" + "=" * 50)
            print("✅ 默认管理员账号已创建！")
            print("=" * 50)
            print(f"用户名: {admin_username}")
            print(f"密码:   {admin_password}")
            print(f"邮箱:   {admin_email}")
            print("=" * 50)
            print("\n⚠️  请立即登录并修改密码！")
            print(f"管理后台: http://localhost:8000/static/admin.html\n")

            return True
        else:
            print(f"✅ 检测到 {admin_count} 个管理员账号")
            return False

    except Exception as e:
        db.rollback()
        print(f"❌ 初始化管理员失败: {str(e)}")
        return False
    finally:
        db.close()


if __name__ == "__main__":
    # 确保数据库表已创建
    Base.metadata.create_all(bind=engine)

    # 如果有命令行参数，使用交互式创建
    if len(sys.argv) > 1:
        if sys.argv[1] == "--auto":
            check_and_create_default_admin()
        elif sys.argv[1] == "--help":
            print(
                """
使用方法:
    python scripts/init_admin.py              # 交互式创建管理员
    python scripts/init_admin.py --auto       # 自动创建默认管理员
    python scripts/init_admin.py --help       # 显示帮助信息
            """
            )
        else:
            interactive_create_admin()
    else:
        # 默认使用交互式创建
        interactive_create_admin()
