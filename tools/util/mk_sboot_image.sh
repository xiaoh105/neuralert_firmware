#!/bin/bash
export LANG=C
echo Start mk_sboot_image.sh
# "$PROJ_DIR$\util\mk_sboot_image.sh" "$PROJ_DIR$" "$TOOLKIT_DIR$" "$TARGET_DIR$" "$TARGET_BNAME$"
# echo PROJ_DIR    :1 $1
# echo TOOLKIT_DIR :2 $2
# echo TARGET_DIR  :3 $3
# echo TARGET_BNAME:4 $4
# echo MODEL_NAME  :5 $5

FILENAME=mk_sboot_image.sh
BIN_FOLDER=$1/SBOOT/image
TOOL_NAME=da16secutool

if [ -f "${BIN_FOLDER}" ]; then
  mkdir "${BIN_FOLDER}"
fi

echo Invoking: GNU ARM Cross Create Flash Image bin
arm-none-eabi-objcopy -O binary "$3/$4.elf"  "$3/$4.bin"

echo copy binary to build tool folder $BIN_FOLDER
cp "$3/$4.bin" "$BIN_FOLDER/DA16xxx_RTOS.bin"

# echo Delete the old error log file.
rm -f "$1/build_error.log"

if [ -f "$1/SBOOT/certtool/$TOOL_NAME" ]; then
	pushd "$1/SBOOT"
	./CM.3.secuboot.sh "$1" "$2" "$3" "$4" "$5"
	popd
else
	echo $TOOL_NAME file does not exist! [%1/SBOOT/certtool/]>>"$1/build_error.log"
	echo Error: DA16xxx_RTOS.bin Not found
	exit 1
fi
