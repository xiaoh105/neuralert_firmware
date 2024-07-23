#!/bin/python3

import sys
import os
import re
import datetime
import string
from string import whitespace
import configparser
import struct
import subprocess
import platform

from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QUrl, QStringListModel
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine, qmlRegisterType


os.environ['QT_SCALE_FACTOR'] = '1'

from fc9ksecuutil import *
from fc9kfuncutil import *
from fc9kfrontutil import *
from fc9kgenutil import *
from fc9kimgutil import *

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

def diasemi_write_log_con(logfile, msg):

    confile = open(logfile, "a")

    # warning filter 
    warn_filter_list = [
        r'[\.]+[\+][\+]',
        r'WARNING: can\'t open config file: /usr/local/ssl/openssl.cnf',
        r'e is 65537 \(0x10001\)',
        r'__main__.PyInstallerImportError:[^\n]+\n',
    ]

    for filterstr in warn_filter_list:
        msg = re.sub(filterstr, '', msg)

    #msg = re.sub(r'[\r|\n]+', '\n', msg)
    print(msg)
    confile.write((msg))

    confile.close()

def diasemi_check_log_con(logfile):
    retval = True

    with open(logfile) as logf:
        if 'error' in logf.read().lower():
            retval = False

    return retval

##########################################################
# secuasset
##########################################################

def module_secuasset():

    man_cfgfile = ""
    
    if "-cfg" in sys.argv:
        man_cfgfile = sys.argv[sys.argv.index("-cfg") + 1]
    else:
        print("not found TPM config file")
        exit(1)

    #print("CFG file is %s" % man_cfgfile )

    pathname, fname = os.path.split(sys.argv[0])

    logfilename = pathname+"\..\example\secure_asset.txt"
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)

    # Default, Fusion, Imagine, Material, Universal
    sys.argv += ['--style', 'Material']
    
    app = QGuiApplication(sys.argv)

    app.setOrganizationName("Dialog Semiconductor")
    app.setOrganizationDomain("www.dialog-semiconductor.com")
    app.setApplicationName("Dialog Security Tool")

    assetman = front_assetman(man_cfgfile)
    assetman.SetWinTitle('Dialog Security Tool : Secure Asset')

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("assetman", assetman)
    
    qmlpath = '%s/assetman.qml' % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)

    app.exec_()

    cfgtext = assetman.gettextfield()
    flagcmkey = assetman.iscmkeytype()
    pkgfilename = assetman.getassetfilename()
    assetprovutil = 'asset_provisioning_util.py'

    if(cfgtext != ''):
        if (flagcmkey == True):
            diasemi_write_log_con(logfilename, "=========================\n")
            diasemi_write_log_con(logfilename, "=*  Secure Asset : CM\n" )
            diasemi_write_log_con(logfilename, "=========================\n")
            diasemi_write_log_con(logfilename, "\n")    

            retval, out, err = genASSET(pathname, assetprovutil, 'CM', man_cfgfile, pkgfilename)
        else:
            diasemi_write_log_con(logfilename, "=========================\n")
            diasemi_write_log_con(logfilename, "=*  Secure Asset : DM\n" )
            diasemi_write_log_con(logfilename, "=========================\n")
            diasemi_write_log_con(logfilename, "\n")    

            retval, out, err = genASSET(pathname, assetprovutil, 'DM', man_cfgfile, pkgfilename)

        mout = re.sub(r'[\r|\n]+', '\n', out.decode("ASCII"))
        merr = re.sub(r'[\r|\n]+', '\n', err.decode("ASCII"))
        diasemi_write_log_con(logfilename, mout )
        diasemi_write_log_con(logfilename, merr )
    else:
        diasemi_write_log_con(logfilename, "=========================\n")
        diasemi_write_log_con(logfilename, "=*  Secure Asset : PASS\n" )
        diasemi_write_log_con(logfilename, "=========================\n")
        diasemi_write_log_con(logfilename, "\n")         

    retval = diasemi_check_log_con(logfilename)    

    if( retval == False ):    
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")
    else:
        diasemi_write_log_con(logfilename, " ======> Procedure has been completed successfully ...")

    del engine
    del assetman 
    del app

    
##########################################################
# secuboot
##########################################################

def module_secuboot():

    man_cfgfile = ""
    
    if "-cfg" in sys.argv:
        man_cfgfile = sys.argv[sys.argv.index("-cfg") + 1]
    else:
        print("not found TPM config file")
        exit(1)

    if "-gui" in sys.argv:
        man_gui = True
    else:
        man_gui = False

    if "-tpm" in sys.argv:
        man_tpmdir = sys.argv[sys.argv.index("-tpm") + 1]
        skip_cfg_update = 0
    else:
        man_tpmdir = ''
        skip_cfg_update = 1

    #print("CFG file is %s" % man_cfgfile )

    
    manobj = manconfig(man_cfgfile)
    manobj.parse()

    sdebug = manobj.gettpmsdebug_int()
    rule   = manobj.getconfig('rule')
    own    = manobj.getconfig('own')
    hbkid = manobj.gettpmhbkid_int()

    dbgmode = manobj.getrawconfig('UEBOOT', 'DBGCERT')

    pathname, fname = os.path.split(sys.argv[0])

    logfilename = pathname+"/../example/secure_boot.txt"
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)

    diasemi_write_log_con(logfilename, '----> %s\n' % (datetime.datetime.now()) )
    diasemi_write_log_con(logfilename, "=========================\n")
    diasemi_write_log_con(logfilename, ("=*  Secure Debug : %s\n" % dbgmode))
    diasemi_write_log_con(logfilename, "=========================\n")
    diasemi_write_log_con(logfilename, "\n")

    if( hbkid == 1 ):
        man_tpmdir = './dmtpmcfg'
    elif( hbkid == 0 ):
        man_tpmdir = './cmtpmcfg'
    
    if dbgmode == 'OFF' :
        # Secure Boot
        if( skip_cfg_update == 0 ):
            target_config_update(man_cfgfile, man_tpmdir, 'BOOT')
    
        # Default, Fusion, Imagine, Material, Universal
        batchlist = [
            [ diasemi_image_generate, pathname, man_cfgfile, 'UEBOOT'      , 'UEBOOT' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.cache'  , 'Cache-Boot' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.sram'   , 'SRAM-Boot' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.combined', 'PTIM&RaLIB' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.PTIM'   , 'PTIM' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.RaLIB'  , 'RaLIB' ]
            #,[ diasemi_image_generate, pathname, man_cfgfile, 'RMA'         , 'RMA' ]
        ]

        if( man_gui == True ):
            sys.argv += ['--style', 'Material']

            app = QGuiApplication(sys.argv)
            
            diasemi_gui_imagebuild(app, pathname, man_cfgfile, batchlist)

            del app
            
        else:
            
            for idx, batchitem in enumerate(batchlist):
                title, out, err = batchitem[0](batchitem[1], batchitem[2], batchitem[3])
                mout = re.sub(r'[\r|\n]+', '\n', out.decode("ASCII"))
                merr = re.sub(r'[\r|\n]+', '\n', err.decode("ASCII"))

                if( title != '' ):
                    diasemi_write_log_con(logfilename, ("\n%s" % title))
                    diasemi_write_log_con(logfilename, mout )
                    diasemi_write_log_con(logfilename, merr )
                    #diasemi_write_log_con(logfilename, "\n")

                retval = diasemi_check_log_con(logfilename) 
                if( retval == False ):
                    diasemi_write_log_con(logfilename, "****************************************\n")
                    diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
                    diasemi_write_log_con(logfilename, "****************************************\n")                    
                    return    

    else:
        # Secure Debug
        updated = True


        socid_file_name = './image/soc_id.bin'
        if os.path.isfile(socid_file_name) == False :
            diasemi_write_log_con(logfilename, ("Error - not found, %s\n" % socid_file_name))
            diasemi_write_log_con(logfilename, " > Run \"Secure Debug\" again to create the correct soc-id file.\n")
            diasemi_write_log_con(logfilename, "****************************************\n")
            diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
            diasemi_write_log_con(logfilename, "****************************************\n")                    
            return 

        binstr = GetDataFromBinFile(socid_file_name)
        socid = str()
        i = 0
        for s in bytes(binstr):
            socid = socid + '%02X '% s
            i = i + 1
            if (i%16) == 0:
                socid= socid + '\n'

        diasemi_write_log_con(logfilename, ("Target SOC-ID:\n%s\n" % socid))

        if( updated == True ):
            if( man_tpmdir == './cmtpmcfg' ):
                target_config_update(man_cfgfile, man_tpmdir, 'DBG')
                title, out, err = step_3_3_build_sdebug_cmdeveloper_cert(pathname, man_cfgfile, 'Developer Cert')
            else:
                if( rule != 'Red    Boot' or own != 'DM' ):
                    target_config_update(man_cfgfile, man_tpmdir, 'DBG')
                    title, out, err = step_3_4_build_sdebug_dmdeveloper_cert(pathname, man_cfgfile, 'Developer Cert')

            mout = re.sub(r'[\r|\n]+', '\n', out.decode("ASCII"))
            merr = re.sub(r'[\r|\n]+', '\n', err.decode("ASCII"))

            if( title != '' ):
                diasemi_write_log_con(logfilename, ("\n%s" % title))
                diasemi_write_log_con(logfilename, mout )
                diasemi_write_log_con(logfilename, merr )
                #diasemi_write_log_con(logfilename, "\n")   

            retval = diasemi_check_log_con(logfilename) 
            if( retval == False ):
                diasemi_write_log_con(logfilename, "****************************************\n")
                diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
                diasemi_write_log_con(logfilename, "****************************************\n")                    
                return    
                            
        #######################################################
        # Default, Fusion, Imagine, Material, Universal
        #######################################################
        batchlist = [
            [ diasemi_image_generate, pathname, man_cfgfile, 'UEBOOT'      , 'UEBOOT' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.cache'  , 'Cache-Boot' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.sram'   , 'SRAM-Boot' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.combined', 'PTIM&RaLIB' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.PTIM'   , 'PTIM' ],
            [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.RaLIB'  , 'RaLIB' ]
            #,[ diasemi_image_generate, pathname, man_cfgfile, 'RMA'         , 'RMA' ]
        ]

        for idx, batchitem in enumerate(batchlist):
            title, out, err = batchitem[0](batchitem[1], batchitem[2], batchitem[3])
            mout = re.sub(r'[\r|\n]+', '\n', out.decode("ASCII"))
            merr = re.sub(r'[\r|\n]+', '\n', err.decode("ASCII"))

            if( title != '' ):
                diasemi_write_log_con(logfilename, ("\n%s" % title))
                diasemi_write_log_con(logfilename, mout )
                diasemi_write_log_con(logfilename, merr )
                #diasemi_write_log_con(logfilename, "\n")       

            retval = diasemi_check_log_con(logfilename) 
            if( retval == False ):
                diasemi_write_log_con(logfilename, "****************************************\n")
                diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
                diasemi_write_log_con(logfilename, "****************************************\n")                    
                return    

    diasemi_write_log_con(logfilename, " ======> Procedure has been completed successfully ...")

##########################################################
# secudebug
##########################################################
            
def module_secudebug():

    man_cfgfile = ""
    
    if "-cfg" in sys.argv:
        man_cfgfile = sys.argv[sys.argv.index("-cfg") + 1]
    else:
        print("not found config file")
        exit(1)

    print("CFG file is %s" % man_cfgfile )


    manobj = manconfig(man_cfgfile)
    manobj.parse()

    sdebug = manobj.gettpmsdebug_int()
    rule   = manobj.getconfig('rule')
    own    = manobj.getconfig('own')
    hbkid = manobj.gettpmhbkid_int()

    if( hbkid == 1 ):
        man_tpmdir = './dmtpmcfg'
    else:
        man_tpmdir = './cmtpmcfg'    
    
    # Default, Fusion, Imagine, Material, Universal
    sys.argv += ['--style', 'Material']
    
    app = QGuiApplication(sys.argv)
    pathname, fname = os.path.split(sys.argv[0])
	 
    #######################################################

    updated, socid = diasemi_gui_sdbg_manager(app, pathname, man_cfgfile, 'DEVELOPER')
    logfilename = getcurrlogfilename(man_cfgfile)
    diasemi_write_log_con(logfilename, ("Target SOC-ID:\n%s\n \n" % socid))

    if( updated == True ):
        if( man_tpmdir == './cmtpmcfg' ):
            target_config_update(man_cfgfile, man_tpmdir, 'DBG')
            title, out, err = step_3_3_build_sdebug_cmdeveloper_cert(pathname, man_cfgfile, 'Developer Cert')
        else:
            if( rule != 'Red    Boot' or own != 'DM' ):
                target_config_update(man_cfgfile, man_tpmdir, 'DBG')
                title, out, err = step_3_4_build_sdebug_dmdeveloper_cert(pathname, man_cfgfile, 'Developer Cert')

        print(title)
        print( out.decode("ASCII") )
        print( err.decode("ASCII") )

    #######################################################
    # 
    # updated = diasemi_gui_sdbg_manager(app, pathname, man_cfgfile, 'DEVELOPER-RMA')    
	# 
    # if( updated == True ):
    #     if( man_tpmdir == './cmtpmcfg' ):
    #         target_config_update(man_cfgfile, man_tpmdir, 'RMA')
    #         title, out, err = step_3_7_build_sdebug_cmdeveloper_rma_cert(pathname, man_cfgfile, 'Developer RMA Cert')
    #     else:
    #         target_config_update(man_cfgfile, man_tpmdir, 'RMA')
    #         title, out, err = step_3_8_build_sdebug_dmdeveloper_rma_cert(pathname, man_cfgfile, 'Developer RMA Cert')
    #
    #    print(title)
    #    print( out.decode("ASCII") )
    #    print( err.decode("ASCII") )

    #######################################################
    # Default, Fusion, Imagine, Material, Universal
    #######################################################
    batchlist = [
        [ diasemi_image_generate, pathname, man_cfgfile, 'UEBOOT'      , 'UEBOOT' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.cache'  , 'Cache-Boot' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.sram'   , 'SRAM-Boot' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.combined', 'PTIM&RaLIB' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.PTIM'   , 'PTIM' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.RaLIB'  , 'RaLIB' ]
        #,[ diasemi_image_generate, pathname, man_cfgfile, 'RMA'         , 'RMA' ]
    ]

    for idx, batchitem in enumerate(batchlist):
        title, out, err = batchitem[0](batchitem[1], batchitem[2], batchitem[3])
        print(title)
        print( out.decode("ASCII") )
        print( err.decode("ASCII") )

    del app


##########################################################
# secuman			
##########################################################

def check_role(roledata, ruledata):
    rolelist = roledata.split('|')
    rulelist = ruledata.split('|')

    for role in rolelist:
        for rule in rulelist:
            if( role == rule ):
                return True

    return False


def parse_actrule(actrule):
    if( actrule == '' ):
        jobflow = []
        authority = [] 
    else:
        actlist = actrule.split('&')

        action = actlist[0].split('=')
        jobflow = action[1].split('|')

        rolelist = actlist[1].split('=')
        authority = rolelist[1].split('|')

    return jobflow, authority


def check_actrole(author, role):
    for auth in author:
        if( auth == role ):
            return True
    return False

def genscript(outstr):
    script_file = open('./public/Download.ttl', 'wt')
    script_file.write(outstr)
    script_file.close()

def genscriptrma(outstr):
    script_file = open('./public/DownloadDbg.ttl', 'wt')
    script_file.write(outstr)
    script_file.close()
    
##########################################################

def diasemi_phase_1_config(app, pathname, cfgfile):

    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs CM')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All CM files')
    cleanoption = 'YesToAll'
    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All Secret files')

    if( cleanoption == 'YesToAll' ):
        #step_1_0_clean_all(pathname, cfgfile, cleanoption)
        batchlist = [[step_1_0_clean_all, pathname, cfgfile, cleanoption, 'Clean CM CFGs']]
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Clean ALL')

    return True


def diasemi_phase_2_image(app, pathname, cfgfile):

    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './cmtpmcfg', 'BOOT')
    genscript( target_script_update(cfgfile, './cmtpmcfg', 'BOOT') )

    batchlist = []
    
    batchlist.append( [step_1_1_build_cmpu, pathname, cfgfile, 'SkipCMPU', 'Gen HBK0'] )

    if( sboot == 2 ):
        batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        batchlist.append( [step_1_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
    elif( sboot == 3 ):
        batchlist.append( [step_1_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
        batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
    
        batchlist.append( [step_1_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
        batchlist.append( [step_1_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen Key & KeyChain')
    del batchlist[:]
    # build image
    batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    
    return True
    

def diasemi_phase_3_cmrma(app, pathname, cfgfile):

    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs CM')
    
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF:RMA')

    if( (check_role(role, 'ALL|Enabler') == True) or (sdebug == 2 and check_role(role, 'TOP') == True) ):
        diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
        
    if( check_role(role, 'ALL|Developer') == True ):
        diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    updated = False    
    #if( check_role(role, 'ALL|Developer') == True ):
    #    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
    updated = True

    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './cmtpmcfg', 'RMA')
        genscriptrma( target_script_update(cfgfile, './cmtpmcfg', 'RMA') )

        if( sboot == 2 ):
            if( check_role(role, 'ALL|Publisher') == True ):
                batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
            if( check_role(role, 'ALL|TOP|Issuer') == True ):
                batchlist.append( [step_1_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sboot == 3 ):
            if( check_role(role, 'ALL|Issuer') == True ):
                batchlist.append( [step_1_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
            if( check_role(role, 'ALL|Publisher') == True ):
                batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        
            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_1_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
            if( check_role(role, 'ALL|Issuer') == True ):
                batchlist.append( [step_1_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )
        
        if( sdebug == 2 ):
            if( check_role(role, 'ALL|Developer') == True ):
                batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
            if( check_role(role, 'ALL|TOP|Enabler') == True ):
                batchlist.append( [step_1_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_1_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
            if( check_role(role, 'ALL|Developer') == True ):
                batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_1_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_1_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_3_5_build_sdebug_cmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_3_7_build_sdebug_cmdeveloper_rma_cert, pathname, cfgfile, 'Developer RMA Cert', 'Developer RMA Cert'] )

    # build rma

    batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RMA', 'RMA' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build RMA Image')    
    del batchlist[:]

    return True


def diasemi_phase_4_cmpu(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen CMPU', ('\'%s\' is not authorized to process this action.' % role))
        return
    
    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Re-Gen HBK0 & CMPU')
    cleanoption = 'YesToAll'

    if( cleanoption == 'YesToAll' ):
        diasemi_gui_xmpu_manager(app, pathname, cfgfile, 'CMPU')

        batchlist = []

        batchlist.append( [step_1_1_build_cmpu , pathname, cfgfile, cleanoption, 'CMPU'] )

        batchlist.append( [step_1_1_build_cmpu_cleanup , pathname, cfgfile, cleanoption, 'Clean'] )
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen CMPU')
        del batchlist[:]
        
    return True


def diasemi_phase_5_keychain(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    actrule = diasemi_gui_key_manager(app, pathname, cfgfile)

    if( actrule == '' ):
        return False
    
    action, authority = parse_actrule(actrule)
    
    target_config_update(cfgfile, './cmtpmcfg', 'ALL')

    batchlist = []

    if( check_actrole(action,'SB1') == True ):
        if( sboot == 2 ):
            if(check_actrole(authority,'Publisher') == True):
                batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        elif( sboot == 3 ):
            if(check_actrole(authority,'Issuer') == True):
                batchlist.append( [step_1_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
            if(check_actrole(authority,'Publisher') == True):
                batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        
    if( check_actrole(action,'SB2') == True ):
        if( sboot == 2 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_1_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sboot == 3 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_1_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
            if(check_actrole(authority,'Issuer') == True):
                batchlist.append( [step_1_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )

    if( check_actrole(action,'SD1') == True ):
        if( sdebug == 2 ):
            if(check_actrole(authority,'Developer') == True):
                batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
        elif( sdebug == 3 ):
            if(check_actrole(authority,'Enabler') == True):
                batchlist.append( [step_1_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
            if(check_actrole(authority,'Developer') == True):
                batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

    if( check_actrole(action,'SD2') == True ):
        if( sdebug == 2 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_1_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_1_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if(check_actrole(authority,'Enabler') == True):
                batchlist.append( [step_1_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

    if( check_actrole(action,'SD3') == True ):
        if(check_actrole(authority,'Enabler') == True):
            batchlist.append( [step_3_1_build_sdebug_cmenabler_cert      , pathname, cfgfile, 'Enabler Cert', 'Enabler Cert'] )
            batchlist.append( [step_3_5_build_sdebug_cmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen Key & KeyChain')
    del batchlist[:]
        
    return True


def diasemi_phase_6_image(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './cmtpmcfg', 'ALL')
    genscript( target_script_update(cfgfile, './cmtpmcfg', 'ALL') )
    
    # build image
    batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]

    return True


def diasemi_phase_7_dmrma(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')
    
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    sdebug = manobj.gettpmsdebug_int()
    sboot = manobj.gettpmsboot_int()

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF:RMA')

    if( (check_role(role, 'ALL|Enabler') == True) or (sdebug == 2 and check_role(role, 'TOP') == True) ):
        diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', 'lcs DM')
        
    if( check_role(role, 'ALL|Developer') == True ):
        diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', 'lcs DM')

    updated = False    
    #if( check_role(role, 'ALL|Developer') == True ):
    #    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
    updated = True
        
    batchlist = []

    target_config_update(cfgfile, './dmtpmcfg', 'RMA')
    genscriptrma( target_script_update(cfgfile, './dmtpmcfg', 'RMA') )
    
    batchlist.append( [step_2_1_build_hbkgt, pathname, cfgfile, 'SkipDMPU', 'Gen HBK1'] )

    if( sboot == 2 ):
        batchlist.append( [step_2_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        batchlist.append( [step_2_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
    elif( sboot == 3 ):
        batchlist.append( [step_2_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
        batchlist.append( [step_2_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
    
        batchlist.append( [step_2_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
        batchlist.append( [step_2_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )
		
    if( sdebug == 2 ):
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
        if( check_role(role, 'ALL|TOP|Enabler') == True ):
            batchlist.append( [step_2_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
    elif( sdebug == 3 ):
        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_2_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

        if( check_role(role, 'ALL|TOP') == True ):
            batchlist.append( [step_2_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_2_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

    if( check_role(role, 'ALL|Enabler') == True ):
        batchlist.append( [step_3_6_build_sdebug_dmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
    if( check_role(role, 'ALL|Developer') == True ):
        batchlist.append( [step_3_8_build_sdebug_dmdeveloper_rma_cert, pathname, cfgfile, 'Developer RMA Cert', 'Developer RMA Cert'] )
        

    # build rma

    batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RMA', 'RMA' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build RMA Image')    
    del batchlist[:] 

    return True


def diasemi_phase_8_config(app, pathname, cfgfile):

    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')
    
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All DM files')
    cleanoption = 'YesToAll'

    if( cleanoption == 'YesToAll' ):
        #step_2_0_clean_all(pathname, cfgfile, cleanoption)
        batchlist = [[step_2_0_clean_all, pathname, cfgfile, cleanoption, ' Clean DM CFGs']]
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Clean ALL')

    return True


def diasemi_phase_9_image(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './dmtpmcfg', 'BOOT')
    genscript( target_script_update(cfgfile, './dmtpmcfg', 'BOOT') )

    batchlist = []
    
    batchlist.append( [step_2_1_build_hbkgt, pathname, cfgfile, 'SkipDMPU', 'Gen HBK1'] )

    if( sboot == 2 ):
        batchlist.append( [step_2_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        batchlist.append( [step_2_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
    elif( sboot == 3 ):
        batchlist.append( [step_2_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
        batchlist.append( [step_2_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
    
        batchlist.append( [step_2_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
        batchlist.append( [step_2_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )

    #diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen Key & KeyChain')
    #del batchlist[:]
    ## build image
    #batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]

    return True

#######

def diasemi_phase_set_temp_dmkey(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen Temp DMKey', ('\'%s\' is not authorized to process this action.' % role))
        return  False   
    
    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove DMKey')
    cleanoption = 'YesToAll' ## 'SkipDMPU'

    batchlist = []

    batchlist.append( [ step_2_1_build_hbkgt, pathname, cfgfile, cleanoption, 'HBKGT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen Temp DMKey')
    del batchlist[:]
        
    return True


#######

def diasemi_phase_10_dmrma(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sdebug = manobj.gettpmsdebug_int()

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF:RMA')

    if( (check_role(role, 'ALL|Enabler') == True) or (sdebug == 2 and check_role(role, 'TOP') == True) ):
        diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
        
    if( check_role(role, 'ALL|Developer') == True ):
        diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    updated = False    
    #if( check_role(role, 'ALL|Developer') == True ):
    #    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
    updated = True
        
    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './dmtpmcfg', 'RMA')
        genscriptrma( target_script_update(cfgfile, './dmtpmcfg', 'RMA') )

        if( sdebug == 2 ):
            if( check_role(role, 'ALL|Developer') == True ):
                batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
            if( check_role(role, 'ALL|TOP|Enabler') == True ):                
                batchlist.append( [step_2_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_2_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
            if( check_role(role, 'ALL|Developer') == True ):
                batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_2_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_2_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_3_6_build_sdebug_dmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_3_8_build_sdebug_dmdeveloper_rma_cert, pathname, cfgfile, 'Developer RMA Cert', 'Developer RMA Cert'] )
        

    # build rma

    batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RMA', 'RMA' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build RMA Image')    
    del batchlist[:] 
    return True


def diasemi_phase_11_hbkgt(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Build HBK1', ('\'%s\' is not authorized to process this action.' % role))
        return False   
    
    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove HBK1')
    cleanoption = 'YesToAll'

    batchlist = []

    batchlist.append( [ step_2_1_build_hbkgt, pathname, cfgfile, cleanoption, 'HBKGT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build HBK1')    
    del batchlist[:] 
    return True


def diasemi_phase_12_okrpt(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Build OemKeyRequestPkg', ('\'%s\' is not authorized to process this action.' % role))
        return False
    
    batchlist = []

    batchlist.append( [ step_2_2_build_okrpt, pathname, cfgfile, 'OKRPT', 'OKRPT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build OemKeyRequestPkg')    
    del batchlist[:] 
    return True


def diasemi_phase_13_ikrpt(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Build IcvKeyResponsePkg', ('\'%s\' is not authorized to process this action.' % role))
        return False
    
    batchlist = []

    batchlist.append( [ step_2_3_build_ikrpt, pathname, cfgfile, 'IKRPT', 'IKRPT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build IcvKeyResponsePkg')    
    del batchlist[:] 
    return True


def diasemi_phase_14_dmpu(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen DMPU', ('\'%s\' is not authorized to process this action.' % role))
        return False
    
    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove DMPU')
    cleanoption = 'YesToAll'

    if( cleanoption == 'YesToAll' ):
        diasemi_gui_xmpu_manager(app, pathname, cfgfile, 'DMPU')
        
        batchlist = []

        batchlist.append( [step_2_4_build_dmpu , pathname, cfgfile, cleanoption, 'DMPU'] )

        batchlist.append( [step_2_4_build_dmpu_cleanup , pathname, cfgfile, cleanoption, 'DMPU'] )
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen DMPU')
        del batchlist[:]
        
    return True

#######

def diasemi_phase_11_build_dmpu(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen DMPU', ('\'%s\' is not authorized to process this action.' % role))
        return False   
    
    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove DMPU')
    cleanoption = 'YesToAll'

    batchlist = []

    batchlist.append( [ step_2_1_build_hbkgt, pathname, cfgfile, cleanoption, 'HBKGT' ] )

    batchlist.append( [ step_2_2_build_okrpt, pathname, cfgfile, 'OKRPT', 'OKRPT' ] )

    batchlist.append( [ step_2_3_build_ikrpt, pathname, cfgfile, 'IKRPT', 'IKRPT' ] )


    if( cleanoption == 'YesToAll' ):
        diasemi_gui_xmpu_manager(app, pathname, cfgfile, 'DMPU')
        
        batchlist.append( [step_2_4_build_dmpu , pathname, cfgfile, cleanoption, 'DMPU'] )

        batchlist.append( [step_2_4_build_dmpu_cleanup , pathname, cfgfile, cleanoption, 'Clean'] )
        

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen DMPU')
    del batchlist[:]
        
    return True


#######

def diasemi_phase_15_keychain(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    actrule = diasemi_gui_key_manager(app, pathname, cfgfile)

    if( actrule == '' ):
        return False
    
    action, authority = parse_actrule(actrule)
    
    target_config_update(cfgfile, './dmtpmcfg', 'ALL')

    batchlist = []

    if( check_actrole(action,'SB1') == True ):
        if( sboot == 2 ):
            if(check_actrole(authority,'Publisher') == True):
                batchlist.append( [step_2_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        elif( sboot == 3 ):
            if(check_actrole(authority,'Issuer') == True):
                batchlist.append( [step_2_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
            if(check_actrole(authority,'Publisher') == True):
                batchlist.append( [step_2_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        
    if( check_actrole(action,'SB2') == True ):
        if( sboot == 2 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_2_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sboot == 3 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_2_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
            if(check_actrole(authority,'Issuer') == True):
                batchlist.append( [step_2_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )

    if( check_actrole(action,'SD1') == True ):
        if( sdebug == 2 ):
            if(check_actrole(authority,'Developer') == True):
                batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
        elif( sdebug == 3 ):
            if(check_actrole(authority,'Enabler') == True):
                batchlist.append( [step_2_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
            if(check_actrole(authority,'Developer') == True):
                batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

    if( check_actrole(action,'SD2') == True ):
        if( sdebug == 2 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_2_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            if(check_actrole(authority,'TOP') == True):
                batchlist.append( [step_2_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if(check_actrole(authority,'Enabler') == True):
                batchlist.append( [step_2_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

    if( check_actrole(action,'SD3') == True ):
        if(check_actrole(authority,'Enabler') == True):
            batchlist.append( [step_3_2_build_sdebug_dmenabler_cert      , pathname, cfgfile, 'Enabler Cert', 'Enabler Cert'] )
            batchlist.append( [step_3_6_build_sdebug_dmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
            
    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen Key & KeyChain')
    del batchlist[:]
    
    return True


def diasemi_phase_16_image(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './dmtpmcfg', 'ALL')
    genscript( target_script_update(cfgfile, './dmtpmcfg', 'ALL') )

    # build image
    batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    return True


def diasemi_phase_Y17_dmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater
    return True


def diasemi_phase_Y18_image(app, pathname, cfgfile):

    diasemi_gui_imgmanager(app, pathname, cfgfile, '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
   
    target_config_update(cfgfile, './dmtpmcfg', 'BOOT')
    genscript( target_script_update(cfgfile, './dmtpmcfg', 'BOOT') )

    # build image
    batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    return True


def diasemi_phase_Y19_dmdbgcert(app, pathname, cfgfile):

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'ON')

    #diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER', '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './dmtpmcfg', 'ALL')
    genscript( target_script_update(cfgfile, './dmtpmcfg', 'ALL') )

    # build debug

    updated, socid = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER')
    logfilename = getcurrlogfilename(cfgfile)
    diasemi_write_log_con(logfilename, ("Target SOC-ID:\n%s\n \n" % socid))


    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './dmtpmcfg', 'DBG')
        genscriptrma( target_script_update(cfgfile, './dmtpmcfg', 'DBG') )
        
        batchlist.append( [step_3_4_build_sdebug_dmdeveloper_cert    , pathname, cfgfile, 'Developer Cert', 'Developer Cert'] )

    # build image


    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    return True


def diasemi_phase_Y20_keychian(app, pathname, cfgfile):

    diasemi_phase_15_keychain(app, pathname, cfgfile)
    
    return True


def diasemi_phase_Y21_cmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater
    return True


def diasemi_phase_Y22_image(app, pathname, cfgfile):

    diasemi_gui_imgmanager(app, pathname, cfgfile, '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './cmtpmcfg', 'BOOT')
    genscript( target_script_update(cfgfile, './cmtpmcfg', 'BOOT') )

    # build image
    batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    return True


def diasemi_phase_Y23_cmdbgcert(app, pathname, cfgfile):

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'ON')

    #diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER', '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './cmtpmcfg', 'ALL')
    genscript( target_script_update(cfgfile, './cmtpmcfg', 'ALL') )

    # build debug

    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER')

    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './cmtpmcfg', 'DBG')
        genscriptrma( target_script_update(cfgfile, './cmtpmcfg', 'DBG') )

        batchlist.append( [step_3_3_build_sdebug_cmdeveloper_cert    , pathname, cfgfile, 'Developer Cert', 'Developer Cert'] )
    
    # build image

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    return True


def diasemi_phase_Y24_keychian(app, pathname, cfgfile):

    diasemi_phase_5_keychain(app, pathname, cfgfile)

    return True


def diasemi_phase_Y25_dmrma(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sdebug = manobj.gettpmsdebug_int()

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF:RMA')
    
    if( (check_role(role, 'ALL|Enabler') == True) or (sdebug == 2 and check_role(role, 'TOP') == True) ):
        diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
        
    if( check_role(role, 'ALL|Developer') == True ):
        diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    updated = False    
    if( check_role(role, 'ALL|Developer') == True ):
        updated, socid = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')

    logfilename = getcurrlogfilename(cfgfile)
    diasemi_write_log_con(logfilename, ("Target SOC-ID:\n%s\n \n" % socid))

    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './dmtpmcfg', 'RMA')
        genscriptrma( target_script_update(cfgfile, './dmtpmcfg', 'RMA') )

        if( sdebug == 2 ):
            if( check_role(role, 'ALL|TOP|Enabler') == True ):
                batchlist.append( [step_2_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_2_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_2_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_3_6_build_sdebug_dmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_3_8_build_sdebug_dmdeveloper_rma_cert, pathname, cfgfile, 'Developer RMA Cert', 'Developer RMA Cert'] )

    # build rma
    
    batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RMA', 'RMA' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build RMA Image')    
    del batchlist[:] 
    return True


def diasemi_phase_Y26_cmrma(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    target_config_update(cfgfile, './cmtpmcfg', 'ALL')
    #genscript( target_script_update(cfgfile, './cmtpmcfg', 'ALL') )

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF:RMA')
    
    if( (check_role(role, 'ALL|Enabler') == True) or (sdebug == 2 and check_role(role, 'TOP') == True) ):
        diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')

    if( check_role(role, 'ALL|Developer') == True ):
        diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')
    
    #updated = False    
    #if( check_role(role, 'ALL|Developer') == True ):
    #    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
    updated = True

    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './cmtpmcfg', 'RMA')
        genscriptrma( target_script_update(cfgfile, './cmtpmcfg', 'RMA') )

        if( sboot == 2 ):
            #if( check_role(role, 'ALL|Publisher') == True ):
            #    batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
            if( check_role(role, 'ALL|TOP|Issuer') == True ):
                batchlist.append( [step_1_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sboot == 3 ):
            #if( check_role(role, 'ALL|Issuer') == True ):
            #    batchlist.append( [step_1_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
            #if( check_role(role, 'ALL|Publisher') == True ):
            #    batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        
            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_1_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
            if( check_role(role, 'ALL|Issuer') == True ):
                batchlist.append( [step_1_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )

        if( sdebug == 2 ):
            #if( check_role(role, 'ALL|Developer') == True ):
            #    batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
            if( check_role(role, 'ALL|TOP|Enabler') == True ):
                batchlist.append( [step_1_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            #if( check_role(role, 'ALL|Enabler') == True ):
            #    batchlist.append( [step_1_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
            #if( check_role(role, 'ALL|Developer') == True ):
            #    batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_1_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_1_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_3_5_build_sdebug_cmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_3_7_build_sdebug_cmdeveloper_rma_cert, pathname, cfgfile, 'Developer RMA Cert', 'Developer RMA Cert'] )
        
    # build rma

    batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RMA', 'RMA' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build RMA Image')    
    del batchlist[:]

    return True


def diasemi_phase_Y27_cmrma(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    target_config_update(cfgfile, './cmtpmcfg', 'ALL')
    #genscript( target_script_update(cfgfile, './cmtpmcfg', 'ALL') )

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF:RMA')

    if( (check_role(role, 'ALL|Enabler') == True) or (sdebug == 2 and check_role(role, 'TOP') == True) ):
        diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
    
    if( check_role(role, 'ALL|Developer') == True ):
        diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    updated = False    
    #if( check_role(role, 'ALL|Developer') == True ):
    #    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
    updated = True

    batchlist = []

    if( updated == True ):
        target_config_update(cfgfile, './cmtpmcfg', 'RMA')
        genscriptrma( target_script_update(cfgfile, './cmtpmcfg', 'RMA') )

        if( sboot == 2 ):
            if( check_role(role, 'ALL|Publisher') == True ):
                batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
            if( check_role(role, 'ALL|TOP|Issuer') == True ):
                batchlist.append( [step_1_B_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sboot == 3 ):
            if( check_role(role, 'ALL|Issuer') == True ):
                batchlist.append( [step_1_B_4_keygen_issuer    , pathname, cfgfile, 'Issuer', 'Issuer Key'] )
            if( check_role(role, 'ALL|Publisher') == True ):
                batchlist.append( [step_1_B_5_keygen_publisher , pathname, cfgfile, 'Publisher', 'Publisher Key'] )
        
            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_1_B_7_keychain_3lvl_isu, pathname, cfgfile, '3lvl Issuer KeyChain', '3lvl Issuer KeyChain'] )
            if( check_role(role, 'ALL|Issuer') == True ):
                batchlist.append( [step_1_B_8_keychain_3lvl_pub, pathname, cfgfile, '3lvl Publisher KeyChain', '3lvl Publisher KeyChain'] )

        if( sdebug == 2 ):
            if( check_role(role, 'ALL|Developer') == True ):
                batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
            if( check_role(role, 'ALL|TOP|Enabler') == True ):
                batchlist.append( [step_1_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
        elif( sdebug == 3 ):
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_1_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
            if( check_role(role, 'ALL|Developer') == True ):
                batchlist.append( [step_1_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

            if( check_role(role, 'ALL|TOP') == True ):
                batchlist.append( [step_1_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
            if( check_role(role, 'ALL|Enabler') == True ):
                batchlist.append( [step_1_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )

        if( check_role(role, 'ALL|Enabler') == True ):
            batchlist.append( [step_3_5_build_sdebug_cmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
        if( check_role(role, 'ALL|Developer') == True ):
            batchlist.append( [step_3_7_build_sdebug_cmdeveloper_rma_cert, pathname, cfgfile, 'Developer RMA Cert', 'Developer RMA Cert'] )
        
    # build rma

    batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RMA', 'RMA' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build RMA Image')    
    del batchlist[:]

    return True


def diasemi_phase_G17_slock(app, pathname, cfgfile):
    print("Set SLOCK")
    return True


def diasemi_phase_G21_nvcount(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    diasemi_gui_imgmanager(app, pathname, cfgfile, '')
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All CM files')

    #step_1_0_clean_all(pathname, cfgfile, cleanoption)
    return True


def diasemi_phase_G24_nvcount(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    diasemi_gui_imgmanager(app, pathname, cfgfile, '')
    
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'ENABLER-RMA', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER', '')
    diasemi_gui_manager(app, pathname, cfgfile, 'DEVELOPER-RMA', '')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater

    #cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All DM files')

    #step_2_0_clean_all(pathname, cfgfile, cleanoption)
    return True


def diasemi_phase_R9_image(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()
    imgmodel = manobj.gettpmimagemodel_int()
    bootmodel = manobj.gettpmbootmodel_int()
    
    target_config_update(cfgfile, './dmtpmcfg', 'BOOT')
    genscript( target_script_update(cfgfile, './dmtpmcfg', 'BOOT') )

    batchlist = []
    
    batchlist.append( [step_2_1_build_hbkgt, pathname, cfgfile, 'SkipDMPU', 'Gen HBK1'] )

    target_config_update(cfgfile, './dmtpmcfg', 'RMA')
    genscriptrma( target_script_update(cfgfile, './dmtpmcfg', 'RMA') )
        
    if( sdebug == 2 ):
        batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )
        batchlist.append( [step_2_D_6_keychain_2lvl    , pathname, cfgfile, '2lvl KeyChain', '2lvl KeyChain'] )
    elif( sdebug == 3 ):
        batchlist.append( [step_2_D_4_keygen_enabler   , pathname, cfgfile, 'Enabler', 'Enabler Key'] )
        batchlist.append( [step_2_D_5_keygen_developer , pathname, cfgfile, 'Developer', 'Developer Key'] )

        batchlist.append( [step_2_D_7_keychain_3lvl_enb, pathname, cfgfile, '3lvl Enabler KeyChain', '3lvl Enabler KeyChain'] )
        batchlist.append( [step_2_D_8_keychain_3lvl_dev, pathname, cfgfile, '3lvl Developer KeyChain', '3lvl Developer KeyChain'] )


    batchlist.append( [step_3_6_build_sdebug_dmenabler_rma_cert  , pathname, cfgfile, 'Enabler RMA Cert', 'Enabler RMA Cert'] )
    
    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen Key & KeyChain')
    del batchlist[:]
    # build image
    batchlist = []

    if( bootmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'UEBOOT', 'UEBOOT' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.cache', 'Cache-Boot' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'RTOS.sram', 'SRAM-Boot' ] )

    if( imgmodel == 0 ):
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.PTIM', 'PTIM' ] )
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.RaLIB', 'RaLIB' ] )
    else:
        batchlist.append( [ diasemi_image_generate, pathname, cfgfile, 'Comp.combined', 'PTIM&RaLIB' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build Secure Boot Image')
    del batchlist[:]
    return True


def diasemi_phase_R11_dmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater    
    return True


def diasemi_phase_R13_cmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater    
    return True

##########################################################
def diasemi_phase_chk_debugmode(cfgfile):

    mancfg = manconfig(cfgfile)
    mancfg.parse()

    tmode = mancfg.getrawconfig('FC9K-MAN-CONFIG','da16_test_mode')
    #tmode = mancfg.getconfig('da16_test_mode')

    if( tmode == 'DEBUG' ):
        print("diasemi_phase_chk_debugmode:DEBUG")
        tretval = True
    else:
        tretval = False

    del mancfg

    return tretval

def diasemi_phase_set_own_cm(app, pathname, cfgfile):

    mancfg = manconfig(cfgfile)
    mancfg.parse()

    #print("diasemi_phase_set_own:CM")

    owner = mancfg.getconfig('own')

    if( owner == 'DM' ):
        mancfg.setconfig('own', 'CM')
        mancfg.settpmconfig("hbk-id=hbk-id 0, CM") 
        mancfg.updatesection('ENABLER-RMA', "hbk-id=hbk-id 0, CM")
        mancfg.update()

    del mancfg

    return True

def diasemi_phase_set_own_dm(app, pathname, cfgfile):

    mancfg = manconfig(cfgfile)
    mancfg.parse()

    #print("diasemi_phase_set_own:DM")

    owner = mancfg.getconfig('own')

    if( owner == 'CM' ):
        mancfg.setconfig('own', 'DM')
        mancfg.settpmconfig("hbk-id=hbk-id 1, DM") 
        mancfg.updatesection('ENABLER-RMA', "hbk-id=hbk-id 1, DM")
        mancfg.update()

    del mancfg

    return True

def diasemi_phase_cleanlog(app, pathname, cfgfile):

    os.remove(cmkrtlkeylist[0])
    os.remove(cmkrtlkeylist[1])
    os.remove(cmkrtlkeylist[2])
 
    cwd = os.getcwd()
    os.chdir('./example/')

    os.remove(cmhbklist[1])
    os.remove(cmhbklist[2])
    os.remove(dmhbklist[1])
    os.remove(dmhbklist[2])    

    os.chdir(cwd)

    return True   

##########################################
logfilelist = [
    "\..\example\secure_production.txt",    ## Sprod
    "\..\example\key_renewal.txt",          ## KeyRe
    "\..\example\secure_debug.txt",         ## SDbg/Sboot
    "\..\example\secure_rma.txt"            ## SRma
]

IDX_SP  = 0
IDX_KR  = 1
IDX_SD  = 2
IDX_SB  = 2
IDX_SR  = 3


def diasemi_gen_socid_null():
    socid_file_name = './image/soc_id.bin'
    if os.path.isfile(socid_file_name) != True :
        bstr = struct.pack('QQQQ', 0, 0, 0, 0)
        hexfile = open(socid_file_name, 'wb')
        hexfile.write(bstr)
        hexfile.close()      


def diasemi_phase_b1_checker(app, pathname, cfgfile):

    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All Secret files')
    if( cleanoption != 'YesToAll'):
        return False

    logfilename = pathname + logfilelist[IDX_SP] #"\..\example\secure_production.txt"
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)
    #diasemi_gui_batchbuild_logfile(logfilename)

    diasemi_write_log_con(logfilename, "****************************************\n")
    diasemi_write_log_con(logfilename, "    SECURE PRODUCTION (%s)\n" % datetime.datetime.now())
    diasemi_write_log_con(logfilename, "****************************************\n")

    if os.path.isfile(cmkrtlkeylist[1]) == True :
        diasemi_write_log_con(logfilename, "KRTL Key is valid.\n")
    else:
        diasemi_write_log_con(logfilename, ("KRTL Key, %s is NOT exist.\n" % cmkrtlkeylist[1]))

        if diasemi_phase_chk_debugmode(cfgfile) != True :
            diasemi_write_log_con(logfilename, "****************************************\n")
            diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
            diasemi_write_log_con(logfilename, "****************************************\n")            
            return False

    mancfg = manconfig(cfgfile)
    mancfg.parse()

    owner = mancfg.getconfig('own')
    if( owner == 'DM' ):
        mancfg.setconfig('own', 'CM')

    mancfg.settpmconfig("lcs=lcs CM&hbk-id=hbk-id 0, CM&nvcount=0") 
    mancfg.update()

    del mancfg

    diasemi_gen_socid_null()

    return True

	
def diasemi_phase_b1_errcheck(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_SP]

    retval = diasemi_check_log_con(logfilename)    
    
    if( retval == False ):
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")
    
    return retval


def diasemi_phase_b1_cleanup(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_SP]
    diasemi_write_log_con(logfilename, " ======> Procedure has been completed successfully ...")
    return True

##########################################

def diasemi_phase_b2_checker(app, pathname, cfgfile):

    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove Some Secret files')
    if( cleanoption != 'YesToAll'):
        return False

    logfilename = pathname + logfilelist[IDX_KR]
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)
    #diasemi_gui_batchbuild_logfile(logfilename)

    diasemi_write_log_con(logfilename, "****************************************\n")
    diasemi_write_log_con(logfilename, ("    KEY RENEWAL (%s)\n" % datetime.datetime.now()))
    diasemi_write_log_con(logfilename, "****************************************\n")

    diasemi_phase_set_own_dm(app, pathname, cfgfile)

    b2checkscecret = [
        './dmsecret/dmenabler_keypair.pem',
        './dmsecret/dmissuer_keypair.pem',
        './dmsecret/dmkey_pair.pem',
        './dmsecret/pwd.dmkey.txt',
        './dmsecret/pwd.enabler.txt',
        './dmsecret/pwd.issuer.txt',
        './dmpubkey/dmenabler_pubkey.pem',
        './dmpubkey/dmissuer_pubkey.pem',
        './dmpubkey/dmpubkey.pem',
    ]

    retval = True
    for filename in b2checkscecret :
        if os.path.isfile(filename) != True :
            diasemi_write_log_con(logfilename, "%s is NOT exist\n" % filename )
            retval = False

    if diasemi_phase_chk_debugmode(cfgfile) == True :
        return True
    
    if( retval == False ):
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")         
    else:
        diasemi_gen_socid_null()

    return retval


	
def diasemi_phase_b2_errcheck(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_KR]

    retval = diasemi_check_log_con(logfilename)    
    
    if( retval == False ):
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")
    
    return retval


def diasemi_phase_b2_cleanup(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_KR]
    diasemi_write_log_con(logfilename, " ======> Procedure has been completed successfully ...")
    return True

##########################################

def diasemi_phase_b3_checker(app, pathname, cfgfile):

    logfilename = pathname + logfilelist[IDX_SD]
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)
    #diasemi_gui_batchbuild_logfile(logfilename)

    diasemi_write_log_con(logfilename, "****************************************\n")
    diasemi_write_log_con(logfilename, ("   SECURE BOOT/DEBUG Configuration (%s)\n" % datetime.datetime.now()))
    diasemi_write_log_con(logfilename, "****************************************\n")

    diasemi_phase_set_own_dm(app, pathname, cfgfile)

    b3checkscecret = [
        './dmsecret/dmdeveloper_keypair.pem',
        './dmsecret/pwd.developer.txt',
        './dmpubkey/dmdeveloper_pubkey.pem',
    ]

    retval = True
    for filename in b3checkscecret :
        if os.path.isfile(filename) != True :
            diasemi_write_log_con(logfilename, "%s is NOT exist\n" % filename )
            retval = False

    if diasemi_phase_chk_debugmode(cfgfile) == True :
        return True

    if( retval == False ):
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")         
    else:
        diasemi_gen_socid_null()

    return retval


def diasemi_phase_b5_checker(app, pathname, cfgfile):

    logfilename = pathname + logfilelist[IDX_SD]
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)
    #diasemi_gui_batchbuild_logfile(logfilename)

    diasemi_write_log_con(logfilename, "****************************************\n")
    diasemi_write_log_con(logfilename, ("   SECURE BOOT/DEBUG Configuration (%s)\n" % datetime.datetime.now()))
    diasemi_write_log_con(logfilename, "****************************************\n")

    diasemi_phase_set_own_dm(app, pathname, cfgfile)

    b3checkscecret = [
        './dmsecret/dmpublisher_keypair.pem',
        './dmsecret/pwd.publisher.txt',
        './dmpubkey/dmpublisher_pubkey.pem',
    ]

    retval = True
    for filename in b3checkscecret :
        if os.path.isfile(filename) != True :
            diasemi_write_log_con(logfilename, "%s is NOT exist\n" % filename )
            retval = False

    if diasemi_phase_chk_debugmode(cfgfile) == True :
        return True

    if( retval == False ):
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")         

    return retval


def diasemi_phase_b3_errcheck(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_SD]

    retval = diasemi_check_log_con(logfilename)    

    if( retval == False ):    
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")

    return retval


def diasemi_phase_b3_cleanup(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_SD]
    diasemi_write_log_con(logfilename, " ======> Procedure has been completed successfully ...")
    return True

##########################################

def diasemi_phase_b4_checker(app, pathname, cfgfile):

    logfilename = pathname + logfilelist[IDX_SR]
    if os.path.isfile(logfilename) == True:
        os.remove(logfilename)
    #diasemi_gui_batchbuild_logfile(logfilename)

    diasemi_write_log_con(logfilename, "****************************************\n")
    diasemi_write_log_con(logfilename, ("   SECURE RMA (%s)\n" % datetime.datetime.now()))
    diasemi_write_log_con(logfilename, "****************************************\n")

    diasemi_phase_set_own_dm(app, pathname, cfgfile)

    b4checkscecret = [
        './cmsecret/cmdeveloper_keypair.pem',
        './cmsecret/cmenabler_keypair.pem',
        './cmsecret/cmkey_pair.pem',
        './cmsecret/pwd.developer.txt',
        './cmsecret/pwd.enabler.txt',
        './cmpubkey/cmdeveloper_pubkey.pem',
        './cmpubkey/cmenabler_pubkey.pem',

        './dmsecret/dmdeveloper_keypair.pem',
        './dmsecret/dmenabler_keypair.pem',
        './dmsecret/dmkey_pair.pem',
        './dmsecret/pwd.developer.txt',
        './dmsecret/pwd.enabler.txt',
        './dmpubkey/dmdeveloper_pubkey.pem',
        './dmpubkey/dmenabler_pubkey.pem',
    ]

    retval = True
    for filename in b4checkscecret :
        if os.path.isfile(filename) != True :
            diasemi_write_log_con(logfilename, "%s is NOT exist\n" % filename )
            retval = False

    if diasemi_phase_chk_debugmode(cfgfile) == True :
        return True

    if( retval == False ):
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")         
    else:
        diasemi_gen_socid_null()

    return retval


def diasemi_phase_b4_errcheck(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_SR]

    retval = diasemi_check_log_con(logfilename)    
    
    if( retval == False ):    
        diasemi_write_log_con(logfilename, "****************************************\n")
        diasemi_write_log_con(logfilename, "    ERROR detected ...\n")
        diasemi_write_log_con(logfilename, "****************************************\n")

    return retval


def diasemi_phase_b4_cleanup(app, pathname, cfgfile):
    logfilename = pathname + logfilelist[IDX_SR]
    diasemi_write_log_con(logfilename, " ======> Procedure has been completed successfully ...")
    return True

##########################################

def diasemi_phase_b6_checker(app, pathname, cfgfile):

    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove Some Secret files')
    if( cleanoption != 'YesToAll'):
        return False

    print( "****************************************")
    print( ("   REMOVE SECRETS (%s)" % datetime.datetime.now()))
    print( "****************************************")

    rmovopt = diasemi_gui_removesecrets(app, pathname, 'Remove Secrets', "Test")

    #tfolderlist = [ "./cmsecret", "./cmpublic", "./cmpubkey" , "./dmsecret", "./dmpublic", "./dmpubkey" ]
    #
    #for curfolder in tfolderlist :
    #    for curfile in os.listdir(curfolder) :
    #        print( "\'%s/%s\'," % (curfolder, curfile) )    

    removescecret = [
        './cmsecret/cmdeveloper_keypair.pem',
        './cmsecret/cmenabler_keypair.pem',
        './cmsecret/cmissuer_keypair.pem',
        './cmsecret/cmkey_pair.pem',
        './cmsecret/cmpublisher_keypair.pem',
        './cmsecret/kceicv.bin',
        './cmsecret/kpicv.bin',
        './cmsecret/pwd.cmkey.txt',
        './cmsecret/pwd.developer.txt',
        './cmsecret/pwd.enabler.txt',
        './cmsecret/pwd.issuer.txt',
        './cmsecret/pwd.kceicv.txt',
        './cmsecret/pwd.kpicv.txt',
        './cmsecret/pwd.publisher.txt',
        './cmpublic/enc.kceicv.bin',
        './cmpublic/enc.kpicv.bin',
        './cmpublic/sboot_hbk0_3lvl_key_chain_issuer.bin',
        './cmpublic/sboot_hbk0_3lvl_key_chain_issuer_Cert.txt',
        './cmpublic/sboot_hbk0_3lvl_key_chain_publisher.bin',
        './cmpublic/sboot_hbk0_3lvl_key_chain_publisher_Cert.txt',
        './cmpublic/sdebug_hbk0_3lvl_key_chain_developer.bin',
        './cmpublic/sdebug_hbk0_3lvl_key_chain_developer_Cert.txt',
        './cmpublic/sdebug_hbk0_3lvl_key_chain_enabler.bin',
        './cmpublic/sdebug_hbk0_3lvl_key_chain_enabler_Cert.txt',
        './cmpublic/sdebug_hbk0_developer_rma_pkg.bin',
        './cmpublic/sdebug_hbk0_enabler_rma_pkg.bin',
        './cmpubkey/cmdeveloper_pubkey.pem',
        './cmpubkey/cmenabler_pubkey.pem',
        './cmpubkey/cmissuer_pubkey.pem',
        './cmpubkey/cmpubkey.pem',
        './cmpubkey/cmpublisher_pubkey.pem',

        './dmsecret/kce.bin',
        './dmsecret/kcp.bin',
        './dmsecret/pwd.kce.txt',
        './dmsecret/pwd.kcp.txt',
        './dmpublic/enc.kce.bin',
        './dmpublic/enc.kcp.bin',

        './dmsecret/dmenabler_keypair.pem',
        './dmsecret/dmissuer_keypair.pem',
        './dmsecret/dmkey_pair.pem',
        './dmsecret/oem_keyreq_key_pair.pem',
        './dmsecret/pwd.dmkey.txt',
        './dmsecret/pwd.enabler.txt',
        './dmsecret/pwd.issuer.txt',
        './dmsecret/pwd.oem_keyreq.txt',
        './dmpublic/sdebug_hbk1_developer_rma_pkg.bin',
        './dmpublic/sdebug_hbk1_enabler_rma_pkg.bin',
        './dmpubkey/dmenabler_pubkey.pem',
        './dmpubkey/dmissuer_pubkey.pem',
        './dmpubkey/dmpubkey.pem',
        './dmpubkey/oem_keyreq_pubkey.pem'
    ]

    for filename in removescecret :
        if os.path.isfile(filename) == True :
            print( "remove %s" % filename )
            os.remove( filename )

    if rmovopt == "SB Publisher" :
        sd_removescecret = [ 
            './dmsecret/dmdeveloper_keypair.pem',
            './dmsecret/pwd.developer.txt',
            './dmpubkey/dmdeveloper_pubkey.pem',
            './dmpublic/sdebug_hbk1_3lvl_key_chain_developer.bin',
            './dmpublic/sdebug_hbk1_3lvl_key_chain_developer_Cert.txt',
            './dmpublic/sdebug_hbk1_3lvl_key_chain_enabler.bin',
            './dmpublic/sdebug_hbk1_3lvl_key_chain_enabler_Cert.txt',
            './dmpublic/sdebug_hbk1_developer_pkg.bin',
            './dmpublic/sdebug_hbk1_enabler_pkg.bin'
        ]

        for filename in sd_removescecret :
            if os.path.isfile(filename) == True :
                print( "remove %s" % filename )
                os.remove( filename )        
 
    exampledir = './example'
    for curfile in os.listdir(exampledir) :
        filename = '%s/%s' % (exampledir, curfile)
        if os.path.isfile(filename) == True and filename.endswith('.log') == True :
            os.remove( filename )

    return True


def diasemi_phase_b6_cleanup(app, pathname, cfgfile):
    print( " ======> Procedure has been completed successfully ...")
    return True

##########################################################
	
diasemi_new_tpm_rule_dict = {

    ##### Green Boot
    'Green  Boot:CM:G.B1' : [ diasemi_phase_b1_checker,
							diasemi_phase_1_config, diasemi_phase_4_cmpu, diasemi_phase_b1_errcheck,
                            diasemi_phase_3_cmrma, diasemi_phase_b1_errcheck,
                            diasemi_phase_set_own_dm,
							diasemi_phase_8_config, diasemi_phase_7_dmrma, diasemi_phase_b1_errcheck, ## diasemi_phase_9_image, 
                            ##### diasemi_phase_set_own_dm, diasemi_phase_set_temp_dmkey, diasemi_phase_10_dmrma, 
                            diasemi_phase_set_own_cm, diasemi_phase_Y27_cmrma, diasemi_phase_b1_errcheck,
                            diasemi_phase_set_own_dm, diasemi_phase_11_build_dmpu, diasemi_phase_b1_errcheck, ## diasemi_phase_G17_slock, 
							diasemi_phase_Y17_dmcfg, diasemi_phase_Y20_keychian, diasemi_phase_b1_errcheck,
                            diasemi_phase_Y18_image, 
                            diasemi_phase_cleanlog, diasemi_phase_b1_cleanup
							],
    'Green  Boot:DM:G.B1' : [ diasemi_phase_b1_checker,
							diasemi_phase_1_config, diasemi_phase_4_cmpu, diasemi_phase_b1_errcheck,
                            diasemi_phase_3_cmrma, diasemi_phase_b1_errcheck,
                            diasemi_phase_set_own_dm,
							diasemi_phase_8_config, diasemi_phase_7_dmrma, diasemi_phase_b1_errcheck, ## diasemi_phase_9_image, 
                            ##### diasemi_phase_set_own_dm, diasemi_phase_set_temp_dmkey, diasemi_phase_10_dmrma, 
                            diasemi_phase_set_own_cm, diasemi_phase_Y27_cmrma, diasemi_phase_b1_errcheck,
                            diasemi_phase_set_own_dm, diasemi_phase_11_build_dmpu, diasemi_phase_b1_errcheck, ## diasemi_phase_G17_slock, 
							diasemi_phase_Y17_dmcfg, diasemi_phase_Y20_keychian, diasemi_phase_b1_errcheck,
                            diasemi_phase_Y18_image, 
                            diasemi_phase_cleanlog, diasemi_phase_b1_cleanup
							],                           
    
    'Green  Boot:DM:G.B2': [ diasemi_phase_b2_checker,
                             diasemi_phase_set_own_dm,
							 diasemi_phase_G21_nvcount , diasemi_phase_Y20_keychian,
                             diasemi_phase_b2_errcheck , diasemi_phase_Y18_image,
                             diasemi_phase_b2_cleanup
                            ],

    'Green  Boot:DM:G.B3': [ diasemi_phase_b3_checker,
                             diasemi_phase_set_own_dm,
							 diasemi_phase_Y19_dmdbgcert,
                             diasemi_phase_b3_errcheck,
                             diasemi_phase_b3_cleanup
                            ],

    'Green  Boot:DM:G.B4': [ diasemi_phase_b4_checker,
							 diasemi_phase_Y25_dmrma, diasemi_phase_b4_errcheck,
                             diasemi_phase_set_own_cm, diasemi_phase_Y26_cmrma, 
                             diasemi_phase_set_own_dm, diasemi_phase_b4_errcheck,
                             diasemi_phase_b4_cleanup
                            ], # Secure

    'Green  Boot:DM:G.B5': [ diasemi_phase_b5_checker,
                             diasemi_phase_set_own_dm,
							 diasemi_phase_Y17_dmcfg,
                             diasemi_phase_Y18_image,   #
                             diasemi_phase_b3_errcheck, #
                             diasemi_phase_b3_cleanup
                            ],     

    'Green  Boot:DM:G.B6': [ diasemi_phase_b6_checker,
                             diasemi_phase_b6_cleanup
							],                                                      
}

##########################################################

def module_secuman():

    man_cfgfile = ""
    
    if "-cfg" in sys.argv:
        man_cfgfile = sys.argv[sys.argv.index("-cfg") + 1]
    else:
        print("not found config file")
        exit(1)

    flag_ownfix = 0

    print("CFG file is %s" % man_cfgfile )

    # Default, Fusion, Imagine, Material, Universal
    sys.argv += ['--style', 'Material']
	
    app = QGuiApplication(sys.argv)
    pathname, fname = os.path.split(sys.argv[0])

    while True:
        currtext, currcfg = diasemi_gui_tpm_manager(app, pathname, man_cfgfile, flag_ownfix)

        if( currtext == '' ):
            break

        if( diasemi_new_tpm_rule_dict.get(currcfg) != None ):
            phaselist = diasemi_new_tpm_rule_dict[currcfg]

            for idx, phaseitem in enumerate(phaselist):
                #print("%s() : " % phaseitem.__name__ )
 
                if idx == 0 :
                    retval = phaseitem(app, pathname, man_cfgfile)
                else:
                    retval = phaseitem(app, pathname, man_cfgfile)

                if retval == False :
                    break
    
            if retval == False :
                print("\n ======> Procedure has been stopped ...")
 
        else:
            print('%s None' % currcfg )

    del app
    
##########################################################
# main
##########################################################

if __name__ == "__main__":

    print( 'da16secutool.py start : %s\n' % datetime.datetime.now() )

    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    if "-mod" in sys.argv:
        module_name = sys.argv[sys.argv.index("-mod") + 1]
    else:
        print("USAGE: secutool -mod [secuasset|secudebug|secuboot] ...")
        exit(1)

    DisableWinQuickEditMode()

    if(	module_name == 'secuasset'):
        module_secuasset()
    elif( module_name == 'secudebug'):
        module_secudebug()
    elif( module_name == 'secuboot'):
        module_secuboot()
    elif( module_name == 'secuman'):
        module_secuman()
    elif( module_name == 'quickedit'):
        print("")
    else:
        print("USAGE: secutool -mod [secuasset|secudebug|secuboot] ...")

    print( 'da16secutool.py end : %s\n' % datetime.datetime.now() )
