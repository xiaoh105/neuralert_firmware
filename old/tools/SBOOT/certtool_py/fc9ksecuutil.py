#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct
import subprocess
import threading
import time

import random
import base64
import binascii
from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives import asymmetric
from cryptography.hazmat.primitives import ciphers
from cryptography.hazmat            import backends
from cryptography.hazmat.primitives import kdf
from cryptography.hazmat.primitives import cmac
from cryptography.hazmat.primitives import hmac
from cryptography import exceptions

from retmimage import *
from secureimage import *
from da16secuact import *

PROD_UNIQUE_BUFF_SIZE = 16

fc9ksecuactlist = ['fc9kcmpupkggen', 'fc9kdmpupkggen', 'hbk_gen_util', 'cert_key_util', 'cert_sb_content_util'  
                   , 'cert_dbg_enabler_util', 'cert_dbg_developer_util', 'cmpu_asset_pkg_util'
                   , 'dmpu_oem_key_request_util', 'dmpu_icv_key_response_util', 'dmpu_oem_asset_pkg_util'
                   , 'asset_provisioning_util' ]

##########################################################
def ConProgressBar (itercnt, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '#', printEnd = "\r"):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (itercnt / float(total)))
    filledLength = int(length * itercnt // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    #print('\r%s |%s| %s %s' % (prefix, bar, percent, suffix), end = printEnd)
    print('\r%s |%s| ' % (prefix, bar), end = printEnd)
    # Print New Line on Complete
    if itercnt == total: 
        print()

def thread_function(stop):
    ConProgressValue = 0
    ConProgressBar(ConProgressValue, 60, prefix = 'Crypto:', suffix = '', length = 50)

    while True:
        ConProgressBar(ConProgressValue + 1, 60, prefix = 'Crypto:', suffix = '', length = 50)
        ConProgressValue += 1
        if stop():
            print('\r                                                                ', end = "\r")
            break

        time.sleep(.2)

def process_call(commandlist):

    out = bytes()
    err = bytes()

    fname, ext = os.path.splitext(commandlist[0])
    pythoncmd = [sys.executable]

    exefilename = '%s.exe' % fname 
    '''
    if( any(fname in s for s in fc9ksecuactlist) == True ) :
        if( (os.path.isfile('da16secuact.exe') != True)
            and (os.path.isfile('da16secuact.py') != True) ) :
            out = ('\ncommand error:: module, %s is not found\n' %  fname).encode()
            err = b''
            return out, err  
    else:
        if( (os.path.isfile(commandlist[0]) != True)
            and (os.path.isfile(exefilename) != True) ) :
            out = ('\ncommand error:: file, %s is not found\n' %  fname).encode()
            err = b''
            return out, err        

    if( (os.path.isfile(exefilename) == True) or (os.path.isfile('da16secuact.exe') == True) ):
        fname, ext = os.path.splitext(exefilename)
        if( any(fname in s for s in fc9ksecuactlist) ):
            newcommandlist = [ 'da16secuact.exe', '-mod', fname ]        
            newcommandlist.extend(commandlist[1:])
        else:
            newcommandlist = [ exefilename ]
            newcommandlist.extend(commandlist[1:])
    else:
        if( any(fname in s for s in fc9ksecuactlist) ):
            newcommandlist = [ 'da16secuact.py', '-mod', fname ]        
            newcommandlist.extend(commandlist[1:])
            print('CASE2:', newcommandlist)
        else:
            newcommandlist =  commandlist
            print('CASE3:', commandlist)
    '''

    if( any(fname in s for s in fc9ksecuactlist) != True ) :
        if( (os.path.isfile(commandlist[0]) != True)
            and (os.path.isfile(exefilename) != True) ) :
            out = ('\ncommand error:: file, %s is not found\n' %  fname).encode()
            err = b''
            return out, err        

        if( (os.path.isfile(exefilename) == True)  ):
            fname, ext = os.path.splitext(exefilename)
            newcommandlist = [ exefilename ]
            newcommandlist.extend(commandlist[1:])
        else:
            ext = ''
            newcommandlist =  commandlist
            print('CASE3:', commandlist)


    thread_kill_switch = False
    x = threading.Thread(target=thread_function, args =(lambda : thread_kill_switch, ))
    x.start()

    if( any(fname in s for s in fc9ksecuactlist) != True ) :
        if ext == '.py':
            pythoncmd.extend(newcommandlist)

            p = subprocess.Popen(pythoncmd
                            , stdout=subprocess.PIPE
                            , stderr=subprocess.PIPE
                            )
        else:
            p = subprocess.Popen(newcommandlist
                            , stdout=subprocess.PIPE
                            , stderr=subprocess.PIPE
                            )

        out, err = p.communicate()
    else:
        #print('process_call:', commandlist)
        module_name = fname
        
        if(	module_name == 'fc9kcmpupkggen'):
            module_fc9kcmpupkggen(commandlist)
        elif( module_name == 'fc9kdmpupkggen'):
            module_fc9kdmpupkggen(commandlist)
        elif( module_name == 'hbk_gen_util'):
            module_hbk_gen_util(commandlist)
        elif( module_name == 'cert_key_util'):
            module_cert_key_util(commandlist)
        elif( module_name == 'cert_sb_content_util'):
            module_cert_sb_content_util(commandlist)
        elif( module_name == 'cert_dbg_enabler_util'):
            module_cert_dbg_enabler_util(commandlist)
        elif( module_name == 'cert_dbg_developer_util'):
            module_cert_dbg_developer_util(commandlist)
        elif( module_name == 'cmpu_asset_pkg_util'):
            module_cmpu_asset_pkg_util(commandlist)
        elif( module_name == 'dmpu_oem_key_request_util'):
            module_oem_key_request_util(commandlist)
        elif( module_name == 'dmpu_icv_key_response_util'):
            module_icv_key_response_util(commandlist)
        elif( module_name == 'dmpu_oem_asset_pkg_util'):
            module_dmpu_asset_pkg_util(commandlist)
        elif( module_name == 'asset_provisioning_util'):
            module_asset_provisioning_util(commandlist)

    thread_kill_switch = True
    x.join()
    del x

    errcode = -1
    if err != b'' :
        errascii = err.decode('ASCII')
        errcode = errascii.find('End rc 0')
    else:
        errcode = 1

    if( errcode < 0 ):
        if ext == '.py':
            print('CMD.py:')
            print(pythoncmd)
        else:
            print('CMD.exe:')
            print(newcommandlist)
            
        print('OUT:')
        print(out)
        print('ERR:')
        print(err)

def process_call_return(commandlist):

    out = bytes()
    err = bytes()

    fname, ext = os.path.splitext(commandlist[0])
    pythoncmd = [sys.executable]

    exefilename = '%s.exe' % fname 
    '''
    if( any(fname in s for s in fc9ksecuactlist) == True ) :
        if( (os.path.isfile('da16secuact.exe') != True)
            and (os.path.isfile('da16secuact.py') != True) ) :
            out = ('\ncommand error:: module, %s is not found\n' %  fname).encode()
            err = b''
            return out, err  
    else:
        if( (os.path.isfile(commandlist[0]) != True)
            and (os.path.isfile(exefilename) != True) ) :
            out = ('\ncommand error:: file, %s is not found\n' %  fname).encode()
            err = b''
            return out, err  
            
    if( (os.path.isfile(exefilename) == True) or (os.path.isfile('da16secuact.exe') == True) ):
        fname, ext = os.path.splitext(exefilename)
        if( any(fname in s for s in fc9ksecuactlist) ):
            newcommandlist = [ 'da16secuact.exe', '-mod', fname ]        
            newcommandlist.extend(commandlist[1:])
        else:
            newcommandlist = [ exefilename ]
            newcommandlist.extend(commandlist[1:])
    else:
        if( any(fname in s for s in fc9ksecuactlist) ):
            newcommandlist = [ 'da16secuact.py', '-mod', fname ]        
            newcommandlist.extend(commandlist[1:])
            print('CASE2:', newcommandlist)
        else:
            newcommandlist =  commandlist
            print('CASE3:', commandlist)
    '''

    if( any(fname in s for s in fc9ksecuactlist) != True ) :
        if( (os.path.isfile(commandlist[0]) != True)
            and (os.path.isfile(exefilename) != True) ) :
            out = ('\ncommand error:: file, %s is not found\n' %  fname).encode()
            err = b''
            return out, err        

        if( (os.path.isfile(exefilename) == True)  ):
            fname, ext = os.path.splitext(exefilename)
            newcommandlist = [ exefilename ]
            newcommandlist.extend(commandlist[1:])
        else:
            ext = ''
            newcommandlist =  commandlist
            print('CASE3:', commandlist)

    thread_kill_switch = False
    x = threading.Thread(target=thread_function, args =(lambda : thread_kill_switch, ))
    x.start()

    if( any(fname in s for s in fc9ksecuactlist) != True ) :
        if ext == '.py':
            pythoncmd.extend(newcommandlist)

            p = subprocess.Popen(pythoncmd
                            , stdout=subprocess.PIPE
                            , stderr=subprocess.PIPE
                            )
        else:
            p = subprocess.Popen(newcommandlist
                            , stdout=subprocess.PIPE
                            , stderr=subprocess.PIPE
                            )

        out, err = p.communicate()
    else:
        #print('process_call_return:', commandlist)
        module_name = fname

        if(	module_name == 'fc9kcmpupkggen'):
            module_fc9kcmpupkggen(commandlist)
        elif( module_name == 'fc9kdmpupkggen'):
            module_fc9kdmpupkggen(commandlist)
        elif( module_name == 'hbk_gen_util'):
            module_hbk_gen_util(commandlist)
        elif( module_name == 'cert_key_util'):
            module_cert_key_util(commandlist)
        elif( module_name == 'cert_sb_content_util'):
            module_cert_sb_content_util(commandlist)
        elif( module_name == 'cert_dbg_enabler_util'):
            module_cert_dbg_enabler_util(commandlist)
        elif( module_name == 'cert_dbg_developer_util'):
            module_cert_dbg_developer_util(commandlist)
        elif( module_name == 'cmpu_asset_pkg_util'):
            module_cmpu_asset_pkg_util(commandlist)
        elif( module_name == 'dmpu_oem_key_request_util'):
            module_oem_key_request_util(commandlist)
        elif( module_name == 'dmpu_icv_key_response_util'):
            module_icv_key_response_util(commandlist)
        elif( module_name == 'dmpu_oem_asset_pkg_util'):
            module_dmpu_asset_pkg_util(commandlist)
        elif( module_name == 'asset_provisioning_util'):
            module_asset_provisioning_util(commandlist)

    thread_kill_switch = True
    x.join()
    del x

    return out, err

##########################################################

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

def WriteArmHexTxtFile(binStr, hexfilename):
	hexfile = open(hexfilename, 'w')

	hexstr = str()

	i = 0
	for s in bytes(binStr):
		if (i) != 0:
			hexstr = hexstr + ','
		hexstr = hexstr + '0x%02x'% s
		i = i + 1

	hexfile.write(hexstr)
		
	# Close files        
	hexfile.close()

	return
    
	
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


def ConvertHexa2Text(hexfile, txtfile):

    hexstr = GetDataFromBinFile(hexfile)
	
    WriteArmHexTxtFile(hexstr, txtfile)

    return True

def SaveUnixFile(strdata, txtfile):
    newstr = strdata.decode('ascii').replace('\r', '').replace('\n', '')
    with open(txtfile, 'wb') as f:
        f.write( newstr.encode() + b'\n' )
    return True

##########################################################

class configman:
    _configupdate = 0
    _config = configparser.ConfigParser()
    _config.optionxform = str
    
    def __init__(self, section, cfgname):
        if section != '':
            self._section = section
        else:
            exit(1)
        if cfgname != '':
            self._cfgname = cfgname
        else:
            exit(1)

        if( os.path.isfile(self._cfgname) == True ):
            fobj = open(self._cfgname, 'r')
            self._config.readfp(fobj)
            fobj.close()			

    def getoption(self, key):
        if( self._config.has_section(self._section) != True):
            self._config.add_section(self._section)
            return ''
        if( self._config.has_option(self._section, key) != True):
            return ''
        return self._config.get(self._section, key)

    def setoption(self, key, strvalue):
        if( self.getoption(key) != strvalue ):
            self._config.set(self._section, key, strvalue)
            self._configupdate = 1

    def loadoptions(self):
        if( self._config.has_section(self._section) != True):
            self._config.add_section(self._section)
            return ''
        tpmstr = ''
        idx = 0
        for key in self._config.options(self._section):
            if idx == 0 :
                tpmstr = key + '=' + self._config.get(self._section, key)
            else:
                tpmstr = tpmstr + '&' + key + '=' + self._config.get(self._section, key)
            idx += 1
        return tpmstr
        
    def updateoptions(self, strvalue):
        items = strvalue.split('&')
        tpmstr = ''
        for item in items :
            key, value = item.split('=')
            self.setoption(key, value)
            self._configupdate = 1

    def update(self):
        if( self._configupdate == 1 ):
            fobj = open(self._cfgname, 'w')
            self._config.write(fobj)
            fobj.close()
            self._configupdate = 0	
          
##########################################################

diasemi_lcs_list = [
    ['lcs CM', 0], ['lcs DM', 1], ['lcs SECURE', 5], ['lcs RMA', 7]
]

diasemi_hdkid_list = [
    ['hbk-id 0, CM', 0], ['hbk-id 1, DM', 1]
]

diasemi_sdebug_list = [
    ['Non SD', 0], ['2 Level SD', 2], ['3 Level SD', 3]
]

diasemi_sboot_list = [
    ['Bypass  SB', 0], ['2 Level SB', 2], ['3 Level SB', 3]
]

diasemi_imagemodel_list = [
    ['Split Image', 0], ['Combined Image', 1]
]

diasemi_bootmodel_list = [
    ['Cache Boot', 0], ['SRAM Boot', 1]
]


class  manconfig:
    _filename = str
    _configupdate = 0
    _config = configparser.ConfigParser()
    _config.optionxform = str
    _section = 'FC9K-MAN-CONFIG'
    _tpmsection = 'FC9K-TPM-CONFIG'

    def __init__(self, filename):
        if filename != '':
            self._filename = filename
        else:
            exit(1)

    def parse(self):
        try:
            fobj = open(self._filename, 'r')
            self._config.readfp(fobj)
            fobj.close()
        except IOError as e:
            self._config.add_section(self._section)
            self._config.set(self._section, 'rule', str('Yellow Boot'))
            self._config.set(self._section, 'own', str('CM'))
            self._config.set(self._section, 'role', str('ALL'))
            self._config.set(self._section, 'step', str('Y.1'))
            self._config.set(self._section, 'image', str(''))
            self._configupdate = 1

    #def getconfig(self):
    #    return self._config

    def getconfig(self, key):
        return self._config.get(self._section, key)

    def setconfig(self, key, strvalue):
        if( self._config.has_option(self._section, key) != True):
            self._config.set(self._section, key, strvalue)
            self._configupdate = 1
        elif( self.getconfig(key) != strvalue ):
            self._config.set(self._section, key, strvalue)
            self._configupdate = 1


    def getrawconfig(self, section, key):
        if( self._config.has_section(section) != True):
            self._config.add_section(section)
            return ''
        if( self._config.has_option(section, key) != True):
            return ''
        return self._config.get(section, key)

    def setrawconfig(self, section, key, strvalue):
        if( self.getrawconfig(section, key) != strvalue ):
            self._config.set(section, key, strvalue)
            self._configupdate = 1

    ##########

    def findtpmoption_str(self, listname, strvalue):
        for idx, item in enumerate(listname):
            if( item[0] == strvalue ):
                return idx
        return -1

    def findtpmoption_int(self, listname, intvalue):
        for idx, item in enumerate(listname):
            if( item[1] == intvalue ):
                return idx
        return -1

    def gettpmlcs_int(self):
        item = self.getrawconfig(self._tpmsection, 'lcs')
        idx = self.findtpmoption_str(diasemi_lcs_list, item)
        if idx == -1 :
            return -1
        return diasemi_lcs_list[idx][1]

    def gettpmlcs(self):
        item = self.getrawconfig(self._tpmsection, 'lcs')
        idx = self.findtpmoption_str(diasemi_lcs_list, item)
        if idx == -1 :
            return ''
        return item

    def gettpmhbkid_int(self):
        item = self.getrawconfig(self._tpmsection, 'hbk-id')
        idx = self.findtpmoption_str(diasemi_hdkid_list, item)
        if idx == -1 :
            return -1
        return diasemi_hdkid_list[idx][1]

    def gettpmhbkid(self):
        item = self.getrawconfig(self._tpmsection, 'hbk-id')
        idx = self.findtpmoption_str(diasemi_hdkid_list, item)
        if idx == -1 :
            return ''
        return item

    def gettpmsdebug_int(self):
        item = self.getrawconfig(self._tpmsection, 'sdebug')
        idx = self.findtpmoption_str(diasemi_sdebug_list, item)
        if idx == -1 :
            return -1
        return diasemi_sdebug_list[idx][1]

    def gettpmsdebug(self):
        item = self.getrawconfig(self._tpmsection, 'sdebug')
        idx = self.findtpmoption_str(diasemi_sdebug_list, item)
        if idx == -1 :
            return ''
        return item

    def gettpmsboot_int(self):
        item = self.getrawconfig(self._tpmsection, 'sboot')
        idx = self.findtpmoption_str(diasemi_sboot_list, item)
        if idx == -1 :
            return -1
        return diasemi_sboot_list[idx][1]

    def gettpmsboot(self):
        item = self.getrawconfig(self._tpmsection, 'sboot')
        idx = self.findtpmoption_str(diasemi_sboot_list, item)
        if idx == -1 :
            return ''
        return item

    def gettpmnvcount(self):
        return self.getrawconfig(self._tpmsection, 'nvcount')

    def gettpmnvcount_int(self):
        return int(self.getrawconfig(self._tpmsection, 'nvcount'))

    def gettpmimagemodel_int(self):
        item = self.getrawconfig(self._tpmsection, 'combined')
        idx = self.findtpmoption_str(diasemi_imagemodel_list, item)
        if idx == -1 :
            return -1
        return diasemi_imagemodel_list[idx][1]

    def gettpmimagemodel(self):
        item = self.getrawconfig(self._tpmsection, 'combined')
        idx = self.findtpmoption_str(diasemi_imagemodel_list, item)
        if idx == -1 :
            return ''
        return item

    def gettpmbootmodel_int(self):
        item = self.getrawconfig(self._tpmsection, 'bootmodel')
        idx = self.findtpmoption_str(diasemi_bootmodel_list, item)
        if idx == -1 :
            return -1
        return diasemi_bootmodel_list[idx][1]

    def gettpmbootmodel(self):
        item = self.getrawconfig(self._tpmsection, 'bootmodel')
        idx = self.findtpmoption_str(diasemi_bootmodel_list, item)
        if idx == -1 :
            return ''
        return item

    ##########

    def gettpmconfig(self):
        if( self._config.has_section(self._tpmsection) != True):
            self._config.add_section(self._tpmsection)
            return ''
        tpmstr = ''
        idx = 0
        for key in self._config.options(self._tpmsection):
            if idx == 0 :
                tpmstr += self._config.get(self._tpmsection, key)
            else:
                tpmstr = tpmstr + ':' + self._config.get(self._tpmsection, key)
            idx += 1
        return tpmstr

    def settpmconfig(self, strvalue):
        items = strvalue.split('&')
        tpmstr = ''
        for item in items :
            key, value = item.split('=')
            self.setrawconfig(self._tpmsection, key, value)
            tpmstr = tpmstr + value + ':'

    def updatesection(self, section, textstr):
        items = textstr.split('&')
        for item in items :
            key, value = item.split('=')
            if( key == 'CRCON'):
                if (value == 'true'):
                    #print( key, '=', 'ON' )
                    self.setrawconfig(section, key, 'ON')
                else:
                    #print( key, '=', 'OFF' )
                    self.setrawconfig(section, key, 'OFF')
            elif( key == 'RETMEM'):
                if (value == 'true'):
                    #print( key, '=', '1' )
                    self.setrawconfig(section, key, '1')
                else:
                    #print( key, '=', '0' )
                    self.setrawconfig(section, key, '0')
            elif( key == 'TAGDATE'):
                if (value == 'true'):
                    #print( key, '=', 'ON' )
                    self.setrawconfig(section, key, 'ON')
                else:
                    #print( key, '=', 'OFF' )
                    self.setrawconfig(section, key, 'OFF')
            elif( key == 'SFDP' ):
                if( value == 'Non-SFDP' ):
                    #print ( key, '=', '' )
                    self.setrawconfig(section, key, '')
                else:
                    #print( key, '=', value )
                    self.setrawconfig(section, key, value)
            else:
                #print( key, '=', value )
                self.setrawconfig(section, key, value)
        

    def update(self):
        if( self._configupdate == 1 ):
            fobj = open(self._filename, 'w')
            self._config.write(fobj)
            fobj.close()
            self._configupdate = 0

##########################################################

cmcfgfilelist = [
	'cmconfig/cmpu.cfg',
	'cmconfig/sboot_hbk0_2lvl_key_chain_publisher.cfg',
	'cmconfig/sboot_hbk0_3lvl_key_chain_issuer.cfg',
	'cmconfig/sboot_hbk0_3lvl_key_chain_publisher.cfg',
	'cmconfig/sdebug_hbk0_2lvl_key_chain_developer.cfg',
	'cmconfig/sdebug_hbk0_3lvl_key_chain_enabler.cfg',
	'cmconfig/sdebug_hbk0_3lvl_key_chain_developer.cfg'
    ]

dmcfgfilelist = [
	'dmconfig/dmpu.cfg',
	'dmconfig/sboot_hbk1_2lvl_key_chain_publisher.cfg',
	'dmconfig/sboot_hbk1_3lvl_key_chain_issuer.cfg',
	'dmconfig/sboot_hbk1_3lvl_key_chain_publisher.cfg',
	'dmconfig/sdebug_hbk1_2lvl_key_chain_developer.cfg',
	'dmconfig/sdebug_hbk1_3lvl_key_chain_enabler.cfg',
	'dmconfig/sdebug_hbk1_3lvl_key_chain_developer.cfg'
    ]

class  nvcountupdate:
    def __init__(self, cfgfile):
        self.cfgfile = cfgfile
        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.manobj.update()

    def updatecfgAll(self):
        nvcount = self.manobj.gettpmnvcount_int()
        hbkid   = self.manobj.gettpmhbkid_int()

        if( self.manobj.getconfig('own') == 'CM' ):
            for cfg_file in cmcfgfilelist:
                if( os.path.isfile(cfg_file) == True ):
                    self.cfg_file_update(cfg_file, nvcount, hbkid)
        else:
            for cfg_file in dmcfgfilelist:
                if( os.path.isfile(cfg_file) == True ):
                    self.cfg_file_update(cfg_file, nvcount, hbkid)

    def updatecfg(self, targetfile):
        nvcount = self.manobj.gettpmnvcount_int()
        hbkid   = self.manobj.gettpmhbkid_int()

        if( self.manobj.getconfig('own') == 'CM' ):
            for cfg_file in cmcfgfilelist:
                if( os.path.isfile(cfg_file) == True ):
                    dpath, dfile = os.path.split(cfg_file)
                    spath, sfile = os.path.split(targetfile)
                    
                    if( dfile == sfile ):
                        #print("update %s %s" % (dfile, sfile)) ####
                        self.cfg_file_update(cfg_file, nvcount, hbkid)
        else:
            for cfg_file in dmcfgfilelist:
                if( os.path.isfile(cfg_file) == True ):
                    dpath, dfile = os.path.split(cfg_file)
                    spath, sfile = os.path.split(targetfile)
                    
                    if( dfile == sfile ):
                        #print("update %s %s" % (dfile, sfile)) ####
                        self.cfg_file_update(cfg_file, nvcount, hbkid)


    def cfg_file_update(self, config_fname, nvcounter, hbkid):

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
                #print( 'minversion = %d' % nvcounter )
                config_update = 1

        section_name = "DMPU-PKG-CFG"
        if config.has_section(section_name):
            if config.has_option(section_name, 'minversion'): 
                config.set(section_name, 'minversion', str(nvcounter))
                #print( 'minversion = %d' % nvcounter )
                config_update = 1

        section_name = "KEY-CFG"
        if config.has_section(section_name):
            if config.has_option(section_name, 'nvcounter-val'): 
                config.set(section_name, 'nvcounter-val', str(nvcounter))
                #print( 'nvcounter-val = %d' % nvcounter )
                config_update = 1

            if config.has_option(section_name, 'hbk-id'): 
                config.set(section_name, 'hbk-id', str(hbkid))
                #print( 'hbk-id = %d' % hbkid )
                config_update = 1

        section_name = "CNT-CFG"
        if config.has_section(section_name):
            if config.has_option(section_name, 'nvcounter-val'): 
                config.set(section_name, 'nvcounter-val', str(nvcounter))
                #print( 'nvcounter-val = %d' % nvcounter )
                config_update = 1

        section_name = "ENABLER-DBG-CFG"
        if config.has_section(section_name):
            if config.has_option(section_name, 'hbk-id'): 
                config.set(section_name, 'hbk-id', str(hbkid))
                #print( 'hbk-id = %d' % hbkid )
                config_update = 1
                            
        if config_update == 1:
            config_file = open(config_fname, 'w')
            config.write(config_file)
            config_file.close()
        
    
##########################################################

def genpassword(keyfile):
    """
    #process_call_return([openssl, 'rand', '-base64', '-out', keyfile, '48'])
    out, err = process_call_return([openssl, 'rand', '-base64', '48'])
    SaveUnixFile(out, keyfile)
    
    return True
    """
    genpasswd = os.urandom(48)
    out = base64.b64encode(genpasswd)
    SaveUnixFile(out, keyfile)

    return True
    
##########################################################
#  EVP_BytesToKey( ) from OpenSSL.
##########################################################

def EVP_BytesToKey(password, salt, key_len, iv_len):
    """
    Derive the key and the IV from the given password and salt.
    """
    # pbkdf1( (pwd || salt), iter_cnt=1, dkLen = 16, MD5)
    m = hashes.Hash(hashes.MD5())
    m.update(password + salt)
    dtot =  m.finalize()
    d = [ dtot ]
    
    while len(dtot)<(iv_len+key_len):
        # hash.MD5( pbkdf1 || pwd || salt )
        m = hashes.Hash(hashes.MD5())
        m.update(d[-1] + password + salt)
        tmp = m.finalize()
        d.append( tmp )
        dtot += d[-1]

    #print('salt=' + binascii.hexlify(salt).decode('ascii').upper())
    #print('key=' + binascii.hexlify(dtot[:key_len]).decode('ascii').upper())
    #print('iv=' + binascii.hexlify( dtot[key_len:key_len+iv_len]).decode('ascii').upper())
        
    return dtot[:key_len], dtot[key_len:key_len+iv_len]


def openssl_get_key_iv(password, salt, klen=32, ilen=16):
    '''
    @param password  The password to use as the seed.
    @param salt      The salt.
    @param klen      The key length.
    @param ilen      The initialization vector length.
    @param msgdgst   The message digest algorithm to use.
    '''
    # equivalent to:
    #   from hashlib import <mdi> as mdf
    #   from hashlib import md5 as mdf
    #   from hashlib import sha512 as mdf
    
    password = password.encode('ascii', 'ignore')  # convert to ASCII

    try:
        maxlen = klen + ilen
        m = hashes.Hash(hashes.MD5())
        m.update(password + salt)
        keyiv = m.finalize()
        tmp = [keyiv]
        while len(tmp) < maxlen:
            m = hashes.Hash(hashes.MD5())
            m.update(tmp[-1] + password + salt)
            ttt = m.finalize()
        
            tmp.append( ttt )
            keyiv += tmp[-1]  # append the last byte
        key = keyiv[:klen]
        iv = keyiv[klen:klen+ilen]
        return key, iv
    except UnicodeDecodeError:
        return None, None


def derive_key_and_iv(password, salt, key_length, iv_length):
    d = d_i = b''
    while len(d) < key_length + iv_length:
        #d_i = md5(d_i + password + salt).digest()
        m = hashes.Hash(hashes.MD5())
        m.update(d_i + password + salt)
        d_i = m.finalize()
        
        d += d_i
    return d[:key_length], d[key_length:key_length+iv_length]


def loadkeybytes(passfile):
    passphrase = b'\x00' * 16
    salt = bytes()
    newpasswd = str()

    with open(passfile, 'rt') as fpasswd:
        passwd = fpasswd.read()

        newpasswd = passwd.strip()
        #print('[', newpasswd, ']')

        #passphrase, iv = openssl_kdf('md5', newpasswd.encode(), salt, 16, 16)
        passphrase, iv = EVP_BytesToKey(newpasswd.encode(), salt, 16, 16)
        #passphrase, iv = openssl_get_key_iv(newpasswd, salt, 16, 16 )
        #print(passphrase)
        
    return newpasswd, passphrase, iv  

##########################################################
#  genkeyfiles
##########################################################

def genkeyfiles(passfile, prikey, pubkey):
    """
    out, err = process_call_return([openssl, 'genrsa', '-aes256',
                          '-passout', ('file:%s' % passfile),
                           '-out', prikey,
                           '3072'
                          ])
                    
    rout = out
    rerr = err

    if (len(out) > 0) and (out.decode('ASCII').find('error') >= 0) :
        return False, rout, rerr      
    
    out, err = process_call_return([openssl, 'rsa',
                           '-in', prikey,
                            '-passin', ('file:%s' % passfile),
                            '-pubout',
                            '-out', pubkey
                          ])

    rout = rout + out
    rerr = rerr + err

    return True, rout, rerr
    """
    # cmsecret remove test
    # cmpubkey remove test
    # key generation test
    status = True
    out = b''
    err = b''
    if( (os.path.isfile(passfile) != True) ):
        err += ("Can't open file %s\r\nError getting password\r\n" % passfile).encode()
        status = False
    else:
        status = True

    #err += ("Loading 'screen' into random state - done\r\n").encode()
    err += ("Generating RSA private key, 3072 bit long modulus\r\n").encode()
    err += ("............................................++\r\n").encode()
    err += ("e is 65537 (0x10001)\r\n").encode()

    key = asymmetric.rsa.generate_private_key(
                public_exponent=65537,
                key_size=3072,
            )

    newpasswd, passphrase, iv = loadkeybytes(passfile)

    # Write prikey to disk for safe keeping
    with open(prikey, "wb") as f:
        f.write(key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            # TraditionalOpenSSL, PKCS8, Raw, OpenSSH
            encryption_algorithm=serialization.BestAvailableEncryption(newpasswd.encode("utf-8")),
            # KeySerializationEncryption(), BestAvailableEncryption(passphrase), NoEncryption()
        ))
        f.close()

     
    # Write pubkey
    with open(pubkey, "wb") as f:
        f.write(key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))
        f.close()

    '''
    ## Interoperability Test ########################

    prikeypem3 = key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption(),
                )        

    newpasswd, passphrase, iv = loadkeybytes(passfile)
    try:
        with open(prikey, "rb") as rf:
            key2 = serialization.load_pem_private_key(
                    rf.read(),
                    password=newpasswd.encode("utf-8"),
                    backend=backends.default_backend()
            )
    except FileNotFoundError:
        raise MissingRSAKey()

    prikeypem4 = key2.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
    
    if prikeypem3 == prikeypem4 :
        print('OK!!!!!!!!!!')
    else:
        print('NOK!!!!!!!!!!')
        print(prikeypem3)
        print(prikeypem4)        

   ## Interoperability Test ########################
    
    out, err = process_call_return([openssl, 'rsa',
                           '-in', prikey,
                            '-passin', ('file:%s' % passfile),
                            '-pubout',
                            '-out', (pubkey+'.test')
                          ])
    print(out)
    print(err)
    '''      
    
    if( (os.path.isfile(pubkey) != True) ):
        err += ("%s: No such file or directory\r\n" % pubkey).encode()
        status = False
    else:
        err += ("writing RSA key\r\n\r\n").encode()
        status = True

    return status, out, err


##########################################################

def genotpkeyfile(passfile, otpkey, encotpkey):
    """
    out, err = process_call_return([openssl, 'rand', '-out', otpkey, '16'])
    if (len(out) > 0) and (out.decode('ASCII').find('error') >= 0) :
        return False 
    
    f = open(otpkey, 'rb')
    oout = f.read()
    f.close()

    process_call_return([openssl, 'enc', '-e', '-nosalt', '-aes-128-cbc',
                              '-in', otpkey, '-out', encotpkey,
                              '-pass', ('file:%s' % passfile)
                          ]
                        )

    out, err = process_call_return([openssl, 'enc', '-d', '-nosalt', '-aes-128-cbc',
                              '-in', encotpkey, 
                              '-pass', ('file:%s' % passfile)
                          ]
                        )

    if( oout == out ):
        return True

    return False
    """

    status = True

    if( (os.path.isfile(passfile) != True) ):
        status = False
    else:
        status = True
        
    genpasswd = os.urandom(16)
    
    # Write otpkey
    try:
        with open(otpkey, "wb") as f:
            f.write(genpasswd)
            f.close()
    except:
        status = False
        raise

    if len(genpasswd) == 16 :
        pks7pad = 16
    else :
        pks7pad = 16 - len(genpasswd)

    paddedkey = genpasswd + (pks7pad.to_bytes(1, byteorder="little") * pks7pad)

    #print('genpasswd:', binascii.hexlify(genpasswd))
    #print('paddedkey:', binascii.hexlify(paddedkey))
        
    newpasswd, passphrase, iv = loadkeybytes(passfile)
    
    encipher = ciphers.Cipher(ciphers.algorithms.AES(passphrase), ciphers.modes.CBC(iv))
    encryptor = encipher.encryptor()
    encedkey = encryptor.update(paddedkey) + encryptor.finalize()

    # Write encotpkey
    try:
        with open(encotpkey, "wb") as f:
            f.write(encedkey)
            f.close()
    except:
        status = False
        raise


    '''
    out, err = process_call_return([openssl, 'enc', '-d', '-nosalt', '-aes-128-cbc',
                              '-in', encotpkey, 
                              '-pass', ('file:%s' % passfile)
                          ]
                        )

    print('out:', binascii.hexlify(out))
    if( genpasswd == out ):
        return True
    '''

    # AES encrypt     
    newpasswd, passphrase, iv = loadkeybytes(passfile)

    # AES decrypt test           
    decipher = ciphers.Cipher(ciphers.algorithms.AES(passphrase), ciphers.modes.CBC(iv))
    decryptor = decipher.decryptor()


    
    try:
        with open(encotpkey, "rb") as rf:
            eout = rf.read()
            padout = decryptor.update(eout)
    except FileNotFoundError:
        status = False
        padout = bytes()
        raise

    #PKS7 Depadding Rule
    depadsiz = int(padout[-1])
    out = padout[:-depadsiz]    
    
    if( genpasswd == out ):
        return True

    return False

##########################################################

def convotpkeyfile(passfile, encotpkey, txtotpkey):
    """
    binout, err = process_call_return([openssl, 'enc', '-d', '-nosalt', '-aes-128-cbc',
                              '-in', encotpkey, 
                              '-pass', ('file:%s' % passfile)
                          ]
                        )

    WriteArmHexTxtFile(binout, txtotpkey)

    return True
    """

    status = True
    
    if( (os.path.isfile(passfile) != True) ):
        return False

    if( (os.path.isfile(encotpkey) != True) ):
        return False

    # AES encrypt     
    newpasswd, passphrase, iv = loadkeybytes(passfile)

    # AES decrypt test           
    decipher = ciphers.Cipher(ciphers.algorithms.AES(passphrase), ciphers.modes.CBC(iv))
    decryptor = decipher.decryptor()

    try:
        with open(encotpkey, "rb") as rf:
            eout = rf.read()
            padout = decryptor.update(eout)
    except FileNotFoundError:
        status = False
        padout = bytes()
        raise

    #PKS7 Depadding Rule
    depadsiz = int(padout[-1])
    binout = padout[:-depadsiz]

    WriteArmHexTxtFile(binout, txtotpkey)

    return status

##########################################################

def enckrtlkeyfile(passfile, krtlkey, enckrtlkey):
    """
    oout = str()

    try:
        f = open(krtlkey, 'rb')
        oout = f.read()
        f.close
    except OSError as err:
        return False

    out, err = process_call_return([openssl, 'enc', '-e', '-nosalt', '-aes-128-cbc',
                              '-in', krtlkey, '-out', enckrtlkey,
                              '-pass', ('file:%s' % passfile)
                          ]
                        )
    if (len(out) > 0) and (out.decode('ASCII').find('error') >= 0) :
        return False                  

    out, err = process_call_return([openssl, 'enc', '-d', '-nosalt', '-aes-128-cbc',
                              '-in', enckrtlkey, 
                              '-pass', ('file:%s' % passfile)
                          ]
                        )

    if( oout == out ):
        return True

    return False
    """

    if( (os.path.isfile(passfile) != True) ):
        return False
        
    rtlk = bytes()

    try:
        f = open(krtlkey, 'rb')
        rtlk = f.read()
        f.close
    except OSError as err:
        return False

    status = True

    if len(rtlk) == 16 :
        pks7pad = 16
    else :
        pks7pad = 16 - len(rtlk)

    paddedkey = rtlk + (pks7pad.to_bytes(1, byteorder="little") * pks7pad)

    #print('rtlkey:', binascii.hexlify(rtlk))
    #print('paddedkey:', binascii.hexlify(paddedkey))
        
    newpasswd, passphrase, iv = loadkeybytes(passfile)
    
    encipher = ciphers.Cipher(ciphers.algorithms.AES(passphrase), ciphers.modes.CBC(iv))
    encryptor = encipher.encryptor()
    encstream = encryptor.update(paddedkey) + encryptor.finalize()

    # Write enckrtlkey
    try:
        with open(enckrtlkey, "wb") as f:
            f.write(encstream)
            f.close()
    except:
        status = False
        raise

    # AES encrypt     
    newpasswd, passphrase, iv = loadkeybytes(passfile)

    # AES decrypt test           
    decipher = ciphers.Cipher(ciphers.algorithms.AES(passphrase), ciphers.modes.CBC(iv))
    decryptor = decipher.decryptor()

    try:
        with open(enckrtlkey, "rb") as rf:
            eout = rf.read()
            padout = decryptor.update(eout)
    except FileNotFoundError:
        status = False
        padout = bytes()
        raise

    #PKS7 Depadding Rule
    depadsiz = int(padout[-1])
    out = padout[:-depadsiz]

    if( out == rtlk ):
        return True
    
    return False

##########################################################
        
def genHBK(pathname, hbk_gen_util, pubkey, hashfile, zerofile, logfile, binfile):
    retcode = True
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ hbk_gen_util,
                '-key', pubkey,
                '-endian', 'L',
                '-hash_format', 'SHA256_TRUNC',
                '-hash_out', hashfile,
                '-zeros_out', zerofile
               ]

    out = b''
    err = b''
    process_call(cmdlist)

    if( os.path.isfile(logfile) == True):
        os.remove(logfile)

    if( os.path.isfile('gen_hbk_log.log') == True):
        os.rename('gen_hbk_log.log', logfile)

    logf = open(logfile, 'rb')
    pout = logf.read()
    logf.close()

    for rline in pout.decode('ASCII').split('\n'):
        if rline.lower().find('error') > 0 :
            out = out + rline.encode()
            retcode = False

    uniqueBuff = list(range(PROD_UNIQUE_BUFF_SIZE))

    if (retcode == True) and (hashfile != "") and (os.path.isfile(hashfile) == True) :
        fname, ext = os.path.splitext(hashfile)

        if ext == '.bin':
            uniqueBuff = GetDataFromBinFile(hashfile)
        else:
            uniqueBuff = GetDataFromTxtFile(hashfile)
    
        #print( uniqueBuff.hex() )
        retcode = True
    else:
        retcode = False

    if binfile != "" and retcode == True:
        wrfile = open(binfile, "wb")
        wrfile.write(uniqueBuff)
        wrfile.close()

    os.chdir(cwd)

    return retcode, out, err

##########################################################

def genKeyAsset(pathname, assetpkgutil, cfgfile, logfile):
    out = b''
    err = b''

    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ assetpkgutil,
                cfgfile,
                logfile
              ]
        
    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode, out, err

##########################################################

def genCMPU(pathname, cmpugenutil, cfgfile):
    out = b''
    err = b''

    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ cmpugenutil,
                '-cfg_file',
                cfgfile
              ]

    process_call(cmdlist)
    
    os.chdir(cwd)

    return True, out, err    
    
##########################################################

def genKeyCert(pathname, certkeyutil, cfgfile, logfile):
    cwd = os.getcwd()
    os.chdir(pathname)

    out = b''
    err = b''

    cmdlist = [ certkeyutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode, out, err

##########################################################

def genOEMKeyReqPkg(pathname, okrptutil, cfgfile, logfile):
    out = b''
    err = b''

    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ okrptutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)
    
    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode, out, err

##########################################################

def genICVKeyResPkg(pathname, ikrptutil, cfgfile, logfile):
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ ikrptutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode

##########################################################

def genOEMKeyAsset(pathname, oapptutil, cfgfile, logfile):
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ oapptutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode

##########################################################

def genDMPU(pathname, dmpugenutil, cfgfile):
    out = b''
    err = b''

    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ dmpugenutil,
                '-cfg_file',
                cfgfile
              ]

    process_call(cmdlist)
    
    os.chdir(cwd)

    return True, out, err   

##########################################################

def genASSET(pathname, assetprovutil, mode, cfgfile, pkgfile):
    out = b''
    err = b''

    cwd = os.getcwd()
    os.chdir(pathname)

    path, cfgname = os.path.split(cfgfile)
    fname, ext = os.path.splitext(cfgname)

    logfile = '../example/%s.log' % fname

    cmdlist = [ assetprovutil,
                ('.%s' % cfgfile),
                logfile
              ]

    process_call(cmdlist)

    retcode = True

    if retcode == True :
        logfptr = open(logfile, 'r')
        logcontent = logfptr.read()
        logfptr.close()
        if logcontent.lower().find('error') > 0 :
            retcode = False
        if logcontent.lower().find('invalid') > 0 :
            retcode = False
    
    if retcode == True :
        path, pkgname = os.path.split(pkgfile)
        fname, ext = os.path.splitext(pkgname)

        txtfilename = '%s/%s.txt' % (path, fname)

        binstr = GetDataFromBinFile(pkgfile)
        
        WriteHexFile(binstr, ('const_%s' % fname), txtfilename)
    
    if retcode == False :
        out = out + b'**** genASSET is error\n'

    os.chdir(cwd)

    return retcode, out, err   

##########################################################

def genEnablerCert(pathname, enablerutil, cfgfile, logfile):
    out = b''
    err = b''

    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ enablerutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode, out, err

##########################################################

def genDeveloperCert(pathname, developerutil, cfgfile, logfile):
    out = b''
    err = b''
    
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ developerutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False

    return retcode, out, err


##########################################################

def genContentCert(pathname, contentutil, cfgfile, logfile):
    out = b''
    err = b''
    
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ contentutil,
                cfgfile,
                logfile
              ]

    process_call(cmdlist)

    logf = open(logfile, 'rb')
    out = logf.read()
    logf.close()

    os.chdir(cwd)

    retcode = True
    pout = out.decode('ASCII')
    for rline in pout.split('\n'):
        if rline.lower().find('error') > 0 :
            retcode = False    

    return retcode, out, err


##########################################################

def genRetmImage(pathname, entrypoint, binfile, rtmfile):
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ 'retmimage',
                '-A16',
                ('-RETM=%s' % entrypoint),
                binfile,
                rtmfile
              ]

    out, err = mkRETMImage(cmdlist)

    os.chdir(cwd)

    if (len(out) > 0) and (out.decode('ASCII').lower().find('error') >= 0) :
        return False     

    return True

##########################################################

def genSecuImage(pathname, secuimageutil, inifile, imgfile, logfile):
    cwd = os.getcwd()
    os.chdir(pathname)

    cmdlist = [ 'secuimageutil',
                inifile,
                imgfile,
                logfile
              ]

    out, err = mkSecureImage(cmdlist)

    os.chdir(cwd)
    
    if (len(out) > 0) and (out.decode('ASCII').lower().find('error') >= 0) :
        return False 

    return True
