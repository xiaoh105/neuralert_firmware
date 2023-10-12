@echo off
chcp 437
rem *========================================
rem * prebuild
rem *========================================
rem
rem PROJ_DIR		= %1
rem TOOLKIT_DIR		= %2
rem TARGET_DIR		= %3
rem TARGET_BNAME	= %4
rem
rem *========================================

rem PUSHD  %1
echo [%0] START
@echo.
@echo.

set arg_count=0
for %%x in (%*) do Set /A arg_count+=1

if exist "%~dp0file2hexa.bat" (
	@echo Generate RamLibrary / TIM
	call "%~dp0"file2hexa.bat
)

REM library
set libdir="%~dp0..\..\library\"

if not exist %libdir% (
    mkdir %libdir%
)

@echo Make build Time Info
@echo.
if exist "%~dp0..\version\genfiles\buildtime.h" (
    del "%~dp0..\version\genfiles\buildtime.h"
)
call "%~dp0"mk_buildtime.bat %1 %2 %3 %4

@echo Make build Version Info
@echo.
if exist "%~dp0..\version\genfiles\rtos_version_str.txt" (
    del "%~dp0..\version\genfiles\rtos_version_str.txt"
)
call "%~dp0"mk_version.bat %1 %2 %3 %4

POPD
echo [%0] END
REM exit
