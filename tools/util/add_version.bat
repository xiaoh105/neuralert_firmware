@echo off
echo [%0] START
setlocal enabledelayedexpansion
rem ==================================================
rem "$PROJ_DIR$\util\add_version.bat" "$PROJ_DIR$" "$TOOLKIT_DIR$" "$TARGET_DIR$" $TARGET_BNAME$
rem PROJ_DIR    :1 %1
rem TOOLKIT_DIR :2 %2
rem TARGET_DIR  :3 %3
rem TARGET_BNAME:4 %4
rem ModelName	:5 %5
rem ==================================================
set ModelName=%5
set FILENAME=add_version.bat
set TARGET_DIR=%3
set VERIFY_RTOS_TITLE=""
set VERIFY_BOOT_TITLE=""
set VER_LEN_MAX=39
set IMG_DIR=""

REM FreeRTOS
set OS_TYPE=F

set TARGET_DIR=%TARGET_DIR:"=%

REM Check path
echo %TARGET_DIR%\..\img>__temp_img_path.txt

if not exist "%TARGET_DIR%\..\img" (
	mkdir "%TARGET_DIR%\..\img"
)

set /p IMG_DIR=<__temp_img_path.txt

rem Version
set /p rtos_version=<"%~dp0\..\..\tools\version\genfiles\rtos_version_str.txt"
set /p ueboot_version_str=<"%~dp0\..\..\tools\version\genfiles\ueboot_version_str.txt"

rem === dmtpmcfg
IF exist "%~dp0"\..\SBOOT\dmtpmcfg (
	pushd "%~dp0"\..\SBOOT\dmtpmcfg\
	IF exist sboot_hbk1_cache_cert.ini (
		if exist TEMP1.ini (
			del /q TEMP1.ini
		)
		rem Erase TITLE,IMAGE Line
		type sboot_hbk1_cache_cert.ini | findstr /v /c:"TITLE =" | findstr /v /c:"IMAGE ="> TEMP1.ini
		echo TITLE = %OS_TYPE%RTOS-%rtos_version%>> TEMP1.ini
		set VERIFY_RTOS_TITLE=%OS_TYPE%RTOS-%rtos_version%
		echo IMAGE = "%IMG_DIR%\%ModelName%_%OS_TYPE%RTOS-%rtos_version%.img">> TEMP1.ini
		rem Erase NULL Line
		type TEMP1.ini|findstr /v "^$">sboot_hbk1_cache_cert.ini
	) else (
		echo [%date% %time%][%FILENAME%] sboot_hbk1_cache_cert.ini file does not exist][%1%\SBOOT\dmtpmcfg]>> ..\..\build_error.log
		exit /b
	)

	IF exist sboot_hbk1_ueboot_cert.ini (
		for /f "tokens=3 delims=\" %%i in ('type sboot_hbk1_ueboot_cert.ini ^|findstr /L "SFDP"') do (
			set sfdpWithFileExtension=%%i
			set sfdp=!sfdpWithFileExtension:~0,-4!
			if exist TEMP3.ini (
				del /q TEMP3.ini
			)
			type sboot_hbk1_ueboot_cert.ini | findstr /v /c:"TITLE =" | findstr /v /c:"IMAGE ="> TEMP3.ini

			if not exist TEMP3.ini (
				echo [%date% %time%][%FILENAME%] #1 SFDP parsing of the sboot_hbk1_ueboot_cert.ini file failed.[%1%\SBOOT\dmtpmcfg]>> ..\..\build_error.log
				exit /b 2
			)

			echo TITLE = %OS_TYPE%BOOT-%ueboot_version_str%_!sfdp!>>TEMP3.ini
			set VERIFY_BOOT_TITLE=%OS_TYPE%BOOT-%ueboot_version_str%_!sfdp!
			echo IMAGE = "%IMG_DIR%\%ModelName%_%OS_TYPE%BOOT-%ueboot_version_str%_!sfdp!.img">>TEMP3.ini
			type TEMP3.ini|findstr /v "^$">sboot_hbk1_ueboot_cert.ini
		)

		if not exist TEMP3.ini (
			echo [%date% %time%][%FILENAME%] #2 SFDP parsing of the sboot_hbk1_ueboot_cert.ini file failed.[%1%\SBOOT\dmtpmcfg]>> ..\..\build_error.log
			exit /b 2
		)

	) else (
		echo [%date% %time%][%FILENAME%] sboot_hbk1_ueboot_cert.ini file does not exist.[%1%\SBOOT\dmtpmcfg]>> ..\..\build_error.log
		exit /b
	)
) else (
	echo [%date% %time%][%FILENAME%]dmtpmcfg Folder does not exist[%1%\SBOOT]>> "%1\build_error.log"
)
REM pause

REM Verify *****************************
call :strlen ver_rtos_len VERIFY_RTOS_TITLE
call :strlen ver_boot_len VERIFY_BOOT_TITLE

if %ver_rtos_len% GTR %VER_LEN_MAX% (
	if not exist "%1\build_error.log" (
		type NUL > "%1\build_error.log"
	)
	echo [%date% %time%][%FILENAME%] [Error] Version string length.^(MAX_LEN: %VER_LEN_MAX%, cur_len: %ver_rtos_len% %VERIFY_RTOS_TITLE%^)>> "%1\build_error.log"
)

if %ver_boot_len% GTR %VER_LEN_MAX% (
	if not exist "%1\build_error.log" (
		type NUL > "%1\build_error.log"
	)
	echo [%date% %time%][%FILENAME%] [Error] Version string length.^(MAX_LEN: %VER_LEN_MAX%, cur_len: %ver_boot_len% %VERIFY_BOOT_TITLE%^)>> "%1\build_error.log"
)

echo [%0] END
goto :eof

REM -------------------------------- 
REM ----------- function ----------- 
REM -------------------------------- 
:strlen <resultVar> <stringVar>
(   
    setlocal EnableDelayedExpansion
    (set^ tmp=!%~2!)
    if defined tmp (
        set "len=1"
        for %%P in (4096 2048 1024 512 256 128 64 32 16 8 4 2 1) do (
            if "!tmp:~%%P,1!" NEQ "" ( 
                set /a "len+=%%P"
                set "tmp=!tmp:~%%P!"
            )
        )
    ) ELSE (
        set len=0
    )
)
( 
    endlocal
    set "%~1=%len%"
    exit /b
)