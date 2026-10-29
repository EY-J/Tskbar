@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title ClearBar v2.3 Installer

rem Keep errors visible when the installer is launched by double-clicking.
call :MAIN
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
    echo.
    echo ClearBar installation stopped with error code %RESULT%.
    echo Review the message above, then press any key to close this window.
    pause >nul
)
exit /b %RESULT%

:MAIN
set "APPDIR=%LOCALAPPDATA%\ClearBar"
set "RUNKEY=HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
set "MODE=Clear"

echo.
echo ========================================
echo              CLEARBAR v2.3
echo       Windows 11 Taskbar Customizer
echo ========================================
echo.

if exist "%APPDIR%\ClearBar.exe" goto :EXISTING
goto :FIRST_INSTALL

:EXISTING
call :READ_MODE
echo ClearBar is already installed.
echo.
echo Current appearance: !LABEL!
echo.
echo Choose an action:
echo  [1] Change appearance
echo  [2] Repair / reinstall
echo  [3] Check status
echo  [4] Uninstall
echo  [5] Exit
echo.
set "ACTION="
set /p "ACTION=Enter choice [1-5]: "
if errorlevel 1 (
    echo ERROR: No menu input received.
    exit /b 1
)
if "!ACTION!"=="1" goto :CHANGE
if "!ACTION!"=="2" goto :REPAIR
if "!ACTION!"=="3" (
    call "%~dp0status.cmd"
    goto :EXISTING
)
if "!ACTION!"=="4" (
    call "%~dp0uninstall.cmd"
    exit /b !ERRORLEVEL!
)
if "!ACTION!"=="5" exit /b 0
echo Invalid choice. Enter a number from 1 to 5.
echo.
goto :EXISTING

:FIRST_INSTALL
call :CHOOSE_MODE
if errorlevel 1 exit /b 1
goto :INSTALL_FILES

:CHANGE
call :CHOOSE_MODE
if errorlevel 1 exit /b 1
if not exist "%APPDIR%\clearbar.ini" (
    echo Upgrading the existing ClearBar installation first...
    goto :INSTALL_FILES
)
call :CHECK_WINDOWS
if errorlevel 1 exit /b 1
call :SAVE_MODE
if errorlevel 1 exit /b 1
call :AUTOSTART
if errorlevel 1 exit /b 1
start "" "%APPDIR%\ClearBar.exe"
call :WAIT_FOR_APPLY
exit /b !ERRORLEVEL!

:REPAIR
echo Repairing ClearBar with !LABEL! appearance...
goto :INSTALL_FILES

:INSTALL_FILES
call :BUILD_IF_NEEDED
if errorlevel 1 exit /b 1
set "CHECK_EXE=build\ClearBar.exe"
call :CHECK_WINDOWS
if errorlevel 1 exit /b 1

set "WAS_INSTALLED=0"
if exist "%APPDIR%\ClearBarTAP.dll" set "WAS_INSTALLED=1"
if "!WAS_INSTALLED!"=="1" (
    taskkill /F /IM ClearBar.exe >nul 2>&1
    echo Reloading Explorer to replace the installed TAP...
    taskkill /F /IM explorer.exe >nul 2>&1
    if errorlevel 1 (
        echo ERROR: Could not stop Explorer for repair.
        start "" "%APPDIR%\ClearBar.exe"
        exit /b 1
    )
    ping -n 3 127.0.0.1 >nul
)
if exist "%APPDIR%\taskbar.ps1" (
    >"%APPDIR%\stop.flag" echo stop
    ping -n 3 127.0.0.1 >nul
)
if not exist "%APPDIR%" mkdir "%APPDIR%"
if errorlevel 1 goto :COPYFAIL
copy /Y "build\ClearBar.exe" "%APPDIR%\ClearBar.exe" >nul
if errorlevel 1 goto :COPYFAIL
copy /Y "build\ClearBarTAP.dll" "%APPDIR%\ClearBarTAP.dll" >nul
if errorlevel 1 goto :COPYFAIL
if "!WAS_INSTALLED!"=="1" start "" explorer.exe
del /F /Q "%APPDIR%\clearbar.state" >nul 2>&1
del /F /Q "%APPDIR%\taskbar.ps1" >nul 2>&1
del /F /Q "%APPDIR%\stop.flag" >nul 2>&1
call :SAVE_MODE
if errorlevel 1 exit /b 1
call :AUTOSTART
if errorlevel 1 exit /b 1

start "" "%APPDIR%\ClearBar.exe"
call :WAIT_FOR_APPLY
exit /b !ERRORLEVEL!

:BUILD_IF_NEEDED
if /I "%CLEARBAR_REBUILD%"=="1" goto :ATTEMPT_BUILD
if not exist "build\ClearBar.exe" goto :ATTEMPT_BUILD
if not exist "build\ClearBarTAP.dll" goto :ATTEMPT_BUILD

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :USE_BUNDLED

echo Visual Studio detected. Rebuilding the updated source before installation...

:ATTEMPT_BUILD
set "HAD_BINARIES=0"
if exist "build\ClearBar.exe" if exist "build\ClearBarTAP.dll" (
    copy /Y "build\ClearBar.exe" "build\ClearBar.exe.bak" >nul
    if errorlevel 1 goto :BACKUP_FAILED
    copy /Y "build\ClearBarTAP.dll" "build\ClearBarTAP.dll.bak" >nul
    if errorlevel 1 goto :BACKUP_FAILED
    set "HAD_BINARIES=1"
)

echo Building ClearBar v2.3...
call "%~dp0build.cmd"
if errorlevel 1 goto :BUILD_FAILED
if not exist "build\ClearBar.exe" goto :BUILD_FAILED
if not exist "build\ClearBarTAP.dll" goto :BUILD_FAILED

del /F /Q "build\ClearBar.exe.bak" "build\ClearBarTAP.dll.bak" >nul 2>&1
echo Source build completed successfully.
exit /b 0

:BACKUP_FAILED
echo WARNING: Could not back up the bundled binaries; skipping the automatic rebuild.
del /F /Q "build\ClearBar.exe.bak" "build\ClearBarTAP.dll.bak" >nul 2>&1
if /I "%CLEARBAR_REBUILD%"=="1" exit /b 1
goto :USE_BUNDLED

:BUILD_FAILED
if "!HAD_BINARIES!"=="1" (
    copy /Y "build\ClearBar.exe.bak" "build\ClearBar.exe" >nul 2>&1
    copy /Y "build\ClearBarTAP.dll.bak" "build\ClearBarTAP.dll" >nul 2>&1
    del /F /Q "build\ClearBar.exe.bak" "build\ClearBarTAP.dll.bak" >nul 2>&1
    if /I not "%CLEARBAR_REBUILD%"=="1" (
        echo WARNING: The source rebuild failed. The original bundled binaries were restored.
        echo The installer will continue, but source-level fixes require a successful rebuild.
        exit /b 0
    )
)
echo ERROR: Build failed. Install Visual Studio C++ x64 Build Tools and the Windows SDK, then retry.
exit /b 1

:USE_BUNDLED
echo Using the bundled ClearBar binaries. Visual Studio Build Tools are not required.
exit /b 0

:CHECK_WINDOWS
if not defined CHECK_EXE set "CHECK_EXE=%APPDIR%\ClearBar.exe"
start /wait "" "!CHECK_EXE!" --supported
if errorlevel 1 (
    echo ERROR: ClearBar requires Windows 11 build 22000 or newer.
    exit /b 1
)
exit /b 0

:SAVE_MODE
start /wait "" "%APPDIR%\ClearBar.exe" --set !MODE!
if errorlevel 1 (
    echo ERROR: Could not save !MODE! in "%APPDIR%\clearbar.ini".
    exit /b 1
)
exit /b 0

:AUTOSTART
reg add "%RUNKEY%" /v ClearBar /t REG_SZ /d "\"%APPDIR%\ClearBar.exe\"" /f >nul
if errorlevel 1 (
    echo ERROR: Could not enable ClearBar auto-start.
    exit /b 1
)
exit /b 0

:WAIT_FOR_APPLY
echo Applying !LABEL! appearance...
for /l %%I in (1,1,60) do (
    timeout /t 1 /nobreak >nul
    start /wait "" "%APPDIR%\ClearBar.exe" --check !MODE!
    if not errorlevel 1 goto :SUCCESS
)
echo.
echo ERROR: ClearBar did not confirm the taskbar change.
echo Explorer or XAML Diagnostics may not be available.
echo Run status.cmd and check "%APPDIR%\clearbar.log".
exit /b 1

:SUCCESS
echo.
echo ClearBar installed successfully.
echo Appearance: !LABEL!
echo Auto-start: Enabled
echo.
echo You may close this window.
exit /b 0

:CHOOSE_MODE
echo.
echo Choose your taskbar appearance:
echo.
echo  [1] Clear
echo      Fully transparent
echo  [2] Blur - Light
echo      High transparency with subtle blur
echo  [3] Blur - Medium
echo      Balanced transparency and blur ^(recommended^)
echo  [4] Blur - Strong
echo      Strong frosted blur while remaining translucent
echo  [5] Windows Default
echo      Normal Windows taskbar appearance
if exist "%APPDIR%\ClearBar.exe" echo Current: !LABEL!
echo.
:CHOOSE_AGAIN
set "CHOICE="
set /p "CHOICE=Enter choice [1-5]: "
if errorlevel 1 (
    echo ERROR: No appearance choice received.
    exit /b 1
)
if "!CHOICE!"=="1" (set "MODE=Clear"& set "LABEL=Clear"& exit /b 0)
if "!CHOICE!"=="2" (set "MODE=BlurLight"& set "LABEL=Blur - Light"& exit /b 0)
if "!CHOICE!"=="3" (set "MODE=BlurMedium"& set "LABEL=Blur - Medium"& exit /b 0)
if "!CHOICE!"=="4" (set "MODE=BlurStrong"& set "LABEL=Blur - Strong"& exit /b 0)
if "!CHOICE!"=="5" (set "MODE=Default"& set "LABEL=Windows Default"& exit /b 0)
echo Invalid choice. Enter a number from 1 to 5.
goto :CHOOSE_AGAIN

:READ_MODE
set "MODE=Clear"
if exist "%APPDIR%\clearbar.ini" (
    set "MODE=Default"
    for /f "usebackq tokens=1,* delims==" %%A in ("%APPDIR%\clearbar.ini") do (
        if /I "%%A"=="Mode" set "MODE=%%B"
    )
)
set "LABEL=Windows Default"
if "!MODE!"=="Clear" set "LABEL=Clear"
if "!MODE!"=="BlurLight" set "LABEL=Blur - Light"
if "!MODE!"=="BlurMedium" set "LABEL=Blur - Medium"
if "!MODE!"=="BlurStrong" set "LABEL=Blur - Strong"
if "!LABEL!"=="Windows Default" set "MODE=Default"
exit /b 0

:COPYFAIL
if "!WAS_INSTALLED!"=="1" (
    start "" explorer.exe
    start "" "%APPDIR%\ClearBar.exe"
)
echo ERROR: Could not create or update files in "%APPDIR%".
exit /b 1
