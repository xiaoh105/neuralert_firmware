#!/bin/bash
export LANG=C
# echo *========================================
# echo * prebuild
# echo *========================================
# echo
# echo PROJ_DIR		= $1
# echo TOOLKIT_DIR	= $2
# echo TARGET_DIR	= $3
# echo TARGET_BNAME	= $4
# echo
# echo *========================================

pushd  "$1/util/"

echo Generate RamLibrary / TIM
if [ -f  file2hexa.sh ]; then
	./file2hexa.sh
fi

# library
cur_path=`pwd`
libdir="$cur_path/../../library/"
if [ ! -d "$libdir" ]; then
    mkdir "$libdir"
fi

echo Make build Time Info
if [ -f  ../version/genfiles/buildtime.h ]; then
	rm -f ../version/genfiles/buildtime.h
fi
./mk_buildtime.sh $1 $2 $3 $4

echo Make build Verstion Info
if [ -f  ../version/genfiles/rtos_version_str.txt ]; then
	rm -f ../version/genfiles/rtos_version_str.txt
fi
./mk_version.sh $1 $2 $3 $4

popd
