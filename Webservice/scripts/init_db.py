"""
数据库完整初始化入口
"""

import argparse
import sys
import os

# 添加项目根目录到路径
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from app.database import Base, engine
from app.models import (  # noqa: F401  # 确保模型被导入，供 metadata 注册
    Application,
    License,
    Order,
    Payment,
    Subscription,
    SubscriptionPlan,
    SystemConfig,
    User,
    UserApplicationBenefit,
)
from scripts.init_system_config import main as init_system_config_main
from scripts.init_user_application_benefits import (
    main as init_user_application_benefits_main,
)
from scripts.update_application_pricing import update_application_pricing
from scripts.update_application_trial_limit import (
    main as update_application_trial_limit_main,
)
from scripts.update_license_expired_flag import main as update_license_expired_flag_main
from scripts.update_license_reminder_flags import (
    main as update_license_reminder_flags_main,
)
from scripts.update_license_table import update_license_table
from scripts.update_license_trial_count_flag import (
    main as update_license_trial_count_flag_main,
)
from scripts.update_no_watermark_benefits import (
    main as update_no_watermark_benefits_main,
)
from scripts.normalize_license_type_values import (
    main as normalize_license_type_values_main,
)
from scripts.update_orders_machine_id import update_orders_machine_id


def run_step(title, func):
    print(f"\n=== {title} ===")
    try:
        result = func()
        if result is False:
            raise RuntimeError(f"步骤失败: {title}")
    except SystemExit as exc:
        if exc.code not in (0, None):
            raise
    print(f"✅ {title} 完成")


def init_database(init_admin=False):
    """初始化数据库表、兼容迁移和基础数据。"""
    print("正在创建数据库表（create_all）...")
    Base.metadata.create_all(bind=engine)
    print("✅ 数据库表创建成功")

    # 对历史数据库执行幂等迁移（新库会自动跳过）
    run_step("applications 表价格字段迁移", update_application_pricing)
    run_step("applications 表试用次数限制迁移", update_application_trial_limit_main)
    run_step("orders 表 machine_id 迁移", update_orders_machine_id)
    run_step("licenses 表 order_id 迁移", update_license_table)
    run_step("licenses 表提醒字段迁移", update_license_reminder_flags_main)
    run_step("licenses 表过期标记迁移", update_license_expired_flag_main)
    run_step("licenses 表试用次数占用标记迁移", update_license_trial_count_flag_main)
    run_step("免水印权益字段迁移", update_no_watermark_benefits_main)
    run_step("licenses 表类型值归一化", normalize_license_type_values_main)

    # 初始化基础数据
    run_step("系统配置表初始化", init_system_config_main)
    run_step("用户应用权益表初始化", init_user_application_benefits_main)

    if init_admin:
        from scripts.init_admin import check_and_create_default_admin

        run_step("默认管理员初始化", check_and_create_default_admin)

    print("\n🎉 数据库初始化完成")


def parse_args():
    parser = argparse.ArgumentParser(description="Webservice 数据库完整初始化")
    parser.add_argument(
        "--init-admin",
        action="store_true",
        help="初始化默认管理员（读取 ADMIN_USERNAME/ADMIN_PASSWORD/ADMIN_EMAIL）",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    init_database(init_admin=args.init_admin)
