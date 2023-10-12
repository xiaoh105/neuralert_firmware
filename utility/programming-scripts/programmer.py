#!/usr/bin/env python

__author__ = "Alvin Park"
__copyright__ = "Copyright 2022, Renesas Electronics"
__credits__ = ["Alvin Park"]
__license__ = "GPL"
__version__ = "0.0.1"
__maintainer__ = "Alvin Park"
__email__ = "alvin.park.pv@renesas.com"
__status__ = "Development"

import os
import sys, getopt
import platform
import subprocess
import shutil

def help():
    print("programmer.py Version:"+__version__+" ("+__status__+")")
    print(__copyright__)
    print("""usage: programmer.py [-h] [-p <path of J-Link>] [-d <device>] <command> [parameters]
      -h : show the usage.
      -p : the path of the J-Link.
      -d : the device
      <command> : 
        chip_erase : do chip erase.
        erase_all : same as chip_erase.
        write_bin <path of the binary file> : write the binary file.
    """)

def main(argv):
    jlink_path = None
    device = 'DA16200.eclipse.4MB'
    os_name = platform.system()

    try:
        opts, args = getopt.getopt(argv,"hd:p:",["device=", "path="])
    except getopt.GetoptError:
        help()
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            help()
            sys.exit()
        elif opt in ("-d", "--device"):
            device = arg
        elif opt in ("-p", "--path"):
            path = os.path.normpath(arg)
            if os.path.isdir(arg) and os.path.exists(path):
                jlink_path = path

    if len(args) == 0:
        help()
        sys.exit(2)

    cmd = args[0]

    if jlink_path is not None:
        if os_name == 'Windows':
            jlink_path = jlink_path+'\JLink.exe'
        else:
            jlink_path = jlink_path+'/JLinkExe'

    if jlink_path is None or not os.path.exists(jlink_path):
        print('JLink has not been found! Please check the installation of JLink.')
        sys.exit(2)

    log_file = './jlink/log.txt'

    if cmd == 'chip_erase' or cmd == 'erase_all':
        result = subprocess.run([jlink_path, "-device", device, "-Log", log_file, "-CommanderScript", "./jlink/erase_all.jlink"])
    elif cmd == 'write_bin':
        if len(args) < 2:
            print('Please input the path of binary file!')
            sys.exit(2)

        file = os.path.normpath(args[1])
        temp_file = './jlink/firmware.bin'
        if os.path.exists(file) and os.path.isfile(file):
            if os.path.exists(temp_file):
                os.remove(temp_file)
            shutil.copyfile(src=file, dst=temp_file)
            result = subprocess.run([jlink_path, "-device", device, "-Log", log_file, "-CommanderScript", "./jlink/loadbin.jlink"])
            os.remove(temp_file)
        else:
            print('The file has not been found! Please check the binary file path.')
            sys.exit(2)
    else:
        print("Uknown command: {}".format(cmd))

if __name__ == '__main__':
    main(sys.argv[1:])
