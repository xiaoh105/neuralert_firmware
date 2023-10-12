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
    filename = []
    ofilename = str()
    MergeStr = bytes()
    OMergeStr = bytes()
    
    if len(sys.argv) < 3 :
        exit(1)

    if not "-out" in sys.argv:
        exit(1)

    for idx in range(1, (sys.argv.index("-out"))):
        filename.append( sys.argv[idx] )

    ofilename = sys.argv[sys.argv.index("-out") + 1]

    sandbox = 0

    if "-sbox" in sys.argv:
        sandbox = int(sys.argv[sys.argv.index("-sbox") + 1], 16)

    #print( filename )
    #print( ofilename )

    for infile in filename:
        fname, ext = os.path.splitext(infile)

        if ext == '.bin':
            MergeStr += ( GetDataFromBinFile(infile) )
        else:
            MergeStr += ( GetDataFromTxtFile(infile) )

    if( sandbox != 0 ):
        tag = 0x52446B63
        print( "sandbox : %08x.%08x" % (tag, sandbox) )
        bytetag = tag.to_bytes(4, 'little')
        bytesbox = sandbox.to_bytes(4, 'little')

        OMergeStr += bytetag
        OMergeStr += bytesbox
        OMergeStr += MergeStr[8:]

    else:
        OMergeStr = MergeStr

    if ofilename != "" :
        wrfile = open(ofilename, "wb")
        wrfile.write(OMergeStr)
        wrfile.close()
    
    
    
