#!/bin/bash
export LANG=C
# "$PROJ_DIR$\util\mk_sboot_image.sh" "$PROJ_DIR$" "$TOOLKIT_DIR$" "$TARGET_DIR$" "$TARGET_BNAME$"
# echo PROJ_DIR    :1 $1
# echo TOOLKIT_DIR :2 $2
# echo TARGET_DIR  :3 $3
# echo TARGET_BNAME:4 $4

BIN_FOLDER=$1/SBOOT/image

if [ -f "${BIN_FOLDER}" ]; then
  mkdir "${BIN_FOLDER}"
fi

arm-none-eabi-objcopy -O binary "$3/$4.elf"  "$3/DA16xxx_$4.bin"

echo copy binary to build tool folder
cp "$3/DA16xxx_$4.bin" "$BIN_FOLDER/DA16xxx_$4.bin"


