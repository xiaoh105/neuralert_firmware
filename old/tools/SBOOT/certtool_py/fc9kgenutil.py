#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct
import subprocess

from fc9ksecuutil import *

##########################################################   

section_list = { "UEBOOT": 0, "RTOS.cache": 1, "RTOS.sram": 2, "Comp.combined": 3, "Comp.PTIM": 4, "Comp.RaLIB": 5, "RMA": 6 }
tag_filename = [ "ueboot", "cache", "sram", "cm", "tim", "ra", "rma" ]


def target_tbl_generate(targetdir, tpmconfig, section_name):

    if( tpmconfig.getrawconfig(section_name, 'TITLE') == '' ) :
        '''
        print('ini: section not found.');
        '''
        return
    
    index = section_list[section_name]

    hbkid = tpmconfig.gettpmhbkid_int()
    sboot = tpmconfig.gettpmsboot_int()
    imgpath = tpmconfig.getconfig('image')

    filename =  "%s/sboot_hbk%d_%s_cert.tbl" % (targetdir, int(hbkid), tag_filename[index] )
    #print( "filename = %s" % filename )

    retmem = tpmconfig.getrawconfig(section_name, 'RETMEM')
    if( str(retmem) == '1' or str(retmem) == 'ON' or str(retmem) == 'true'):
        extname = 'rtm'
    else:
        extname = 'bin'

    aesmode = tpmconfig.getrawconfig(section_name, 'KEYENC')

    if( (str(aesmode) == 'ICV_CODE_ENC') or (str(aesmode) == 'OEM_CODE_ENC') ):
        aesflag = 1
        #aestag  = '_enc'
        #extname = 'bin'
        aestag  = ''
    else:
        aesflag = 0
        aestag  = ''

    flashoffset = tpmconfig.getrawconfig(section_name, 'FLASHADDR')
    if( str(flashoffset) != ''):
        intflashoffset = int(flashoffset, 16)
    else:
        intflashoffset = 0

    comp1 = tpmconfig.getrawconfig(section_name, 'COMP1')
    comp2 = tpmconfig.getrawconfig(section_name, 'COMP2')
    comp3 = tpmconfig.getrawconfig(section_name, 'COMP3')

    if( (str(comp1) == '') and (str(comp2) == '') and (str(comp3) == '') ):
        return

    tbl_file = open(filename, 'w')
    
    comp = tpmconfig.getrawconfig(section_name, 'COMP1')

    if( str(comp) != ''):
        compaddr = tpmconfig.getrawconfig(section_name, 'COMP1ADDR')
        listcompaddr = str(compaddr).split()
        if( int(listcompaddr[1], 16) == 0xffffffff ):
            list1addr = int(listcompaddr[1], 16)
        else:
            list1addr = intflashoffset + int(listcompaddr[1], 16)
        
        #compstr = "../image/%s%s.%s %s 0x%d\n" % (str(comp), aestag, extname, str(compaddr), aesflag)
        if( imgpath == '' or imgpath == '../image' ):
            compstr = "../image/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        else:
            compstr = "%s/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(imgpath), str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        #print( compstr )
        tbl_file.write( compstr )

    comp = tpmconfig.getrawconfig(section_name, 'COMP2')

    if( str(comp) != ''):
        compaddr = tpmconfig.getrawconfig(section_name, 'COMP2ADDR')
        listcompaddr = str(compaddr).split()
        if( int(listcompaddr[1], 16) == 0xffffffff ):
            list1addr = int(listcompaddr[1], 16)
        else:
            list1addr = intflashoffset + int(listcompaddr[1], 16)
            
        #compstr = "../image/%s%s.%s %s 0x%d\n" % (str(comp), aestag, extname, str(compaddr), aesflag)
        if( imgpath == '' or imgpath == '../image' ):
            compstr = "../image/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        else:
            compstr = "%s/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(imgpath), str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        #print( compstr )
        tbl_file.write( compstr )

    comp = tpmconfig.getrawconfig(section_name, 'COMP3')

    if( str(comp) != ''):
        compaddr = tpmconfig.getrawconfig(section_name, 'COMP3ADDR')
        listcompaddr = str(compaddr).split()
        if( int(listcompaddr[1], 16) == 0xffffffff ):
            list1addr = int(listcompaddr[1], 16)
        else:
            list1addr = intflashoffset + int(listcompaddr[1], 16)
            
        #compstr = "../image/%s%s.%s %s 0x%d\n" % (str(comp), aestag, extname, str(compaddr), aesflag)
        if( imgpath == '' or imgpath == '../image' ):
            compstr = "../image/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        else:
            compstr = "%s/%s%s.%s %s 0x%08X %s 0x%d\n" % (str(imgpath), str(comp), aestag, extname, str(listcompaddr[0]), list1addr, str(listcompaddr[2]), aesflag)
        #print( compstr )
        tbl_file.write( compstr )

    tbl_file.close()

##########################################################

hbkid_list = { 0: "cm", 1: "dm" }
load_scheme_list = { "LOAD_AND_VERIFY": 0, "VERIFY_ONLY_IN_FLASH": 1, "VERIFY_ONLY_IN_MEM": 2, "LOAD_ONLY": 3 }
aes_ce_id_list   = { "NO_IMAGE_ENC": 0, "ICV_CODE_ENC": 1, "OEM_CODE_ENC": 2 }
aes_key_list     = [ "", "../cmsecret/kceicv.txt", "../dmsecret/kce.txt" ]

def target_cfg_generate(targetdir, tpmconfig, section_name):

    if( tpmconfig.getrawconfig(section_name, 'TITLE') == '' ) :
        '''
        print('ini: section not found.');
        '''
        return
    
    index = section_list[section_name]

    hbkid = tpmconfig.gettpmhbkid_int()
    sboot = tpmconfig.gettpmsboot_int()
    nvcount = tpmconfig.gettpmnvcount()

    comp1 = tpmconfig.getrawconfig(section_name, 'COMP1')
    comp2 = tpmconfig.getrawconfig(section_name, 'COMP2')
    comp3 = tpmconfig.getrawconfig(section_name, 'COMP3')

    if( (str(comp1) == '') and (str(comp2) == '') and (str(comp3) == '') ):
        return
    
    filename =  "%s/sboot_hbk%d_%s_cert.cfg" % (targetdir, int(hbkid), tag_filename[index] )
    #print( "filename = %s" % filename )

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

    tpm_keyload = tpmconfig.getrawconfig(section_name, 'KEYLOAD')
    keyload = load_scheme_list[str(tpm_keyload)]
    targetcfg.set(targetsection, 'load-verify-scheme', str(keyload))

    tpm_keyenc = tpmconfig.getrawconfig(section_name, 'KEYENC')
    aesceid = aes_ce_id_list[str(tpm_keyenc)]
    targetcfg.set(targetsection, 'aes-ce-id', str(aesceid))

    targetcfg.set(targetsection, 'crypto-type', str(0))
        
    targetcfg.set(targetsection, 'aes-enc-key', str(aes_key_list[int(aesceid)]) )

    certpkg = "../%spublic/sboot_hbk%d_%s_cert.bin" % ( hbkid_list[int(hbkid)], int(hbkid), tag_filename[index] )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg) )


    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()

##########################################################

def target_enbcert_generate(targetdir, tpmconfig):

    hbkid = tpmconfig.gettpmhbkid_int()
    sdebug = tpmconfig.gettpmsdebug_int()
    nvcount = tpmconfig.gettpmnvcount()
    lcs = tpmconfig.gettpmlcs_int()

    filename =  "%s/sdebug_hbk%d_enabler_cert.cfg" % (targetdir, int(hbkid) )
    #print( "filename = %s" % filename )

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

    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-mask[0-31]')
    targetcfg.set(targetsection, 'debug-mask[0-31]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-mask[32-63]')
    targetcfg.set(targetsection, 'debug-mask[32-63]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-mask[64-95]')
    targetcfg.set(targetsection, 'debug-mask[64-95]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-mask[96-127]')
    targetcfg.set(targetsection, 'debug-mask[96-127]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-lock[0-31]')
    targetcfg.set(targetsection, 'debug-lock[0-31]', str('0xffffffff'))
    
    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-lock[32-63]')
    targetcfg.set(targetsection, 'debug-lock[32-63]', str('0xffffffff'))
    
    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-lock[64-95]')
    targetcfg.set(targetsection, 'debug-lock[64-95]', str('0xffffffff'))
    
    debugmask = tpmconfig.getrawconfig('ENABLER', 'debug-lock[96-127]')
    targetcfg.set(targetsection, 'debug-lock[96-127]', str('0xffffffff'))

    targetcfg.set(targetsection, 'hbk-id', str(hbkid))

    nextpubkey = "../%spubkey/%sdeveloper_pubkey.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    targetcfg.set(targetsection, 'next-cert-pubkey', str(nextpubkey))

    certpkg = "../%spublic/sdebug_hbk%d_enabler_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()

##########################################################

def target_rmaenbcert_generate(targetdir, tpmconfig):

    hbkid = tpmconfig.gettpmhbkid_int()
    sdebug = tpmconfig.gettpmsdebug_int()
    nvcount = tpmconfig.gettpmnvcount()
    lcs = tpmconfig.gettpmlcs_int()

    filename =  "%s/sdebug_hbk%d_enabler_rma_cert.cfg" % (targetdir, int(hbkid) )
    #print( "filename = %s" % filename )

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

    debugmask = tpmconfig.getrawconfig('ENABLER-RMA', 'debug-lock[0-31]')
    targetcfg.set(targetsection, 'debug-lock[0-31]', str('0xffffffff'))
    
    debugmask = tpmconfig.getrawconfig('ENABLER-RMA', 'debug-lock[32-63]')
    targetcfg.set(targetsection, 'debug-lock[32-63]', str('0xffffffff'))
    
    debugmask = tpmconfig.getrawconfig('ENABLER-RMA', 'debug-lock[64-95]')
    targetcfg.set(targetsection, 'debug-lock[64-95]', str('0xffffffff'))
    
    debugmask = tpmconfig.getrawconfig('ENABLER-RMA', 'debug-lock[96-127]')
    targetcfg.set(targetsection, 'debug-lock[96-127]', str('0xffffffff'))


    targethbk = tpmconfig.getrawconfig('ENABLER-RMA', 'hbk-id')
    targetbhkid = tpmconfig.findtpmoption_str(diasemi_hdkid_list, targethbk)

    targetcfg.set(targetsection, 'hbk-id', str(hbkid))

    nextpubkey = "../%spubkey/%sdeveloper_pubkey.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    targetcfg.set(targetsection, 'next-cert-pubkey', str(nextpubkey))

    certpkg = "../%spublic/sdebug_hbk%d_enabler_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()
    
##########################################################

def target_devcert_generate(targetdir, tpmconfig):

    hbkid = tpmconfig.gettpmhbkid_int()
    sdebug = tpmconfig.gettpmsdebug_int()
    nvcount = tpmconfig.gettpmnvcount()
    lcs = tpmconfig.gettpmlcs_int()

    filename =  "%s/sdebug_hbk%d_developer_cert.cfg" % (targetdir, int(hbkid) )
    #print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "DEVELOPER-DBG-CFG"

    targetcfg.add_section(targetsection)

    keypair = "../%ssecret/%sdeveloper_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    keypwd  = "../%ssecret/pwd.developer.txt"       % ( hbkid_list[int(hbkid)] )

    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypwd))   

    targetcfg.set(targetsection, 'soc-id', str('../image/soc_id.bin'))

    debugmask = tpmconfig.getrawconfig('DEVELOPER', 'debug-mask[0-31]')
    targetcfg.set(targetsection, 'debug-mask[0-31]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('DEVELOPER', 'debug-mask[32-63]')
    targetcfg.set(targetsection, 'debug-mask[32-63]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('DEVELOPER', 'debug-mask[64-95]')
    targetcfg.set(targetsection, 'debug-mask[64-95]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('DEVELOPER', 'debug-mask[96-127]')
    targetcfg.set(targetsection, 'debug-mask[96-127]', str(debugmask))

    enbcertpkg = "../%spublic/sdebug_hbk%d_enabler_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'enabler-cert-pkg', str(enbcertpkg))

    certpkg = "../%spublic/sdebug_hbk%d_developer_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close() 

##########################################################

def target_rmadevcert_generate(targetdir, tpmconfig):

    hbkid = tpmconfig.gettpmhbkid_int()
    sdebug = tpmconfig.gettpmsdebug_int()
    nvcount = tpmconfig.gettpmnvcount()
    lcs = tpmconfig.gettpmlcs_int()

    filename =  "%s/sdebug_hbk%d_developer_rma_cert.cfg" % (targetdir, int(hbkid) )
    #print( "filename = %s" % filename )

    targetcfg = configparser.ConfigParser()
    targetcfg.optionxform = str

    targetsection = "DEVELOPER-DBG-CFG"

    targetcfg.add_section(targetsection)

    keypair = "../%ssecret/%sdeveloper_keypair.pem" % ( hbkid_list[int(hbkid)], hbkid_list[int(hbkid)] )
    keypwd  = "../%ssecret/pwd.developer.txt"       % ( hbkid_list[int(hbkid)] )

    targetcfg.set(targetsection, 'cert-keypair', str(keypair))   
    targetcfg.set(targetsection, 'cert-keypair-pwd', str(keypwd))   

    targetcfg.set(targetsection, 'soc-id', str('../image/soc_id.bin'))

    debugmask = tpmconfig.getrawconfig('DEVELOPER-RMA', 'debug-mask[0-31]')
    targetcfg.set(targetsection, 'debug-mask[0-31]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('DEVELOPER-RMA', 'debug-mask[32-63]')
    targetcfg.set(targetsection, 'debug-mask[32-63]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('DEVELOPER-RMA', 'debug-mask[64-95]')
    targetcfg.set(targetsection, 'debug-mask[64-95]', str(debugmask))

    debugmask = tpmconfig.getrawconfig('DEVELOPER-RMA', 'debug-mask[96-127]')
    targetcfg.set(targetsection, 'debug-mask[96-127]', str(debugmask))

    enbcertpkg = "../%spublic/sdebug_hbk%d_enabler_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'enabler-cert-pkg', str(enbcertpkg))

    certpkg = "../%spublic/sdebug_hbk%d_developer_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
    targetcfg.set(targetsection, 'cert-pkg', str(certpkg))

    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()

##########################################################

def target_ini_generate(targetdir, tpmconfig, section_name):

    if( tpmconfig.getrawconfig(section_name, 'TITLE') == '' ) :
        '''
        print('ini: section not found.');
        '''
        return
    
    index = section_list[section_name]

    hbkid = tpmconfig.gettpmhbkid_int()
    sboot = tpmconfig.gettpmsboot_int()
    sdebug = tpmconfig.gettpmsdebug_int()
    nvcount = tpmconfig.gettpmnvcount()
    imgpath = tpmconfig.getconfig('image')
    lcs = tpmconfig.gettpmlcs_int()

    filename =  "%s/sboot_hbk%d_%s_cert.ini" % (targetdir, int(hbkid), tag_filename[index] )
    #print( "filename = %s" % filename )

    retmem = tpmconfig.getrawconfig(section_name, 'RETMEM')
    if( str(retmem) == '1' or str(retmem) == 'ON' or str(retmem) == 'true'):
        extname = 'rtm'
    else:
        extname = 'bin'
        
    aesmode = tpmconfig.getrawconfig(section_name, 'KEYENC')

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
    comp = tpmconfig.getrawconfig(section_name, 'COMP1')
    if( str(comp) != ''):
        swcomp = swcomp + 1
        
    comp = tpmconfig.getrawconfig(section_name, 'COMP2')
    if( str(comp) != ''):
        swcomp = swcomp + 1
        
    comp = tpmconfig.getrawconfig(section_name, 'COMP3')
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
    
    if sys.platform != 'win32' :
        cert1 = cert1.replace('\\', '/')
        cert2 = cert2.replace('\\', '/')
        cert3 = cert3.replace('\\', '/')

    targetcfg.set(targetsection, 'CERT1', str(cert1))
    targetcfg.set(targetsection, 'CERT2', str(cert2))
    targetcfg.set(targetsection, 'CERT3', str(cert3))

    title = tpmconfig.getrawconfig(section_name, 'TITLE')
    sfdp  = tpmconfig.getrawconfig(section_name, 'SFDP')

    if( (str(sfdp) != '') and (str(sfdp) != 'No-SFDP') ):
        str_title = "%s %s%s" % ( str(title), str(sfdp), str(aespost) )
        str_sfdp  = "..\\SFDP\\%s.bin" % str(sfdp)
    else:
        str_title = "%s%s" % ( str(title) , str(aespost) )
        str_sfdp  = ""

    if sys.platform != 'win32' :
        str_sfdp = str_sfdp.replace('\\', '/')

    targetcfg.set(targetsection, 'TITLE', str(str_title))
    targetcfg.set(targetsection, 'SFDP', str(str_sfdp))

    if( swcomp != 0 ):
        targetcfg.set(targetsection, 'KEYLEVEL', str(sboot))
    else:
        targetcfg.set(targetsection, 'KEYLEVEL', str(0))
    
    targetcfg.set(targetsection, 'KEYSIZE', str(3072))

    tpm_keyload = tpmconfig.getrawconfig(section_name, 'KEYLOAD')
    targetcfg.set(targetsection, 'KEYLOAD', str(tpm_keyload))
    tpm_keyenc = tpmconfig.getrawconfig(section_name, 'KEYENC')
    targetcfg.set(targetsection, 'KEYENC', str(tpm_keyenc))


    comp = tpmconfig.getrawconfig(section_name, 'COMP1')
    if( str(comp) != ''):
        if( imgpath == '' or imgpath == '../image' ):
            comstr = "..\\image\\%s%s.%s" % (str(comp), aestag, extname)
        else:
            winpath = imgpath.replace('/', '\\')
            comstr = "%s\\%s%s.%s" % (winpath, str(comp), aestag, extname)
    else:
        comstr = ""

    if sys.platform != 'win32' :
        comstr = comstr.replace('\\', '/')

    targetcfg.set(targetsection, 'COMP1', str(comstr))  
    
    comp = tpmconfig.getrawconfig(section_name, 'COMP2')
    if( str(comp) != ''):
        if( imgpath == '' or imgpath == '../image' ):
            comstr = "..\\image\\%s%s.%s" % (str(comp), aestag, extname)
        else:
            winpath = imgpath.replace('/', '\\')
            comstr = "%s\\%s%s.%s" % (winpath, str(comp), aestag, extname)
    else:
        comstr = ""

    if sys.platform != 'win32' :
        comstr = comstr.replace('\\', '/')

    targetcfg.set(targetsection, 'COMP2', str(comstr))  

    comp = tpmconfig.getrawconfig(section_name, 'COMP3')
    if( str(comp) != ''):
        if( imgpath == '' or imgpath == '../image' ):
            comstr = "..\\image\\%s%s.%s" % (str(comp), aestag, extname)
        else:
            winpath = imgpath.replace('/', '\\')
            comstr = "%s\\%s%s.%s" % (winpath, str(comp), aestag, extname)
    else:
        comstr = ""

    if sys.platform != 'win32' :
        comstr = comstr.replace('\\', '/')

    targetcfg.set(targetsection, 'COMP3', str(comstr))  

    crcon = tpmconfig.getrawconfig(section_name, 'CRCON')
    if( str(crcon) == '1' or str(crcon) == 'ON' or str(crcon) == 'true'):
        crcon = 'ON'
    else:
        crcon = 'OFF'

    targetcfg.set(targetsection, 'CRCON', str(crcon))

    if( int(hbkid) == 0 ):
        postfix = '_icv'
    else:
        postfix = ''

    dbgcert = tpmconfig.getrawconfig(section_name, 'DBGCERT')
    if( str(dbgcert) == 'RMA' or str(dbgcert) == 'rma'):
        if( lcs == 0):
            postfix =  postfix + '_lcscm'
        elif( lcs == 1):
            postfix =  postfix + '_lcsdm'
          

    imagename  = tpmconfig.getrawconfig(section_name, 'IMAGE')

    if( (str(sfdp) != '') and (str(sfdp) != 'No-SFDP') ):
        if( aesflag == 1 ):
            imagestr = "..\\public\\%s_%s_aes%s.img" % ( str(imagename), str(sfdp), str(postfix) )
        else:
            imagestr = "..\\public\\%s_%s%s.img" % ( str(imagename), str(sfdp), str(postfix) )
    else:
        if( aesflag == 1 ):
            imagestr = "..\\public\\%s_aes%s.img" % ( str(imagename), str(postfix) )
        else:
            imagestr = "..\\public\\%s%s.img" % ( str(imagename), str(postfix) )

    if sys.platform != 'win32' :
        imagestr = imagestr.replace('\\', '/')

    targetcfg.set(targetsection, 'IMAGE', str(imagestr))  

    # Offset Calc
    # retmem
    # swcomp

    flashoffset = tpmconfig.getrawconfig(section_name, 'FLASHADDR')
    if( str(flashoffset) != ''):
        intflashoffset = int(flashoffset, 16)
    else:
        intflashoffset = 0
        

    if( swcomp >= 1):
        listcom1addr = str(tpmconfig.getrawconfig(section_name, 'COMP1ADDR')).split()
    else:
        listcomp1addr = []

    #print( 'sw=%d, retmem=%d, sboot=%d %s' % (swcomp, int(retmem), int(sboot), str(listcom1addr[0])));        
    if( (swcomp == 1) and (int(retmem) == 0) ):
        hex_offset =  str(listcom1addr[0])
    elif( (swcomp == 1) and (int(retmem) == 1) ): #elif( (swcomp == 1) and (int(retmem) == 1) and (int(sboot) == 0 ) ):
        hex_offset =  str(listcom1addr[0])
    elif( (swcomp > 1) and (int(retmem) == 1) ): #elif( (swcomp == 1) and (int(retmem) == 1) and (int(sboot) == 0 ) ):
        hex_offset =  str(listcom1addr[0]) # compatibility issue between OTA flash_image_test( )
    else:
        hex_offset = ''

    if( (swcomp == 1) and (int(retmem) == 0) and (int(listcom1addr[0], 16) != 0xffffffff) and (int(listcom1addr[1], 16) != 0xffffffff) ):
        #sram
        hex_comp1addr = str(listcom1addr[1])
        hex_version   = str(listcom1addr[1])
        #hexformat = "%08x" % (intflashoffset + int(str(listcom1addr[1]), 16))
        #hex_version   = str(hexformat)
    elif( (swcomp == 1) and (int(retmem) == 1) and (int(listcom1addr[0], 16) != 0xffffffff) and (int(listcom1addr[1], 16) != 0xffffffff) ): #elif( (swcomp == 1) and (int(retmem) == 1) and (int(sboot) == 0 ) and (int(listcom1addr[0], 16) != 0xffffffff) and (int(listcom1addr[1], 16) != 0xffffffff) ):
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
        listcom2addr = str(tpmconfig.getrawconfig(section_name, 'COMP2ADDR')).split()
        hex_comp2addr = str(listcom2addr[1])
    else:
        hex_comp2addr = ''
        
    if( swcomp >= 3 ):
        listcom3addr = str(tpmconfig.getrawconfig(section_name, 'COMP3ADDR')).split()
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

    sbox = tpmconfig.getrawconfig(section_name, 'SBOX')

    dbgcert = tpmconfig.getrawconfig(section_name, 'DBGCERT')
    if( str(dbgcert) == '1' or str(dbgcert) == 'ON' or str(dbgcert) == 'true'):
        dbgcert = 'ON'
    elif( str(dbgcert) == 'RMA' or str(dbgcert) == 'rma'):
        dbgcert = 'RMA'
    else:
        dbgcert = 'OFF'

    if( int(sbox, 16) == 0x00000000 ):
        if( str(dbgcert) == 'ON' ):
            str_dbgcert = "..\\%spublic\\sdebug_hbk%d_developer_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
            if( int(sdebug) == 2 ):
                #str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_2lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
                str_dbgcertk = ''
            else:
                str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_3lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
        elif( str(dbgcert) == 'RMA' ):
            str_dbgcert = "..\\%spublic\\sdebug_hbk%d_developer_rma_pkg.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
            if( int(sdebug) == 2 ):
                #str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_2lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
                str_dbgcertk = ''
            else:
                str_dbgcertk = "..\\%spublic\\sdebug_hbk%d_3lvl_key_chain_enabler.bin" % (hbkid_list[int(hbkid)], int(hbkid))
        else:
            str_dbgcert = ''
            str_dbgcertk = ''
    else:
        str_dbgcert = "..\\%spublic\\sdebug_hbk%d_developer_sbox.bin" % ( hbkid_list[int(hbkid)], int(hbkid) )
        str_dbgcertk = ''

    if sys.platform != 'win32' :
        str_dbgcert = str_dbgcert.replace('\\', '/')
        str_dbgcertk = str_dbgcertk.replace('\\', '/')

    targetcfg.set(targetsection, 'DBGCERT', str(str_dbgcert))
    targetcfg.set(targetsection, 'DBGCERTK', str(str_dbgcertk))

    #TODO
    targetcfg.set(targetsection, 'VERSION', str(hex_version))  


    config_file = open(filename, 'w')
    targetcfg.write(config_file)
    config_file.close()

##########################################################

ttlscript = [
    ':download_%s\n',
    '    getdir srcfile\n',
    '    strconcat srcfile \'\\\\\'\n',
    '    strconcat srcfile \"%s\"\n',
    '    send \"loady %x 1000\" #13#10\n',
    '    waitln \"Load Y-Modem\"\n',
    '    mpause 400\n',
    '    ymodemsend srcfile\n',
    '    mpause 4000\n',
    '    send  #13#10\n',
    '    waitln \"[MROM]\"\n'
]

ttdbglscript = [
    ':download_%s\n',
    '    getdir srcfile\n',
    '    strconcat srcfile \'\\\\\'\n',
    '    strconcat srcfile \"%s\"\n',
    '    send \"loady %x 1000 bin\" #13#10\n',
    '    waitln \"Load Y-Modem\"\n',
    '    mpause 400\n',
    '    ymodemsend srcfile\n',
    '    mpause 4000\n',
    '    send  #13#10\n',
    '    waitln \"[MROM]\"\n'
]

def target_script_generate(targetdir, tpmconfig, section_name):

    if( tpmconfig.getrawconfig(section_name, 'TITLE') == '' ) :
        '''
        print('script: section not found.');
        '''
        return ''
		
    hbkid = tpmconfig.gettpmhbkid_int()

    aesmode = tpmconfig.getrawconfig(section_name, 'KEYENC')

    if( (str(aesmode) == 'ICV_CODE_ENC') or (str(aesmode) == 'OEM_CODE_ENC') ):
        aesflag = 1
    else:
        aesflag = 0

    if( int(hbkid) == 0 ):
        postfix = '_icv'
    else:
        postfix = ''

    imagename  = tpmconfig.getrawconfig(section_name, 'IMAGE')
    sfdp       = tpmconfig.getrawconfig(section_name, 'SFDP')
    dbgcert    = tpmconfig.getrawconfig(section_name, 'DBGCERT')
    sbox       = tpmconfig.getrawconfig(section_name, 'SBOX')
    

    if( (str(sfdp) != '') and (str(sfdp) != 'No-SFDP') ):
        if( aesflag == 1 ):
            imagestr = "%s_%s_aes%s.img" % ( str(imagename), str(sfdp), str(postfix) )
        else:
            imagestr = "%s_%s%s.img" % ( str(imagename), str(sfdp), str(postfix) )
    else:
        if( aesflag == 1 ):
            imagestr = "%s_aes%s.img" % ( str(imagename), str(postfix) )
        else:
            imagestr = "%s%s.img" % ( str(imagename), str(postfix) )


    flashoffset = tpmconfig.getrawconfig(section_name, 'FLASHADDR')
    if( str(flashoffset) != ''):
        intflashoffset = int(flashoffset, 16)
    else:
        intflashoffset = 0
        
    #TODO
    out = ''
    
    for idx, ttline in enumerate(ttlscript):
        if( idx == 0 ):
            out = out + (ttline % str(imagename))
        elif( idx == 3 ):
            out = out + (ttline % str(imagestr))
        elif( idx == 4 ):
            out = out + (ttline % intflashoffset)
        else:
            out = out + ttline

    if( (str(dbgcert) != 'OFF') and (int(sbox, 16) != 0 ) ):
        imagename = ('sdebug_hbk%d_developer_certificate' % hbkid)
        imagestr  = ('sdebug_hbk%d_developer_certificate.bin' % hbkid)
        intflashoffset = int(sbox, 16)
        for idx, ttline in enumerate(ttdbglscript):
            if( idx == 0 ):
                out = out + (ttline % str(imagename))
            elif( idx == 3 ):
                out = out + (ttline % str(imagestr))
            elif( idx == 4 ):
                out = out + (ttline % intflashoffset)
            else:
                out = out + ttline
    

    return out
                          
##########################################################

def modestr_checker(modestr, filterstr):
    modeitems = modestr.split(':')
    filteritems = filterstr.split(':')

    for mode in modeitems:
        for filter in filteritems:
            if( mode == filter ):
                return True

    return False


def target_config_update(config_fname, targetdir, modestr):

        manobj = manconfig(config_fname)
        manobj.parse()

        hbkid = manobj.gettpmhbkid_int()
        sboot = manobj.gettpmsboot_int()
        sdebug = manobj.gettpmsdebug_int()
        bootmodel = manobj.gettpmbootmodel_int()
        combined = manobj.gettpmimagemodel_int()

        if( modestr_checker(modestr, 'ALL:DBG:SDBG') == True):
            target_enbcert_generate(targetdir, manobj)
            target_devcert_generate(targetdir, manobj)
        
        if( modestr_checker(modestr, 'ALL:RMA:SDBG') == True):
            target_rmaenbcert_generate(targetdir, manobj)
            target_rmadevcert_generate(targetdir, manobj)

        if( modestr_checker(modestr, 'ALL:BOOT:SBOOT') == True):
            if( int(bootmodel) == 0 ): 
                
                if( int(sboot) != 0 ):
                    target_tbl_generate(targetdir, manobj, "UEBOOT")
                    target_cfg_generate(targetdir, manobj, "UEBOOT")
                target_ini_generate(targetdir, manobj, "UEBOOT")
                
                if( int(sboot) != 0 ):
                    target_tbl_generate(targetdir, manobj, "RTOS.cache")
                    target_cfg_generate(targetdir, manobj, "RTOS.cache")
                target_ini_generate(targetdir, manobj, "RTOS.cache")
            else:
                
                if( int(sboot) != 0 ):
                    target_tbl_generate(targetdir, manobj, "RTOS.sram")
                    target_cfg_generate(targetdir, manobj, "RTOS.sram")
                target_ini_generate(targetdir, manobj, "RTOS.sram")


            if( int(combined) == 1 ): 
                
                if( int(sboot) != 0 ):
                    target_tbl_generate(targetdir, manobj, "Comp.combined")
                    target_cfg_generate(targetdir, manobj, "Comp.combined")
                target_ini_generate(targetdir, manobj, "Comp.combined")
            else:
                
                if( int(sboot) != 0 ):
                    target_tbl_generate(targetdir, manobj, "Comp.PTIM")
                    target_cfg_generate(targetdir, manobj, "Comp.PTIM")
                target_ini_generate(targetdir, manobj, "Comp.PTIM")
                
                if( int(sboot) != 0 ):
                    target_tbl_generate(targetdir, manobj, "Comp.RaLIB")
                    target_cfg_generate(targetdir, manobj, "Comp.RaLIB")
                target_ini_generate(targetdir, manobj, "Comp.RaLIB")

        if( modestr_checker(modestr, 'ALL:RMA:SDBG') == True):
            target_tbl_generate(targetdir, manobj, "RMA")
            target_cfg_generate(targetdir, manobj, "RMA")
            target_ini_generate(targetdir, manobj, "RMA")        

##########################################################

def target_script_update(config_fname, targetdir, modestr):

    manobj = manconfig(config_fname)
    manobj.parse()

    hbkid = manobj.gettpmhbkid_int()
    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    bootmodel = manobj.gettpmbootmodel_int()
    combined = manobj.gettpmimagemodel_int()

    out = ''
        
    if( modestr_checker(modestr, 'ALL:BOOT:SBOOT') == True):
        if( int(bootmodel) == 0 ): 
            out = out + target_script_generate(targetdir, manobj, "UEBOOT")
            out = out + target_script_generate(targetdir, manobj, "RTOS.cache")
        else:
            out = out + target_script_generate(targetdir, manobj, "RTOS.sram")

        if( int(combined) == 1 ): 
            out = out + target_script_generate(targetdir, manobj, "Comp.combined")
        else:
            out = out + target_script_generate(targetdir, manobj, "Comp.PTIM")
            out = out + target_script_generate(targetdir, manobj, "Comp.RaLIB")

    if( modestr_checker(modestr, 'RMA') == True):
        out = out + target_script_generate(targetdir, manobj, "RMA")

    return out

##########################################################
