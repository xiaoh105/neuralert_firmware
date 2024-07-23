#!/bin/python3

import sys
import os
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

    #print("CFG file is %s" % man_cfgfile )

    # Default, Fusion, Imagine, Material, Universal
    sys.argv += ['--style', 'Material']
    
    app = QGuiApplication(sys.argv)

    assetman = front_assetman(man_cfgfile)
    assetman.SetWinTitle('Dialog Security Tool : Secure Asset')

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("assetman", assetman)

    pathname, fname = os.path.split(sys.argv[0])
    
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
            print('CM ASSET generate')
            genASSET(pathname, assetprovutil, 'CM', man_cfgfile, pkgfilename)
        else:
            print('DM ASSET generate')
            genASSET(pathname, assetprovutil, 'DM', man_cfgfile, pkgfilename)
    
    
    del engine
    del assetman 

        
