@echo off
title BookBazzar - Drogon REST Backend Server (:8080)
echo ===================================================
echo   BookBazzar: Starting C++ Drogon Backend (:8080)
echo ===================================================

cd /d "%~dp0backend"

REM Load environment variables from .env if present
if exist ".env" (
    echo [INFO] Loading configuration from backend\.env
    for /f "usebackq eol=# tokens=1* delims==" %%A in (".env") do (
        set "%%A=%%B"
    )
) else (
    echo [WARN] backend\.env not found. Using system environment variables or defaults.
    echo [WARN] See backend\.env.example to configure your MongoDB and Cloudinary credentials.
)

set "PATH=%CD%\build\Release;%CD%\build\Debug;%PATH%"

if exist ".\build\Release\BookExchangeBackend.exe" (
    echo [INFO] Found .\build\Release\BookExchangeBackend.exe (Release)
    echo [INFO] Listening at http://127.0.0.1:8080
    echo.
    ".\build\Release\BookExchangeBackend.exe"
) else if exist ".\build\Debug\BookExchangeBackend.exe" (
    echo [INFO] Found .\build\Debug\BookExchangeBackend.exe (Debug)
    echo [INFO] Listening at http://127.0.0.1:8080
    echo.
    ".\build\Debug\BookExchangeBackend.exe"
) else (
    echo [ERROR] Backend executable not found.
    echo Build the backend first or inspect backend/build directory.
    pause
)

