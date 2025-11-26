@echo off
setlocal

set ROOT_DIR=%~dp0..
set NATIVE_DIR=%ROOT_DIR%\unreal_engine\Source\ReactorMQ\Native

if not exist "%NATIVE_DIR%" mkdir "%NATIVE_DIR%"

if exist "%NATIVE_DIR%\src" rmdir "%NATIVE_DIR%\src" /S /Q
if exist "%NATIVE_DIR%\include" rmdir "%NATIVE_DIR%\include" /S /Q
if exist "%NATIVE_DIR%\fixtures" rmdir "%NATIVE_DIR%\fixtures" /S /Q

mklink /J "%NATIVE_DIR%\src" "%ROOT_DIR%\src"
mklink /J "%NATIVE_DIR%\include" "%ROOT_DIR%\include"

rem Create fixtures directory and link only the specific fixture files used by Unreal
mkdir "%NATIVE_DIR%\fixtures"

if exist "%ROOT_DIR%\tests\fixtures\docker_container.h" mklink "%NATIVE_DIR%\fixtures\docker_container.h" "%ROOT_DIR%\tests\fixtures\docker_container.h"
if exist "%ROOT_DIR%\tests\fixtures\docker_container.cpp" mklink "%NATIVE_DIR%\fixtures\docker_container.cpp" "%ROOT_DIR%\tests\fixtures\docker_container.cpp"
if exist "%ROOT_DIR%\tests\fixtures\port_utils.h" mklink "%NATIVE_DIR%\fixtures\port_utils.h" "%ROOT_DIR%\tests\fixtures\port_utils.h"

endlocal
