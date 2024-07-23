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


def target_asset_update(config_fname):

    config_new = 1
    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        config_new = 0
    
    section_name =  "ASSET-PROV-CFG"
    
    config = configparser.ConfigParser()
    config.optionxform = str

    if( config_new == 1 ):
        config.readfp(config_file)
        config_file.close()
    else:
        config.add_section(section_name)

    config_update = 0

    if( str(config_fname).find('dmtpmcfg') > 0 ):
        dmpart = 1
    else:
        dmpart = 0
    
    if config.has_section(section_name):

        if not config.has_option(section_name, 'key-filename') :
            if( dmpart == 1 ):
                config.set(section_name, 'key-filename', '../dmpublic/enc.kcp.bin')
            else:
                config.set(section_name, 'key-filename', '../cmpublic/enc.kpicv.bin')

        print( 'Current key-filename = %s' % str(config.get(section_name, 'key-filename')) )
            
        
        value = input("Enter key-filename (skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'key-filename', str(value) )
            print( 'key-filename = %s' % str(value) )
            config_update = 1

        if not config.has_option(section_name, 'keypwd-filename') :
            if( dmpart == 1 ):
                config.set(section_name, 'keypwd-filename', '../dmsecret/pwd.kcp.txt')
            else:
                config.set(section_name, 'keypwd-filename', '../cmsecret/pwd.kpicv.txt')

        print( 'Current keypwd-filename = %s' % str(config.get(section_name, 'keypwd-filename')) )
            

        value = input("Enter keypwd-filename (skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'keypwd-filename', str(value) )
            print( 'keypwd-filename = %s' % str(value) )		
            config_update = 1

        if not config.has_option(section_name, 'asset-id') :
            config.set(section_name, 'asset-id', '0x01234567')
            
        print( 'Current asset-id = %s' % (config.get(section_name, 'asset-id')) )

        value = input("Enter asset-id (skip = Enter): ")

        if str(value) != '':
            config.set(section_name, 'asset-id', str(value) )
            print( 'asset-id = %s' % str(value) )
            config_update = 1

        if not config.has_option(section_name, 'asset-filename') :
            if( dmpart == 1 ):
                config.set(section_name, 'asset-filename', '../image/asset512.bin')
            else:
                config.set(section_name, 'asset-filename', '../image/asset512.bin')

        print( 'Current asset-filename = %s' % str(config.get(section_name, 'asset-filename')) )

        value = input("Enter asset-filename (skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'asset-filename', str(value) )
            print( 'asset-filename = %s' % str(value) )
            config_update = 1

        if not config.has_option(section_name, 'asset-pkg') :
            if( dmpart == 1 ):
                config.set(section_name, 'asset-pkg', '../public/asset_pkg.bin')
            else:
                config.set(section_name, 'asset-pkg', '../public/asset_pkg_icv.bin')

        print( 'Current asset-pkg = %s' % str(config.get(section_name, 'asset-pkg')) )

        value = input("Enter asset-pkg (skip = Enter): ")
        
        if str(value) != '':
            config.set(section_name, 'asset-pkg', str(value) )
            print( 'asset-pkg = %s' % str(value) )
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

    PROJ_CONFIG = "../cmtpmcfg/asset.cfg"
    
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    target_asset_update( PROJ_CONFIG )

    
    
