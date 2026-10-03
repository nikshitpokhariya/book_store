@echo off
title BookBazzar - Drogon REST Backend Server (:8080)
echo ===================================================
echo   BookBazzar: Starting C++ Drogon Backend (:8080)
echo ===================================================

cd /d "%~dp0backend"

if not defined JWT_SECRET set JWT_SECRET=WhWRp7LUnP+BUdQaOxcjpf67hKIBcnk3WAqTrTbzfjOy+CWOdXlgIqsqGTCTHYu7
if not defined MONGO_URI set MONGO_URI=mongodb://127.0.0.1:27017
if not defined MONGO_DATABASE set MONGO_DATABASE=myapp

if exist ".\build\Debug\BookExchangeBackend_new.exe" (
    echo [INFO] Found .\build\Debug\BookExchangeBackend_new.exe
    echo [INFO] Listening at http://127.0.0.1:8080
    echo.
    ".\build\Debug\BookExchangeBackend_new.exe"
) else if exist ".\build\Debug\BookExchangeBackend.exe" (
    echo [INFO] Found .\build\Debug\BookExchangeBackend.exe
    echo [INFO] Listening at http://127.0.0.1:8080
    echo.
    ".\build\Debug\BookExchangeBackend.exe"
) else (
    echo [ERROR] Backend executable not found at .\build\Debug\BookExchangeBackend.exe
    echo Build the backend first or inspect backend/build directory.
    pause
)

