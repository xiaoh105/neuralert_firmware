#!/bin/bash
export LANG=C
# echo *========================================
# echo * postbuild start
# echo *========================================
# echo
# echo PROJ_DIR		= $1
# echo TOOLKIT_DIR	= $2
# echo TARGET_DIR	= $3
# echo TARGET_BNAME	= $4
# echo MODEL_NAME	= $5
# echo
# echo *========================================

org_path=`pwd`
cd  "$1"

OS_TYPE=`uname`

if [[ $OS_TYPE == "Linux" || $OS_TYPE == "Darwin" ]]; then
        echo *========================================
        echo *Post-Build Start for Linux
        echo *========================================
        ./util/mk_sboot_image.sh "$1" "$2" "$3" "$4" "$5"

		if [ $? -eq 0 ]
		then
			echo *========================================
			echo *Post-Build Clean Start for Linux
			echo *========================================
			./util/mk_sboot_image_clean.sh
		else
		   echo "Build Error"
		fi

        echo *========================================
        echo *Post-Build End for Linux
        echo *========================================
else
        echo *========================================
        echo *Post-Build Start for Windows
        echo *========================================
      	./util/mk_sboot_image.bat "$1" "$2" "$3" "$4" "$5"

		if [ $? -eq 0 ]
		then
			echo *========================================
			echo *Post-Build Clean Start for Windows
			echo *========================================
			./util/mk_sboot_image_clean.bat
		else
		   echo "Build Error"
		fi

        echo *========================================
        echo *Post-Build End for Windows
        echo *========================================
fi
cd "$org_path"
