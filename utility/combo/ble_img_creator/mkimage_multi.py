#!/usr/bin/env python
import os, sys, subprocess, time

def find(pattern, str_list):
    for name in str_list:
        if (name.find(pattern)) != -1:
            return name
    return "not_found"

def os_runprog(path):
	subprocess.Popen(path)

#############################

files = os.listdir()

# get ble bin file name
ble_bin = find(".bin", files)
if ble_bin == "not_found":
    sys.exit(0)

# get version file name
ver_file = find(".h", files)
if ver_file == "not_found":
    sys.exit(0)

# get mkimage exe file name
mkimg_file = find(".exe", files)
if mkimg_file == "not_found":
    sys.exit(0)

# run mkimage command
SPACE = " "
SINGLE = "single"
MULTI = "multi"
SPI = "spi"
IMG1_OFFSET = "0x20"
IMG2_OFFSET = "0x8000"
PH_OFFSET = "0xFF20"
IMG_POST = ".img"
USER_QUERY = "please enter output img name (def: da14531_multi_part_proxr): "
DEF_MULTI_IMG = "da14531_multi_part_proxr"

img1_out = "ble_1.img"
img2_out = "ble_2.img"
temp_imgs = [img1_out, img2_out]

multi_part_img_out = input(USER_QUERY) or DEF_MULTI_IMG

cmdline_1 = mkimg_file + SPACE + \
            SINGLE     + SPACE + \
            ble_bin    + SPACE + \
            ver_file   + SPACE + \
            img1_out

cmdline_2 = mkimg_file + SPACE + \
            SINGLE     + SPACE + \
            ble_bin    + SPACE + \
            ver_file   + SPACE + \
            img2_out

cmdline_3 = mkimg_file  + SPACE + \
            MULTI       + SPACE + \
            SPI         + SPACE + \
            img2_out    + SPACE + \
            IMG1_OFFSET + SPACE + \
            img1_out    + SPACE + \
            IMG2_OFFSET + SPACE + \
            PH_OFFSET   + SPACE + \
            multi_part_img_out + IMG_POST

print(cmdline_1)
print(cmdline_2)
print(cmdline_3)

cmdlines = [cmdline_1, cmdline_2, cmdline_3]

for cmd in cmdlines:
    os_runprog(cmd)
    time.sleep(1)

for temp_img in temp_imgs:
    os.remove(temp_img)
    time.sleep(1)