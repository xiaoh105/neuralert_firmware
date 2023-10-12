#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct

from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QUrl, QStringListModel
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine, qmlRegisterType

from fc9ksecuutil import *
from fc9kfuncutil import *
from fc9kfrontutil import *
from fc9kgenutil import *
from fc9kimgutil import *

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

    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All CM files')

    if( cleanoption == 'YesToAll' ):
        #step_1_0_clean_all(pathname, cfgfile, cleanoption)
        batchlist = [[step_1_0_clean_all, pathname, cfgfile, cleanoption, 'Clean CM CFGs']]
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Clean ALL')

    return


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
    
    return
    

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
    if( check_role(role, 'ALL|Developer') == True ):
        updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
        
    batchlist = []

    if( updated == True ):
        target_config_update(man_cfgfile, './cmtpmcfg', 'RMA')
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

    return


def diasemi_phase_4_cmpu(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen CMPU', ('\'%s\' is not authorized to process this action.' % role))
        return
    
    cleanoption = diasemi_gui_messagebox(app, pathname, 'Re-Gen HBK0 & CMPU')

    if( cleanoption == 'YesToAll' ):
        diasemi_gui_xmpu_manager(app, pathname, cfgfile, 'CMPU')

        batchlist = []

        batchlist.append( [step_1_1_build_cmpu , pathname, cfgfile, cleanoption, 'CMPU'] )

        batchlist.append( [step_1_1_build_cmpu_cleanup , pathname, cfgfile, cleanoption, 'Clean'] )
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen CMPU')
        del batchlist[:]
        
    return


def diasemi_phase_5_keychain(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    actrule = diasemi_gui_key_manager(app, pathname, cfgfile)

    if( actrule == '' ):
        return
    
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
        
    return


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

    return


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
    if( check_role(role, 'ALL|Developer') == True ):
        updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
        
    batchlist = []

    target_config_update(man_cfgfile, './dmtpmcfg', 'RMA')
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
    return


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

    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove All DM files')

    if( cleanoption == 'YesToAll' ):
        #step_2_0_clean_all(pathname, cfgfile, cleanoption)
        batchlist = [[step_2_0_clean_all, pathname, cfgfile, cleanoption, ' Clean DM CFGs']]
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Clean ALL')

    return


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
    return


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
    if( check_role(role, 'ALL|Developer') == True ):
        updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')
        
    batchlist = []

    if( updated == True ):
        target_config_update(man_cfgfile, './dmtpmcfg', 'RMA')
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
    return


def diasemi_phase_11_hbkgt(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Build HBK1', ('\'%s\' is not authorized to process this action.' % role))
        return    
    
    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove HBK1')

    batchlist = []

    batchlist.append( [ step_2_1_build_hbkgt, pathname, cfgfile, cleanoption, 'HBKGT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build HBK1')    
    del batchlist[:] 
    return


def diasemi_phase_12_okrpt(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Build OemKeyRequestPkg', ('\'%s\' is not authorized to process this action.' % role))
        return
    
    batchlist = []

    batchlist.append( [ step_2_2_build_okrpt, pathname, cfgfile, 'OKRPT', 'OKRPT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build OemKeyRequestPkg')    
    del batchlist[:] 
    return


def diasemi_phase_13_ikrpt(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Build IcvKeyResponsePkg', ('\'%s\' is not authorized to process this action.' % role))
        return
    
    batchlist = []

    batchlist.append( [ step_2_3_build_ikrpt, pathname, cfgfile, 'IKRPT', 'IKRPT' ] )

    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Build IcvKeyResponsePkg')    
    del batchlist[:] 
    return


def diasemi_phase_14_dmpu(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen DMPU', ('\'%s\' is not authorized to process this action.' % role))
        return
    
    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove DMPU')

    if( cleanoption == 'YesToAll' ):
        diasemi_gui_xmpu_manager(app, pathname, cfgfile, 'DMPU')
        
        batchlist = []

        batchlist.append( [step_2_4_build_dmpu , pathname, cfgfile, cleanoption, 'DMPU'] )

        batchlist.append( [step_2_4_build_dmpu_cleanup , pathname, cfgfile, cleanoption, 'DMPU'] )
        
        diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Gen DMPU')
        del batchlist[:]
        
    return

#######

def diasemi_phase_11_build_dmpu(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')

    if( check_role(role, 'ALL|TOP') != True ):
        diasemi_gui_warnbox(app, pathname, 'Gen DMPU', ('\'%s\' is not authorized to process this action.' % role))
        return    
    
    cleanoption = diasemi_gui_messagebox(app, pathname, 'Remove DMPU')

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
        
    return


#######

def diasemi_phase_15_keychain(app, pathname, cfgfile):
    manobj = manconfig(cfgfile)
    manobj.parse()

    role = manobj.getconfig('role')
    
    sboot = manobj.gettpmsboot_int()
    sdebug = manobj.gettpmsdebug_int()

    actrule = diasemi_gui_key_manager(app, pathname, cfgfile)

    if( actrule == '' ):
        return
    
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
    
    return


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
    return


def diasemi_phase_Y17_dmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater
    return


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
    return


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

    updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER')

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
    return


def diasemi_phase_Y20_keychian(app, pathname, cfgfile):

    diasemi_phase_15_keychain(app, pathname, cfgfile)
    
    return


def diasemi_phase_Y21_cmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs SECURE')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater
    return


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
    return


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
    return


def diasemi_phase_Y24_keychian(app, pathname, cfgfile):

    diasemi_phase_5_keychain(app, pathname, cfgfile)

    return


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
        updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')

    batchlist = []

    if( updated == True ):
        target_config_update(man_cfgfile, './dmtpmcfg', 'RMA')
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
    return


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
    
    updated = False    
    if( check_role(role, 'ALL|Developer') == True ):
        updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')

    batchlist = []

    if( updated == True ):
        target_config_update(man_cfgfile, './cmtpmcfg', 'RMA')
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

    return


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
    if( check_role(role, 'ALL|Developer') == True ):
        updated = diasemi_gui_sdbg_manager(app, pathname, cfgfile, 'DEVELOPER-RMA')

    batchlist = []

    if( updated == True ):
        target_config_update(man_cfgfile, './cmtpmcfg', 'RMA')
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

    return


def diasemi_phase_G17_slock(app, pathname, cfgfile):
    print("Set SLOCK")
    return


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
    return


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
    return


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

    target_config_update(man_cfgfile, './dmtpmcfg', 'RMA')
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
    return


def diasemi_phase_R11_dmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater    
    return


def diasemi_phase_R13_cmcfg(app, pathname, cfgfile):
    diasemi_gui_manager(app, pathname, cfgfile, 'TPMCONFIG', 'lcs DM')

    diasemi_gui_imgmanager(app, pathname, cfgfile, 'OFF')

    nvupdater = nvcountupdate(cfgfile)
    nvupdater.updatecfgAll()
    del nvupdater    
    return

##########################################################

diasemi_tpm_rule_dict = {
    ##### Yellow Boot
    'Yellow Boot:CM:Y.1' : [diasemi_phase_1_config],
    'Yellow Boot:CM:Y.2' : [diasemi_phase_4_cmpu], 
    'Yellow Boot:CM:Y.3' : [diasemi_phase_3_cmrma],   # CM
    'Yellow Boot:DM:Y.4' : [diasemi_phase_7_dmrma],   # DM

    'Yellow Boot:DM:Y.5' : [diasemi_phase_8_config],  # DM 
    'Yellow Boot:DM:Y.6' : [diasemi_phase_9_image], 
    'Yellow Boot:DM:Y.7' : [diasemi_phase_10_dmrma],  # DM
    'Yellow Boot:DM:Y.8' : [diasemi_phase_11_build_dmpu],

    'Yellow Boot:DM:Y.9' : [diasemi_phase_Y17_dmcfg], # Secure
    'Yellow Boot:DM:Y.10': [diasemi_phase_Y18_image],
    'Yellow Boot:DM:Y.11': [diasemi_phase_Y19_dmdbgcert],
    'Yellow Boot:DM:Y.12': [diasemi_phase_Y20_keychian],

    'Yellow Boot:DM:Y.13': [diasemi_phase_Y25_dmrma], # Secure

    'Yellow Boot:CM:Y.14': [diasemi_phase_Y26_cmrma], # Secure
    'Yellow Boot:CM:Y.15': [diasemi_phase_Y27_cmrma], # DM

    ##### Green Boot
    'Green  Boot:CM:G.1' : [diasemi_phase_1_config],
    'Green  Boot:CM:G.2' : [diasemi_phase_4_cmpu], 
    'Green  Boot:CM:G.3' : [diasemi_phase_3_cmrma],   # CM
    'Green  Boot:DM:G.4' : [diasemi_phase_7_dmrma],   # DM

    'Green  Boot:DM:G.5' : [diasemi_phase_8_config],  # DM 
    'Green  Boot:DM:G.6' : [diasemi_phase_9_image], 
    'Green  Boot:DM:G.7' : [diasemi_phase_10_dmrma],  # DM
    'Green  Boot:DM:G.8' : [diasemi_phase_11_build_dmpu],

    'Green  Boot:DM:G.9' : [diasemi_phase_G17_slock ],
    
    'Green  Boot:DM:G.10': [diasemi_phase_Y17_dmcfg], # Secure
    'Green  Boot:DM:G.11': [diasemi_phase_Y18_image],
    'Green  Boot:DM:G.12': [diasemi_phase_Y19_dmdbgcert],
    'Green  Boot:DM:G.13': [diasemi_phase_G21_nvcount ],
    'Green  Boot:DM:G.14': [diasemi_phase_Y20_keychian],

    'Green  Boot:DM:G.15': [diasemi_phase_Y25_dmrma], # Secure

    'Green  Boot:CM:G.16': [diasemi_phase_Y26_cmrma], # Secure
    'Green  Boot:CM:G.17': [diasemi_phase_Y27_cmrma], # DM

    ##### Red Boot
    'Red    Boot:CM:R.1' : [diasemi_phase_1_config],
    'Red    Boot:CM:R.2' : [diasemi_phase_4_cmpu],
    'Red    Boot:CM:R.3' : [diasemi_phase_3_cmrma],   # CM
    'Red    Boot:DM:R.4' : [diasemi_phase_7_dmrma],   # DM

    'Red    Boot:DM:R.5' : [diasemi_phase_8_config],  # DM 
    'Red    Boot:DM:R.6' : [diasemi_phase_9_image],     

    'Red    Boot:DM:R.7' : [diasemi_phase_10_dmrma],  # DM

    'Red    Boot:CM:R.8' : [diasemi_phase_Y27_cmrma], # DM
}

##########################################################

if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

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

        if( diasemi_tpm_rule_dict.get(currcfg) != None ):
            phaselist = diasemi_tpm_rule_dict[currcfg]

            for phaseitem in phaselist:
                phaseitem(app, pathname, man_cfgfile)
        else:
            print('%s None' % currcfg )
    
