#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct
import subprocess
import re

from fc9ksecuutil import *

##########################################################   

removeallfolders = [
    'cmpublic', 'cmtpmcfg', 'dmpublic', 'dmtpmcfg', 'public'
]

removefilterfolders = [
    ['example', '.log'],
    ['cmpubkey', '.pem'],
    ['cmsecret', '.pem'], ['cmsecret', '.bin'], ['cmsecret', '.txt'], 
    ['dmpubkey', '.pem'],
    ['dmsecret', '.pem'], ['dmsecret', '.bin'], ['dmsecret', '.txt'],
    ['image'   ,  'enc.bin']
]

def step_1_0_clean_all(pathname, cfgfile, cleanoption):

    title = '[+] clean_all'
    out = b'\n'
    err = b'\n'

    for tfolder in removeallfolders:
        if os.path.isdir(tfolder) == False:
            out = out + ('\nmkdir %s' % tfolder).encode()
            os.mkdir(tfoler)
        else:
            for tfile in os.listdir(tfolder):
                if os.path.isdir(os.path.join(tfolder, tfile)) == False:
                    out = out + ('\nremove %s' % os.path.join(tfolder, tfile)).encode()
                    os.remove(os.path.join(tfolder, tfile))

    for titems in removefilterfolders:
        if os.path.isdir(titems[0]) == False:
            out = out + ('\nmkdir %s' % titems[0]).encode()
            os.mkdir(titems[0])
        else:
            filelist = [ f for f in os.listdir(titems[0]) if f.endswith(titems[1]) ]
            for tfile in filelist:
                if str(cleanoption) == 'YesToAll':                
                    out = out + ('\nremove %s' % os.path.join(titems[0], tfile)).encode()
                    os.remove(os.path.join(titems[0], tfile))
    
    out = out + b'\n'
    return title, out, err

##########################################################

cmpasswordlist = [
    './cmsecret/pwd.cmkey.txt', 
    './cmsecret/pwd.kceicv.txt', 
    './cmsecret/pwd.kpicv.txt', 
    './cmsecret/pwd.krtl.txt'
    ]

cmkeylist = [
    './cmsecret/pwd.cmkey.txt',
    './cmsecret/cmkey_pair.pem',
    './cmpubkey/cmpubkey.pem'
    ]

cmotpkeylist = [
    ['./cmsecret/pwd.kceicv.txt', './cmsecret/kceicv.bin', './cmpublic/enc.kceicv.bin'],
    ['./cmsecret/pwd.kpicv.txt' , './cmsecret/kpicv.bin' , './cmpublic/enc.kpicv.bin' ]
    ]

cmkrtlkeylist = [
    './cmsecret/pwd.krtl.txt' , './cmsecret/krtl.key' , './cmsecret/enc.krtl.bin'
    ]

cmhbklist = [
    '../cmpubkey/cmpubkey.pem',
    '../example/cmpubkey_hash.txt',
    '../example/cmpubkey_zero_bits_in_hash.txt',
    '../example/gen_hbk0_log.log',
    '../public/hbk0.bin'
    ]

cmkeyassetlist = [
    [ '../cmconfig/asset_icv_ce.cfg', '../example/asset_icv_ce.log' ],
    [ '../cmconfig/asset_icv_cp.cfg', '../example/asset_icv_cp.log' ]
    ]

cmpucfgfile = '../cmconfig/cmpu.cfg'

def step_1_1_build_cmpu(pathname, cfgfile, cleanoption):
    retval = True
    
    hbk_gen_util   = 'hbk_gen_util.py'
    assetpkgutil   = 'cmpu_asset_pkg_util.py'
    fc9kcmpupkggen = 'fc9kcmpupkggen.py'

    title = '[+] build_cmpu'
    out = b'\n'
    err = b'\n'
    
    if( (str(cleanoption) != 'YesToAll') and (str(cleanoption) != 'SkipCMPU') ):
        return title, out, err
                    
    for passwd in cmpasswordlist:
        if( genpassword(passwd) == False):
            out = out + ('\ngen passwd (%s) is error' % passwd).encode()
            retval = False
        else:
            out = out + ('\ngen passwd (%s)' % passwd).encode()
            
        if str(cleanoption) == 'SkipCMPU':
            break

    rval, rout, rerr = genkeyfiles(cmkeylist[0], cmkeylist[1], cmkeylist[2])
    out = out + rout
    err = err + rerr
    out = out + ('\ngen cmkey (%s)' % cmkeylist[1]).encode()
    if( rval == False):
        retval = False

    if retval == True and str(cleanoption) != 'SkipCMPU':
        for otpkeys in cmotpkeylist:
            rval = genotpkeyfile(otpkeys[0], otpkeys[1], otpkeys[2])
           
            if( rval == False):
                out = out + ( '\ngen OTP Key, %s is error' % otpkeys[1] ).encode()
                retval = False
            else:
                out = out + ( '\ngen OTP Key, %s' % otpkeys[1]).encode()

        if( enckrtlkeyfile(cmkrtlkeylist[0], cmkrtlkeylist[1], cmkrtlkeylist[2]) == False):
            out = out + ( '\nenc KRTL Key, %s is error' % cmkrtlkeylist[1] ).encode()
            retval = False
        else:
            out = out + ('\nenc KRTL Key, %s' % cmkrtlkeylist[2]).encode()

    # get HBK

    if( retval == True ):
        retcode, pout, perr = genHBK(pathname, hbk_gen_util, cmhbklist[0], cmhbklist[1], cmhbklist[2], cmhbklist[3], cmhbklist[4])
        if( retcode == False):
            out = out + ( '\ngen HBK, %s is error\n\t' % cmhbklist[4] ).encode()
            out = out + pout
            err = err + perr
            retval = False
        else:
            out = out + ('\ngen HBK, %s' % cmhbklist[4]).encode()

    # update asset cfgfile?

    if retval == True and str(cleanoption) != 'SkipCMPU':
        for keyassets in cmkeyassetlist:
            if( genKeyAsset(pathname, assetpkgutil, keyassets[0], keyassets[1]) == False):
                out = out + ( '\ngen Asset, %s is error' % keyassets[0] ).encode()
                retval = False
            else:
                out = out + ('\ngen Asset (%s)' % keyassets[0]).encode()

    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cmpucfgfile)
    del nvupdater

    if retval == True and str(cleanoption) == 'SkipCMPU':
        out = out + b'\n'
        return title, out, err

    retval, retout, reterr = genCMPU(pathname, fc9kcmpupkggen, cmpucfgfile)
    if( retval == False):
        out = out + ('\ngen CMPU (%s) is error' % cmpucfgfile).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen CMPU (%s)' % cmpucfgfile).encode()

    out = out + b'\n'
    return title, out, err

##########################################################

def step_1_1_build_cmpu_cleanup(pathname, cfgfile, cleanoption):
    retval = True

    title = '[+] cleanup_cmpu_materials'
    out = b'\n'
    err = b'\n'

    cwd = os.getcwd()
    os.chdir(pathname)
    
    if( os.path.isfile(cmpucfgfile) == True):
        config = configparser.ConfigParser()
        config.optionxform = str        
        fobj = open(cmpucfgfile, 'r')
        config.readfp(fobj)
        fobj.close()

        sectionname = 'CMPU-PKG-CFG'

        if( config.has_section(sectionname) == True):
            binfilename =  config.get(sectionname, 'hbk-data')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

            binfilename =  config.get(sectionname, 'kpicv-pkg')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

            binfilename =  config.get(sectionname, 'kceicv-pkg')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

        del config

        os.chdir(cwd)

    
    out = out + b'\n'
    return title, out, err

    
##########################################################    

cmissuerkeylist = [
    './cmsecret/pwd.issuer.txt',
    './cmsecret/cmissuer_keypair.pem',
    './cmpubkey/cmissuer_pubkey.pem'
    ]

def step_1_B_4_keygen_issuer(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sboot_keygen_isu'
    out = b'\n'
    err = b'\n'
    
    if( genpassword(cmissuerkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % cmissuerkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % cmissuerkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(cmissuerkeylist[0], cmissuerkeylist[1], cmissuerkeylist[2])
    out = out + rout
    err = err + rerr    
    out = out + ('\ngen issuer key (%s)' % cmissuerkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

cmpublisherkeylist = [
    './cmsecret/pwd.publisher.txt',
    './cmsecret/cmpublisher_keypair.pem',
    './cmpubkey/cmpublisher_pubkey.pem'
    ]

def step_1_B_5_keygen_publisher(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sboot_keygen_pub'
    out = b'\n'
    err = b'\n'
    
    if( genpassword(cmpublisherkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % cmpublisherkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % cmpublisherkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(cmpublisherkeylist[0], cmpublisherkeylist[1], cmpublisherkeylist[2])
    out = out + rout
    err = err + rerr  
    out = out + ('\ngen publisher key (%s)' % cmpublisherkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

cm2lvlsbootkeychainlist = [
    '../cmconfig/sboot_hbk0_2lvl_key_chain_publisher.cfg',
    '../example/sboot_hbk0_2lvl_key_chain_publisher.log'
    ]

def step_1_B_6_keychain_2lvl(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sboot_chain_2lvl'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cm2lvlsbootkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, cm2lvlsbootkeychainlist[0], cm2lvlsbootkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % cm2lvlsbootkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % cm2lvlsbootkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

cm3lvlsbootisukeychainlist = [
    '../cmconfig/sboot_hbk0_3lvl_key_chain_issuer.cfg',
    '../example/sboot_hbk0_3lvl_key_chain_issuer.log'
    ]

def step_1_B_7_keychain_3lvl_isu(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sboot_chain_3lvl_isu'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cm3lvlsbootisukeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, cm3lvlsbootisukeychainlist[0], cm3lvlsbootisukeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % cm3lvlsbootisukeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % cm3lvlsbootisukeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

cm3lvlsbootpubkeychainlist = [
    '../cmconfig/sboot_hbk0_3lvl_key_chain_publisher.cfg',
    '../example/sboot_hbk0_3lvl_key_chain_publisher.log'
    ]

def step_1_B_8_keychain_3lvl_pub(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sboot_chain_3lvl_pub'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cm3lvlsbootpubkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, cm3lvlsbootpubkeychainlist[0], cm3lvlsbootpubkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % cm3lvlsbootpubkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % cm3lvlsbootpubkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

cmenablerkeylist = [
    './cmsecret/pwd.enabler.txt',
    './cmsecret/cmenabler_keypair.pem',
    './cmpubkey/cmenabler_pubkey.pem'
    ]

def step_1_D_4_keygen_enabler(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sdbg_keygen_enb'
    out = b'\n'
    err = b'\n'
    
    if( genpassword(cmenablerkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % cmenablerkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % cmenablerkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(cmenablerkeylist[0], cmenablerkeylist[1], cmenablerkeylist[2])
    out = out + rout
    err = err + rerr  
    out = out + ('\ngen enabler key (%s)' % cmenablerkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

cmdeveloperkeylist = [
    './cmsecret/pwd.developer.txt',
    './cmsecret/cmdeveloper_keypair.pem',
    './cmpubkey/cmdeveloper_pubkey.pem'
    ]

def step_1_D_5_keygen_developer(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sdbg_keygen_dev'
    out = b'\n'
    err = b'\n'
    
    if( genpassword(cmdeveloperkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % cmdeveloperkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % cmdeveloperkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(cmdeveloperkeylist[0], cmdeveloperkeylist[1], cmdeveloperkeylist[2])
    out = out + rout
    err = err + rerr 
    out = out + ('\ngen developer key (%s)' % cmdeveloperkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

cm2lvlsdbgkeychainlist = [
    '../cmconfig/sdebug_hbk0_2lvl_key_chain_developer.cfg',
    '../example/sdebug_hbk0_2lvl_key_chain_developer.log'
    ]

def step_1_D_6_keychain_2lvl(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sdbg_chain_2lvl'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cm2lvlsdbgkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, cm2lvlsdbgkeychainlist[0], cm2lvlsdbgkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % cm2lvlsdbgkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % cm2lvlsdbgkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

cm3lvlsdbgenbkeychainlist = [
    '../cmconfig/sdebug_hbk0_3lvl_key_chain_enabler.cfg',
    '../example/sdebug_hbk0_3lvl_key_chain_enabler.log'
    ]

def step_1_D_7_keychain_3lvl_enb(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sdbg_chain_3lvl_enb'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cm3lvlsdbgenbkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, cm3lvlsdbgenbkeychainlist[0], cm3lvlsdbgenbkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % cm3lvlsdbgenbkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % cm3lvlsdbgenbkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

cm3lvlsdbgdevkeychainlist = [
    '../cmconfig/sdebug_hbk0_3lvl_key_chain_developer.cfg',
    '../example/sdebug_hbk0_3lvl_key_chain_developer.log'
    ]

def step_1_D_8_keychain_3lvl_dev(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sdbg_chain_3lvl_dev'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(cm3lvlsdbgdevkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, cm3lvlsdbgdevkeychainlist[0], cm3lvlsdbgdevkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % cm3lvlsdbgdevkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % cm3lvlsdbgdevkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

dmremoveallfolders = [
    'dmpublic', 'dmtpmcfg'   ##, 'example', 'public'
]

dmremovefilterfolders = [
    ['dmpubkey', '.pem'],
    ['dmsecret', '.pem'], ['dmsecret', '.bin'], ['dmsecret', '.txt'],
    ['image'   ,  'enc.bin']
]

def step_2_0_clean_all(pathname, cfgfile, cleanoption):

    title = '[+] clean_all'
    out = b'\n'
    err = b'\n'

    for tfolder in dmremoveallfolders:
        if os.path.isdir(tfolder) == False:
            out = out + ('mkdir %s\n' % tfolder).encode()
            os.mkdir(tfoler)
        else:
            for tfile in os.listdir(tfolder):
                if os.path.isdir(os.path.join(tfolder, tfile)) == False:
                    out = out + ('remove %s\n' % os.path.join(tfolder, tfile)).encode()
                    os.remove(os.path.join(tfolder, tfile))

    for titems in dmremovefilterfolders:
        if os.path.isdir(titems[0]) == False:
            out = out + ('mkdir %s\n' % titems[0]).encode()
            os.mkdir(titems[0])
        else:
            filelist = [ f for f in os.listdir(titems[0]) if f.endswith(titems[1]) ]
            for tfile in filelist:
                if str(cleanoption) == 'YesToAll':                
                    out = out + ('remove %s\n' % os.path.join(titems[0], tfile)).encode()
                    os.remove(os.path.join(titems[0], tfile))
    
    out = out + b'\n'
    return title, out, err

##########################################################    

dmpasswordlist = [
    './dmsecret/pwd.dmkey.txt', 
    './dmsecret/pwd.kce.txt', 
    './dmsecret/pwd.kcp.txt'
]

dmkeylist = [
    './dmsecret/pwd.dmkey.txt',
    './dmsecret/dmkey_pair.pem',
    './dmpubkey/dmpubkey.pem'
]

dmotpkeylist = [
    ['./dmsecret/pwd.kce.txt', './dmsecret/kce.bin', './dmpublic/enc.kce.bin' ],
    ['./dmsecret/pwd.kcp.txt', './dmsecret/kcp.bin', './dmpublic/enc.kcp.bin' ]
]

dmhbklist = [
    '../dmpubkey/dmpubkey.pem',
    '../example/dmpubkey_hash.txt',
    '../example/dmpubkey_zero_bits_in_hash.txt',
    '../example/gen_hbk1_log.log',
    '../public/hbk1.bin'
    ]

def step_2_1_build_hbkgt(pathname, cfgfile, cleanoption):
    retval = True

    hbk_gen_util   = 'hbk_gen_util.py'

    title = '[+] build_hbkgt'
    out = b'\n'
    err = b'\n'
    
    if( (str(cleanoption) != 'YesToAll') and (str(cleanoption) != 'SkipDMPU') ):
        return title, out, err
                    
    for passwd in dmpasswordlist:
        if( genpassword(passwd) == False):
            out = out + ('\ngen passwd (%s) is error' % passwd).encode()
            retval = False
        else:
            out = out + ('\ngen passwd (%s)' % passwd).encode()
            
        if str(cleanoption) == 'SkipDMPU':
            break
        
    rval, rout, rerr = genkeyfiles(dmkeylist[0], dmkeylist[1], dmkeylist[2])
    out = out + rout
    err = err + rerr 
    out = out + ('\ngen dmkey (%s)' % dmkeylist[1]).encode()
    if( rval == False):
        retval = False

    if str(cleanoption) != 'SkipDMPU':
        for otpkeys in dmotpkeylist:
            rval = genotpkeyfile(otpkeys[0], otpkeys[1], otpkeys[2])
           
            if( rval == False):
                out = out + ( '\ngen OTP Key, %s is error' % otpkeys[1] ).encode()
                retval = False
            else:
                out = out + ( '\ngen OTP Key, %s' % otpkeys[1] ).encode()

    # gen HBK
    
    retcode, pout, perr = genHBK(pathname, hbk_gen_util, dmhbklist[0], dmhbklist[1], dmhbklist[2], dmhbklist[3], dmhbklist[4])
    if( retcode == False):
        out = out + ( '\ngen HBK, %s is error\n\t' % dmhbklist[4] ).encode()
        out = out + pout
        err = err + perr
        retval = False
    else:
        out = out + ( '\ngen HBK, %s' % dmhbklist[4] ).encode()
    
    out = out + b'\n'
    return title, out, err

##########################################################

dmpwokptlist = './dmsecret/pwd.oem_keyreq.txt'

dmokptkeylist = [
    './dmsecret/pwd.oem_keyreq.txt',
    './dmsecret/oem_keyreq_key_pair.pem',
    './dmpubkey/oem_keyreq_pubkey.pem'
]

dmokptpkglist = [
    '../dmconfig/dmpu_oem_key_request.cfg',
    '../example/dmpu_oem_key_request.log'
]

def step_2_2_build_okrpt(pathname, cfgfile, cleanoption):
    retval = True

    okrptutil      = 'dmpu_oem_key_request_util.py'

    title = '[+] build_okrgt'
    out = b'\n'
    err = b'\n'
    
    if( genpassword(dmpwokptlist) == False):
        out = out + ('\ngen passwd (%s) is error' % dmpwokptlist).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % dmpwokptlist).encode()
            
    rval, rout, rerr = genkeyfiles(dmokptkeylist[0], dmokptkeylist[1], dmokptkeylist[2])
    out = out + rout
    err = err + rerr 
    out = out + ('\ngen dmokptkey (%s)' % dmokptkeylist[1]).encode()
    if( rval == False):
        retval = False

    retval, retout, reterr = genOEMKeyReqPkg(pathname, okrptutil, dmokptpkglist[0], dmokptpkglist[1])
    if( retval == False):
        out = out + ( "\nOEMKeyReq Package is error" ).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ( "\nOEMKeyReq Package (%s)" % dmokptpkglist[0] ).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

dmikrptpkglist = [
    '../cmconfig/dmpu_icv_key_response.cfg',
    '../example/dmpu_icv_key_response.log'
]

def step_2_3_build_ikrpt(pathname, cfgfile, cleanoption):
    retval = True

    ikrptutil      = 'dmpu_icv_key_response_util.py'

    title = '[+] build_ikrpt'
    out = b'\n'
    err = b'\n'
    
    if( genICVKeyResPkg(pathname, ikrptutil, dmikrptpkglist[0], dmikrptpkglist[1]) == False):
        out = out + ( "\nICVKeyRes Package is error" ).encode()
        retval = False
    else:
        out = out + ( "\nICVKeyRes Package (%s)" % dmikrptpkglist[0] ).encode()

    out = out + b'\n'
    return title, out, err

##########################################################

dmkeyassetlist = [
    [ '../dmconfig/asset_oem_ce.cfg', '../example/asset_oem_ce.log' ],
    [ '../dmconfig/asset_oem_cp.cfg', '../example/asset_oem_cp.log' ]
]

dmpucfgfile = '../dmconfig/dmpu.cfg'

def step_2_4_build_dmpu(pathname, cfgfile, cleanoption):
    retval = True

    oapptutil       = 'dmpu_oem_asset_pkg_util.py'
    fc9kdmpupkggen  = 'fc9kdmpupkggen.py'

    title = '[+] build_dmpu'
    out = b'\n'
    err = b'\n'
    
    for keyassets in dmkeyassetlist:
        if( genOEMKeyAsset(pathname, oapptutil, keyassets[0], keyassets[1]) == False):
            out = out + ( "\ngenOEMKeyAsset is error" ).encode()
            retval = False
        else:
            out = out + ( "\ngenOEMKeyAsset (%s)" % keyassets[0] ).encode()


    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dmpucfgfile)

    retval, retout, reterr = genDMPU(pathname, fc9kdmpupkggen, dmpucfgfile)
    if( retval == False):
        out = out + ( "\ngenDMPU is error" ).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ( "\ngenDMPU (%s)" % dmpucfgfile).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err


##########################################################

def step_2_4_build_dmpu_cleanup(pathname, cfgfile, cleanoption):
    retval = True

    title = '[+] cleanup_dmpu_materials'
    out = b'\n'
    err = b'\n'

    cwd = os.getcwd()
    os.chdir(pathname)
    
    if( os.path.isfile(dmpucfgfile) == True):
        config = configparser.ConfigParser()
        config.optionxform = str        
        fobj = open(dmpucfgfile, 'r')
        config.readfp(fobj)
        fobj.close()

        sectionname = 'DMPU-PKG-CFG'

        if( config.has_section(sectionname) == True):
            binfilename =  config.get(sectionname, 'hbk-data')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

            binfilename =  config.get(sectionname, 'kcp-pkg')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

            binfilename =  config.get(sectionname, 'kce-pkg')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

        del config

    if( os.path.isfile(dmikrptpkglist[0]) == True):
        config = configparser.ConfigParser()
        config.optionxform = str        
        fobj = open(dmikrptpkglist[0], 'r')
        config.readfp(fobj)
        fobj.close()
        
        sectionname = 'DMPU-ICV-KEY-RES-CFG'

        if( config.has_section(sectionname) == True):
            binfilename =  config.get(sectionname, 'oem-cert-pkg')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

            binfilename =  config.get(sectionname, 'icv-enc-oem-key')
            out = out + ('\n > remove %s' % binfilename).encode()
            if( os.path.isfile(binfilename) == True):
                os.remove(binfilename)

        del config
    
    os.chdir(cwd)

    
    out = out + b'\n'
    return title, out, err

##########################################################    

dmissuerkeylist = [
    './dmsecret/pwd.issuer.txt',
    './dmsecret/dmissuer_keypair.pem',
    './dmpubkey/dmissuer_pubkey.pem'
    ]

def step_2_B_4_keygen_issuer(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sboot_keygen_isu'
    out = b'\n'
    err = b'\n'    

    if( genpassword(dmissuerkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % dmissuerkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % dmissuerkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(dmissuerkeylist[0], dmissuerkeylist[1], dmissuerkeylist[2])
    out = out + rout
    err = err + rerr 
    out = out + ('\ngen issuer key (%s)' % dmissuerkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

dmpublisherkeylist = [
    './dmsecret/pwd.publisher.txt',
    './dmsecret/dmpublisher_keypair.pem',
    './dmpubkey/dmpublisher_pubkey.pem'
    ]

def step_2_B_5_keygen_publisher(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sboot_keygen_pub'
    out = b'\n'
    err = b'\n'
    
    if( genpassword(dmpublisherkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % dmpublisherkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % dmpublisherkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(dmpublisherkeylist[0], dmpublisherkeylist[1], dmpublisherkeylist[2])
    out = out + rout
    err = err + rerr
    out = out + ('\ngen publisher key (%s)' % dmpublisherkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

dm2lvlsbootkeychainlist = [
    '../dmconfig/sboot_hbk1_2lvl_key_chain_publisher.cfg',
    '../example/sboot_hbk1_2lvl_key_chain_publisher.log'
    ]

def step_2_B_6_keychain_2lvl(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sboot_chain_2lvl'
    out = b'\n'
    err = b'\n'
    
    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dm2lvlsbootkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, dm2lvlsbootkeychainlist[0], dm2lvlsbootkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % dm2lvlsbootkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % dm2lvlsbootkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

dm3lvlsbootisukeychainlist = [
    '../dmconfig/sboot_hbk1_3lvl_key_chain_issuer.cfg',
    '../example/sboot_hbk1_3lvl_key_chain_issuer.log'
    ]

def step_2_B_7_keychain_3lvl_isu(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sboot_chain_3lvl_isu'
    out = b'\n'
    err = b'\n'

    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dm3lvlsbootisukeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, dm3lvlsbootisukeychainlist[0], dm3lvlsbootisukeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % dm3lvlsbootisukeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % dm3lvlsbootisukeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

dm3lvlsbootpubkeychainlist = [
    '../dmconfig/sboot_hbk1_3lvl_key_chain_publisher.cfg',
    '../example/sboot_hbk1_3lvl_key_chain_publisher.log'
    ]

def step_2_B_8_keychain_3lvl_pub(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sboot_chain_3lvl_pub'
    out = b'\n'
    err = b'\n'

    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dm3lvlsbootpubkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, dm3lvlsbootpubkeychainlist[0], dm3lvlsbootpubkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
 
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % dm3lvlsbootpubkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % dm3lvlsbootpubkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

dmenablerkeylist = [
    './dmsecret/pwd.enabler.txt',
    './dmsecret/dmenabler_keypair.pem',
    './dmpubkey/dmenabler_pubkey.pem'
    ]

def step_2_D_4_keygen_enabler(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sdbg_keygen_enb'
    out = b'\n'
    err = b'\n'

    if( genpassword(dmenablerkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % dmenablerkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % dmenablerkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(dmenablerkeylist[0], dmenablerkeylist[1], dmenablerkeylist[2])
    out = out + rout
    err = err + rerr
    out = out + ('\ngen enabler key (%s)' % dmenablerkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

dmdeveloperkeylist = [
    './dmsecret/pwd.developer.txt',
    './dmsecret/dmdeveloper_keypair.pem',
    './dmpubkey/dmdeveloper_pubkey.pem'
    ]

def step_2_D_5_keygen_developer(pathname, cfgfile, cleanoption):
    retval = True
    
    title = '[+] build_sdbg_keygen_dev'
    out = b'\n'
    err = b'\n'

    if( genpassword(dmdeveloperkeylist[0]) == False):
        out = out + ('\ngen passwd (%s) is error' % dmdeveloperkeylist[0]).encode()
        retval = False
    else:
        out = out + ('\ngen passwd (%s)' % dmdeveloperkeylist[0]).encode()

    rval, rout, rerr = genkeyfiles(dmdeveloperkeylist[0], dmdeveloperkeylist[1], dmdeveloperkeylist[2])
    out = out + rout
    err = err + rerr
    out = out + ('\ngen developer key (%s)' % dmdeveloperkeylist[1]).encode()
    if( rval == False):
        retval = False

    out = out + b'\n'
    return title, out, err

##########################################################    

dm2lvlsdbgkeychainlist = [
    '../dmconfig/sdebug_hbk1_2lvl_key_chain_developer.cfg',
    '../example/sdebug_hbk1_2lvl_key_chain_developer.log'
    ]

def step_2_D_6_keychain_2lvl(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sdbg_chain_2lvl'
    out = b'\n'
    err = b'\n'

    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dm2lvlsdbgkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, dm2lvlsdbgkeychainlist[0], dm2lvlsdbgkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
   
    if(retval == False):
        out = out + ('\ngen keychain (%s) is error' % dm2lvlsdbgkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % dm2lvlsdbgkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

dm3lvlsdbgenbkeychainlist = [
    '../dmconfig/sdebug_hbk1_3lvl_key_chain_enabler.cfg',
    '../example/sdebug_hbk1_3lvl_key_chain_enabler.log'
    ]

def step_2_D_7_keychain_3lvl_enb(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sdbg_chain_3lvl_enb'
    out = b'\n'
    err = b'\n'

    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dm3lvlsdbgenbkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, dm3lvlsdbgenbkeychainlist[0], dm3lvlsdbgenbkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False

    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % dm3lvlsdbgenbkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % dm3lvlsdbgenbkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################    

dm3lvlsdbgdevkeychainlist = [
    '../dmconfig/sdebug_hbk1_3lvl_key_chain_developer.cfg',
    '../example/sdebug_hbk1_3lvl_key_chain_developer.log'
    ]

def step_2_D_8_keychain_3lvl_dev(pathname, cfgfile, cleanoption):
    retval = True
    
    certkeyutil        = 'cert_key_util.py'

    title = '[+] build_sdbg_chain_3lvl_dev'
    out = b'\n'
    err = b'\n'

    # update nvcount 
    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfg(dm3lvlsdbgdevkeychainlist[0])

    retval, retout, reterr = genKeyCert(pathname, certkeyutil, dm3lvlsdbgdevkeychainlist[0], dm3lvlsdbgdevkeychainlist[1])
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen keychain (%s) is error' % dm3lvlsdbgdevkeychainlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen keychain (%s)' % dm3lvlsdbgdevkeychainlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

cmenablercertlist = [
    '../cmtpmcfg/sdebug_hbk0_enabler_cert.cfg',
    '../example/sdebug_hbk0_enabler_cert.log'
    ]

def step_3_1_build_sdebug_cmenabler_cert(pathname, cfgfile, cleanoption):
    retval = True

    enablerutil        = 'cert_dbg_enabler_util.py'

    title = '[+] build_sdebug_cmenabler_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genEnablerCert(pathname, enablerutil, cmenablercertlist[0], cmenablercertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Enabler Cert (%s) is error' % cmenablercertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Enabler Cert (%s)' % cmenablercertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

dmenablercertlist = [
    '../dmtpmcfg/sdebug_hbk1_enabler_cert.cfg',
    '../example/sdebug_hbk1_enabler_cert.log'
    ]

def step_3_2_build_sdebug_dmenabler_cert(pathname, cfgfile, cleanoption):
    retval = True

    enablerutil        = 'cert_dbg_enabler_util.py'

    title = '[+] build_sdebug_dmenabler_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genEnablerCert(pathname, enablerutil, dmenablercertlist[0], dmenablercertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Enabler Cert (%s) is error' % dmenablercertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Enabler Cert (%s)' % dmenablercertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

cmdevelopercertlist = [
    '../cmtpmcfg/sdebug_hbk0_developer_cert.cfg',
    '../example/sdebug_hbk0_developer_cert.log'
    ]

def step_3_3_build_sdebug_cmdeveloper_cert(pathname, cfgfile, cleanoption):
    retval = True

    developerutil        = 'cert_dbg_developer_util.py'

    title = '[+] build_sdebug_cmdeveloper_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genDeveloperCert(pathname, developerutil, cmdevelopercertlist[0], cmdevelopercertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Developer Cert (%s) is error' % cmdevelopercertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Developer Cert (%s)' % cmdevelopercertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

dmdevelopercertlist = [
    '../dmtpmcfg/sdebug_hbk1_developer_cert.cfg',
    '../example/sdebug_hbk1_developer_cert.log'
    ]

def step_3_4_build_sdebug_dmdeveloper_cert(pathname, cfgfile, cleanoption):
    retval = True

    developerutil        = 'cert_dbg_developer_util.py'

    title = '[+] build_sdebug_dmdeveloper_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genDeveloperCert(pathname, developerutil, dmdevelopercertlist[0], dmdevelopercertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False

    if( retval == False):
        out = out + ('\ngen Developer Cert (%s) is error' % dmdevelopercertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Developer Cert (%s)' % dmdevelopercertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

cmenablerrmacertlist = [
    '../cmtpmcfg/sdebug_hbk0_enabler_rma_cert.cfg',
    '../example/sdebug_hbk0_enabler_rma_cert.log'
    ]

def step_3_5_build_sdebug_cmenabler_rma_cert(pathname, cfgfile, cleanoption):
    retval = True

    enablerutil        = 'cert_dbg_enabler_util.py'

    title = '[+] build_sdebug_cmenabler_rma_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genEnablerCert(pathname, enablerutil, cmenablerrmacertlist[0], cmenablerrmacertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Enabler Rma Cert (%s) is error' % cmenablerrmacertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Enabler Rma Cert (%s)' % cmenablerrmacertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

dmenablerrmacertlist = [
    '../dmtpmcfg/sdebug_hbk1_enabler_rma_cert.cfg',
    '../example/sdebug_hbk1_enabler_rma_cert.log'
    ]

def step_3_6_build_sdebug_dmenabler_rma_cert(pathname, cfgfile, cleanoption):
    retval = True

    enablerutil        = 'cert_dbg_enabler_util.py'

    title = '[+] build_sdebug_dmenabler_rma_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genEnablerCert(pathname, enablerutil, dmenablerrmacertlist[0], dmenablerrmacertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Enabler Rma Cert (%s) is error' % dmenablerrmacertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Enabler Rma Cert (%s)' % dmenablerrmacertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

cmdeveloperrmacertlist = [
    '../cmtpmcfg/sdebug_hbk0_developer_rma_cert.cfg',
    '../example/sdebug_hbk0_developer_rma_cert.log'
    ]

def step_3_7_build_sdebug_cmdeveloper_rma_cert(pathname, cfgfile, cleanoption):
    retval = True

    developerutil        = 'cert_dbg_developer_util.py'

    title = '[+] build_sdebug_cmdeveloper_rma_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genDeveloperCert(pathname, developerutil, cmdeveloperrmacertlist[0], cmdeveloperrmacertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Developer Rma Cert (%s) is error' % cmdeveloperrmacertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Developer Rma Cert (%s)' % cmdeveloperrmacertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################

dmdeveloperrmacertlist = [
    '../dmtpmcfg/sdebug_hbk1_developer_rma_cert.cfg',
    '../example/sdebug_hbk1_developer_rma_cert.log'
    ]

def step_3_8_build_sdebug_dmdeveloper_rma_cert(pathname, cfgfile, cleanoption):
    retval = True

    developerutil        = 'cert_dbg_developer_util.py'

    title = '[+] build_sdebug_dmdeveloper_rma_cert'
    out = b'\n'
    err = b'\n'

    retval, retout, reterr = genDeveloperCert(pathname, developerutil, dmdeveloperrmacertlist[0], dmdeveloperrmacertlist[1] )
    if( retval == True):
        #print(retout.decode("ASCII"))
        if retout.decode("ASCII").find("completed successfully") < 0 :
            retval = False
            
    if( retval == False):
        out = out + ('\ngen Developer Rma Cert (%s) is error' % dmdeveloperrmacertlist[0]).encode()
        out = out + b'\n' + retout + reterr
        retval = False
    else:
        out = out + ('\ngen Developer Rma Cert (%s)' % dmdeveloperrmacertlist[0]).encode()
        out = out + b'\n' + retout

    out = out + b'\n'
    return title, out, err

##########################################################


