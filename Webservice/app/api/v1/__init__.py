"""
API v1版本路由
"""

from fastapi import APIRouter
from app.api.v1 import license, admin, auth, orders, statistics

api_router = APIRouter()

# 注册所有路由
api_router.include_router(auth.router)
api_router.include_router(license.router)
api_router.include_router(admin.router)
api_router.include_router(orders.router)
api_router.include_router(statistics.router)
