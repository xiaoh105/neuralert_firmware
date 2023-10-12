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

class SDebugCert:
    config_fname = str()
    
    config = configparser.ConfigParser()
    config.optionxform = str

    sectionname = str()
    is_enb_flag = 0
    is_dev_flag = 0
    is_rma_flag = 0
    is_cm_flag = 0
    is_update = 0

    def __init__(self, config_fname):
        self.config_fname = config_fname

    def parse(self):
        try:
            config_file = open(self.config_fname, 'r')
        except IOError as e:
            print("Error %s" % self.config_fname)
            sys.exit(e.errno)
            
        self.config.readfp(config_file)
        config_file.close()

        #print( self.config.sections() )

        if( self.config.has_section('ENABLER-DBG-CFG') ):
            self.is_enb_flag = 1
            self.sectionname = 'ENABLER-DBG-CFG'
        elif( self.config.has_section('DEVELOPER-DBG-CFG') ):
            self.is_dev_flag = 1
            self.sectionname = 'DEVELOPER-DBG-CFG'

        if( self.is_enb_flag == 1 ):
            if( self.config.has_option(self.sectionname, 'rma-mode') and (str( self.config.get(self.sectionname, 'rma-mode') ) == '1') ):
                self.is_rma_flag = 1 # RMA
            else:
                self.is_rma_flag = 0

        if( self.is_enb_flag == 1 ):
            if( self.config.has_option(self.sectionname, 'hbk-id') and (str( self.config.get(self.sectionname, 'hbk-id') ) == '1') ):
                self.is_cm_flag = 0 # DM
            else:
                self.is_cm_flag = 1 # CM

        if( self.is_dev_flag == 1 ):
            if( self.config.has_option(self.sectionname, 'cert-pkg') and ( str( self.config.get(self.sectionname, 'cert-pkg') ).find('rma') != -1 ) ):
                self.is_rma_flag = 1 # RMA
            else:
                self.is_rma_flag = 0
            
        if( self.is_dev_flag == 1 ):
            if( self.config.has_option(self.sectionname, 'cert-pkg') and ( str( self.config.get(self.sectionname, 'cert-pkg') ).find('hbk1') != -1 ) ):
                self.is_cm_flag  = 0 # DM
            else:
                self.is_cm_flag = 1 # CM

            return

    def info(self):
        print( '----------------------------------------')
        print( 'Section = %s' % self.sectionname )

        if( self.is_cm_flag == 1 ):
            print( 'CM Part')
        else:
            print( 'DM Part')
            
        if( self.is_enb_flag == 1 ):
            print( 'Enabler Certificate')
        if( self.is_dev_flag == 1 ):
            print( 'Developer Certificate')
        if( self.is_rma_flag == 1 ):
            print( 'RMA Certificate')
        print( '----------------------------------------')
        return

    def update(self):
        if( self.is_rma_flag == 1 and self.is_enb_flag == 1 ):
            self.update_lcs()
            self.update_debug_lock()
        elif( self.is_rma_flag == 0 and self.is_enb_flag == 1 ):
            self.update_lcs()
            self.update_debug_mask()
            self.update_debug_lock()
        elif( self.is_rma_flag == 1 and self.is_enb_flag == 0 ):
            self.update_socid()
            self.update_debug_mask()
        elif( self.is_rma_flag == 0 and self.is_enb_flag == 0 ):
            self.update_socid()
            self.update_debug_mask()

        return

    def update_lcs(self):
        print('Current LCS = %s' % str( self.config.get(self.sectionname, 'lcs') ))
        value = input( 'Enter the lcs\n(0 = CM; 1 = DM LCS; 5 = Secure; 7 = RMA, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'lcs', str(value) )
            self.is_update = 1
        return

    def update_debug_mask(self):
        print('Current debug-mask[0-31] = %s' % str( self.config.get(self.sectionname, 'debug-mask[0-31]') ))
        print('Current debug-mask[32-63] = %s' % str( self.config.get(self.sectionname, 'debug-mask[32-63]') ))
        print('Current debug-mask[64-95] = %s' % str( self.config.get(self.sectionname, 'debug-mask[64-95]') ))
        print('Current debug-mask[96-127] = %s' % str( self.config.get(self.sectionname, 'debug-mask[96-127]') ))
        if( self.is_cm_flag == 0 ):
            #DM
            print('CON-Disable: [0-31]=0x7fffffff; CON&SWD-Disable: [0-31]=0x7ffff7ff') # FC9050SLR

        value = input( 'Enter debug-mask[0-31](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-mask[0-31]', str(value) )
            self.is_update = 1

        value = input( 'Enter debug-mask[32-63](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-mask[32-63]', str(value) )
            self.is_update = 1

        value = input( 'Enter debug-mask[64-95](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-mask[64-95]', str(value) )
            self.is_update = 1

        value = input( 'Enter debug-mask[96-127](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-mask[96-127]', str(value) )
            self.is_update = 1
            
        return
        
        
    def update_debug_lock(self):
        print('Current debug-lock[0-31] = %s' % str( self.config.get(self.sectionname, 'debug-lock[0-31]') ))
        print('Current debug-lock[32-63] = %s' % str( self.config.get(self.sectionname, 'debug-lock[32-63]') ))
        print('Current debug-lock[64-95] = %s' % str( self.config.get(self.sectionname, 'debug-lock[64-95]') ))
        print('Current debug-lock[96-127] = %s' % str( self.config.get(self.sectionname, 'debug-lock[96-127]') ))

        value = input( 'Enter debug-lock[0-31](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-lock[0-31]', str(value) )
            self.is_update = 1

        value = input( 'Enter debug-lock[32-63](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-lock[32-63]', str(value) )
            self.is_update = 1

        value = input( 'Enter debug-lock[64-95](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-lock[64-95]', str(value) )
            self.is_update = 1

        value = input( 'Enter debug-lock[96-127](HEX-string, skip = Enter):' )
        if str(value) != '':
            self.config.set(self.sectionname, 'debug-lock[96-127]', str(value) )
            self.is_update = 1

        return

    def update_socid(self):
        socidfile = str( self.config.get(self.sectionname, 'soc-id') )
        print('Current Soc-ID = %s' % socidfile )

        binstr = GetDataFromBinFile(socidfile)
        hexstr = str()
        i = 0
        for s in bytes(binstr):
            hexstr = hexstr + '%02X '% s
            i = i + 1
            if (i%16) == 0:
                hexstr = hexstr + '\r\n'
        print(hexstr)

        value1 = input( 'Enter SOC-ID[0-15] (skip = Enter):' )
        if str(value1) != '':
            print( 'SocID[0-15] = %s' % str(value1) )
        value2 = input( 'Enter SOC-ID[16-31] (skip = Enter):' )
        if str(value2) != '':
            print( 'SocID[16-31] = %s' % str(value2) )

        if( str(value1) != '' and str(value2) != '' ):
            outStr = bytes()

            value1list = value1.split()
            for binitem in value1list:
                intvalue = int(binitem, 16)
                outStr += intvalue.to_bytes(1, byteorder='little')
                
            value2list = value2.split()
            for binitem in value2list:
                intvalue = int(binitem, 16)
                outStr += intvalue.to_bytes(1, byteorder='little')

            wrfile = open(socidfile, "wb")
            wrfile.write(outStr)
            wrfile.close()
            self.is_update = 1
        
        return

    def save(self):
        if( self.is_update == 1 ):
            config_file = open(self.config_fname, 'w')
            self.config.write(config_file)
            config_file.close()
            print(" %s Updated ..." % self.config_fname )
        return

##################################
#       Main function
##################################
        
if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    PROJ_CONFIG = str()
    
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]

    sdebug = SDebugCert(PROJ_CONFIG)

    sdebug.parse()
    sdebug.info()

    sdebug.update()
    sdebug.save()
    
    
