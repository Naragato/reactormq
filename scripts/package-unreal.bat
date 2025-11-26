@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0..
set OUT_DIR=%ROOT_DIR%\dist
set PKG_NAME=reactormq-unreal
set PKG_DIR=%OUT_DIR%\%PKG_NAME%

if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"
mkdir "%PKG_DIR%"

xcopy "%ROOT_DIR%\unreal_engine" "%PKG_DIR%" /E /I /Y >nul

set NATIVE_DIR=%PKG_DIR%\Source\ReactorMQ\Native
if not exist "%NATIVE_DIR%" mkdir "%NATIVE_DIR%"
if not exist "%NATIVE_DIR%\src" mkdir "%NATIVE_DIR%\src"
if not exist "%NATIVE_DIR%\include" mkdir "%NATIVE_DIR%\include"
if not exist "%NATIVE_DIR%\fixtures" mkdir "%NATIVE_DIR%\fixtures"

xcopy "%ROOT_DIR%\src"     "%NATIVE_DIR%\src"     /E /I /Y >nul
xcopy "%ROOT_DIR%\include" "%NATIVE_DIR%\include" /E /I /Y >nul

if exist "%ROOT_DIR%\tests\fixtures\docker_container.h" copy "%ROOT_DIR%\tests\fixtures\docker_container.h" "%NATIVE_DIR%\fixtures" >nul
if exist "%ROOT_DIR%\tests\fixtures\docker_container.cpp" copy "%ROOT_DIR%\tests\fixtures\docker_container.cpp" "%NATIVE_DIR%\fixtures" >nul
if exist "%ROOT_DIR%\tests\fixtures\port_utils.h" copy "%ROOT_DIR%\tests\fixtures\port_utils.h" "%NATIVE_DIR%\fixtures" >nul

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
cd /D "%OUT_DIR%"
tar -czf %PKG_NAME%.tar.gz %PKG_NAME%
cd /D "%ROOT_DIR%"
if exist "%PKG_DIR%" rmdir /S /Q "%PKG_DIR%"

endlocal
