"""
认证相关Schema
"""
from pydantic import BaseModel, EmailStr, Field
from typing import Optional


class UserRegister(BaseModel):
    """用户注册"""
    username: str = Field(..., min_length=3, max_length=50, description="用户名")
    email: EmailStr = Field(..., description="邮箱")
    password: str = Field(..., min_length=6, description="密码")
    full_name: Optional[str] = Field(None, max_length=255, description="全名")
    company: Optional[str] = Field(None, max_length=255, description="公司")


class UserLogin(BaseModel):
    """用户登录"""
    username: str = Field(..., description="用户名或邮箱")
    password: str = Field(..., description="密码")


class Token(BaseModel):
    """令牌响应"""
    access_token: str = Field(..., description="访问令牌")
    token_type: str = Field(default="bearer", description="令牌类型")
    expires_in: int = Field(..., description="过期时间（秒）")


class TokenData(BaseModel):
    """令牌数据"""
    user_id: Optional[str] = None
    username: Optional[str] = None


class PasswordChange(BaseModel):
    """修改密码"""
    old_password: str = Field(..., description="旧密码")
    new_password: str = Field(..., min_length=6, description="新密码")


class PasswordReset(BaseModel):
    """重置密码"""
    email: EmailStr = Field(..., description="邮箱")


class UserResponse(BaseModel):
    """用户响应"""
    id: str
    username: str
    email: str
    full_name: Optional[str] = None
    company: Optional[str] = None
    is_active: bool
    is_admin: bool
    
    class Config:
        from_attributes = True


class UserUpdate(BaseModel):
    """管理员信息更新"""
    full_name: Optional[str] = Field(None, max_length=255, description="全名")
    company: Optional[str] = Field(None, max_length=255, description="公司")
    email: Optional[EmailStr] = Field(None, description="邮箱")
