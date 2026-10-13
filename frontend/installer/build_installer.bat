@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "ISS_PATH=%SCRIPT_DIR%BookBazzar.iss"
set "ISCC_EXE="

:: Check common ISCC.exe locations
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" set "ISCC_EXE=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not defined ISCC_EXE if exist "C:\Program Files\Inno Setup 6\ISCC.exe" set "ISCC_EXE=C:\Program Files\Inno Setup 6\ISCC.exe"
if not defined ISCC_EXE if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC_EXE=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not defined ISCC_EXE (
    where ISCC.exe >nul 2>&1
    if not errorlevel 1 set "ISCC_EXE=ISCC.exe"
)

if not defined ISCC_EXE (
    echo [ERROR] Inno Setup compiler (ISCC.exe) not found.
    echo Please install Inno Setup 6 from: https://jrsoftware.org/isdl.php
    echo Or install via winget: winget install JRSoftware.InnoSetup
    pause
    exit /b 1
)

echo [BUILD] Compiling BookBazzar installer with: %ISCC_EXE%
"%ISCC_EXE%" "%ISS_PATH%"
if errorlevel 1 (
    echo [ERROR] Installer compilation failed.
    pause
    exit /b 1
)

echo [SUCCESS] Installer successfully built in frontend\dist\BookBazzar-Setup-0.1.0.exe
endlocal
