#!/usr/bin/python

__author__ = "Alvin Park"
__copyright__ = "Copyright 2022, Renesas Electronics"
__credits__ = ["Alvin Park"]
__license__ = "GPL"
__version__ = "0.1.1"
__maintainer__ = "Alvin Park"
__email__ = "alvin.park.pv@renesas.com"
__status__ = "Development"

import os
import sys, getopt

def is_hex(s):
    try:
        if s.startswith('0x'):
            int(s, 0)
        else:
            int(s, 16)
        return True
    except ValueError:
        return False

def addr2hex(addr):
    if not addr.startswith('0x'):
        hex = "0x" + addr
    else:
        hex = addr
    return int(hex, 0)
    
def help():
    print("mkimage.py Version:"+__version__+" ("+__status__+")")
    print(__copyright__)
    print("""usage: mkimage.py [-h] [-f] 
    [-o <path of output file>] [-i <index of a bootable image>] [-s <size of the flash memory map>]
    <image#1.img> <address#1> <image#2.img> <address#2> ...
      -h : show the usage.
      -f : force create the output file.
      -o : a path of the output file. (Default: images.bin)
      -i : the index of a bootable image. 0 or 1 (Default: 0)
      -s : the size of the flash memory map. 2(2MB) or 4(4MB) (Default : 4)
    """)

def main(argv):
    force = False
    index = 0
    output = 'images.bin'
    images = {}
    size = 4

    try:
        opts, args = getopt.getopt(argv,"hfi:o:s:",["index=","output=","size="])
    except getopt.GetoptError:
        help()
        sys.exit(2)

    if len(args) == 0 or (len(args) % 2):
        help()
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            help()
            sys.exit()
        elif opt == '-f':
            force = True
        elif opt in ("-i", "--index"):
            try:
                index = int(arg)
            except ValueError:
                help()
                sys.exit()
        elif opt in ("-o", "--output"):
            output = arg
        elif opt in ("-s", "--size"):
            try:
                size = int(arg)
            except ValueError:
                help()
                sys.exit()

    for idx in range(len(args)):
        if idx % 2 == 0 and args[idx].endswith('.img') and is_hex(args[idx+1]):
            images[addr2hex(args[idx+1])] = args[idx]

    if len(images) == 0:
        help()
        sys.exit()

    if size != 4 and size != 2:
        print('ERROR: Wrong size. Please check the size! - ' + str(size))
        sys.exit()

    mode = 'xb'
    if force == True:
        mode = 'wb'
    try:
        with open(output, mode) as outfile:
            for addr, image in images.items():
                print('Writing ' + image + ' at ' + hex(addr))

                # Append padding
                if addr > outfile.tell():
                    padding_size = addr - outfile.tell()
                    while padding_size > 0:
                        outfile.write(b'\xFF')
                        padding_size -= 1
                elif addr < outfile.tell():
                    outfile.seek(addr)

                # Write image
                try:
                    with open(image, 'rb') as infile:
                        outfile.write(infile.read())
                except FileNotFoundError as e:
                    print(e)

            # boot index at 0x22000
            # index 0       : 0C AF BE AD DE 00 00 00 00 00 30 02 00
            # index 1 - 4MB : 0C AF BE AD DE 01 00 00 00 00 20 1E 00
            # index 1 - 2MB : 0C AF BE AD DE 01 00 00 00 00 B0 10 00
            bootinfo = bytearray(b'\x0C\xAF\xBE\xAD\xDE\x00\x00\x00\x00\x00\x30\x02\x00')
            if index > 1:
                print('ERROR: Wrong boot index. Please check the boot index! - ' + str(index))
                sys.exit()
            elif index == 1:
                if size == 4:
                    bootinfo = bytearray(b'\x0C\xAF\xBE\xAD\xDE\x01\x00\x00\x00\x00\x20\x1E\x00')
                elif size == 2:
                    bootinfo = bytearray(b'\x0C\xAF\xBE\xAD\xDE\x01\x00\x00\x00\x00\xB0\x10\x00')
            
            addr = addr2hex('0x22000')
            if addr > outfile.tell():
                padding_size = addr - outfile.tell()
                while padding_size > 0:
                    outfile.write(b'\xFF')
                    padding_size -= 1
            elif addr < outfile.tell():
                outfile.seek(addr)
            outfile.write(bootinfo)

    except FileExistsError as e:
        print(e)

    size = os.path.getsize(output)
    print("The binary has been generated - {0}, Size : {1} Bytes".format(output, size, 'd'))

if __name__ == "__main__":
   main(sys.argv[1:])
