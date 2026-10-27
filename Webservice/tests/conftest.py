"""pytest 全局配置与测试夹具。

* 把 Webservice 根目录加入 sys.path，便于 `import app`
* 提供基于内存 SQLite 的数据库 / FastAPI 客户端夹具
* 屏蔽真实 SMTP，避免测试触碰网络
"""

import sys
from pathlib import Path

import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import app.models  # noqa: E402,F401 - 导入所有模型以注册表结构
from app.database import Base, get_db  # noqa: E402
from app.main import app as fastapi_app  # noqa: E402
from app.services import license_service  # noqa: E402


@pytest.fixture(autouse=True)
def _no_email(monkeypatch):
    """测试中不发送真实邮件（否则会读取真实数据库并尝试连 SMTP）。"""
    monkeypatch.setattr(
        license_service.email_service,
        "send_license_activation_email",
        lambda *args, **kwargs: True,
    )
    monkeypatch.setattr(
        license_service.email_service,
        "send_payment_success_email",
        lambda *args, **kwargs: True,
    )


@pytest.fixture()
def db_engine(tmp_path):
    # 使用临时文件（而非内存库），让请求会话与测试会话各自持有独立连接，
    # 避免共享同一 SQLite 连接导致的事务互相干扰。
    engine = create_engine(
        f"sqlite:///{tmp_path / 'test.db'}",
        connect_args={"check_same_thread": False},
    )
    Base.metadata.create_all(bind=engine)
    yield engine
    Base.metadata.drop_all(bind=engine)
    engine.dispose()


@pytest.fixture()
def session_factory(db_engine):
    return sessionmaker(bind=db_engine, autocommit=False, autoflush=False)


@pytest.fixture()
def db_session(session_factory):
    session = session_factory()
    try:
        yield session
    finally:
        session.close()


@pytest.fixture()
def client(session_factory):
    def override_get_db():
        db = session_factory()
        try:
            yield db
        finally:
            db.close()

    fastapi_app.dependency_overrides[get_db] = override_get_db
    # 不使用 with 上下文，避免触发 startup（会初始化真实数据库 / 启动提醒任务）
    yield TestClient(fastapi_app)
    fastapi_app.dependency_overrides.clear()
