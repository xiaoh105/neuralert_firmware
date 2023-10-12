#!/bin/bash
export LANG=C
echo Setup secure tool for python code
rm ./mk_sboot_image.sh
cp mk_sboot_image.sh.py mk_sboot_image.sh

rm ./prebuild_start.sh
cp prebuild_start.sh.py prebuild_start.sh

./set_linux_perm.sh