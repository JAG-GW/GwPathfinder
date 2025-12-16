@echo off
echo ========================================
echo   GWPathfinder Build Script
echo ========================================
echo.

powershell -ExecutionPolicy Bypass -File "%~dp0Build.ps1" %*

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
pause
