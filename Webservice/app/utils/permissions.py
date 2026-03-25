"""
权限验证工具
"""

from fastapi import Depends, HTTPException, status
from app.models.user import User
from app.utils.dependencies import get_current_user


def get_current_admin_user(current_user: User = Depends(get_current_user)) -> User:
    """
    获取当前管理员用户

    验证当前用户是否具有管理员权限
    如果不是管理员，抛出403 Forbidden异常

    Args:
        current_user: 当前登录的用户

    Returns:
        User: 管理员用户对象

    Raises:
        HTTPException: 如果用户不是管理员
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="需要管理员权限才能执行此操作"
        )
    return current_user


def get_current_active_user(current_user: User = Depends(get_current_user)) -> User:
    """
    获取当前活跃用户

    验证当前用户账号是否处于活跃状态
    如果账号被禁用，抛出403 Forbidden异常

    Args:
        current_user: 当前登录的用户

    Returns:
        User: 活跃用户对象

    Raises:
        HTTPException: 如果用户账号被禁用
    """
    if not current_user.is_active:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="账号已被禁用"
        )
    return current_user


def require_admin(current_user: User = Depends(get_current_user)):
    """
    装饰器：要求管理员权限

    这是一个简化的依赖函数，直接在路由中使用

    使用示例:
        @router.get("/admin-only")
        def admin_only_route(admin: User = Depends(require_admin)):
            return {"message": "Admin access granted"}

    Args:
        current_user: 当前登录的用户

    Raises:
        HTTPException: 如果用户不是管理员
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="需要管理员权限"
        )
    return current_user


def check_resource_owner(
    resource_user_id: str, current_user: User = Depends(get_current_user)
):
    """
    检查资源所有权

    验证当前用户是否是资源的所有者，或者是管理员
    管理员可以访问所有资源

    Args:
        resource_user_id: 资源所属用户ID
        current_user: 当前登录的用户

    Returns:
        bool: 是否有权限访问

    Raises:
        HTTPException: 如果没有权限访问该资源
    """
    # 管理员可以访问所有资源
    if current_user.is_admin:
        return True

    # 检查是否是资源所有者
    if current_user.id != resource_user_id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="无权访问该资源"
        )

    return True


class PermissionChecker:
    """
    权限检查器类

    提供灵活的权限检查功能
    """

    @staticmethod
    def is_admin(user: User) -> bool:
        """检查用户是否是管理员"""
        return user.is_admin if user else False

    @staticmethod
    def is_active(user: User) -> bool:
        """检查用户是否活跃"""
        return user.is_active if user else False

    @staticmethod
    def is_owner_or_admin(user: User, resource_user_id: str) -> bool:
        """检查用户是否是资源所有者或管理员"""
        if not user:
            return False
        return user.is_admin or user.id == resource_user_id

    @staticmethod
    def can_manage_users(user: User) -> bool:
        """检查用户是否可以管理其他用户"""
        return user.is_admin if user else False

    @staticmethod
    def can_manage_licenses(user: User) -> bool:
        """检查用户是否可以管理许可证"""
        return user.is_admin if user else False

    @staticmethod
    def can_view_statistics(user: User) -> bool:
        """检查用户是否可以查看统计数据"""
        return user.is_admin if user else False
