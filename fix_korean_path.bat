@echo off
chcp 65001 > nul
echo ========================================
echo  VR_Tracker - Korean Path Fix
echo  한글 경로 문제 해결 스크립트
echo ========================================
echo.

set JUNCTION_PATH=C:\VR_Tracker

:: 이미 존재하면 삭제
if exist "%JUNCTION_PATH%" (
    echo [INFO] Removing existing junction at %JUNCTION_PATH%...
    rmdir "%JUNCTION_PATH%"
)

:: 현재 스크립트 위치(프로젝트 루트)를 기준으로 Junction 생성
echo [INFO] Creating directory junction...
echo   FROM: %JUNCTION_PATH%
echo   TO  : %~dp0
mklink /J "%JUNCTION_PATH%" "%~dp0"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Junction creation failed.
    echo         Run this script as Administrator.
    echo         이 스크립트를 관리자 권한으로 실행하세요.
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Junction created!
echo.
echo ========================================
echo  NEXT STEPS:
echo ========================================
echo  1. Open: C:\VR_Tracker\VR_Knee_Collision.sln
echo     (NOT the original path)
echo.
echo  2. Build from Visual Studio
echo.
echo  3. To open the project in UE5 Editor:
echo     C:\VR_Tracker\VR_Knee_Collision.uproject
echo ========================================
echo.
pause
