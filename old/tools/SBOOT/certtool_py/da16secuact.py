#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct
import platform

# Definitions for paths
#######################
if sys.platform != "win32" :
    path_div = "//"    
else : #platform = win32
    path_div = "\\"

# In case the scripts were run from current directory
CURRENT_PATH_SCRIPTS = os.path.dirname(os.path.abspath(sys.argv[0])) + path_div + 'common_utils'

# this is the scripts local path, from where the program was called
sys.path.append(CURRENT_PATH_SCRIPTS)

# this is the path of the proj config file
CURRENT_PROJ_CONFIG = os.path.dirname(os.path.abspath(sys.argv[0])) + path_div + 'proj.cfg'

##########################################################

from fc9kcmpupkggen         import CreateCMPUPkgUtility
from fc9kdmpupkggen         import CreateDMPUPkgUtility
from hbk_gen_util           import HBKGenUtil
from cert_key_util          import CreateCertUtility
from cert_sb_content_util   import CreateContentCertUtility
from cert_dbg_enabler_util  import CreateEnbCertUtility
from cert_dbg_developer_util  import CreateDevCertUtility

from cmpu_asset_pkg_util        import CMPUAssetPkgUtility
from dmpu_oem_key_request_util  import OEMKeyReqUtility
from dmpu_icv_key_response_util import ICVKeyResUtility
from dmpu_oem_asset_pkg_util    import DMPUAssetPkgUtility

from asset_provisioning_util    import AssetProvisioningUtility

##########################################################

def DisableWinQuickEditMode():
    '''    Disable quickedit mode on Windows terminal  '''

    if (not os.name == 'posix') and (platform.win32_ver()[0] == '8' or platform.win32_ver()[0] == '10') :

        try:
            import msvcrt
            import ctypes
            kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
            device = r'\\.\CONIN$'
            with open(device, 'r') as con:
                hCon = msvcrt.get_osfhandle(con.fileno())
                kernel32.SetConsoleMode(hCon, 0x0080)
        except Exception as e:
            print('Cannot disable quickedit mode! ' + str(e))

##########################################################

def module_fc9kcmpupkggen(sys_argv):
    
    PROJ_CONFIG = "./cmpu.cfg"
    if "-cfg_file" in sys_argv:
        PROJ_CONFIG = sys_argv[sys_argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    CreateCMPUPkgUtility(PROJ_CONFIG)    

##########################################################

def module_fc9kdmpupkggen(sys_argv):
    
    PROJ_CONFIG = "./dmpu.cfg"
    if "-cfg_file" in sys_argv:
        PROJ_CONFIG = sys_argv[sys_argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    CreateDMPUPkgUtility(PROJ_CONFIG)

##########################################################

def module_hbk_gen_util(sys_argv):
    HBKGenUtil(sys_argv)

##########################################################

def module_cert_key_util(sys_argv):
    '''
    PROJ_CONFIG = CURRENT_PROJ_CONFIG

    if "-cfg_file" in sys_argv:
        PROJ_CONFIG = sys_argv[sys_argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    PrjDefines = [1, 0] parseConfFile(PROJ_CONFIG,LIST_OF_CONF_PARAMS)
    '''
    PrjDefines = [1, 0]
    CreateCertUtility(sys_argv, PrjDefines)

##########################################################

def module_cert_sb_content_util(sys_argv):
    '''
    PROJ_CONFIG = CURRENT_PROJ_CONFIG

    if "-cfg_file" in sys_argv:
        PROJ_CONFIG = sys_argv[sys_argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    PrjDefines = parseConfFile(PROJ_CONFIG,LIST_OF_CONF_PARAMS)
    '''
    PrjDefines = [1, 0]
    CreateContentCertUtility(sys_argv, PrjDefines)
    
##########################################################

def module_cert_dbg_enabler_util(sys_argv):
    '''
    PROJ_CONFIG = CURRENT_PROJ_CONFIG

    if "-cfg_file" in sys_argv:
        PROJ_CONFIG = sys_argv[sys_argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    PrjDefines = parseConfFile(PROJ_CONFIG,LIST_OF_CONF_PARAMS)
    '''
    PrjDefines = [1, 0]
    CreateEnbCertUtility(sys_argv, PrjDefines)
    
##########################################################

def module_cert_dbg_developer_util(sys_argv):
    '''
    PROJ_CONFIG = CURRENT_PROJ_CONFIG

    if "-cfg_file" in sys_argv:
        PROJ_CONFIG = sys_argv[sys_argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    PrjDefines = parseConfFile(PROJ_CONFIG,LIST_OF_CONF_PARAMS)
    '''
    PrjDefines = [1, 0]
    CreateDevCertUtility(sys_argv, PrjDefines)
    
##########################################################

def module_cmpu_asset_pkg_util(sys_argv):
    CMPUAssetPkgUtility(sys_argv)

##########################################################

def module_oem_key_request_util(sys_argv):
    OEMKeyReqUtility(sys_argv)

##########################################################

def module_icv_key_response_util(sys_argv):
    ICVKeyResUtility(sys_argv)

##########################################################

def module_dmpu_asset_pkg_util(sys_argv):
    DMPUAssetPkgUtility(sys_argv)

##########################################################

def module_asset_provisioning_util(sys_argv):
    AssetProvisioningUtility(sys_argv)
    
##########################################################
# main
##########################################################

if __name__ == "__main__":

    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    if "-mod" in sys.argv:
        module_name = sys.argv[sys.argv.index("-mod") + 1]
    else:
        print("USAGE: secutool -mod [secuasset|secudebug|secuboot] ...")
        exit(1)

    DisableWinQuickEditMode()
	
    if(	module_name == 'fc9kcmpupkggen'):
        module_fc9kcmpupkggen(sys.argv[2:])
    elif( module_name == 'fc9kdmpupkggen'):
        module_fc9kdmpupkggen(sys.argv[2:])
    elif( module_name == 'hbk_gen_util'):
        module_hbk_gen_util(sys.argv[2:])
    elif( module_name == 'cert_key_util'):
        module_cert_key_util(sys.argv[2:])
    elif( module_name == 'cert_sb_content_util'):
        module_cert_sb_content_util(sys.argv[2:])
    elif( module_name == 'cert_dbg_enabler_util'):
        module_cert_dbg_enabler_util(sys.argv[2:])
    elif( module_name == 'cert_dbg_developer_util'):
        module_cert_dbg_developer_util(sys.argv[2:])
    elif( module_name == 'cmpu_asset_pkg_util'):
        module_cmpu_asset_pkg_util(sys.argv[2:])
    elif( module_name == 'dmpu_oem_key_request_util'):
        module_oem_key_request_util(sys.argv[2:])
    elif( module_name == 'dmpu_icv_key_response_util'):
        module_icv_key_response_util(sys.argv[2:])
    elif( module_name == 'dmpu_oem_asset_pkg_util'):
        module_dmpu_asset_pkg_util(sys.argv[2:])
    elif( module_name == 'asset_provisioning_util'):
        module_asset_provisioning_util(sys.argv[2:])
    else:
        print("USAGE: secuact -mod [fc9kcmpupkggen|...] ...")
