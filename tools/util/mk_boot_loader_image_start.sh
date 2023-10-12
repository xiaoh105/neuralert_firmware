#!/bin/bash
export LANG=C
# echo *========================================
# echo * boot loader image  start
# echo *========================================
# echo
# echo PROJ_DIR		= $1
# echo TOOLKIT_DIR	= $2
# echo TARGET_DIR		= $3
# echo TARGET_BNAME	= $4
# echo
# echo *========================================

org_path=`pwd`
cd "$1"

OS_TYPE=`uname`

if [[ $OS_TYPE == "Linux" ]]; then
        echo *========================================
        echo *Post-Build Start for Linux
        echo *========================================
	      ./util/mk_boot_loader_image.sh "$1" "$2" "$3" "$4"
        echo *========================================
        echo *Post-Build End for Linux
        echo *========================================
else
        echo *========================================
        echo *Post-Build Start for Windows
        echo *========================================
        cmd
      	./util/mk_boot_loader_image.bat "$1" "$2" "$3" "$4"
        echo *========================================
        echo *Post-Build End for Windows
        echo *========================================
fi
cd "$org_path"
