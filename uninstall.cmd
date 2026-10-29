@echo off
setlocal EnableExtensions
set "APPDIR=%LOCALAPPDATA%\ClearBar"
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v ClearBar /f >nul 2>&1
taskkill /F /IM ClearBar.exe >nul 2>&1
taskkill /F /IM explorer.exe >nul 2>&1
ping -n 3 127.0.0.1 >nul
for %%F in (ClearBar.exe ClearBarTAP.dll clearbar.ini clearbar.state clearbar.log taskbar.ps1 stop.flag) do (
  if exist "%APPDIR%\%%F" del /F /Q "%APPDIR%\%%F" >nul 2>&1
)
if exist "%APPDIR%" rmdir "%APPDIR%" >nul 2>&1
start explorer.exe
if exist "%APPDIR%" (
  echo ERROR: ClearBar files could not be fully removed from "%APPDIR%".
  exit /b 1
)
echo.
echo ClearBar removed and Explorer restarted.
exit /b 0
