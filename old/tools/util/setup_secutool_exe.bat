@echo off
echo Setup secure tool for exe file
del ./mk_sboot_image.bat
copy mk_sboot_image.bat.exe mk_sboot_image.bat

del ./prebuild_start.sh
copy prebuild_start.sh.exe prebuild_start.sh
pause