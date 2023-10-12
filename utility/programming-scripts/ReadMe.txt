
Programming DA16x00 with SEGGER J-Link Commander.

Introduction

- The following chapter introduces the way of programming DA16x00 with SEGGER J-Link Commander.
- For support, please contact: alvin.park@diasemi.com

Requirements

- J-Link Lite or higher (https://www.segger.com/products/debug-probes/j-link/models/model-overview/)
- J-Link software V6.98 or later (https://www.segger.com/downloads/jlink/)

Preparation

- The J-Link setup procedure required in order to work with J-Flash is described in chapter 2 of the J-Link / J-Trace User Guide (UM08001).
- The Flash loader for DA16x00 should be installed.

Programming

1. Make one combined binary with all images:
  > python3 mkimage.py -o output.bin BOOT.img 0x0 RTOS#1.img 0x23000 RTOS#2.img 0x1e2000 BLE.img 0x3ad000
    -h : show the usage
    -f : force create the output file
    -o : output file path (Default: images.bin)
    -i : select boot image index (Default: 0)

2. Do the chip erase:
  > python3 programmer.py -p "C:\Program Files (x86)\SEGGER\JLink" chip_erase
    -h : show the usage
    -p : the path of the installation of J-Link
    -d : select the device

3. Do programming the combined binary file:
  > python3 programmer.py -p "C:\Program Files (x86)\SEGGER\JLink" write_bin output.bin
