@echo off
rem "$PROJ_DIR$\util\mk_boot_loader_image.bat" "$PROJ_DIR$" "$TOOLKIT_DIR$" "$TARGET_DIR$" $TARGET_BNAME$
rem PROJ_DIR    :1 %1
rem TOOLKIT_DIR :2 %2
rem TARGET_DIR  :3 %3
rem TARGET_BNAME:4 %4
rem ==========================================================================

set BIN_FOLDER=%1\SBOOT\image
set BIN_FOLDER=%BIN_FOLDER:/=\%
set BIN_FOLDER=%BIN_FOLDER:"=%

if not exist "%BIN_FOLDER%" mkdir "%BIN_FOLDER%"

echo Invoking: GNU ARM Cross Create Flash Image(bin)
arm-none-eabi-objcopy -O binary %3\%4.elf  %3\DA16xxx_%4.bin

echo copy binary to build tool folder %BIN_FOLDER%

set COPY_BIN=%BIN_FOLDER%\DA16xxx_%4.bin
set ORG_BIN=%3\DA16xxx_%4.bin
set ORG_BIN=%ORG_BIN:"=%

copy /Y  "%ORG_BIN%" "%COPY_BIN%"