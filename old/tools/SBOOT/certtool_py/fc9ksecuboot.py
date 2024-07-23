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

from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QUrl, QStringListModel
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine, qmlRegisterType

from fc9ksecuutil import *
from fc9kfuncutil import *
from fc9kfrontutil import *
from fc9kgenutil import *
from fc9kimgutil import *

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

    if( hbkid == 1 ):
        man_tpmdir = './dmtpmcfg'
    elif( hbkid == 0 ):
        man_tpmdir = './cmtpmcfg'
    
    if( skip_cfg_update == 0 ):
        target_config_update(man_cfgfile, man_tpmdir, 'BOOT')

	
    # Default, Fusion, Imagine, Material, Universal
    batchlist = [
        [ diasemi_image_generate, pathname, man_cfgfile, 'UEBOOT'      , 'UEBOOT' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.cache'  , 'Cache-Boot' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'RTOS.sram'   , 'SRAM-Boot' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.combined', 'PTIM&RaLIB' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.PTIM'   , 'PTIM' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'Comp.RaLIB'  , 'RaLIB' ],
        [ diasemi_image_generate, pathname, man_cfgfile, 'RMA'         , 'RMA' ]
    ]

    if( man_gui == True ):
        sys.argv += ['--style', 'Material']

        app = QGuiApplication(sys.argv)
        pathname, fname = os.path.split(sys.argv[0])
        
        diasemi_gui_imagebuild(app, pathname, man_cfgfile, batchlist)
    else:
        
        for idx, batchitem in enumerate(batchlist):
            title, out, err = batchitem[0](batchitem[1], batchitem[2], batchitem[3])
            print(title)
            print( out.decode("ASCII") )
            print( err.decode("ASCII") )

