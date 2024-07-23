#!/bin/bash
echo TPM Renewal

#PROJ_DIR    :1 $1
#TOOLKIT_DIR :2 $2
#TARGET_DIR  :3 $3
#TARGET_BNAME:4 $4
#MODEL_NAME:5 $5

MODEL_NAME=$5
FILE_NAME=CM3.secuboot.sh
TARGET_DIR=$3

TOOLNAME=./certtool/da16secutool
#TOOLNAME=./certtool_py/da16secutool.py

if [[ "MODEL_NAME" -eq "DA16200" ]]; then
	MODEL_PATH=da16200
elif [[ "MODEL_NAME" -eq "DA16600" ]]; then
	MODEL_PATH=da16600
fi

echo FreeRTOS
OS_TYPE=F

called_path=${0%/*}
stripped=${called_path#[^/]*}
script_path=`pwd`/$stripped

if [ $# -le 1 ]; then
	IMG_PATH=$TARGET_DIR/../img
	PRJ_PATH=$script_path
else
	IMG_PATH=$TARGET_DIR/../img
	PRJ_PATH=$1
fi

# secure tool log
if [ ! -d "$script_path/example" ]; then
	mkdir "$script_path/example"
fi

# secure tool dmtpmcfg
if [ ! -d "$script_path/dmtpmcfg" ]; then
	mkdir "$script_path/dmtpmcfg"
fi

echo Delete old img
rm -f "$script_path/public/DA16xxx_RTOS.img"
rm -f "$script_path/public/DA16xxx_ueboot_*.img"

if [ $4 == DA16xxx_ueboot ]; then
	pushd "${IMG_PATH}"
	rm -f ${MODEL_NAME}_BOOT*.img
	popd
elif [ $# -ne 1 ]; then
	# For normal build
	pushd "${IMG_PATH}"
	rm -f ${MODEL_NAME}_${OS_TYPE}BOOT*.img
	rm -f ${MODEL_NAME}_${OS_TYPE}RT*.img
	popd
fi

pushd "$script_path"

echo PATH:$script_path

# Generation ini and SBOOT\public\*.img files
# OTA IMG Build or Single Run for CM3.secu_free_rtos.sh
if [ $# -eq 0 ]; then
       	${TOOLNAME} -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg -tpm ./cmtpmcfg
fi

# Normal Build
if [ $# -gt 1 ]; then
       	${TOOLNAME} -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg -tpm ./cmtpmcfg
fi

# === dmtpmcfg
# Check file generation status.
if [ ! -f "$script_path/dmtpmcfg/sboot_hbk1_cache_cert.ini" ]; then
	tmp_date=`date "+%Y%m%d%H%M%S"`
	echo [$tmp_date][$FILE_NAME] The generated 'sboot_hbk1_cache_cert.ini' file does not exist! [$script_path/dmtpmcfg]>>$PRJ_PATH/build_error.log
        exit 2
fi

if [ ! -f "$script_path/dmtpmcfg/sboot_hbk1_ueboot_cert.ini" ]; then
	tmp_date=`date "+%Y%m%d%H%M%S"`
        echo [$tmp_date][$FILE_NAME] The generated 'sboot_hbk1_ueboot_cert.ini' file does not exist! [$script_path/dmtpmcfg]>>$PRJ_PATH/build_error.log
        exit 2
fi

if [ ! -f "$script_path/../util/add_version.sh" ]; then
	tmp_date=`date "+%Y%m%d%H%M%S"`
        echo [$tmp_date][$FILE_NAME] add_version.sh file does not exist! [$script_path/dmtpmcfg]>>$PRJ_PATH/build_error.log
        exit 2
fi

# Add Version Info.
if [ $# -le 1 ]; then
	pushd "$script_path/../util"
	# Trim /SBOOT/
	./add_version.sh "$script_path/../" "$2" "$3" DA16xxx_ueboot $MODEL_NAME
	pushd "$script_path"
else
	pushd "$script_path/../util"
	./add_version.sh "$PRJ_PATH" "$2" "$3" "$4" $MODEL_NAME
	pushd "$script_path"
fi

if [ ! -f "$script_path/dmtpmcfg/TEMP1.ini" ]; then
	exit 2
fi

if [ ! -f "$script_path/dmtpmcfg/TEMP3.ini" ]; then
        exit 2
fi

# rm -f  "$script_path/dmtpmcfg/TEMP*.ini"

IMG_PATH=$(<../util/__temp_img_path.txt)

# Image Generate 
${TOOLNAME} -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg

# Check img files
if [ ! -f "${IMG_PATH}/${MODEL_NAME}_${OS_TYPE}RTOS-*.img" ]; then
	tmp_date=`date "+%Y%m%d%H%M%S"`
	echo [$tmp_date][$FILE_NAME] ${MODEL_NAME}_${OS_TYPE}RTOS-*.img file does not exist! [${IMG_PATH}]>> "$PRJ_PATH/build_error.log"
        exit 2
elif [ ! -f "${IMG_PATH}/${MODEL_NAME}_${OS_TYPE}BOOT-*.img" ]; then
	tmp_date=`date "+%Y%m%d%H%M%S"`
        echo [$tmp_date][$FILE_NAME] ${MODEL_NAME}_${OS_TYPE}BOOT-*.img file does not exist! [${IMG_PATH}]>> "$PRJ_PATH/build_error.log"
        exit 2
else
	exit 0
fi
