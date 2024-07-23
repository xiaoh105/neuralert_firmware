rem PROJ_DIR    :1 %1
rem TOOLKIT_DIR :2 %2
rem TARGET_DIR  :3 %3
rem TARGET_BNAME:4 %4
rem MODEL_NAME:5 %5

@echo off
setlocal enabledelayedexpansion
echo [%0] START
set ModelName=%5
set FILENAME=%0
set TARGET_DIR=%3

set TOOLNAME=".\certtool\da16secutool.exe"
rem set TOOLNAME=py -3.8 ".\certtool_py\da16secutool.py"

REM FreeRTOS
set OS_TYPE=F

set argc=0

for %%x in (%*) do Set /A argc+=1
REM echo argc=%argc%

if %1 equ OTA (
	REM For OTA IMAGE Build
	echo Generate OTA Image
	call set ModelName=DA16200
  call set IMG_PATH=%~dp0..\..\apps\!ModelName!\get_started\projects\!ModelName!\img
	call set PRJ_PATH=%TARGET_DIR%
) else (
	REM For Normal Build
	set PRJ_PATH="%1"
)

REM secure tool log
if not exist "%~dp0\example" (
	mkdir "%~dp0\example"
)

REM secure tool dmtpmcfg
if not exist "%~dp0\dmtpmcfg" (
	mkdir "%~dp0\dmtpmcfg"
)

REM Delete Old img
if exist "%~dp0public\DA16xxx_RTOS.img" (
	del /q "%~dp0public\DA16xxx_RTOS.img"
)
if exist "%~dp0public\DA16xxx_slib_tim.img" (
	del /q %~dp0public\DA16xxx_slib_tim.img"
)
if exist "%~dp0public\DA16xxx_ueboot_*.img" (
	del /q "%~dp0public\DA16xxx_ueboot_*.img"
)

pushd "%~dp0"

@echo *========================================
@echo *Generation ini
@echo *========================================
@echo.
REM OTA IMG Build or Single Run for CM3.secu_free_rtos.bat
if %argc% equ 0 %TOOLNAME% -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg -tpm ./cmtpmcfg

REM Normal Build
if %argc% gtr 1 %TOOLNAME% -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg -tpm ./cmtpmcfg

rem === dmtpmcfg
REM Check file generation status.
if not exist "%~dp0"dmtpmcfg\sboot_hbk1_cache_cert.ini (
	@echo [%date% %time%][%FILENAME%] The generated 'sboot_hbk1_cache_cert.ini' file does not exist! [%~dp0dmtpmcfg]>> "%PRJ_PATH%\build_error.log"
	@echo Error: dmtpmcfg\sboot_hbk1_cache_cert.ini
	exit /b 2
)

if not exist "%~dp0"dmtpmcfg\sboot_hbk1_ueboot_cert.ini (
	@echo [%date% %time%][%FILENAME%] The generated 'sboot_hbk1_ueboot_cert.ini' file does not exist! [%~dp0dmtpmcfg]>> "%PRJ_PATH%\build_error.log"
	@echo Error: dmtpmcfg\sboot_hbk1_ueboot_cert.ini
	exit /b 2
)

@echo *========================================
@echo * Start add_version.bat
@echo *========================================
if not exist "%~dp0..\util\add_version.bat" (
	echo [%date% %time%][%FILENAME%] 'add_version.bat' file does not exist! [%~dp0dmtpmcfg]>> "%PRJ_PATH%\build_error.log"
	@echo Error: not found \util\add_version.bat
	echo [%0] END
	exit /b 2
)

@echo *----------------------------------------
@echo *Add Version Info. for ini
@echo *----------------------------------------
@echo.

REM Add Version Info.
if %1 equ OTA (
	pushd "%~dp0..\util"
	REM Trim \SBOOT\
	cmd /C add_version.bat "%PRJ_PATH%" dummy  %PRJ_PATH% dummy %ModelName%
	pushd "%~dp0"
) else (
	pushd "%~dp0..\util"
	cmd /C add_version.bat "%PRJ_PATH%" %2 %3 %4 %ModelName%
	pushd "%~dp0"
)

if not exist "%~dp0"dmtpmcfg\TEMP1.ini (
	echo [%0] END
	exit /b 2
)

if not exist "%~dp0"dmtpmcfg\TEMP3.ini (
	echo [%0] END
	exit /b 2
)

REM type ..\util\__temp_img_path.txt
set /p IMG_PATH=<..\util\__temp_img_path.txt

REM Trim ../
set IMG_PATH=%IMG_PATH%
REM echo [DEBUG] %IMG_PATH%

if "%4" == "DA16xxx_ueboot" (
	if exist "%IMG_PATH%\%ModelName%_BOOT*.img" (
		del /q "%IMG_PATH%\%ModelName%_BOOT*.img"
	)
) else if %argc% neq 1 (
	REM For Normal Build
	if exist "%IMG_PATH%\%ModelName%_%OS_TYPE%BOOT*.img" (
		del /q "%IMG_PATH%\%ModelName%_%OS_TYPE%BOOT*.img"
	)
	if exist "%IMG_PATH%\%ModelName%_%OS_TYPE%RT*.img" (
		del /q "%IMG_PATH%\%ModelName%_%OS_TYPE%RT*.img"
	)
)

REM Image Generate
@echo *----------------------------------------
@echo *Image Generating
@echo *----------------------------------------
@echo.
%TOOLNAME% -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg

if %1 equ OTA (
	echo [%0] END
	exit /b 0
)

REM Debug
REM echo [DEBUG]-------------------------
REM cd
REM echo %IMG_PATH%\%ModelName%_%OS_TYPE%RTOS-*.img
REM echo [DEBUG]-------------------------

REM Check img files
if not exist "%IMG_PATH%\%ModelName%_%OS_TYPE%RTOS-*.img" (
	@echo [%date% %time%][%FILENAME%] '%ModelName%_%OS_TYPE%RTOS-*.img' file does not exist! [%IMG_PATH%]>> "%PRJ_PATH%\build_error.log"
	@echo [%FILENAME%] Generate Error: %ModelName%_%OS_TYPE%RTOS-*.img
	echo [%0] END
	exit /b 2
) else if not exist "%IMG_PATH%\%ModelName%_%OS_TYPE%BOOT-*.img" (
	@echo [%date% %time%][%FILENAME%] '%ModelName%_%OS_TYPE%BOOT-*.img' file does not exist! [%IMG_PATH%]>> "%PRJ_PATH%\build_error.log"
	@echo [%FILENAME%] Generate Error: %ModelName%_%OS_TYPE%BOOT-*.img
	echo [%0] END
	exit /b 2
) else (
	@echo *----------------------------------------
	@echo *Image Generate success
	@echo *----------------------------------------
	@echo.
	echo [%0] END
	exit /b 0
)