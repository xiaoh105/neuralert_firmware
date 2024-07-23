#!/bin/bash
export LANG=C
echo Setup secure tool for exe file
rm ./mk_sboot_image.sh
cp mk_sboot_image.sh.exe mk_sboot_image.sh

rm ./prebuild_start.sh
cp prebuild_start.sh.exe prebuild_start.sh

./set_linux_perm.sh