#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct


def CreateBinFile(binStr, certFileName):
    try:
        # Open a binary file and write the data to it
        FileObj = open(certFileName, "wb")
        FileObj.write(bytes(binStr.encode('iso-8859-1')))
        FileObj.close()

    except IOError as Error7:
        sys.exit(1)
    return       


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

def nvcounter_parser (nvcnt_fname):

    try:
        config_file = open(nvcnt_fname, 'r')
    except IOError as e:
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.readfp(config_file)
    config_file.close()    
    
    NVCount = 0
    HBKID   = 0
    
    section_name = "FC9K-TPM-CONFIG"
    if not config.has_section(section_name):
        return NVCount, HBKID

    if config.has_option(section_name, 'NVCount'): 
        NVCount = int(config.get(section_name, 'NVCount'))

    if config.has_option(section_name, 'hbk-id'): 
        HBKID = int(config.get(section_name, 'hbk-id'), 16)

    return NVCount, HBKID

##########################################################

def target_config_update(config_fname, nvcounter, hbkid):

    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.optionxform = str
    config.readfp(config_file)
    config_file.close()

    config_update = 0
    
    section_name = "CMPU-PKG-CFG"
    if config.has_section(section_name):
        if config.has_option(section_name, 'minversion'): 
            config.set(section_name, 'minversion', str(nvcounter))
            print( 'minversion = %d' % nvcounter )
            config_update = 1

    section_name = "DMPU-PKG-CFG"
    if config.has_section(section_name):
        if config.has_option(section_name, 'minversion'): 
            config.set(section_name, 'minversion', str(nvcounter))
            print( 'minversion = %d' % nvcounter )
            config_update = 1

    section_name = "KEY-CFG"
    if config.has_section(section_name):
        if config.has_option(section_name, 'nvcounter-val'): 
            config.set(section_name, 'nvcounter-val', str(nvcounter))
            print( 'nvcounter-val = %d' % nvcounter )
            config_update = 1

        if config.has_option(section_name, 'hbk-id'): 
            config.set(section_name, 'hbk-id', str(hbkid))
            print( 'hbk-id = %d' % hbkid )
            config_update = 1

    section_name = "CNT-CFG"
    if config.has_section(section_name):
        if config.has_option(section_name, 'nvcounter-val'): 
            config.set(section_name, 'nvcounter-val', str(nvcounter))
            print( 'nvcounter-val = %d' % nvcounter )
            config_update = 1

    section_name = "ENABLER-DBG-CFG"
    if config.has_section(section_name):
        if config.has_option(section_name, 'hbk-id'): 
            config.set(section_name, 'hbk-id', str(hbkid))
            print( 'hbk-id = %d' % hbkid )
            config_update = 1
			
    if config_update == 1:
        config_file = open(config_fname, 'w')
        config.write(config_file)
        config_file.close()
    
##################################
#       Main function
##################################
        
if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    PROJ_CONFIG = "../cmconfig/cmpu.cfg"
    NVCNT_CONFIG = "../cmpublic/nvcounter.cfg"
    
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    if "-nvc_file" in sys.argv:
        NVCNT_CONFIG = sys.argv[sys.argv.index("-nvc_file") + 1]
    #print("NVC File  - %s\n" %NVCNT_CONFIG)

    nvcounter, hbkid = nvcounter_parser(NVCNT_CONFIG)

    print("NVCounter = %d" % nvcounter )
    print("HBK-ID    = %d" % hbkid )

    target_config_update( PROJ_CONFIG, nvcounter, hbkid )

    
    
