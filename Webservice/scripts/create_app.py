"""
创建应用脚本
"""

import sys
import os
import uuid

# 添加项目根目录到路径
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from app.database import SessionLocal
from app.models.application import Application


def create_application(
    project_id: str,
    project_name: str,
    passphrase: str,
    description: str = "",
    trial_duration_hours: float = 72.0,
):
    """创建一个新应用"""
    db = SessionLocal()
    try:
        # 检查是否已存在
        existing = (
            db.query(Application).filter(Application.project_id == project_id).first()
        )

        if existing:
            print(f"❌ 应用 '{project_id}' 已存在！")
            return False

        # 创建应用
        app = Application(
            id=str(uuid.uuid4()),
            project_id=project_id,
            project_name=project_name,
            description=description,
            passphrase=passphrase,
            trial_duration_hours=trial_duration_hours,
            is_active=True,
        )

        db.add(app)
        db.commit()

        print(f"✅ 应用创建成功！")
        print(f"   项目ID: {project_id}")
        print(f"   项目名称: {project_name}")
        print(f"   试用时长: {trial_duration_hours}小时")
        print(f"   加密口令: {passphrase}")

        return True

    except Exception as e:
        print(f"❌ 创建应用失败: {str(e)}")
        db.rollback()
        return False
    finally:
        db.close()


if __name__ == "__main__":
    print("=== 创建应用 ===\n")

    # 交互式输入
    project_id = input("请输入项目ID (例如: MyGame): ").strip()
    project_name = input("请输入项目名称: ").strip()
    passphrase = input("请输入加密口令 (强烈建议使用复杂口令): ").strip()
    description = input("请输入项目描述 (可选): ").strip()

    trial_hours_input = input("请输入试用时长（小时，默认72）: ").strip()
    trial_hours = float(trial_hours_input) if trial_hours_input else 72.0

    if not project_id or not project_name or not passphrase:
        print("❌ 项目ID、项目名称和加密口令不能为空！")
        sys.exit(1)

    create_application(
        project_id=project_id,
        project_name=project_name,
        passphrase=passphrase,
        description=description,
        trial_duration_hours=trial_hours,
    )
