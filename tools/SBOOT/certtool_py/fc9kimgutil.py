#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct
import subprocess
import datetime

from fc9ksecuutil import *
from fc9kfuncutil import *
from fc9kgenutil  import *
from retmimage import *
from secureimage import *

##########################################################

complist     = {'COMP1': 0, 'COMP2': 1, 'COMP3': 2 }
compaddrlist = ['COMP1ADDR', 'COMP2ADDR', 'COMP3ADDR' ]

def diasemi_image_generate(pathname, cfgfile, sectionname):

    mancfg = manconfig(cfgfile)
    mancfg.parse()

    owner = mancfg.getconfig('own')

    if( owner == 'CM' ):
        owntag    = 'cm'
        tpmcfgdir = '../cmtpmcfg'
        hbkid     = 'hbk0'
    else:
        owntag    = 'dm'
        tpmcfgdir = '../dmtpmcfg'
        hbkid     = 'hbk1'

    certsbcontentutil = 'cert_sb_content_util.py'

    bootmodel = mancfg.gettpmbootmodel_int()
    combined = mancfg.gettpmimagemodel_int()
    imgpath = mancfg.getconfig('image')

    index = section_list[sectionname]
    tagname = tag_filename[index]

    title = ''
    out = b'\n'
    err = b'\n'

    if( mancfg.getrawconfig(sectionname, 'TITLE') == '' ) :
        '''
        title = '[+] DIASEMI_Image_Build : %s' % ( tagname )
        out = b'\nsection not found.\n'
        '''
        return title, out, err
    
    if( int(bootmodel) == 0 ):
        if( int(index) ==  2 ) : #sram
            return title, out, err
    else:
        if( int(index) ==  0 ) : #ueboot
            return title, out, err
        if( int(index) ==  1 ) : #cache
            return title, out, err
        

    if( int(combined)  == 0 ):
        if( int(index) ==  3 ) : #combined
            return title, out, err
    else:
        if( int(index) ==  4 ) : #ptim
            return title, out, err
        if( int(index) ==  5 ) : #ralib
            return title, out, err
        

    title = '[+] DIASEMI_Image_Build : %s' % ( tagname )
    out = b'\n#############################################\n'
    err = b'\n'        
        
    contentcfg = '%s/sboot_%s_%s_cert.cfg' % ( tpmcfgdir, hbkid, tagname )
    contentlog = '%s/sboot_%s_%s_cert.log' % ( '../example', hbkid, tagname )

    contentini = '%s/sboot_%s_%s_cert.ini' % ( tpmcfgdir, hbkid, tagname )
    contentimg = 'CONFIGNAME'
    contentimglog = '%s/sboot_%s_%s_image.log' % ( '../example', hbkid, tagname )

    cwd = os.getcwd()
    os.chdir(pathname)

    # Remove Image 
    if os.path.isfile(contentini) == True :
        inicfg = configparser.ConfigParser()
        inicfg.optionxform = str    

        inifobj = open(contentini, 'r')
        inicfg.readfp(inifobj)
        inifobj.close()  

        if( inicfg.has_section("IMAGE") == True):  
            if( inicfg.has_option("IMAGE", "IMAGE") == True ):
                imgfilename = inicfg.get("IMAGE", "IMAGE")
                if os.path.isfile(imgfilename) == True :
                    out = out + (' \n[*] Remove Image (%s) \n\n' % (imgfilename)).encode()
                    out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()
                    os.remove(imgfilename)
                else:
                    out = out + (' \n[*] Image (%s) does not exist.\n\n' % (imgfilename)).encode()
        
        del inicfg

    # Check
    retmem = mancfg.getrawconfig(sectionname, 'RETMEM')

    # retmemimage
    finderror = False

    if( str(retmem) == '1' or str(retmem) == 'ON' or str(retmem) == 'true'):
        for comp, cidx in complist.items():
            compname = mancfg.getrawconfig(sectionname, comp)
            compaddr = mancfg.getrawconfig(sectionname, compaddrlist[cidx])
            if( compname != '' ):
                compaddrs = compaddr.split()

                out = out + (' \n[*] Make RETMEM Image (%s.rtm) \n\n' % (compname)).encode()
                out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()

                if( imgpath == '' ):
                    sourcefilename =  ('../image/%s.bin' % compname)
                    targetfilename =  ('../image/%s.rtm' % compname)
                else:
                    sourcefilename =  ('%s/%s.bin' % (str(imgpath), compname))
                    targetfilename =  ('%s/%s.rtm' % (str(imgpath), compname))

                # check source file
                if( os.path.isfile(sourcefilename) == False ):
                    out = out + (' \n[*] Cannot find the file, %s.bin \n\n' % (compname)).encode()
                    err = err + ('Error - not found, %s.bin\n' % (compname)).encode()
                    finderror = True
                else:
                    if( os.path.isfile(targetfilename) == True ):
                        os.remove(targetfilename)

                    cmdlist = [ 'retmimage', '-A16', ('-RETM=%s' % compaddrs[3] )
                                , sourcefilename
                                , targetfilename ]
                        
                    #print ( cmdlist )
                    pout, perr = mkRETMImage( cmdlist )
                    out = out + pout.encode()
                    err = err + perr.encode()

                    # check target file
                    if( os.path.isfile(targetfilename) == False ):
                        out = out + (' \n[*] Cannot find the file, %s.rtm \n\n' % (compname)).encode()
                        err = err + ('Error - not found, %s.rtm\n' % (compname)).encode()
                        finderror = True          

    else:
        for comp, cidx in complist.items():
            compname = mancfg.getrawconfig(sectionname, comp)
            compaddr = mancfg.getrawconfig(sectionname, compaddrlist[cidx])
            if( compname != '' ):
                compaddrs = compaddr.split()
                    
                if( os.path.isfile(('../image/%s.bin' % compname)) == True ):
                    out = out + (' \n[*] Check Image (%s.bin) \n\n' % (compname)).encode()
                    out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()
                    fsize = os.path.getsize(('../image/%s.bin' % compname))
                    psize = 4 - (fsize % 4)
                else:
                    out = out + (' \n[*] Cannot find the file, %s.bin \n\n' % (compname)).encode()
                    err = err + ('Error - not found, %s.bin\n' % (compname)).encode()
                    fsize = 0
                    psize = 0
                    finderror = True
				
                if( (fsize != 0) and (psize != 4) ):
                    out = out + ('\tfile size (%d B) is invalid, so it will be padded with %d-byte null. \n\n' % (fsize, psize)).encode()
                    FileObj = open(('../image/%s.bin' % compname), "r+b")

                    FileObj.seek(fsize)
                    bdata = bytes(psize)
                    FileObj.write(bdata)
                    FileObj.close()

    if finderror == True :
        os.chdir(cwd)
        out = out + b'\n#############################################\n'
        return title, out, err        

    # cc312 image
    aesmode = mancfg.getrawconfig(sectionname, 'KEYENC')
    if( str(aesmode) == 'ICV_CODE_ENC' ):
        aesflag = 1
    elif( str(aesmode) == 'OEM_CODE_ENC' ):
        aesflag = 2
    else:
        aesflag = 0

    if( aesflag == 1 ):
        passfile  = '.' + cmotpkeylist[0][0]
        encotpkey = '.' + cmotpkeylist[0][2]
        txtotpkey = aes_key_list[1]
        #print( passfile, encotpkey, txtotpkey )
        convotpkeyfile( passfile, encotpkey, txtotpkey )
        
    elif( aesflag == 2 ):
        passfile  = '.' + dmotpkeylist[0][0]
        encotpkey = '.' + dmotpkeylist[0][2]
        txtotpkey = aes_key_list[2]
        #print( passfile, encotpkey, txtotpkey )
        convotpkeyfile( passfile, encotpkey, txtotpkey )
        
    retcode = True

    if os.path.isfile(contentcfg) == True :
        cmdlist = [ certsbcontentutil, os.path.abspath(contentcfg), os.path.abspath(contentlog) ]
        #print ( cmdlist )
        out = out + (' \n[*] Make Content Certificate (%s) \n\n' % (contentcfg)).encode()
        out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()
        
        pout, perr = process_call_return( cmdlist )
        out = out + pout
        err = err + perr

        for rline in pout.decode('ASCII').split('\n'):
            if rline.lower().find('error') > 0 :
                retcode = False   

        if   os.path.isfile(os.path.abspath(contentlog)) == False :
            retcode = False     

    if( aesflag == 1 ):
        txtotpkey = aes_key_list[1]
        os.remove(txtotpkey)
                
    elif( aesflag == 2 ):
        txtotpkey = aes_key_list[2]
        os.remove(txtotpkey)
    
    if retcode == False :
        os.chdir(cwd)
        return title, out, err        

    # sbox image
    sbox = mancfg.getrawconfig(sectionname, 'SBOX')
    dbgcert = mancfg.getrawconfig(sectionname, 'DBGCERT')
    sdebug = mancfg.gettpmsdebug_int()

    if( str(dbgcert) == '1' or str(dbgcert) == 'ON' or str(dbgcert) == 'true'):
        dbgcert = 'ON'
    elif( str(dbgcert) == 'RMA' or str(dbgcert) == 'rma'):
        dbgcert = 'RMA'
    else:
        dbgcert = 'OFF'
        
    if( int(sbox,16) != 0x00000000 ):
        out = out + (' \n[*] Make SandBox Binary for Debug Cert (%s) \n\n' % (sbox)).encode()
        out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()

        if( str(dbgcert) == 'ON' ):
            str_dbgcert = "../%spublic/sdebug_%s_developer_pkg.bin" % (owntag, hbkid)
            if( int(sdebug) == 2 ):
                str_dbgcertk = ''
            else:
                str_dbgcertk = "../%spublic/sdebug_%s_3lvl_key_chain_enabler.bin" % (owntag, hbkid)
        elif( str(dbgcert) == 'RMA' ):
            str_dbgcert = "../%spublic/sdebug_%s_developer_rma_pkg.bin" % (owntag, hbkid)
            if( int(sdebug) == 2 ):
                str_dbgcertk = ''
            else:
                str_dbgcertk = "../%spublic/sdebug_%s_3lvl_key_chain_enabler.bin" % (owntag, hbkid)
        else:
            str_dbgcert = ''
            str_dbgcertk = ''

        str_sboxcert = "../%spublic/sdebug_%s_developer_sbox.bin" % ( owntag, hbkid )
        str_realcert = "../public/sdebug_%s_developer_certificate.bin" % ( hbkid )

        out = out + ('   %s\n' % str_dbgcertk).encode()
        out = out + (' + %s\n' % str_dbgcert).encode()
        out = out + (' [%s] => %s\n' % (sbox, str_sboxcert)).encode()
        out = out + (' [%s] => %s\n' % (sbox, str_realcert)).encode()

        rbfilename = [ str_dbgcertk, str_dbgcert]
        MergeStr = bytes()
        OMergeStr = bytes()
    
        for infile in rbfilename:
            if( infile != '' ):
                fname, ext = os.path.splitext(infile)
                if ext == '.bin':
                    MergeStr += ( GetDataFromBinFile(infile) )
                else:
                    MergeStr += ( GetDataFromTxtFile(infile) )            

        tag = 0x52446B63
        bytetag = tag.to_bytes(4, 'little')
        sflashoffset = int(sbox,16) | 0x40000000
        bytesbox = sflashoffset.to_bytes(4, 'little')

        owrfile = open(str_realcert, "wb")
        owrfile.write(MergeStr)
        owrfile.close()
    
        OMergeStr += bytetag
        OMergeStr += bytesbox
        OMergeStr += bytes( len(MergeStr[8:]) ).replace(b'\x00', b'\xA5')

        wrfile = open(str_sboxcert, "wb")
        wrfile.write(OMergeStr)
        wrfile.close()

    # fc9k image
    
    if os.path.isfile(contentini) == True :
        cmdlist = [ 'secureimage', os.path.abspath(contentini), contentimg, os.path.abspath(contentimglog) ]
        #print ( cmdlist )
        out = out + (' \n[*] Make Secure Boot Image (%s) \n\n' % (contentini)).encode()
        out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()

        pout, perr = mkSecureImage( cmdlist )

        out = out + pout.encode()
        err = err + perr.encode()

        if os.path.isfile(os.path.abspath(contentimglog)) == True :
            logfile = open(os.path.abspath(contentimglog), 'rb')
            out = out + logfile.read()
            logfile.close()
        else:
            out = out + (' \n[*] Cannot open the file, %s\n\n' % (contentimglog)).encode()
            err = err + ('Error - Cannot open %s\n' % (contentimglog)).encode()            

    os.chdir(cwd)

    out = out + ('\n------> %s\n' % (datetime.datetime.now())).encode()
    out = out + b'\n#############################################\n'
 
    return title, out, err


##########################################################




