"""FastAPI主应用"""

import asyncio
from datetime import datetime, timedelta

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from app.config import settings
from app.database import init_db, SessionLocal
from app.api.v1 import api_router
from app.services.license_reminder_service import send_license_expiry_notifications


license_reminder_task = None


def run_license_reminder_job():
    db = SessionLocal()
    try:
        result = send_license_expiry_notifications(db)
        print(
            f"📬 许可证提醒任务完成: 检查 {result['checked']} 个，"
            f"即将到期 {result['upcoming_sent']} 封，已到期 {result['expired_sent']} 封"
        )
    except Exception as exc:
        print(f"⚠️  许可证提醒任务执行失败: {exc}")
    finally:
        db.close()


def seconds_until_next_run() -> float:
    now = datetime.now()
    next_run = now.replace(
        hour=settings.LICENSE_REMINDER_DAILY_HOUR,
        minute=settings.LICENSE_REMINDER_DAILY_MINUTE,
        second=0,
        microsecond=0,
    )
    if next_run <= now:
        next_run = next_run + timedelta(days=1)
    return max((next_run - now).total_seconds(), 1.0)


async def license_reminder_loop():
    while True:
        await asyncio.to_thread(run_license_reminder_job)
        await asyncio.sleep(seconds_until_next_run())


# 创建FastAPI应用
app = FastAPI(
    title=settings.APP_NAME,
    version=settings.APP_VERSION,
    description="许可证授权管理服务 - 与UE插件加密算法完全兼容",
    docs_url="/docs",
    redoc_url="/redoc",
)

# 配置CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 注册所有API路由
app.include_router(api_router, prefix="/api/v1")

# 提供静态文件服务
app.mount("/static", StaticFiles(directory="static"), name="static")


@app.on_event("startup")
async def startup_event():
    """应用启动事件"""
    global license_reminder_task

    print(f"🚀 {settings.APP_NAME} v{settings.APP_VERSION} 正在启动...")
    print(f"📚 API文档: http://{settings.HOST}:{settings.PORT}/docs")
    print(f"🔒 加密算法: AES-256-CBC + SHA256")
    print(
        f"💾 数据库: {settings.DATABASE_URL.split('@')[-1] if '@' in settings.DATABASE_URL else 'SQLite'}"
    )

    # 初始化数据库（仅用于开发环境）
    if settings.DEBUG:
        init_db()
        print("✅ 数据库表已初始化")

    # 检查并创建默认管理员账号
    try:
        from scripts.init_admin import check_and_create_default_admin

        check_and_create_default_admin()
    except Exception as e:
        print(f"⚠️  管理员初始化警告: {str(e)}")

    if settings.LICENSE_REMINDER_ENABLED:
        license_reminder_task = asyncio.create_task(license_reminder_loop())
        print(
            f"⏰ 许可证到期提醒任务已启动，每天 {settings.LICENSE_REMINDER_DAILY_HOUR:02d}:{settings.LICENSE_REMINDER_DAILY_MINUTE:02d} 执行"
        )


@app.on_event("shutdown")
async def shutdown_event():
    """应用关闭事件"""
    global license_reminder_task

    if license_reminder_task:
        license_reminder_task.cancel()
        try:
            await license_reminder_task
        except asyncio.CancelledError:
            pass
        license_reminder_task = None

    print("👋 服务正在关闭...")


@app.get("/")
async def root():
    """根路径"""
    return {
        "name": settings.APP_NAME,
        "version": settings.APP_VERSION,
        "status": "running",
        "docs": "/docs",
        "redoc": "/redoc",
    }


@app.get("/health")
async def health_check():
    """健康检查"""
    return {"status": "healthy", "version": settings.APP_VERSION}
