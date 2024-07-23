@echo off
echo Setup secure tool for python code
del ./mk_sboot_image.bat
copy mk_sboot_image.bat.py mk_sboot_image.bat

del ./prebuild_start.sh
copy prebuild_start.sh.py prebuild_start.sh
pause