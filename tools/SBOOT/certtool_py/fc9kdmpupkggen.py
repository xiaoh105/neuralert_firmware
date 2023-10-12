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

# CCDmpuHBKType_t
DMPU_HBK_TYPE_HBK1 = 1
DMPU_HBK_TYPE_HBK = 2

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

class CCDmpuData:

    uniqueDataType = struct.pack('<I', DMPU_HBK_TYPE_HBK1)
    uniqueBuff = list(range(PROD_UNIQUE_BUFF_SIZE*2)) #[PROD_UNIQUE_BUFF_SIZE*2]

    kcpDataType  = struct.pack('<I', ASSET_NO_KEY)
    kcp  = list(range(PROD_ASSET_SIZE)) #[PROD_ASSET_SIZE]

    kceDataType  = struct.pack('<I', ASSET_NO_KEY)
    kce  = list(range(PROD_ASSET_SIZE)) #[PROD_ASSET_SIZE]

    oemMinVersion = struct.pack('<I', 0)
    oemDcuDefaultLock  = list(range(PROD_DCU_LOCK_BYTE_SIZE)) #[PROD_DCU_LOCK_BYTE_SIZE]

    def __init__(self, uniquetype, kcptype, kcetype):
        self.uniqueDataType = struct.pack('<I', uniquetype)
        self.kcpDataType = struct.pack('<I', kcptype)
        self.kceDataType = struct.pack('<I', kcetype)

    def load_hbk(self, filename):
        fname, ext = os.path.splitext(filename)
        if ext == '.bin':
            tmpuniqueBuff = GetDataFromBinFile(filename)
        else:
            tmpuniqueBuff = GetDataFromTxtFile(filename)
        i = 0
        for s in bytes(self.uniqueBuff):
            self.uniqueBuff[i] = 0
            i = i + 1
        i = 0
        for s in bytes(tmpuniqueBuff):
            self.uniqueBuff[i] = tmpuniqueBuff[i]
            i = i + 1

    def load_kcp(self, filename):
        self.kcp = GetDataFromBinFile(filename)
        

    def load_kce(self, filename):
        self.kce = GetDataFromBinFile(filename)

    def set_oemvalue(self, swver, dculock):
        self.oemMinVersion = struct.pack('<I', swver)
        self.oemDcuDefaultLock = dculock

    def write_binfile(self, filename):
        try:
            FileObj = open(filename, "wb")
            FileObj.write(bytes(self.uniqueDataType))
            FileObj.write(bytes(self.uniqueBuff))
            FileObj.write(bytes(self.kcpDataType))
            FileObj.write(bytes(self.kcp))
            FileObj.write(bytes(self.kceDataType))
            FileObj.write(bytes(self.kce))

            FileObj.write(bytes(self.oemMinVersion))
            FileObj.write(bytes(self.oemDcuDefaultLock))
            FileObj.close()
        except IOError as Error7:
            sys.exit(1)
        return   

    def write_hexfile(self, hexfilename):
        hexfile = open(hexfilename, 'w')

        hexstr = str()

        hexstr = "unsigned char dmpu_hex_list[] = {\r\n"           

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
        for s in bytes(self.kcpDataType):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kcpDataType\r\n'           
        i = 0
        for s in bytes(self.kcp):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kcp\r\n'           
        i = 0
        for s in bytes(self.kceDataType):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kceDataType\r\n'           
        i = 0
        for s in bytes(self.kce):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//kce\r\n'           

        i = 0
        for s in bytes(self.oemMinVersion):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//oemMinVersion\r\n'           
        i = 0
        for s in bytes(self.oemDcuDefaultLock):
            hexstr = hexstr + '0x%02x, '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        hexstr = hexstr + '\t\t//oemDcuDefaultLock\r\n'           

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

def dmpupkg_config_file_parser (config_fname):

    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.readfp(config_file)
    config_file.close()    
    
    local_dict = dict()
    
    section_name = "DMPU-PKG-CFG"
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
    if config.has_option(section_name, 'kcp-pkg'): 
        local_dict['kcp_pkg'] = config.get(section_name, 'kcp-pkg')
        is_kpicv_pkg = 1

    is_kceicv_pkg = 0
    if config.has_option(section_name, 'kce-pkg'): 
        local_dict['kce_pkg'] = config.get(section_name, 'kce-pkg')
        is_kceicv_pkg = 1

    is_cmpu_pkg = 0
    if config.has_option(section_name, 'dmpu-pkg'): 
        local_dict['dmpu_pkg'] = config.get(section_name, 'dmpu-pkg')
        is_cmpu_pkg = 1

    oemMinVersion = 0
    if config.has_option(section_name, 'MinVersion'): 
        oemMinVersion = int(config.get(section_name, 'MinVersion'), 16)

    local_dict['debug_mask0'] = int(0)                                         
    local_dict['debug_mask1'] = int(0)                                         
    local_dict['debug_mask2'] = int(0)                                         
    local_dict['debug_mask3'] = int(0)                                         

    local_dict['MinVersion'] = oemMinVersion                                         

    if is_debug == 1: 
        local_dict['debug_mask0'] = debug_mask0
        local_dict['debug_mask1'] = debug_mask1
        local_dict['debug_mask2'] = debug_mask2
        local_dict['debug_mask3'] = debug_mask3
   
    return local_dict

##########################################################

def CreateDMPUPkgUtility(projcfg):
    data_dict = dmpupkg_config_file_parser(projcfg)
    if data_dict == None:
        exit(1)

    dmpu = CCDmpuData(DMPU_HBK_TYPE_HBK1, ASSET_PKG_KEY, ASSET_PKG_KEY)

    dmpu.load_hbk(data_dict['hbk_data'])
    dmpu.load_kcp(data_dict['kcp_pkg'])
    dmpu.load_kce(data_dict['kce_pkg'])

    dataToSign = (struct.pack('<I', data_dict['debug_mask0']))
    dataToSign = dataToSign + (struct.pack('<I', data_dict['debug_mask1']))
    dataToSign = dataToSign + (struct.pack('<I', data_dict['debug_mask2']))
    dataToSign = dataToSign + (struct.pack('<I', data_dict['debug_mask3']))

    dmpu.set_oemvalue( data_dict['MinVersion'], dataToSign)

    dmpu.write_binfile(data_dict['dmpu_pkg'])
    dmpu.write_hexfile((data_dict['dmpu_pkg']+'.txt'))
    
##################################
#       Main function
##################################
        
if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    PROJ_CONFIG = "./dmpu.cfg"
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]
    print("Config File  - %s\n" %PROJ_CONFIG)

    CreateDMPUPkgUtility(PROJ_CONFIG)
