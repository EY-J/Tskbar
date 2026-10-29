<<<<<<< HEAD
@echo off
=======
﻿@echo off
>>>>>>> origin/main
setlocal EnableExtensions
cd /d "%~dp0"
title ClearBar v2.3 Builder
echo.
echo ==========================================
echo          ClearBar v2.3 Builder
echo ==========================================
echo.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: Visual Studio Build Tools were not found.
  exit /b 1
)
set "VSROOT="
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT (
  echo ERROR: Visual C++ x64 Build Tools were not found.
  exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist build mkdir build

echo [1/2] Building ClearBarTAP.dll...
cl /nologo /std:c++20 /EHsc /O2 /MT /permissive- /DUNICODE /D_UNICODE ^
 /LD "src\ClearBarTAP.cpp" /Fe:"build\ClearBarTAP.dll" ^
 /link /DEF:"src\ClearBarTAP.def" windowsapp.lib ole32.lib oleaut32.lib user32.lib
if errorlevel 1 (
  echo.
  echo BUILD FAILED: ClearBarTAP.dll
  exit /b 1
)

echo.
echo [2/2] Building ClearBar.exe...
cl /nologo /std:c++20 /EHsc /O2 /MT /permissive- /DUNICODE /D_UNICODE ^
 "src\ClearBar.cpp" /Fe:"build\ClearBar.exe" ^
 /link /SUBSYSTEM:WINDOWS windowsapp.lib ole32.lib user32.lib
if errorlevel 1 (
  echo.
  echo BUILD FAILED: ClearBar.exe
  exit /b 1
)
del /q ClearBar.obj ClearBarTAP.obj >nul 2>&1
echo.
echo BUILD COMPLETE.
exit /b 0
