@echo off
title BookBazzar - MongoDB Server
echo ===================================================
echo   BookBazzar: Starting MongoDB Daemon (:27017)
echo ===================================================
if not exist "%~dp0mongodb_data" mkdir "%~dp0mongodb_data"

set MONGOD="C:\Program Files\MongoDB\Server\8.3\bin\mongod.exe"
if not exist %MONGOD% (
    echo [WARNING] Default mongod path not found at %MONGOD%
    echo Searching for mongod.exe on system...
    where mongod >nul 2>&1
    if %errorlevel% equ 0 (
        set MONGOD=mongod
    ) else (
        echo [ERROR] MongoDB Server executable not found!
        echo Please ensure MongoDB is installed or update the path in this script.
        pause
        exit /b 1
    )
)

echo Using: %MONGOD%
echo Database storage path: "%~dp0mongodb_data"
echo.
%MONGOD% --dbpath "%~dp0mongodb_data" --bind_ip 127.0.0.1 --port 27017
pause
