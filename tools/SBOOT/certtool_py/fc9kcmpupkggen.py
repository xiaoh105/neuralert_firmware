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

# CCCmpuUniqueDataType_t
CMPU_UNIQUE_IS_HBK0 = 1
CMPU_UNIQUE_IS_USER_DATA = 2

# CCCmpuUniqueBuff_t
PROD_UNIQUE_BUFF_SIZE = 16

# CCAssetType_t
ASSET_NO_KEY = 0
ASSET_PLAIN_KEY = 1
ASSET_PKG_KEY = 2

# CCAssetBuff_t
PROD_ASSET_SIZE = 16

# DCU lock bits
PROD_DCU_LOCK_BYTE_SIZE = 16

class CCCmpuData:

    uniqueDataType = struct.pack('<I', CMPU_UNIQUE_IS_HBK0)
    uniqueBuff = list(range(PROD_UNIQUE_BUFF_SIZE)) #[PROD_UNIQUE_BUFF_SIZE]

    kpicvDataType  = struct.pack('<I', ASSET_NO_KEY)
    kpicv  = list(range(PROD_ASSET_SIZE)) #[PROD_ASSET_SIZE]

    kceicvDataType  = struct.pack('<I', ASSET_NO_KEY)
    kceicv  = list(range(PROD_ASSET_SIZE)) #[PROD_ASSET_SIZE]

    icvMinVersion = struct.pack('<I', 0)
    icvConfigWord = struct.pack('<I', 0)
    icvDcuDefaultLock  = list(range(PROD_DCU_LOCK_BYTE_SIZE)) #[PROD_DCU_LOCK_BYTE_SIZE]

    def __init__(self, uniquetype, kpicvtype, kceicvtype):
        self.uniqueDataType = struct.pack('<I', uniquetype)
        self.kpicvDataType = struct.pack('<I', kpicvtype)
        self.kceicvDataType = struct.pack('<I', kceicvtype)

    def load_hbk(self, filename):
        fname, ext = os.path.splitext(filename)
        if ext == '.bin':
            self.uniqueBuff = GetDataFromBinFile(filename)
        else:
            self.uniqueBuff = GetDataFromTxtFile(filename)

    def load_kpicv(self, filename):
        self.kpicv = GetDataFromBinFile(filename)
        

    def load_kceicv(self, filename):
        self.kceicv = GetDataFromBinFile(filename)

    def set_icvvalue(self, swver, cfgword, dculock):
        self.icvMinVersion = struct.pack('<I', swver)
        self.icvConfigWord = struct.pack('<I', cfgword)
        self.icvDcuDefaultLock = dculock

    def write_binfile(self, filename):
        try:
            FileObj = open(filename, "wb")
            FileObj.write(bytes(self.uniqueDataType))
            FileObj.write(bytes(self.uniqueBuff))
            FileObj.write(bytes(self.kpicvDataType))
            FileObj.write(bytes(self.kpicv))
            FileObj.write(bytes(self.kceicvDataType))
            FileObj.write(bytes(self.kceicv))

            FileObj.write(bytes(self.icvMinVersion))
            FileObj.write(bytes(self.icvConfigWord))
            FileObj.write(bytes(self.icvDcuDefaultLock))
            FileObj.close()
        except IOError as Error7:
            sys.exit(1)
        return   

    def write_hexfile(self, hexfilename):
        hexfile = open(hexfilename, 'w')

        hexstr = str()

        hexstr = "unsigned char cmpu_hex_list[] = {\r\n"           

        i = 0
        for s in bytes(self.uniqueDataType):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//uniqueDataType\r\n'           
        i = 0
        for s in bytes(self.uniqueBuff):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//uniqueBuff\r\n'           
        i = 0
        for s in bytes(self.kpicvDataType):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kpicvDataType\r\n'           
        i = 0
        for s in bytes(self.kpicv):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kpicv\r\n'           
        i = 0
        for s in bytes(self.kceicvDataType):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kceicvDataType\r\n'           
        i = 0
        for s in bytes(self.kceicv):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kceicv\r\n'           

        i = 0
        for s in bytes(self.icvMinVersion):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//icvMinVersion\r\n'           
        i = 0
        for s in bytes(self.icvConfigWord):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//icvConfigWord\r\n'           
        i = 0
        for s in bytes(self.icvDcuDefaultLock):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//icvDcuDefaultLock\r\n'           

        hexstr = hexstr + "};\r\n"
            
        hexfile.write(hexstr)
            
        # Close files        
        hexfile.close()

        return

##########################################################

def parse_shell_arguments ():
    len_arg =  len(sys.argv)
    if len_arg < 2:
        print_sync("len " + str(len_arg) + " invalid. Usage:" + sys.argv[0] + "<test configuration file>\n")
        for i in range(1,len_arg):
            print_sync("i " + str(i) + " arg " + sys.argv[i] + "\n")
        sys.exit(1)
    config_fname = sys.argv[1]
    return config_fname

##########################################################

def cmpupkg_config_file_parser (config_fname):

    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.readfp(config_file)
    config_file.close()    
    
    local_dict = dict()
    
    section_name = "CMPU-PKG-CFG"
    if not config.has_section(section_name):
        return None, None

    is_debug = 0
    if config.has_option(section_name, 'debug-mask[0-31]'): 
        debug_mask0 = int(config.get(section_name, 'debug-mask[0-31]'), 16)
        debug_mask1 = int(config.get(section_name, 'debug-mask[32-63]'), 16)
        debug_mask2 = int(config.get(section_name, 'debug-mask[64-95]'), 16)
        debug_mask3 = int(config.get(section_name, 'debug-mask[96-127]'), 16)
        is_debug = 1

    is_hbk = 0
    if config.has_option(section_name, 'hbk-data'): 
        local_dict['hbk_data'] = config.get(section_name, 'hbk-data')
        is_hbk = 1

    is_kpicv_pkg = 0
    if config.has_option(section_name, 'kpicv-pkg'): 
        local_dict['kpicv_pkg'] = config.get(section_name, 'kpicv-pkg')
        is_kpicv_pkg = 1

    is_kceicv_pkg = 0
    if config.has_option(section_name, 'kceicv-pkg'): 
        local_dict['kceicv_pkg'] = config.get(section_name, 'kceicv-pkg')
        is_kceicv_pkg = 1

    is_cmpu_pkg = 0
    if config.has_option(section_name, 'cmpu-pkg'): 
        local_dict['cmpu_pkg'] = config.get(section_name, 'cmpu-pkg')
        is_cmpu_pkg = 1

    icvMinVersion = 0
    if config.has_option(section_name, 'MinVersion'): 
        icvMinVersion = int(config.get(section_name, 'MinVersion'), 16)

    icvConfigWord = 0
    if config.has_option(section_name, 'ConfigWord'): 
        icvConfigWord = int(config.get(section_name, 'ConfigWord'), 16)

    local_dict['debug_mask0'] = int(0)                                         
    local_dict['debug_mask1'] = int(0)                                         
    local_dict['debug_mask2'] = int(0)                                         
    local_dict['debug_mask3'] = int(0)                                         

    local_dict['MinVersion'] = icvMinVersion                                         
    local_dict['ConfigWord'] = icvConfigWord                                         

    if is_debug == 1: 
        local_dict['debug_mask0'] = debug_mask0
        local_dict['debug_mask1'] = debug_mask1
        local_dict['debug_mask2'] = debug_mask2
        local_dict['debug_mask3'] = debug_mask3
   
    return local_dict

##########################################################

def CreateCMPUPkgUtility(projcfg):
    data_dict = cmpupkg_config_file_parser(projcfg)
    if data_dict == None:
        exit(1)

    cmpu = CCCmpuData(CMPU_UNIQUE_IS_HBK0, ASSET_PKG_KEY, ASSET_PKG_KEY)

    cmpu.load_hbk(data_dict['hbk_data'])
    cmpu.load_kpicv(data_dict['kpicv_pkg'])
    cmpu.load_kceicv(data_dict['kceicv_pkg'])

    dataToSign = (struct.pack('<I', data_dict['debug_mask0']))
    dataToSign = dataToSign + (struct.pack('<I', data_dict['debug_mask1']))
    dataToSign = dataToSign + (struct.pack('<I', data_dict['debug_mask2']))
    dataToSign = dataToSign + (struct.pack('<I', data_dict['debug_mask3']))

    cmpu.set_icvvalue( data_dict['MinVersion'], data_dict['ConfigWord'], dataToSign)

    cmpu.write_binfile(data_dict['cmpu_pkg'])
    cmpu.write_hexfile((data_dict['cmpu_pkg']+'.txt'))
    
##################################
#       Main function
##################################
        
if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    PROJ_CONFIG = "./cmpu.cfg"
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]
    print("Config File  - %s\n" %PROJ_CONFIG)

    CreateCMPUPkgUtility(PROJ_CONFIG)
