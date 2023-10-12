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

section_list = { "UEBOOT": 0, "RTOS.cache": 1, "RTOS.sram": 2, "Comp.combined": 3, "Comp.PTIM": 4, "Comp.RaLIB": 5, "RMA": 6 }
tag_filename = [ "ueboot", "cache", "sram", "cm", "tim", "ra", "rma" ]


def target_tbl_generate(config, section_name, tpmcfg):
    index = section_list[section_name]

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sboot = config.get("FC9K-TPM-CONFIG", 'sboot')

    filename =  "%s/sboot_hbk%d_%s_cert.tbl" % (tpmcfg, int(hbkid), tag_filename[index] )
    print( "filename = %s" % filename )

    retmem = config.get(section_name, 'RETMEM')
    if( str(retmem) == '1' ):
        extname = 'rtm'
    else:
        extname = 'bin'

    aesmode = config.get(section_name, 'KEYENC')

    if( (str(aesmode) == 'ICV_CODE_ENC') or (str(aesmode) == 'OEM_CODE_ENC') ):
        aesflag = 1
        #aestag  = '_enc'
        #extname = 'bin'
        aestag  = ''
    else:
        aesflag = 0
        aestag  = ''

    flashoffset = config.get(section_name, 'FLASHADDR')
    if( str(flashoffset) != ''):
        intflashoffset = int(flashoffset, 16)
    else:
        intflashoffset = 0
        

    tbl_file = open(filename, 'w')
    
    comp = config.get(section_name, 'COMP1')

    if( str(comp) != ''):
        compaddr = config.get(section_name, 'COMP1ADDR')
        listcompaddr = str(compaddr).split()
        if( int(listcompaddr[1], 16) == 0xffffffff ):
            list1addr = int(listcompaddr[1], 16)
        else:
            list1addr = intflashoffset + int(listcompaddr[1], 16)
        
        #compstr = "../image/%s%s.%s %s 0x%d\n" % (str(comp), aestag, extname, str(compaddr), aesflag)
        compstr = "../image/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        #print( compstr )
        tbl_file.write( compstr )

    comp = config.get(section_name, 'COMP2')

    if( str(comp) != ''):
        compaddr = config.get(section_name, 'COMP2ADDR')
        listcompaddr = str(compaddr).split()
        if( int(listcompaddr[1], 16) == 0xffffffff ):
            list1addr = int(listcompaddr[1], 16)
        else:
            list1addr = intflashoffset + int(listcompaddr[1], 16)
            
        #compstr = "../image/%s%s.%s %s 0x%d\n" % (str(comp), aestag, extname, str(compaddr), aesflag)
        compstr = "../image/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        #print( compstr )
        tbl_file.write( compstr )

    comp = config.get(section_name, 'COMP3')

    if( str(comp) != ''):
        compaddr = config.get(section_name, 'COMP3ADDR')
        listcompaddr = str(compaddr).split()
        if( int(listcompaddr[1], 16) == 0xffffffff ):
            list1addr = int(listcompaddr[1], 16)
        else:
            list1addr = intflashoffset + int(listcompaddr[1], 16)
            
        #compstr = "../image/%s%s.%s %s 0x%d\n" % (str(comp), aestag, extname, str(compaddr), aesflag)
        compstr = "../image/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        #print( compstr )
        tbl_file.write( compstr )

    tbl_file.close()

##########################################################

hbkid_list = { 0: "cm", 1: "dm" }
load_scheme_list = { "LOAD_AND_VERIFY": 0, "VERIFY_ONLY_IN_FLASH": 1, "VERIFY_ONLY_IN_MEM": 2, "LOAD_ONLY": 3 }
aes_ce_id_list   = { "NO_IMAGE_ENC": 0, "ICV_CODE_ENC": 1, "OEM_CODE_ENC": 2 }
aes_key_list     = [ "", "../cmsecret/kceicv.txt", "../dmsecret/kce.txt" ]

def target_cfg_generate(config, section_name, tpmcfg):
    index = section_list[section_name]

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sboot = config.get("FC9K-TPM-CONFIG", 'sboot')
    nvcount = config.get("FC9K-TPM-CONFIG", 'nvcount')

    filename =  "%s/sboot_hbk%d_%s_cert.cfg" % (tpmcfg, int(hbkid), tag_filename[index] )
    print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "CNT-CFG"

    targetcfg.add_section(targetsection)

    keypair = "../%ssecret/%spublisher_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   

    keypairpwd = "../%ssecret/pwd.publisher.txt" % ( hbkid_list[int(hbkid)] )
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypairpwd))   

    tabname =  "../%stpmcfg/sboot_hbk%d_%s_cert.tbl" % (hbkid_list[int(hbkid)], int(hbkid), tag_filename[index] )
    targetcfg.set(targetsection, 'images-table', str(tabname))   
    
    targetcfg.set(targetsection, 'nvcounter-val', str(nvcount))

    keyload = load_scheme_list[str(config.get(section_name, 'KEYLOAD'))]
    targetcfg.set(targetsection, 'load-verify-scheme', str(keyload))

    aesceid = aes_ce_id_list[str(config.get(section_name, 'KEYENC'))]
    targetcfg.set(targetsection, 'aes-ce-id', str(aesceid))

    targetcfg.set(targetsection, 'crypto-type', str(0))
        
    targetcfg.set(targetsection, 'aes-enc-key', str(aes_key_list[int(aesceid)]) )

    certpkg = "../%spublic/sboot_hbk%d_%s_cert.bin" % ( hbkid_list[int(hbkid)], int(hbkid), tag_filename[index] )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg) )


    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()
                             
##########################################################

def target_enbcert_generate(config, tpmcfg):

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sdebug = config.get("FC9K-TPM-CONFIG", 'sdebug')
    nvcount = config.get("FC9K-TPM-CONFIG", 'nvcount')
    lcs = config.get("FC9K-TPM-CONFIG", 'lcs')

    filename =  "%s/sdebug_hbk%d_enabler_cert.cfg" % (tpmcfg, int(hbkid) )
    print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "ENABLER-DBG-CFG"

    targetcfg.add_section(targetsection)

    if( int(sdebug) == 2 ):
        keypair = "../%ssecret/%skey_pair.pem"        % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
        keypwd  = "../%ssecret/pwd.%skey.txt"         % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    else:
        keypair = "../%ssecret/%senabler_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
        keypwd  = "../%ssecret/pwd.enabler.txt"       % ( hbkid_list[int(hbkid)] )

    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypwd))   

    targetcfg.set(targetsection, 'lcs', str(lcs))

    targetcfg.set(targetsection, 'debug-mask[0-31]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[32-63]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[64-95]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[96-127]', str('0xffffffff'))

    targetcfg.set(targetsection, 'debug-lock[0-31]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-lock[32-63]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-lock[64-95]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-lock[96-127]', str('0xffffffff'))

    targetcfg.set(targetsection, 'hbk-id', str(hbkid))

    nextpubkey = "../%spubkey/%sdeveloper_pubkey.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    targetcfg.set(targetsection, 'next-cert-pubkey', str(nextpubkey))

    certpkg = "../%spublic/sdebug_hbk%d_enabler_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()    


def target_rmaenbcert_generate(config, tpmcfg):

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sdebug = config.get("FC9K-TPM-CONFIG", 'sdebug')
    nvcount = config.get("FC9K-TPM-CONFIG", 'nvcount')
    lcs = config.get("FC9K-TPM-CONFIG", 'lcs')

    filename =  "%s/sdebug_hbk%d_enabler_rma_cert.cfg" % (tpmcfg, int(hbkid) )
    print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "ENABLER-DBG-CFG"

    targetcfg.add_section(targetsection)

    if( int(sdebug) == 2 ):
        keypair = "../%ssecret/%skey_pair.pem"        % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
        keypwd  = "../%ssecret/pwd.%skey.txt"         % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    else:
        keypair = "../%ssecret/%senabler_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
        keypwd  = "../%ssecret/pwd.enabler.txt"       % ( hbkid_list[int(hbkid)] )

    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypwd))   

    targetcfg.set(targetsection, 'lcs', str(lcs))

    targetcfg.set(targetsection, 'rma-mode', str(1))

    targetcfg.set(targetsection, 'debug-lock[0-31]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-lock[32-63]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-lock[64-95]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-lock[96-127]', str('0xffffffff'))

    targetcfg.set(targetsection, 'hbk-id', str(hbkid))

    nextpubkey = "../%spubkey/%sdeveloper_pubkey.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    targetcfg.set(targetsection, 'next-cert-pubkey', str(nextpubkey))

    certpkg = "../%spublic/sdebug_hbk%d_enabler_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()    


def target_devcert_generate(config, tpmcfg):

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sdebug = config.get("FC9K-TPM-CONFIG", 'sdebug')
    nvcount = config.get("FC9K-TPM-CONFIG", 'nvcount')
    lcs = config.get("FC9K-TPM-CONFIG", 'lcs')

    filename =  "%s/sdebug_hbk%d_developer_cert.cfg" % (tpmcfg, int(hbkid) )
    print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "DEVELOPER-DBG-CFG"

    targetcfg.add_section(targetsection)

    keypair = "../%ssecret/%sdeveloper_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    keypwd  = "../%ssecret/pwd.developer.txt"       % ( hbkid_list[int(hbkid)] )

    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypwd))   

    targetcfg.set(targetsection, 'soc-id', str('../image/soc_id.bin'))

    targetcfg.set(targetsection, 'debug-mask[0-31]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[32-63]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[64-95]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[96-127]', str('0xffffffff'))

    enbcertpkg = "../%spublic/sdebug_hbk%d_enabler_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'enabler-cert-pkg', str(enbcertpkg))

    certpkg = "../%spublic/sdebug_hbk%d_developer_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()    

        
def target_rmadevcert_generate(config, tpmcfg):

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sdebug = config.get("FC9K-TPM-CONFIG", 'sdebug')
    nvcount = config.get("FC9K-TPM-CONFIG", 'nvcount')
    lcs = config.get("FC9K-TPM-CONFIG", 'lcs')

    filename =  "%s/sdebug_hbk%d_developer_rma_cert.cfg" % (tpmcfg, int(hbkid) )
    print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "DEVELOPER-DBG-CFG"

    targetcfg.add_section(targetsection)

    keypair = "../%ssecret/%sdeveloper_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    keypwd  = "../%ssecret/pwd.developer.txt"       % ( hbkid_list[int(hbkid)] )

    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypwd))   

    targetcfg.set(targetsection, 'soc-id', str('../image/soc_id.bin'))

    targetcfg.set(targetsection, 'debug-mask[0-31]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[32-63]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[64-95]', str('0xffffffff'))
    targetcfg.set(targetsection, 'debug-mask[96-127]', str('0xffffffff'))

    enbcertpkg = "../%spublic/sdebug_hbk%d_enabler_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'enabler-cert-pkg', str(enbcertpkg))

    certpkg = "../%spublic/sdebug_hbk%d_developer_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()    

##########################################################

def target_ini_generate(config, section_name, tpmcfg):
    index = section_list[section_name]

    hbkid = config.get("FC9K-TPM-CONFIG", 'hbk-id')
    sboot = config.get("FC9K-TPM-CONFIG", 'sboot')
    sdebug = config.get("FC9K-TPM-CONFIG", 'sdebug')
    nvcount = config.get("FC9K-TPM-CONFIG", 'nvcount')

    filename =  "%s/sboot_hbk%d_%s_cert.ini" % (tpmcfg, int(hbkid), tag_filename[index] )
    print( "filename = %s" % filename )

    retmem = config.get(section_name, 'RETMEM')
    if( str(retmem) == '1' ):
        extname = 'rtm'
    else:
        extname = 'bin'

    aesmode = config.get(section_name, 'KEYENC')

    if( (str(aesmode) == 'ICV_CODE_ENC') or (str(aesmode) == 'OEM_CODE_ENC') ):
        aesflag = 1
        aestag  = '_enc'
        extname = 'bin'
        aespost = ' aes'
    else:
        aesflag = 0
        aestag  = ''
        aespost = ''

    ############################
    swcomp = 0
    comp = config.get(section_name, 'COMP1')
    if( str(comp) != ''):
        swcomp = swcomp + 1
        
    comp = config.get(section_name, 'COMP2')
    if( str(comp) != ''):
        swcomp = swcomp + 1
        
    comp = config.get(section_name, 'COMP3')
    if( str(comp) != ''):
        swcomp = swcomp + 1
    ############################


    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "IMAGE"

    targetcfg.add_section(targetsection)

    if( (swcomp != 0) and (int(sboot) == 3) ):
        cert1 = "..\\%spublic\\sboot_hbk%d_3lvl_key_chain_issuer.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
        cert2 = "..\\%spublic\\sboot_hbk%d_3lvl_key_chain_publisher.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
        cert3 = "..\\%spublic\\sboot_hbk%d_%s_cert.bin" % ( hbkid_list[int(hbkid)], int(hbkid), tag_filename[index] )
    elif( (swcomp != 0)  and (int(sboot) == 2) ):
        cert1 = "..\\%spublic\\sboot_hbk%d_2lvl_key_chain_publisher.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
        cert2 = "..\\%spublic\\sboot_hbk%d_%s_cert.bin" % ( hbkid_list[int(hbkid)], int(hbkid), tag_filename[index] )
        cert3 = ""
    else:
        cert1 = ""
        cert2 = ""
        cert3 = ""

    targetcfg.set(targetsection, 'CERT1', str(cert1))
    targetcfg.set(targetsection, 'CERT2', str(cert2))
    targetcfg.set(targetsection, 'CERT3', str(cert3))

    title = config.get(section_name, 'TITLE')
    sfdp  = config.get(section_name, 'SFDP')

    if( str(sfdp) != '' ):
        str_title = "%s %s%s" % ( str(title), str(sfdp), str(aespost) )
        str_sfdp  = "..\\SFDP\\%s.bin" % str(sfdp)
    else:
        str_title = "%s%s" % ( str(title) , str(aespost) )
        str_sfdp  = ""

    targetcfg.set(targetsection, 'TITLE', str(str_title))
    targetcfg.set(targetsection, 'SFDP', str(str_sfdp))

    if( swcomp != 0 ):
        targetcfg.set(targetsection, 'KEYLEVEL', str(sboot))
    else:
        targetcfg.set(targetsection, 'KEYLEVEL', str(0))
    
    targetcfg.set(targetsection, 'KEYSIZE', str(3072))
    targetcfg.set(targetsection, 'KEYLOAD', str(config.get(section_name, 'KEYLOAD')))
    targetcfg.set(targetsection, 'KEYENC', str(config.get(section_name, 'KEYENC')))


    comp = config.get(section_name, 'COMP1')
    if( str(comp) != ''):
        comstr = "..\\image\\%s%s.%s" % (str(comp), aestag, extname)
    else:
        comstr = ""
        
    targetcfg.set(targetsection, 'COMP1', str(comstr))  
    
    comp = config.get(section_name, 'COMP2')
    if( str(comp) != ''):
        comstr = "..\\image\\%s%s.%s" % (str(comp), aestag, extname)
    else:
        comstr = ""
        
    targetcfg.set(targetsection, 'COMP2', str(comstr))  

    comp = config.get(section_name, 'COMP3')
    if( str(comp) != ''):
        comstr = "..\\image\\%s%s.%s" % (str(comp), aestag, extname)
    else:
        comstr = ""
        
    targetcfg.set(targetsection, 'COMP3', str(comstr))  


    targetcfg.set(targetsection, 'CRCON', str(config.get(section_name, 'CRCON')))

    if( int(hbkid) == 0 ):
        postfix = '_icv'
    else:
        postfix = ''

    if( str(sfdp) != '' ):
        if( aesflag == 1 ):
            imagestr = "..\\public\\%s_%s_aes%s.img" % ( str(config.get(section_name, 'IMAGE')), str(sfdp), str(postfix) )
        else:
            imagestr = "..\\public\\%s_%s%s.img" % ( str(config.get(section_name, 'IMAGE')), str(sfdp), str(postfix) )
    else:
        if( aesflag == 1 ):
            imagestr = "..\\public\\%s_aes%s.img" % ( str(config.get(section_name, 'IMAGE')), str(postfix) )
        else:
            imagestr = "..\\public\\%s%s.img" % ( str(config.get(section_name, 'IMAGE')), str(postfix) )

    targetcfg.set(targetsection, 'IMAGE', str(imagestr))  

    # Offset Calc
    # retmem
    # swcomp

    flashoffset = config.get(section_name, 'FLASHADDR')
    if( str(flashoffset) != ''):
        intflashoffset = int(flashoffset, 16)
    else:
        intflashoffset = 0
        

    if( swcomp >= 1):
        listcom1addr = str(config.get(section_name, 'COMP1ADDR')).split()
    else:
        listcomp1addr = []
        
    if( (swcomp == 1) and (int(retmem) == 0) ):
        hex_offset =  str(listcom1addr[0])
    elif( (swcomp == 1) and (int(retmem) == 1) and (int(sboot) == 0 ) ):
        hex_offset =  str(listcom1addr[0])
    else:
        hex_offset = ''

    if( (swcomp == 1) and (int(retmem) == 0) and (int(listcom1addr[0], 16) != 0xffffffff) and (int(listcom1addr[1], 16) != 0xffffffff) ):
        #sram
        hex_comp1addr = str(listcom1addr[1])
        hex_version   = str(listcom1addr[1])
        #hexformat = "%08x" % (intflashoffset + int(str(listcom1addr[1]), 16))
        #hex_version   = str(hexformat)
    elif( (swcomp == 1) and (int(retmem) == 1) and (int(sboot) == 0 ) and (int(listcom1addr[0], 16) != 0xffffffff) and (int(listcom1addr[1], 16) != 0xffffffff) ):
        # unsecure retmem
        hex_comp1addr = str(listcom1addr[1])
        hex_version   = str(listcom1addr[1])
        #hexformat = "%08x" % (intflashoffset + int(str(listcom1addr[1]), 16))
        #hex_version   = str(hexformat)
    elif( (swcomp >= 1) and (int(listcom1addr[1], 16) == 0xffffffff) ):
        # cache 
        int_comp1addr = int(listcom1addr[0], 16) - 0x00100000  # FC9050SLR
        hex_comp1addr = hex(int_comp1addr)
        hex_version   = ''
    elif( (swcomp >= 1) and (int(listcom1addr[0], 16) == 0xffffffff) ):
        # xip
        hex_comp1addr = str(listcom1addr[1])
        hex_version   = ''
    elif( swcomp >= 1 ):
        # others
        hex_comp1addr = str(listcom1addr[1])
        hex_version   = ''
    else:
        hex_comp1addr = ''
        hex_version   = ''
    

    if( swcomp >= 2 ):
        listcom2addr = str(config.get(section_name, 'COMP2ADDR')).split()
        hex_comp2addr = str(listcom2addr[1])
    else:
        hex_comp2addr = ''
        
    if( swcomp >= 3 ):
        listcom3addr = str(config.get(section_name, 'COMP3ADDR')).split()
        hex_comp3addr = str(listcom3addr[1])
    else:
        hex_comp3addr = ''

    hex_flash = '0'


    #TODO
    targetcfg.set(targetsection, 'OFFSET', str(hex_offset))  
    targetcfg.set(targetsection, 'COMP1ADDR', str(hex_comp1addr))  
    targetcfg.set(targetsection, 'COMP2ADDR', str(hex_comp2addr))  
    targetcfg.set(targetsection, 'COMP3ADDR', str(hex_comp3addr))  
    targetcfg.set(targetsection, 'FLASH', str(hex_flash))  

    if( str(config.get(section_name, 'DBGCERT')) == 'ON' ):
        str_dbgcert = "..\\%spublic\\sdebug_hbk%d_developer_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
        if( int(sdebug) == 2 ):
            #str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_2lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
            str_dbgcertk = ''
        else:
            str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_3lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
    elif( str(config.get(section_name, 'DBGCERT')) == 'RMA' ):
        str_dbgcert = "..\\%spublic\\sdebug_hbk%d_developer_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
        if( int(sdebug) == 2 ):
            #str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_2lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
            str_dbgcertk = ''
        else:
            str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_3lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
    else:
        str_dbgcert = ''
        str_dbgcertk = ''

    targetcfg.set(targetsection, 'DBGCERT', str(str_dbgcert))
    targetcfg.set(targetsection, 'DBGCERTK', str(str_dbgcertk))

    #TODO
    targetcfg.set(targetsection, 'VERSION', str(hex_version))  


    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()
                          
    
##########################################################

image_config_list = [
     [ 'TITLE'  , '' ]
    ,[ 'SFDP'   , 'SFLASH Name;' ]
    ,[ 'KEYLOAD', 'verify-scheme (LOAD_AND_VERIFY; VERIFY_ONLY_IN_FLASH; VERIFY_ONLY_IN_MEM; LOAD_ONLY);' ]
    ,[ 'KEYENC' , 'AES-option (NO_IMAGE_ENC; ICV_CODE_ENC; OEM_CODE_ENC);' ]
    ,[ 'FLASHADDR' , 'hex-string;' ]
    ,[ 'RETMEM'    , 'Load into RETMEM ( 0=unload, 1=load );' ]
    ,[ 'CRCON'     , 'CRC-option ( ON/OFF );' ]
    ,[ 'DBGCERT'   , 'DebugCert tagging ( ON;OFF;RMA );' ]
    ,[ 'COMP1'     , '1st SW component filename (w/o extention name);' ]
    ,[ 'COMP2'     , '2nd SW component filename (w/o extention name);' ]
    ,[ 'COMP3'     , '3rd SW component filename (w/o extention name);' ]
    ,[ 'IMAGE'     , 'target image filename (w/o extention name);' ]
    ,[ 'COMP1ADDR' , '1st SW component offset (load-addr store-addr max-size, hex-string);' ]
    ,[ 'COMP2ADDR' , '2nd SW component offset (load-addr store-addr max-size, hex-string);' ]
    ,[ 'COMP3ADDR' , '3rd SW component offset (load-addr store-addr max-size, hex-string);' ]
]
    
def target_image_config_update(config, config_update, section_name):

    if config.has_section(section_name):
        print( '\n>>>>> Target Image Model : %s' % str(section_name) )
        value = input( 'skip configuration options ? (skip = Enter)' )

        if str(value) != '':

            for config_item in image_config_list:
                if( config.get(section_name, config_item[0]) != '' ):
                    print( 'Current %s   = %s' % (config_item[0], str(config.get(section_name, config_item[0]))) )

                value = input('Enter New %s (%s Skip=Enter):\n' % (config_item[0], config_item[1]))
                if str(value) != '':
                    config.set(section_name, config_item[0], str(value) )
                    config_update = 1

        print( ' >>> ' )

    return config_update


def target_config_update(config_fname, tpmcfgname):

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
               '*     Return to the step 1.B.1  or 2.B.1 !!                          \n'
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

        sboot = config.get(section_name, 'sboot')

        if( int(sboot) == 0 ):
            print( 'Change to split mode on unsecure boot' )
            value = '0'
        else:
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

        target_enbcert_generate(config, tpmcfgname)
        target_devcert_generate(config, tpmcfgname)

        target_rmaenbcert_generate(config, tpmcfgname)
        target_rmadevcert_generate(config, tpmcfgname)

        if( str(config.get(section_name, 'bootmodel')) == '0' ): 
            config_update = target_image_config_update(config, config_update, "UEBOOT")
            if( int(sboot) != 0 ):
                target_tbl_generate(config, "UEBOOT", tpmcfgname)
                target_cfg_generate(config, "UEBOOT", tpmcfgname)
            target_ini_generate(config, "UEBOOT", tpmcfgname)
            config_update = target_image_config_update(config, config_update, "RTOS.cache")
            if( int(sboot) != 0 ):
                target_tbl_generate(config, "RTOS.cache", tpmcfgname)
                target_cfg_generate(config, "RTOS.cache", tpmcfgname)
            target_ini_generate(config, "RTOS.cache", tpmcfgname)
        else:
            config_update = target_image_config_update(config, config_update, "RTOS.sram")
            if( int(sboot) != 0 ):
                target_tbl_generate(config, "RTOS.sram", tpmcfgname)
                target_cfg_generate(config, "RTOS.sram", tpmcfgname)
            target_ini_generate(config, "RTOS.sram", tpmcfgname)

        if( str(config.get(section_name, 'combined')) == '1' ): 
            config_update = target_image_config_update(config, config_update, "Comp.combined")
            if( int(sboot) != 0 ):
                target_tbl_generate(config, "Comp.combined", tpmcfgname)
                target_cfg_generate(config, "Comp.combined", tpmcfgname)
            target_ini_generate(config, "Comp.combined", tpmcfgname)
        else:
            config_update = target_image_config_update(config, config_update, "Comp.PTIM")
            if( int(sboot) != 0 ):
                target_tbl_generate(config, "Comp.PTIM", tpmcfgname)
                target_cfg_generate(config, "Comp.PTIM", tpmcfgname)
            target_ini_generate(config, "Comp.PTIM", tpmcfgname)
            config_update = target_image_config_update(config, config_update, "Comp.RaLIB")
            if( int(sboot) != 0 ):
                target_tbl_generate(config, "Comp.RaLIB", tpmcfgname)
                target_cfg_generate(config, "Comp.RaLIB", tpmcfgname)
            target_ini_generate(config, "Comp.RaLIB", tpmcfgname)

        target_ini_generate(config, "RMA", tpmcfgname)    

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
    TPM_CONFIG  = "../cmtpmcfg"
    
    if "-cfg_file" in sys.argv:
        PROJ_CONFIG = sys.argv[sys.argv.index("-cfg_file") + 1]
    #print("Config File  - %s\n" %PROJ_CONFIG)
    if "-tpmcfg_dir" in sys.argv:
        TPM_CONFIG = sys.argv[sys.argv.index("-tpmcfg_dir") + 1]

    target_config_update( PROJ_CONFIG , TPM_CONFIG)

    
    
