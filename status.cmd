@echo off
setlocal
set "APPDIR=%LOCALAPPDATA%\ClearBar"
echo.
echo ===== ClearBar v2.3 Status =====
echo.
if exist "%APPDIR%\clearbar.ini" (
  type "%APPDIR%\clearbar.ini"
) else (
  echo No saved configuration. Missing configuration uses Clear for v2 compatibility.
)
echo.
tasklist /FI "IMAGENAME eq ClearBar.exe" 2>nul | find /I "ClearBar.exe"
if errorlevel 1 echo ClearBar.exe is NOT running.
echo.
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v ClearBar 2>nul
echo.
echo ===== Applied state =====
if exist "%APPDIR%\clearbar.state" (
  type "%APPDIR%\clearbar.state"
) else (
  echo No taskbar application has been confirmed yet.
)
echo.
echo ===== Recent log =====
if exist "%APPDIR%\clearbar.log" (
  powershell -NoProfile -Command "Get-Content -LiteralPath (Join-Path $env:LOCALAPPDATA 'ClearBar\clearbar.log') -Tail 20"
) else (
  echo No log file yet.
)
echo.
pause
