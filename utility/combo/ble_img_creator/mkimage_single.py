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
SINGLE_OTA = "single_ota"
IMG_POST = ".img"
USER_QUERY = "please enter output img name (def: da14531_pxr_single): "
DEF_SINGLE_IMG = "da14531_pxr_single"

#temp_imgs = [img1_out, img2_out]

single_img_out = input(USER_QUERY) or DEF_SINGLE_IMG

cmdline_single = mkimg_file + SPACE + \
                 SINGLE_OTA + SPACE + \
                 ble_bin    + SPACE + \
                 ver_file   + SPACE + \
                 single_img_out + IMG_POST

print(cmdline_single)

os_runprog(cmdline_single)
time.sleep(1)
