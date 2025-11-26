@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0..
set OUT_DIR=%ROOT_DIR%\dist
set PKG_NAME=reactormq-dev
set PKG_DIR=%OUT_DIR%\%PKG_NAME%

if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"
mkdir "%PKG_DIR%"

xcopy "%ROOT_DIR%\cmake"   "%PKG_DIR%\cmake"   /E /I /Y >nul
xcopy "%ROOT_DIR%\tests"   "%PKG_DIR%\tests"   /E /I /Y >nul
xcopy "%ROOT_DIR%\src"     "%PKG_DIR%\src"     /E /I /Y >nul
xcopy "%ROOT_DIR%\include" "%PKG_DIR%\include" /E /I /Y >nul
copy "%ROOT_DIR%\CMakeLists.txt" "%PKG_DIR%" >nul
if exist "%ROOT_DIR%\README.md" copy "%ROOT_DIR%\README.md" "%PKG_DIR%" >nul

set FIX_DIR=%PKG_DIR%\tests\fixtures
if exist "%FIX_DIR%" (
  for %%F in ("%FIX_DIR%\*") do (
    if /I not "%%~nxF"=="docker_container.cpp" if /I not "%%~nxF"=="docker_container.h" if /I not "%%~nxF"=="port_utils.h" del /Q "%%F"
  )
)

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
cd /D "%OUT_DIR%"
tar -czf %PKG_NAME%.tar.gz %PKG_NAME%
cd /D "%ROOT_DIR%"
if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"

endlocal
