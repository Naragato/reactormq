@echo off
REM SPDX-License-Identifier: MPL-2.0
REM Copyright 2025 Simon Balarabe
REM Project: ReactorMQ — https://github.com/Naragato/reactormq

setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
set BUILD_DIR=%ROOT_DIR%\cmake-build-fuzz

set C_COMPILER=%1
set CXX_COMPILER=%2

set C_COMPILER=!C_COMPILER:-DCMAKE_C_COMPILER=!
set CXX_COMPILER=!CXX_COMPILER:-DCMAKE_CXX_COMPILER=!

set CMAKE_ARGS=-DREACTORMQ_BUILD_TESTS=ON -DREACTORMQ_BUILD_FUZZERS=ON -DENABLE_ASAN=ON -DENABLE_UBSAN=ON

if not "%C_COMPILER%"=="" (
  set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_C_COMPILER=%C_COMPILER%
  echo [fuzz-build] Using C compiler: %C_COMPILER%
)

if not "%CXX_COMPILER%"=="" (
  set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_CXX_COMPILER=%CXX_COMPILER%
  echo [fuzz-build] Using C++ compiler: %CXX_COMPILER%
)

echo [fuzz-build] Configuring fuzz targets in: %BUILD_DIR%

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" %CMAKE_ARGS%

if errorlevel 1 goto :error

echo [fuzz-build] Building fuzz targets...

cmake --build "%BUILD_DIR%" --target ^
  fuzz_mqtt_codec fuzz_fixed_header fuzz_variable_byte_integer ^
  fuzz_publish_packet fuzz_connect_packet

if errorlevel 1 goto :error

echo [fuzz-build] Done.
goto :eof

:error
echo [fuzz-build] Failed.
exit /b 1
