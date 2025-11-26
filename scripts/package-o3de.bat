@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0..
set OUT_DIR=%ROOT_DIR%\dist
set PKG_NAME=reactormq-o3de
set PKG_DIR=%OUT_DIR%\%PKG_NAME%

if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"
mkdir "%PKG_DIR%\Code\Source" "%PKG_DIR%\Code\Include"

xcopy "%ROOT_DIR%\src"     "%PKG_DIR%\Code\Source"  /E /I /Y >nul
xcopy "%ROOT_DIR%\include" "%PKG_DIR%\Code\Include" /E /I /Y >nul

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
cd /D "%OUT_DIR%"
tar -czf %PKG_NAME%.tar.gz %PKG_NAME%
cd /D "%ROOT_DIR%"
if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"

endlocal
