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

def target_config_init(config_fname):

    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.optionxform = str
    config.readfp(config_file)
    config_file.close()

    config_update = 0
    
    section_name = "FC9K-TPM-CONFIG"
    if config.has_section(section_name):

        print( '*===================================================================*\n'
               '* [NOTICE] After updating the LCS,                                   \n'
               '*     You MUST regenerate all certificates and images !!             \n'
               '*===================================================================*\n' )

        print( 'Current lcs = %s' % str(config.get(section_name, 'lcs')) )
        value = input("Enter the lcs\n(0 = CM; 1 = DM LCS; 5 = Secure; 7 = RMA, skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'lcs', str(value) )
            print( 'lcs = %s' % str(value) )
            config_update = 1

        print( 'Current hbk-id = %s' % str(config.get(section_name, 'hbk-id')) )
        value = input("Enter the hbk-id\n(0 = CM; 1 = DM, skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'hbk-id', str(value) )
            print( 'hbk-id = %s' % str(value) )		
            config_update = 1

        print( '*===================================================================*\n'
               '* [NOTICE] After updating the NVCOUNT,                               \n'
               '*     You MUST regenerate all keychains, certificates and images !!  \n'
               '*===================================================================*\n' )
        
        print( 'Current nvcount = %s' % str(config.get(section_name, 'nvcount')) )
        value = input("Enter the initial value of nvcount\n(0..63 for HBK0 Trusted FW; 0..95 for HBK1 Trusted FW, skip = Enter): ")

        if str(value) != '':
            config.set(section_name, 'nvcount', str(value) )
            print( 'nvcount = %s' % str(value) )
            config_update = 1

        print( 'Current sdebug = %s' % str(config.get(section_name, 'sdebug')) )
        value = input("Select the number of keychain levels for sdebug (2 or 3, skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'sdebug', str(value) )
            print( 'sdebug = %s' % str(value) )
            config_update = 1

        print( 'Current sboot = %s' % str(config.get(section_name, 'sboot')) )
        value = input("Select the number of keychain levels for sboot (2 or 3, 0 unsecure, skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'sboot', str(value) )
            print( 'sboot = %s' % str(value) )
            config_update = 1

        print( 'Current combined = %s' % str(config.get(section_name, 'combined')) )
        value = input("Select the mode of Image for RamLIB & PTIM (0 = split, 1 = combined, skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'combined', str(value) )
            print( 'combined = %s' % str(value) )
            config_update = 1

        print( 'Current bootmodel = %s' % str(config.get(section_name, 'bootmodel')) )
        value = input("Select the Bot Model (0 = Cache Boot, 1 = SRAM Boot, skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'bootmodel', str(value) )
            print( 'bootmodel = %s' % str(value) )
            config_update = 1

        print( ' >>> ' )

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
    
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    target_config_init( PROJ_CONFIG )

    
    
