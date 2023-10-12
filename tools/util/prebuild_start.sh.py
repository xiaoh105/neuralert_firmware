#!/bin/bash
export LANG=C
# echo *========================================
# echo * prebuild start
# echo *========================================
# echo
# echo PROJ_DIR		= $1
# echo TOOLKIT_DIR	= $2
# echo TARGET_DIR		= $3
# echo TARGET_BNAME	= $4
# echo
# echo *========================================

org_path=`pwd`
cd  "$1"

OS_TYPE=`uname`

if [[ $OS_TYPE == "Linux" || $OS_TYPE == "Darwin" ]]; then
        echo *========================================
        echo *Pre-Build Start for Linux
        echo *========================================
        if [ -f ./SBOOT/cmconfig/da16xtpmconfig.cfg ]; then
                echo "da16xtpmconfig.cfg already exist"
        else
                cd ../utility/cfg_generator
                python3 da16x_gencfg.py
                cd "$1"
        fi

        if [ "$#" -eq 5 ]; then
                ./util/prebuild.sh "$1" "$2" "$3" "$4" "$5"
        else
                ./util/prebuild.sh "$1" "$2" "$3" "$4"
        fi
        echo *========================================
        echo *Pre-Build End for Linux
        echo *========================================
else
        echo *========================================
        echo *Pre-Build Start for Windows
        echo *========================================
        if [ -f ./SBOOT/cmconfig/da16xtpmconfig.cfg ]; then
                echo "da16xtpmconfig.cfg already exist"
        else
                cd ../utility/cfg_generator
                python da16x_gencfg.py
                cd "$1"
        fi

        if ! [ -f ./SBOOT/cmconfig/da16xtpmconfig.cfg ]; then
                echo "ERROR: da16xtpmconfig.cfg is not exist!!"
                echo "=============================="
                exit 0
        fi
        
        if [ "$#" -eq 5 ]; then
                cmd
                ./util/prebuild.bat "$1" "$2" "$3" "$4" "$5"
        else
                cmd
                ./util/prebuild.bat "$1" "$2" "$3" "$4"
        fi
        echo *========================================
        echo *Pre-Build End for Windows
        echo *========================================
fi
cd "$org_path"
