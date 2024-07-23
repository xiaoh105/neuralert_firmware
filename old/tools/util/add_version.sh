#!/bin/bash
####################################
# Do not edit with Windows editor! #
####################################
export LANG=C
## "$PROJ_DIR/util/add_version.sh" "$PROJ_DIR" "$TOOLKIT_DIR" "$TARGET_DIR" $TARGET_BNAME $ModelName
# ======================================"
# ADD VERSION
# ======================================"
# PROJ_DIR    :1 $1
# TOOLKIT_DIR :2 $2
# TARGET_DIR  :3 $3
# TARGET_BNAME:4 $4
# ModelName	 :5 $5

ModelName=$5
FILENAME=add_version.sh
VERIFY_RTOS_TITLE=""
VERIFY_BOOT_TITLE=""
VER_LEN_MAX=39

# FreeRTOS
OS_TYPE=F

called_path=${0%/*}
stripped=${called_path#[^/]*}
script_path=`pwd`/$stripped
if [[ "$ModelName" -eq "DA16200" ]];then
	ModelPath=da16200
elif [[ "$ModelName" -eq "DA16600" ]];then
	ModelPath=da16600
fi

# Check path
TARGET_DIR=$3
SDK_ROOT_DIR=${1%/tools*}
echo $TARGET_DIR/../img>__temp_img_path.txt

# PWD: build/util/ :: ../../img
if [ ! -d "$TARGET_DIR/../img" ]; then
	mkdir "$TARGET_DIR/../img"
fi


IMG_DIR=`cat __temp_img_path.txt`

# Version
rtos_version_str=`cat "$SDK_ROOT_DIR/tools/version/genfiles/rtos_version_str.txt"`
rtos_version_str=`echo ${rtos_version_str%}`

ueboot_version_str=`cat "$SDK_ROOT_DIR/tools/version/genfiles/ueboot_version_str.txt"`
ueboot_version_str=`echo ${ueboot_version_str%}`

echo RTOS Ver: ${rtos_version_str}
echo Boot Ver: ${ueboot_version_str}

#=== cmtpmcfg
if [ -d "$script_path/../SBOOT/dmtpmcfg" ]; then
	pushd "$script_path/../SBOOT/dmtpmcfg/"
	if [ -f sboot_hbk1_cache_cert.ini ]; then
		rm -f TEMP.ini
		# Erase TITLE, IMAGE Line
		cat sboot_hbk1_cache_cert.ini | grep -v "TITLE =" | grep -v "IMAGE =">TEMP1.ini
		echo TITLE = ${OS_TYPE}RTOS-$rtos_version_str>>TEMP1.ini
		VERIFY_RTOS_TITLE=${OS_TYPE}RTOS-$rtos_version_str
		echo IMAGE = ${IMG_DIR}/${ModelName}_${OS_TYPE}RTOS-${rtos_version_str}.img>>TEMP1.ini
		# Erase empty line
		cat TEMP1.ini | grep -v -e '^$'>sboot_hbk1_cache_cert.ini
	else
		tmp_date=`date "+%Y%m%d%H%M%S"`
		echo [$tmp_date][$FILENAME] sboot_hbk1_cache_cert.ini file does not exist][$SDK_ROOT_DIR/build/SBOOT/dmtpmcfg]>> ../../build_error.log
                exit 1
	fi
        
	if [ -f sboot_hbk1_ueboot_cert.ini ]; then
		sfdpWithFileExtension=`cat sboot_hbk1_ueboot_cert.ini | grep SFDP`
		sfdpWithFileExtension=`echo ${sfdpWithFileExtension/SFDP = ..\/SFDP\//}`
		sfdp=`echo ${sfdpWithFileExtension/.bin/}`
		rm -f TEMP3.ini
		cat sboot_hbk1_ueboot_cert.ini | grep -v "TITLE =" | grep -v "IMAGE =">TEMP3.ini

		echo TITLE = ${OS_TYPE}BOOT-${ueboot_version_str}_${sfdp}>>TEMP3.ini
		VERIFY_BOOT_TITLE=${OS_TYPE}BOOT-$ueboot_version_str_${sfdp}
		echo IMAGE = ${IMG_DIR}/${ModelName}_${OS_TYPE}BOOT-${ueboot_version_str}_${sfdp}.img>>TEMP3.ini
		# Erase empty line
                cat TEMP3.ini | grep -v -e '^$'>sboot_hbk1_ueboot_cert.ini
	else
		tmp_date=`date "+%Y%m%d%H%M%S"`
		echo [$tmp_date][$FILENAME] sboot_hbk1_ueboot_cert.ini file does not exist.[$SDK_ROOT_DIR/build/SBOOT/dmtpmcfg]>> ../../build_error.log
                exit 1
	fi
else
	tmp_date=`date "+%Y%m%d%H%M%S"`
	echo [$tmp_date][$FILENAME] dmtpmcfg Folder does not exist[$SDK_ROOT_DIR/build/SBOOT]>> $SDK_ROOT_DIR/build/build_error.log
fi

# Verify *****************************
tmp_date=`date "+%Y%m%d%H%M%S"`
if [ ${#VERIFY_RTOS_TITLE} -gt $VER_LEN_MAX ]; then
	if [ ! -f "$1/build_error.log" ]; then
		touch > "$1/build_error.log"
	fi
	echo [$tmp_date][$FILENAME] [Error] Version string length.[MAX_LEN: $VER_LEN_MAX, cur_len: ${#VERIFY_RTOS_TITLE} $VERIFY_RTOS_TITLE]>> "$1/build_error.log"
fi

if [ ${#VERIFY_BOOT_TITLE} -gt $VER_LEN_MAX ]; then
	if [ ! -f "$1/build_error.log" ]; then
		touch > "$1/build_error.log"
	fi
	echo [$tmp_date][$FILENAME] [Error] Version string length.[MAX_LEN: $VER_LEN_MAX, cur_len: ${#VERIFY_RTOS_TITLE} $VERIFY_BOOT_TITLE]>> "$1/build_error.log"
fi
