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

def GetDataFromTxtFile(certFileName):
    binStr = str()
    outStr = bytes()
    try:
        # Open a binary file and write the data to it
        FileObj = open(certFileName, "rt")
        binStr = FileObj.read()
        FileObj.close()

        binList = binStr.split()
        i = 0
        for binitem in binList:
            binvalue =  binitem.replace(',', '')
            #print( binvalue )
            intvalue = int(binvalue, 16)
            #print( intvalue.to_bytes(4, byteorder='little') )
            outStr += intvalue.to_bytes(4, byteorder='little')


    except IOError as Error7:
        (errno, strerror) = Error7.args
        sys.exit(1)

    #print(outStr)
    return outStr



##########################################################
# Main
##########################################################

if __name__ == "__main__":

    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)
        
    PROD_UNIQUE_BUFF_SIZE = 16
    filename = ''
    ofilename = ''
    
    if len(sys.argv) == 5 :
        filename = sys.argv[sys.argv.index("-in") + 1]
        ofilename = sys.argv[sys.argv.index("-out") + 1]


    uniqueBuff = list(range(PROD_UNIQUE_BUFF_SIZE)) #[PROD_UNIQUE_BUFF_SIZE]

    if filename != "" :
        fname, ext = os.path.splitext(filename)

        if ext == '.bin':
            uniqueBuff = GetDataFromBinFile(filename)
        else:
            uniqueBuff = GetDataFromTxtFile(filename)
    
        #print( uniqueBuff.hex() )

    if ofilename != "" :
        wrfile = open(ofilename, "wb")
        wrfile.write(uniqueBuff)
        wrfile.close()

