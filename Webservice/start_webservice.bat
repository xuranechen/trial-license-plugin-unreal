@echo off
chcp 65001 >nul

set PYTHONIOENCODING=utf-8

setlocal enabledelayedexpansion

rem 设置虚拟环境名称
set ENV_NAME=trial-license

echo.
echo ========================================
echo  License Webservice 启动脚本
echo ========================================
echo.

rem -----------------------------------------------
rem 自动定位 conda.bat（无需依赖 conda init）
rem -----------------------------------------------
set "CONDA_BAT="

rem 优先匹配常见的 condabin\conda.bat 路径
for %%P in (
    "%USERPROFILE%\miniconda3\condabin\conda.bat"
    "%USERPROFILE%\anaconda3\condabin\conda.bat"
    "C:\\ProgramData\\Miniconda3\\condabin\\conda.bat"
    "C:\\ProgramData\\Anaconda3\\condabin\\conda.bat"
    "%ProgramFiles%\Anaconda3\condabin\conda.bat"
    "D:\\Software\\Anaconda\condabin\conda.bat"
) do (
    if exist %%~P set "CONDA_BAT=%%~P"
)

rem 若仍未找到，尝试查找 conda.bat
if not defined CONDA_BAT (
    for /f "delims=" %%i in ('where conda.bat 2^>nul') do (
        set "CONDA_BAT=%%i"
        goto :found_conda
    )
)

rem 退而求其次：查找 conda.exe 并根据其位置推断 conda.bat/activate.bat
if not defined CONDA_BAT (
    for /f "delims=" %%i in ('where conda 2^>nul') do (
        set "CONDA_EXE=%%i"
        for %%j in ("%%i") do set "CONDA_DIR=%%~dpj"
        for %%k in ("!CONDA_DIR!..") do set "CONDA_ROOT=%%~fk"
        if exist "!CONDA_ROOT!\condabin\conda.bat" set "CONDA_BAT=!CONDA_ROOT!\condabin\conda.bat"
        if not defined CONDA_BAT if exist "!CONDA_DIR!conda.bat" set "CONDA_BAT=!CONDA_DIR!conda.bat"
        goto :found_conda
    )
)

:found_conda

if not defined CONDA_BAT (
    echo [错误] 未找到 conda.bat，请安装 Miniconda/Anaconda 或将 conda 加入 PATH
    echo 典型路径: %%USERPROFILE%%\miniconda3 或 C:\\ProgramData\\Miniconda3
    echo.
    pause
    exit /b 1
)

echo [成功] 找到 Conda: %CONDA_BAT%
echo.

rem 检查虚拟环境是否存在
echo.
echo [检查] 正在检查虚拟环境 %ENV_NAME%
call "%CONDA_BAT%" env list 2>nul | findstr /C:"%ENV_NAME%" >nul 2>&1
if %errorlevel% neq 0 (
    echo [创建] 虚拟环境不存在，正在创建 %ENV_NAME%
    call "%CONDA_BAT%" create -y -n %ENV_NAME% python=3.9
    rem 等待环境创建完成并再次检查
    timeout /t 2 /nobreak >nul
    call "%CONDA_BAT%" env list 2>nul | findstr /C:"%ENV_NAME%" >nul 2>&1
    if %errorlevel% neq 0 (
        echo [错误] 虚拟环境创建失败
        pause
        exit /b 1
    )
    echo [成功] 虚拟环境创建完成
) else (
    echo [成功] 找到现有虚拟环境 %ENV_NAME%
)
echo.

rem 激活虚拟环境
echo.
echo [激活] 正在激活虚拟环境
set "ACTIVATE_BAT="
for %%F in ("%CONDA_BAT%") do set "CONDA_DIR=%%~dpF"
if exist "%CONDA_DIR%activate.bat" set "ACTIVATE_BAT=%CONDA_DIR%activate.bat"
if not defined ACTIVATE_BAT (
    for %%P in ("%CONDA_DIR%..\Scripts\activate.bat") do (
        if exist %%~fP set "ACTIVATE_BAT=%%~fP"
    )
)

if defined ACTIVATE_BAT (
    call "%ACTIVATE_BAT%" %ENV_NAME%
) else (
    call "%CONDA_BAT%" activate %ENV_NAME%
)
if %errorlevel% neq 0 (
    echo [错误] 虚拟环境激活失败
    pause
    exit /b 1
)
echo [成功] 虚拟环境已激活
echo.

rem 检查是否存在 .env 文件
if not exist ".env" (
    echo [警告] 未找到 .env 配置文件
    echo [提示] 将从 .env.example 复制默认配置...
    if exist ".env.example" (
        copy ".env.example" ".env" >nul
        echo [成功] 已创建 .env 文件，请根据需要修改配置
        echo.
        set /p EDIT_ENV="是否现在编辑 .env 配置文件？(y/n，默认n): "
        if /i "!EDIT_ENV!"=="y" (
            notepad .env
        )
    ) else (
        echo [错误] .env.example 文件也不存在
        pause
        exit /b 1
    )
) else (
    echo [成功] 找到配置文件 .env
)
echo.

rem 升级pip
echo [更新] 正在升级 pip...
python -m pip install --upgrade pip -q
if %errorlevel% neq 0 (
    echo [警告] pip 升级失败，但将继续执行
) else (
    echo [成功] pip 升级完成
)
echo.

rem 安装依赖
echo [依赖] 正在安装/更新依赖包...
echo.
pip install -r requirements.txt
if %errorlevel% neq 0 (
    echo.
    echo [错误] 依赖安装失败
    pause
    exit /b 1
)
echo.
echo [成功] 依赖安装完成
echo.

rem 检查数据库是否存在
if not exist "license.db" (
    echo [初始化] 数据库不存在，正在初始化...
    if exist "scripts\init_db.py" (
        python scripts\init_db.py
        if %errorlevel% neq 0 (
            echo [警告] 数据库初始化失败，但将尝试启动服务（首次运行会自动创建）
        ) else (
            echo [成功] 数据库初始化完成
        )
    ) else (
        echo [提示] 首次运行将自动创建数据库
    )
    echo.
)

rem -----------------------------------------------
rem 服务配置（使用默认值）
rem -----------------------------------------------
set HOST=0.0.0.0
set PORT=8000
set RELOAD_FLAG=--reload

echo.
echo ========================================
echo 启动配置
echo ========================================
echo  - 虚拟环境: %ENV_NAME%
echo  - 服务地址: http://%HOST%:%PORT%
echo  - API文档: http://%HOST%:%PORT%/docs
echo  - 热重载: 已启用
echo ========================================
echo.

rem 启动服务
echo [启动] 正在启动 License Webservice...
echo.
echo ----------------------------------------
echo  服务已启动！
echo  - API文档 (Swagger): http://localhost:%PORT%/docs
echo  - API文档 (ReDoc):   http://localhost:%PORT%/redoc
echo  - 管理后台:          http://localhost:%PORT%/static/admin.html
echo  - 用户门户:          http://localhost:%PORT%/static/portal.html
echo  
echo  按 Ctrl+C 停止服务
echo ----------------------------------------
echo.

uvicorn app.main:app --host %HOST% --port %PORT% %RELOAD_FLAG%

pause
