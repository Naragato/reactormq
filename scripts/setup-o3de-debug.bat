@echo off
setlocal

set ROOT_DIR=%~dp0..
set CODE_DIR=%ROOT_DIR%\o3de\Code

if not exist "%CODE_DIR%" mkdir "%CODE_DIR%"

rem Link source and include directories
if exist "%CODE_DIR%\src" rmdir "%CODE_DIR%\src" /S /Q
if exist "%CODE_DIR%\include" rmdir "%CODE_DIR%\include" /S /Q

mklink /J "%CODE_DIR%\src" "%ROOT_DIR%\src"
mklink /J "%CODE_DIR%\include" "%ROOT_DIR%\include"

rem Create fixtures directory and link only the specific fixture files used by O3DE
if exist "%CODE_DIR%\fixtures" rmdir "%CODE_DIR%\fixtures" /S /Q
mkdir "%CODE_DIR%\fixtures"

if exist "%ROOT_DIR%\tests\fixtures\docker_container.h" mklink "%CODE_DIR%\fixtures\docker_container.h" "%ROOT_DIR%\tests\fixtures\docker_container.h"
if exist "%ROOT_DIR%\tests\fixtures\docker_container.cpp" mklink "%CODE_DIR%\fixtures\docker_container.cpp" "%ROOT_DIR%\tests\fixtures\docker_container.cpp"
if exist "%ROOT_DIR%\tests\fixtures\port_utils.h" mklink "%CODE_DIR%\fixtures\port_utils.h" "%ROOT_DIR%\tests\fixtures\port_utils.h"

endlocal
