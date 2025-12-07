@echo off
REM SPDX-License-Identifier: MPL-2.0
REM Copyright 2025 Simon Balarabe
REM Project: ReactorMQ — https://github.com/Naragato/reactormq

setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
set BUILD_DIR=%ROOT_DIR%\cmake-build-fuzz

set FUZZER_NAME=%1
if "%FUZZER_NAME%"=="" set FUZZER_NAME=fuzz_mqtt_codec

set CORPUS_DIR=%2
if "%CORPUS_DIR%"=="" set CORPUS_DIR=%BUILD_DIR%\tests\fuzz_corpus\%FUZZER_NAME%

pushd "%BUILD_DIR%"

set BIN_PATH=tests\fuzz\%FUZZER_NAME%

if not exist "%BIN_PATH%" (
  echo [fuzz-run] Fuzzer binary not found: %BIN_PATH%
  echo            Make sure you have run scripts\fuzz-build.bat first.
  popd
  exit /b 1
)

if not exist "%CORPUS_DIR%" mkdir "%CORPUS_DIR%"

echo [fuzz-run] Running %FUZZER_NAME% with -max_total_time=30 and corpus: %CORPUS_DIR%
"%BIN_PATH%" -max_total_time=30 "%CORPUS_DIR%"

echo [fuzz-run] Running %FUZZER_NAME% with -runs=100000 (no corpus)
"%BIN_PATH%" -runs=100000

echo [fuzz-run] Done.

popd
