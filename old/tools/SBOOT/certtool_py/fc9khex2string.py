#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct



def GetDataFromBinFile(certFileName):
    binStr = str()
    try:
        # Open a binary file and write the data to it
        FileObj = open(certFileName, "rb")
        binStr = FileObj.read()
        FileObj.close()

    except IOError as Error7:
        (errno, strerror) = Error7.args
        sys.exit(1)
    return binStr


def WriteHexFile(binStr, hex_var_name, hexfilename):
	hexfile = open(hexfilename, 'w')

	hexstr = str()

	hexstr = "unsigned char " + hex_var_name + "[] = {\r\n"           

	i = 0
	for s in bytes(binStr):
		hexstr = hexstr + '0x%02x, '% s
		i = i + 1
		if (i%16) == 0:
			hexstr = hexstr + '\r\n'

	hexstr = hexstr + "};\r\n"
		
	hexfile.write(hexstr)
		
	# Close files        
	hexfile.close()

	return

   
##################################
#       Main function
##################################
        
if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    binstr = GetDataFromBinFile(sys.argv[2])
	
    WriteHexFile(binstr, sys.argv[1], sys.argv[3])

