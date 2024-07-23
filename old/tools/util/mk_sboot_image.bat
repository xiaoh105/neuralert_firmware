@echo off
echo [%0] START
chcp 437
rem "$PROJ_DIR$\util\mk_sboot_image.bat" "$PROJ_DIR$" "$TOOLKIT_DIR$" "$TARGET_DIR$" $TARGET_BNAME$
rem PROJ_DIR    :1 %1
rem TOOLKIT_DIR :2 %2
rem TARGET_DIR  :3 %3
rem TARGET_BNAME:4 %4
rem MODEL_NAME:5 %5

set BUILD_PATH=%1
REM Change Windows type Path
set BUILD_PATH=%BUILD_PATH:/=\% 

set FILENAME=mk_sboot_image.bat
set BIN_FOLDER=%~dp0..\SBOOT\image
set TOOLNAME=da16secutool.exe

if not exist "%BIN_FOLDER%" mkdir "%BIN_FOLDER%"

@echo Invoking: GNU ARM Cross Create Flash Image(bin)
arm-none-eabi-objcopy -O binary %3\%4.elf  %3\%4.bin

@echo copy binary to build tool folder %BIN_FOLDER%
copy %3\%4.bin "%BIN_FOLDER%\DA16xxx_RTOS.bin"

REM Delete the old error log file.
if exist %1\build_error.log (
  del /q %1\build_error.log
)

if not exist "%~dp0\..\SBOOT\certtool\%TOOLNAME%" (
	echo [%date% %time%][%FILENAME%] %TOOLNAME% file does not exist! [%1\SBOOT\certtool\]>> %1\build_error.log
	@echo Error: DA16xxx_RTOS.bin Not found
) else (
	pushd "%~dp0"\..\SBOOT
	call CM.3.secuboot.bat %BUILD_PATH% %2 %3 %4 %5
	popd
)
echo [%0] END
exit /b %ERRORLEVEL%


