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

    updated = diasemi_gui_sdbg_manager(app, pathname, man_cfgfile, 'DEVELOPER')
	
    if( updated == True ):
        if( man_tpmdir == './cmtpmcfg' ):
            target_config_update(man_cfgfile, man_tpmdir, 'DBG')
            step_3_3_build_sdebug_cmdeveloper_cert(pathname, man_cfgfile, 'Developer Cert')
        else:
            if( rule != 'Red    Boot' or own != 'DM' ):
                target_config_update(man_cfgfile, man_tpmdir, 'DBG')
                step_3_4_build_sdebug_dmdeveloper_cert(pathname, man_cfgfile, 'Developer Cert')
            
    #######################################################

    updated = diasemi_gui_sdbg_manager(app, pathname, man_cfgfile, 'DEVELOPER-RMA')    
	
    if( updated == True ):
        if( man_tpmdir == './cmtpmcfg' ):
            target_config_update(man_cfgfile, man_tpmdir, 'RMA')
            step_3_7_build_sdebug_cmdeveloper_rma_cert(pathname, man_cfgfile, 'Developer RMA Cert')
        else:
            target_config_update(man_cfgfile, man_tpmdir, 'RMA')
            step_3_8_build_sdebug_dmdeveloper_rma_cert(pathname, man_cfgfile, 'Developer RMA Cert')

        
    #######################################################
    
