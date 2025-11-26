@echo off
setlocal

set ROOT_DIR=%~dp0..
set UNREAL_NATIVE=%ROOT_DIR%\unreal_engine\Source\ReactorMQ\Native
set O3DE_CODE=%ROOT_DIR%\o3de\Code

if exist "%UNREAL_NATIVE%\src" rmdir "%UNREAL_NATIVE%\src" /S /Q
if exist "%UNREAL_NATIVE%\include" rmdir "%UNREAL_NATIVE%\include" /S /Q
if exist "%UNREAL_NATIVE%\fixtures" rmdir "%UNREAL_NATIVE%\fixtures" /S /Q

if exist "%O3DE_CODE%\src" rmdir "%O3DE_CODE%\src" /S /Q
if exist "%O3DE_CODE%\include" rmdir "%O3DE_CODE%\include" /S /Q
if exist "%O3DE_CODE%\fixtures" rmdir "%O3DE_CODE%\fixtures" /S /Q

endlocal
