#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct

from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QUrl, QStringListModel, pyqtSlot
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine, qmlRegisterType


from fc9ksecuutil import *
from fc9kimgutil import *

##########################################################

def getcurrlogfilename(cfgfile):

    mancfg = manconfig(cfgfile)
    mancfg.parse()
    curstep = mancfg.getconfig('step')
    pathname = os.path.dirname(os.path.abspath(sys.argv[0]))

    if   curstep == 'G.B1':
        logfilename = pathname+"\..\example\secure_production.txt"
    elif curstep == 'G.B2':
        logfilename = pathname+"\..\example\key_renewal.txt"
    elif curstep == 'G.B3':
        logfilename = pathname+"\..\example\secure_debug.txt"
    elif curstep == 'G.B4':
        logfilename = pathname+"\..\example\secure_rma.txt"
    elif curstep == 'G.B5':
        logfilename = pathname+"\..\example\secure_debug.txt"
    else:
        logfilename = ""

    del mancfg

    return logfilename

##########################################################

class front_msgman(QObject):
    textChanged = pyqtSignal(str)

    def __init__(self, parent=None):
        QObject.__init__(self, parent)
        self.m_text = ""
        self.m_title = ""
        self.m_detail = ""

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal(str)
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)

    def SetWinDetail(self, detail):
        self.m_detail = detail
    
    def GetWinDetail(self):
        return self.m_detail

    on_detail = pyqtSignal(str)
    detail = pyqtProperty(str, GetWinDetail, notify=on_detail)


    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text
        
    @text.setter
    def text(self, text):
        self.m_text = text

    def gettext(self):
        return self.m_text

##########################################################

class front_rmovman(QObject):
    textChanged = pyqtSignal(str)

    def __init__(self, parent=None):
        QObject.__init__(self, parent)
        self.m_text = ""
        self.m_title = ""
        self.m_detail = ""

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal(str)
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text
        
    @text.setter
    def text(self, text):
        self.m_text = text

    def gettext(self):
        return self.m_text

##########################################################

class front_secuman(QObject):
    cfgfile = ""
    textChanged = pyqtSignal(str)
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)
        self.m_text = ""
        self.cfgfile = cfgfile

        self.flag_ownfix = 0

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.manobj.update()

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal(str)
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)    


    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text
        
    @text.setter
    def text(self, text):
        if( (text != '') and (self.m_text != text) ):
            self.m_text = text
            items = self.m_text.split('&')
            for item in items :
                key, value = item.split('=')
                self.manobj.setconfig(key, value)
            self.manobj.update()

    def gettext(self):
        return self.m_text
		
    def gettextfield(self):
        return ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('role') + ':' + self.manobj.getconfig('step') )

    on_textfield = pyqtSignal()
    textfield = pyqtProperty(str, gettextfield, notify=on_textfield)

    def getimagefolder(self):
        return ( self.manobj.getconfig('image') )

    on_imagefolder = pyqtSignal()
    imagefolder = pyqtProperty(str, getimagefolder, notify=on_imagefolder)

    def gettextmessage(self):
        return ( datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S') )

    on_textmessage = pyqtSignal()
    message = pyqtProperty(str, gettextmessage, notify=on_textmessage)

    def getscenario(self):
        return ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('step') )

    def setownfix(self,flag):
        self.flag_ownfix = flag

    def getownfix(self):
        return self.flag_ownfix

    on_getownfix = pyqtSignal()
    ownfix = pyqtProperty(int, getownfix, notify=on_getownfix)

    
##########################################################
    
class front_cfgman(QObject):
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.m_config = ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('step') )

        self.m_text = self.manobj.gettpmconfig()
        self.m_newlcs = ''

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)   


    textChanged = pyqtSignal(str)

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.manobj.settpmconfig(text)
        self.manobj.update()


    def getimagemodel(self):
        return self.manobj.gettpmimagemodel()

    def getbootmodel(self):
        return self.manobj.gettpmbootmodel()
        

    def gettextfield(self):
        return ( self.m_text )

    def _get_config(self):
        return self.m_config

    on_textfield = pyqtSignal()
    textfield = pyqtProperty(str, gettextfield, notify=on_textfield)
    
    on_config = pyqtSignal()
    config = pyqtProperty(str, _get_config, notify=on_config)

    def setnewlcs(self, newlcs):
        self.m_newlcs = newlcs
        
    def getnewlcs(self):
        return self.m_newlcs
    
    on_newlcs = pyqtSignal()
    newlcs = pyqtProperty(str, getnewlcs, notify=on_newlcs)


##########################################################
    
class front_imgman(QObject):
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.m_config = ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('step') )

        self.m_text = self.manobj.gettpmconfig()

        self.m_dbgcert = ''

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)   


    def setsection(self, secname):
        self._section = secname


    textChanged = pyqtSignal(str)

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.manobj.updatesection(self._section, self.m_text)
        self.manobj.update()


    def gettextfield(self):
        return ( self.m_text )

    def _get_config(self):
        return self.m_config

    def gettitle(self):
        return self.manobj.getrawconfig(self._section, 'TITLE')

    def getsfdp(self):
        return self.manobj.getrawconfig(self._section, 'SFDP')

    def getkeyload(self):
        return self.manobj.getrawconfig(self._section, 'KEYLOAD')

    def getkeyenc(self):
        return self.manobj.getrawconfig(self._section, 'KEYENC')

    def getflashaddr(self):
        return self.manobj.getrawconfig(self._section, 'FLASHADDR')

    def getretmem(self):
        return self.manobj.getrawconfig(self._section, 'RETMEM')

    def getcrcon(self):
        return self.manobj.getrawconfig(self._section, 'CRCON')

    def getdbgcert(self):
        if( self.m_dbgcert != '' ):
            return self.m_dbgcert
        return self.manobj.getrawconfig(self._section, 'DBGCERT')

    def getcomp1(self):
        return self.manobj.getrawconfig(self._section, 'COMP1')

    def getcomp1addr(self):
        return self.manobj.getrawconfig(self._section, 'COMP1ADDR')

    def getcomp2(self):
        return self.manobj.getrawconfig(self._section, 'COMP2')

    def getcomp2addr(self):
        return self.manobj.getrawconfig(self._section, 'COMP2ADDR')

    def getcomp3(self):
        return self.manobj.getrawconfig(self._section, 'COMP3')

    def getcomp3addr(self):
        return self.manobj.getrawconfig(self._section, 'COMP3ADDR')

    def getimage(self):
        return self.manobj.getrawconfig(self._section, 'IMAGE')

    def getdateon(self):
        return self.manobj.getrawconfig(self._section, 'TAGDATE')

    def getsandbox(self):
        return self.manobj.getrawconfig(self._section, 'SBOX')

    def setsfdpindex(self, index):
        self.m_sfdpindex = index

    def getsfdpindex(self):
        return self.m_sfdpindex

    def forcedbgcert(self, dbgcert):
        self.m_dbgcert = dbgcert

    on_textfield = pyqtSignal()
    textfield = pyqtProperty(str, gettextfield, notify=on_textfield)
    
    on_config = pyqtSignal()
    config = pyqtProperty(str, _get_config, notify=on_config)

    on_title = pyqtSignal()
    title = pyqtProperty(str, gettitle, notify=on_title)

    on_sfdp = pyqtSignal()
    sfdp = pyqtProperty(str, getsfdp, notify=on_sfdp)

    on_keyload = pyqtSignal()
    keyload = pyqtProperty(str, getkeyload, notify=on_keyload)

    on_keyenc = pyqtSignal()
    keyenc = pyqtProperty(str, getkeyenc, notify=on_keyenc)

    on_flashaddr = pyqtSignal()
    flashaddr = pyqtProperty(str, getflashaddr, notify=on_flashaddr)

    on_retmem = pyqtSignal()
    retmem = pyqtProperty(str, getretmem, notify=on_retmem)

    on_crcon = pyqtSignal()
    crcon = pyqtProperty(str, getcrcon, notify=on_crcon)

    on_dbgcert = pyqtSignal()
    dbgcert = pyqtProperty(str, getdbgcert, notify=on_dbgcert)

    on_comp1 = pyqtSignal()
    comp1 = pyqtProperty(str, getcomp1, notify=on_comp1)

    on_comp1addr = pyqtSignal()
    comp1addr = pyqtProperty(str, getcomp1addr, notify=on_comp1addr)

    on_comp2 = pyqtSignal()
    comp2 = pyqtProperty(str, getcomp2, notify=on_comp2)

    on_comp2addr = pyqtSignal()
    comp2addr = pyqtProperty(str, getcomp2addr, notify=on_comp2addr)

    on_comp3 = pyqtSignal()
    comp3 = pyqtProperty(str, getcomp3, notify=on_comp3)

    on_comp3addr = pyqtSignal()
    comp3addr = pyqtProperty(str, getcomp3addr, notify=on_comp3addr)

    on_image = pyqtSignal()
    image = pyqtProperty(str, getimage, notify=on_image)

    on_sfdpindex = pyqtSignal()
    sfdpindex = pyqtProperty(int, getsfdpindex, notify=on_sfdpindex)

    on_dateon = pyqtSignal()
    dateon = pyqtProperty(str, getdateon, notify=on_dateon)

    on_sandbox = pyqtSignal()
    sandbox = pyqtProperty(str, getsandbox, notify=on_sandbox)

##########################################################
    
class front_sfdpman:

    def __init__(self):
        self.sfdplist = []

    def setpath(self, pathname):
        self._path = pathname

    def gensfdplist(self):
        for filename in os.listdir(('%s/../SFDP' % self._path)):
            fname, fext = os.path.splitext(filename)
            if( fext == '.bin' ):
                self.sfdplist.append(fname)
        self.sfdplist.sort()
        self.sfdplist.insert(0, 'No-SFDP')
        return self.sfdplist

    def findsfdplist(self, flashname):
        for idx, item in enumerate(self.sfdplist):
            if( item == flashname ):
                self.m_index = idx
                return idx
        self.m_index = 0
        return 0

##########################################################

class front_cmpuman(QObject):
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile
        self._section = 'CMPU-PKG-CFG'
        self.cfgobj = configman(self._section, self.cfgfile)
        self.m_text = self.cfgobj.loadoptions()

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle) 


    def gethbkdata(self):
        return self.cfgobj.getoption('hbk-data')
        
    def getdebugmask0(self):
        return self.cfgobj.getoption('debug-mask[0-31]')

    def getdebugmask1(self):
        return self.cfgobj.getoption('debug-mask[32-63]')

    def getdebugmask2(self):
        return self.cfgobj.getoption('debug-mask[64-95]')

    def getdebugmask3(self):
        return self.cfgobj.getoption('debug-mask[96-127]')

    def getkpicvpkg(self):
        return self.cfgobj.getoption('kpicv-pkg')

    def getkceicvpkg(self):
        return self.cfgobj.getoption('kceicv-pkg')

    def getminversion(self):
        return self.cfgobj.getoption('minversion')

    def getconfigword(self):
        return self.cfgobj.getoption('configword')

    def getcmpupkg(self):
        return self.cfgobj.getoption('cmpu-pkg')

    textChanged = pyqtSignal()

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.cfgobj.updateoptions(self.m_text)
        self.cfgobj.update()


    on_hbkdata = pyqtSignal()
    hbkdata = pyqtProperty(str, gethbkdata, notify=on_hbkdata)
    
    on_debugmask0 = pyqtSignal()
    debugmask0 = pyqtProperty(str, getdebugmask0, notify=on_debugmask0)

    on_debugmask1 = pyqtSignal()
    debugmask1 = pyqtProperty(str, getdebugmask1, notify=on_debugmask1)

    on_debugmask2 = pyqtSignal()
    debugmask2 = pyqtProperty(str, getdebugmask2, notify=on_debugmask2)

    on_debugmask3 = pyqtSignal()
    debugmask3 = pyqtProperty(str, getdebugmask3, notify=on_debugmask3)

    on_kpicvpkg = pyqtSignal()
    kpicvpkg = pyqtProperty(str, getkpicvpkg, notify=on_kpicvpkg)
    
    on_kceicvpkg = pyqtSignal()
    kceicvpkg = pyqtProperty(str, getkceicvpkg, notify=on_kceicvpkg)

    on_minversion = pyqtSignal()
    minversion = pyqtProperty(str, getminversion, notify=on_minversion)

    on_configword = pyqtSignal()
    configword = pyqtProperty(str, getconfigword, notify=on_configword)

    on_cmpupkg = pyqtSignal()
    cmpupkg = pyqtProperty(str, getcmpupkg, notify=on_cmpupkg)

##########################################################

class front_dmpuman(QObject):
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile
        self._section = 'DMPU-PKG-CFG'
        self.cfgobj = configman(self._section, self.cfgfile)
        self.m_text = self.cfgobj.loadoptions()

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)   


    def gethbkdata(self):
        return self.cfgobj.getoption('hbk-data')
        
    def getdebugmask0(self):
        return self.cfgobj.getoption('debug-mask[0-31]')

    def getdebugmask1(self):
        return self.cfgobj.getoption('debug-mask[32-63]')

    def getdebugmask2(self):
        return self.cfgobj.getoption('debug-mask[64-95]')

    def getdebugmask3(self):
        return self.cfgobj.getoption('debug-mask[96-127]')

    def getkcppkg(self):
        return self.cfgobj.getoption('kcp-pkg')

    def getkcepkg(self):
        return self.cfgobj.getoption('kce-pkg')

    def getminversion(self):
        return self.cfgobj.getoption('minversion')

    def getdmpupkg(self):
        return self.cfgobj.getoption('dmpu-pkg')

    textChanged = pyqtSignal(str)
    
    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.cfgobj.updateoptions(self.m_text)
        self.cfgobj.update()


    on_hbkdata = pyqtSignal()
    hbkdata = pyqtProperty(str, gethbkdata, notify=on_hbkdata)
    
    on_debugmask0 = pyqtSignal()
    debugmask0 = pyqtProperty(str, getdebugmask0, notify=on_debugmask0)

    on_debugmask1 = pyqtSignal()
    debugmask1 = pyqtProperty(str, getdebugmask1, notify=on_debugmask1)

    on_debugmask2 = pyqtSignal()
    debugmask2 = pyqtProperty(str, getdebugmask2, notify=on_debugmask2)

    on_debugmask3 = pyqtSignal()
    debugmask3 = pyqtProperty(str, getdebugmask3, notify=on_debugmask3)

    on_kcppkg = pyqtSignal()
    kcppkg = pyqtProperty(str, getkcppkg, notify=on_kcppkg)
    
    on_kcepkg = pyqtSignal()
    kcepkg = pyqtProperty(str, getkcepkg, notify=on_kcepkg)

    on_minversion = pyqtSignal()
    minversion = pyqtProperty(str, getminversion, notify=on_minversion)

    on_dmpupkg = pyqtSignal()
    dmpupkg = pyqtProperty(str, getdmpupkg, notify=on_dmpupkg)


##########################################################

class front_enabler(QObject):
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.m_config = ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('step') )

        self.m_text = self.manobj.gettpmconfig()

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)  


    def setsection(self, secname):
        self._section = secname

    textChanged = pyqtSignal()

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.manobj.updatesection(self._section, self.m_text)
        self.manobj.update()


    def gettextfield(self):
        return ( self.m_text )

    def _get_config(self):
        return self.m_config

    def getoption(self, key):
        value = self.manobj.getrawconfig(self._section, key)
        if  value == '' :
            value = '0xffffffff'
        return value

    def getdebugmask0(self):
        return self.getoption('debug-mask[0-31]')

    def getdebugmask1(self):
        return self.getoption('debug-mask[32-63]')

    def getdebugmask2(self):
        return self.getoption('debug-mask[64-95]')

    def getdebugmask3(self):
        return self.getoption('debug-mask[96-127]')

    def getdebuglock0(self):
        return self.getoption('debug-lock[0-31]')

    def getdebuglock1(self):
        return self.getoption('debug-lock[32-63]')

    def getdebuglock2(self):
        return self.getoption('debug-lock[64-95]')

    def getdebuglock3(self):
        return self.getoption('debug-lock[96-127]')


    on_debugmask0 = pyqtSignal()
    debugmask0 = pyqtProperty(str, getdebugmask0, notify=on_debugmask0)

    on_debugmask1 = pyqtSignal()
    debugmask1 = pyqtProperty(str, getdebugmask1, notify=on_debugmask1)

    on_debugmask2 = pyqtSignal()
    debugmask2 = pyqtProperty(str, getdebugmask2, notify=on_debugmask2)

    on_debugmask3 = pyqtSignal()
    debugmask3 = pyqtProperty(str, getdebugmask3, notify=on_debugmask3)

    on_debuglock0 = pyqtSignal()
    debuglock0 = pyqtProperty(str, getdebuglock0, notify=on_debuglock0)

    on_debuglock1 = pyqtSignal()
    debuglock1 = pyqtProperty(str, getdebuglock1, notify=on_debuglock1)

    on_debuglock2 = pyqtSignal()
    debuglock2 = pyqtProperty(str, getdebuglock2, notify=on_debuglock2)

    on_debuglock3 = pyqtSignal()
    debuglock3 = pyqtProperty(str, getdebuglock3, notify=on_debuglock3)

##########################################################

class front_enabler_rma(QObject):
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.m_config = ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('step') )

        self.m_text = self.manobj.gettpmconfig()

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle) 


    def setsection(self, secname):
        self._section = secname

    textChanged = pyqtSignal()

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.manobj.updatesection(self._section, self.m_text)
        self.manobj.update()


    def gettextfield(self):
        return ( self.m_text )

    def _get_config(self):
        return self.m_config

    def getoption(self, key):
        value = self.manobj.getrawconfig(self._section, key)
        if  value == '' :
            value = '0xffffffff'
        return value

    def getdebuglock0(self):
        return self.getoption('debug-lock[0-31]')

    def getdebuglock1(self):
        return self.getoption('debug-lock[32-63]')

    def getdebuglock2(self):
        return self.getoption('debug-lock[64-95]')

    def getdebuglock3(self):
        return self.getoption('debug-lock[96-127]')

		
    on_config = pyqtSignal()
    config = pyqtProperty(str, _get_config, notify=on_config)


    on_debuglock0 = pyqtSignal()
    debuglock0 = pyqtProperty(str, getdebuglock0, notify=on_debuglock0)

    on_debuglock1 = pyqtSignal()
    debuglock1 = pyqtProperty(str, getdebuglock1, notify=on_debuglock1)

    on_debuglock2 = pyqtSignal()
    debuglock2 = pyqtProperty(str, getdebuglock2, notify=on_debuglock2)

    on_debuglock3 = pyqtSignal()
    debuglock3 = pyqtProperty(str, getdebuglock3, notify=on_debuglock3)

##########################################################

class front_developer(QObject):
  
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.m_config = ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('step') )

        self.m_text = self.manobj.gettpmconfig()
        self.m_socid = str()
        
        self.flag_issocid = 0
        

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)   

    #############################

    def SetIsSoCID(self, flag):
        self.flag_issocid = flag
        
    def getissocid(self):
        return self.flag_issocid
    
    on_issocid = pyqtSignal()
    issocid = pyqtProperty(int, getissocid, notify=on_issocid)   


    def SetSocIDFile(self, socidname):
        self._socidname = socidname


    on_socid = pyqtSignal()
        
    @pyqtProperty(str, notify=on_socid)
    def socid(self):
        if( self.flag_issocid == 1 ):
            if( os.path.isfile(self._socidname) ): 
                binstr = GetDataFromBinFile(self._socidname)
            else:
                binstr = bytes(32)
                wrfile = open(self._socidname, "wb")
                wrfile.write(binstr)
                wrfile.close()

            self.m_socid = str()
            i = 0
            for s in bytes(binstr):
                self.m_socid = self.m_socid + '%02X '% s
                i = i + 1
                if (i%16) == 0:
                    self.m_socid = self.m_socid + '\n'
            
        return self.m_socid

    @socid.setter
    def socid(self, text):
        if self.m_socid == text:
            return

        self.m_socid = text
        valuelist = self.m_socid.split()
        #print(valuelist)

        outStr = bytes()
        for binitem in valuelist:
            intvalue = int(binitem, 16)
            outStr += intvalue.to_bytes(1, byteorder='little')        
        
        wrfile = open(self._socidname, "wb")
        wrfile.write(outStr)
        wrfile.close()

    #############################

    def setsection(self, secname):
        self._section = secname

    textChanged = pyqtSignal()

    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )
        self.manobj.updatesection(self._section, self.m_text)
        self.manobj.update()


    def gettextfield(self):
        return ( self.m_text )

    def _get_config(self):
        return self.m_config

    def getoption(self, key):
        value = self.manobj.getrawconfig(self._section, key)
        if  value == '' :
            value = '0xffffffff'
        return value

    def getdebugmask0(self):
        return self.getoption('debug-mask[0-31]')

    def getdebugmask1(self):
        return self.getoption('debug-mask[32-63]')

    def getdebugmask2(self):
        return self.getoption('debug-mask[64-95]')

    def getdebugmask3(self):
        return self.getoption('debug-mask[96-127]')


    on_debugmask0 = pyqtSignal()
    debugmask0 = pyqtProperty(str, getdebugmask0, notify=on_debugmask0)

    on_debugmask1 = pyqtSignal()
    debugmask1 = pyqtProperty(str, getdebugmask1, notify=on_debugmask1)

    on_debugmask2 = pyqtSignal()
    debugmask2 = pyqtProperty(str, getdebugmask2, notify=on_debugmask2)

    on_debugmask3 = pyqtSignal()
    debugmask3 = pyqtProperty(str, getdebugmask3, notify=on_debugmask3)

##########################################################

class front_assetman(QObject):

    sectionname = 'ASSET-PROV-CFG'
    cmkeyoption = 'CM Provisioning Key (Kpicv)'
    dmkeyoption = 'DM Provisioning Key (Kcp)'
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)

        self.cfgfile = cfgfile
        self.m_text  = str()

        self.cfgman = configman(self.sectionname, self.cfgfile)
        

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)   

    #############################

    textChanged = pyqtSignal()
    
    @pyqtProperty(str, notify=textChanged)
    def text(self):
        return self.m_text

    @text.setter
    def text(self, text):
        if self.m_text == text:
            return
        self.m_text = text
        #print( self.m_text )

        items = text.split('&')
        tmpstr = ''
        for item in items :
            key, value = item.split('=')
            if( key == 'keytype'):
                if( value == self.cmkeyoption ):
                    tmpstr = ((tmpstr + '&') if (tmpstr != '') else '')  + 'key-filename=../cmpublic/enc.kpicv.bin'
                    tmpstr = ((tmpstr + '&') if (tmpstr != '') else '') + 'keypwd-filename=../cmsecret/pwd.kpicv.txt'
                else:
                    tmpstr = ((tmpstr + '&') if (tmpstr != '') else '') + 'key-filename=../dmpublic/enc.kcp.bin'
                    tmpstr = ((tmpstr + '&') if (tmpstr != '') else '') + 'keypwd-filename=../dmsecret/pwd.kcp.txt'
            elif( key == 'assetid'):
                tmpstr = ((tmpstr + '&') if (tmpstr != '') else '') + 'asset-id=' + value + ''
            elif( key == 'assetfile'):
                tmpstr = ((tmpstr + '&') if (tmpstr != '') else '') + 'asset-filename=' + value + ''
            elif( key == 'pkgfile'):
                tmpstr = ((tmpstr + '&') if (tmpstr != '') else '') + 'asset-pkg=' + value + ''
                
        #print(tmpstr)
        self.cfgman.updateoptions(tmpstr)
        self.cfgman.update()


    def gettextfield(self):
        return ( self.m_text )

    def iscmkeytype(self):
        if self.m_text != '' :
            cfgitems = self.m_text.split('&')
            for cfgitem in cfgitems:
                key, value = cfgitem.split('=')
                if( key == 'keytype' ):
                    if( value == self.cmkeyoption ):
                        return True
        return False

    def getassetfilename(self):
        if self.m_text != '' :
            cfgitems = self.m_text.split('&')
            for cfgitem in cfgitems:
                key, value = cfgitem.split('=')
                if( key == 'pkgfile' ):
                    return value
        return ''    

    def getkeytype(self):
        keyfile = self.cfgman.getoption('key-filename')
        if( (keyfile.find('kpicv') >= 0) or (keyfile.find('Kpicv') >= 0)):
            return self.cmkeyoption
        return self.dmkeyoption

    on_keytype = pyqtSignal()
    keytype = pyqtProperty(str, getkeytype, notify=on_keytype)
    
    def getassetid(self):
        assetid = self.cfgman.getoption('asset-id')
        return assetid

    on_assetid = pyqtSignal()
    assetid = pyqtProperty(str, getassetid, notify=on_assetid)

    def getassetfile(self):
        assetfilename = self.cfgman.getoption('asset-filename')
        return assetfilename

    on_assetfile = pyqtSignal()
    assetfile = pyqtProperty(str, getassetfile, notify=on_assetfile)

    def getpkgfile(self):
        assetpkg = self.cfgman.getoption('asset-pkg')
        return assetpkg

    on_pkgfile = pyqtSignal()
    pkgfile = pyqtProperty(str, getpkgfile, notify=on_pkgfile)

##########################################################
    
class front_console(QObject):
    
    def __init__(self, parent=None):
        QObject.__init__(self, parent)

        self.m_contitle = str()
        self.m_console = str()
        self.m_title = str()
        self.m_index  = 0
        self.m_batchlist = []
        self.m_click = str()
        
        self.m_logfile = None

    #############################

    on_contitle = pyqtSignal()

    @pyqtProperty(str, notify=on_contitle)
    def contitle(self):
        return self.m_contitle

    @contitle.setter
    def contitle(self, text):
        if self.m_contitle == text:
            return
        self.m_contitle = text

    def setcontitle(self, text):
        if self.m_contitle == text:
            return
        self.m_contitle = text

    on_console = pyqtSignal()
    
    @pyqtProperty(str, notify=on_console)
    def console(self):
        return self.m_console

    @console.setter
    def console(self, text):
        if self.m_console == text:
            return
        self.m_console = text

    def setconsole(self, text):
        if self.m_console == text:
            return
        self.m_console = text


    @pyqtSlot(str)
    def pyqtSlot_exec(self, text):
        #print( "pyqtSlot %s" % text )

        if ( text == 'Run' ):
            self.newcontitle('Batch : ')
            self.newconsole('')
        
            for batchitem in self.m_batchlist:
                title, out, err = batchitem[0](batchitem[1], batchitem[2], batchitem[3])
                #print('%s, ' % batchitem[4])
                
                aout = out.decode("ASCII")
                aerr = err.decode("ASCII")

                # warning filter 
                warn_filter_list = [
                    r'[\.]+[\+][\+]',
                    r'WARNING: can\'t open config file: /usr/local/ssl/openssl.cnf',
                    r'e is 65537 \(0x10001\)',
                ]

                for filterstr in warn_filter_list:
                    aout = re.sub(filterstr, '', aout)
                    aerr = re.sub(filterstr, '', aerr)

                aout = re.sub(r'[\r|\n]+', '\n', aout)
                aerr = re.sub(r'[\r|\n]+', '\n', aerr)

                print(title)
                print( aout )
                print( aerr )
                print( "\n")

                self.addcontitle('%s, ' % batchitem[4])
                self.addconsole(title)
                self.addconsole( aout )
                self.addconsole( aerr )

                if self.m_logfile != None :
                    self.m_logfile.write("%s\n" % title)
                    self.m_logfile.write( aout )
                    self.m_logfile.write( aerr )
                    self.m_logfile.write( "\n\n" )

                if out.decode("ASCII").find("error") > 0 :
                    break

            self.m_index  = 0
        else:

            if self.m_index >= len(self.m_batchlist) :
                self.m_index  = 0
                
            for idx, batchitem in enumerate(self.m_batchlist):
                if( idx == self.m_index ):
                    self.newcontitle('Build : %s' % batchitem[4])
                    title, out, err = batchitem[0](batchitem[1], batchitem[2], batchitem[3])
                    print(title)
                    print( out.decode("ASCII") )
                    print( err.decode("ASCII") )
                                        
                    self.newconsole(title)
                    self.addconsole( out.decode("ASCII") )
                    self.addconsole( err.decode("ASCII") )
                    self.m_index = self.m_index + 1

                    if self.m_logfile != None :
                        self.m_logfile.write("%s\n" % title)
                        self.m_logfile.write( out.decode("ASCII") )
                        self.m_logfile.write( err.decode("ASCII") )
                        self.m_logfile.write( "\n\n" )

                    return
        
    #############################
    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal()
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)   
	
    def newcontitle(self, text):
        self.m_contitle = text

    def newconsole(self, text):
        self.m_console = text

    def addcontitle(self, text):
        self.m_contitle = self.m_contitle + text

    def addconsole(self, text):
        self.m_console = self.m_console + text

    def SetEngine(self, engine):
        self.engine = engine
        self.engine.pushButton.connect(self.pyqtSlot_exec)

    def SetBatch(self, batchlist):
        self.m_batchlist = batchlist
        self.m_index  = 0

    #############################
    on_runclick = pyqtSignal()
    
    @pyqtProperty(str, notify=on_runclick)
    def runclick(self):
        return self.m_click

    @runclick.setter
    def runclick(self, text):
        if self.m_click == text:
            return
        self.m_click = text
        
    def ClickRun(self):
        self.m_click = 'Run'

    def SetLogFile(self, fpointer):
        self.m_logfile = fpointer

    def NonGUIRun(self):
        self.pyqtSlot_exec('Run')

##########################################################

def diasemi_gui_tpm_manager(app, pathname, cfgfile, ownfix):
    
    secuman = front_secuman(cfgfile)
    secuman.SetWinTitle('Dialog Security Tool')
    secuman.setownfix(ownfix)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("secuman", secuman)

    app.setOrganizationName("Dialog Semiconductor")
    app.setOrganizationDomain("www.dialog-semiconductor.com")
    app.setApplicationName("Dialog Security Tool")

    qmlpath = '%s/secuman.qml' % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)

    app.exec_()

    currcfg = secuman.getscenario()
    currtext = secuman.gettext()

    del engine
    del secuman

    return currtext, currcfg
    
##########################################################

diasemi_gui_xmpulist = {
    'CMPU'    : [ front_cmpuman, './cmconfig/cmpu.cfg', 'Dialog Security Tool : CMPU', 'cmpuman', '%s/cmpuman.qml' ],
    'DMPU'    : [ front_dmpuman, './dmconfig/dmpu.cfg', 'Dialog Security Tool : DMPU', 'dmpuman', '%s/dmpuman.qml' ],
}

def diasemi_gui_xmpu_manager(app, pathname, cfgfile, xmpukey):
    guiitem = diasemi_gui_xmpulist[xmpukey]
    
    xmpuman =  guiitem[0](guiitem[1])
    xmpuman.SetWinTitle(guiitem[2])

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty(guiitem[3], xmpuman)

    qmlpath = guiitem[4] % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)
    app.exec_()

    del engine
    del xmpuman    

##########################################################

diasemi_gui_cfglist = {
    'TPMCONFIG'     : [ front_cfgman
                        , 'Dialog Security Tool : Secure Parameters', '', 'cfgman', '%s/cfgman.qml'
                        , ''
                      ],
    'ENABLER'       : [ front_enabler
                        , 'Dialog Security Tool : Enabler', 'ENABLER', 'envman', '%s/envman.qml'
                        , 'nolcs'
                      ],
    'ENABLER-RMA'   : [ front_enabler_rma
                        , 'Dialog Security Tool : Enabler RMA', 'ENABLER-RMA', 'envrmaman', '%s/envrmaman.qml'
                        , 'nolcs'
                      ],
    'DEVELOPER'     : [ front_developer
                        , 'Dialog Security Tool : Developer', 'DEVELOPER', 'devman', '%s/devman.qml'
                        , 'nolcs'
                      ],
    'DEVELOPER-RMA' : [ front_developer
                        , 'Dialog Security Tool : Developer RMA', 'DEVELOPER-RMA', 'devrmaman', '%s/devrmaman.qml'
                        , 'nolcs'
                      ],
}

def diasemi_gui_manager(app, pathname, cfgfile, guikey, newlcs):
    guiitem = diasemi_gui_cfglist[guikey]
    
    frontman = guiitem[0](cfgfile)
    frontman.SetWinTitle(guiitem[1])

    if( guiitem[2] != '' ):
        frontman.setsection(guiitem[2])

    if( guiitem[5] != 'nolcs' ):
        frontman.setnewlcs(newlcs)
        

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty(guiitem[3], frontman)
    qmlpath = guiitem[4] % pathname
    
    engine.addImportPath( pathname )
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)
    app.exec_()

    del engine
    del frontman

##########################################################

diasemi_gui_sdbgcfglist = {
    'DEVELOPER'     : [ front_developer, 'Dialog Security Tool : Developer'     , 'DEVELOPER'    , 'devman'     , '%s/devman.qml'   , './image/soc_id.bin'],
    'DEVELOPER-RMA' : [ front_developer, 'Dialog Security Tool : Developer RMA' , 'DEVELOPER-RMA', 'devrmaman'  , '%s/devrmaman.qml', './image/soc_id.bin'],
}

def diasemi_gui_sdbg_manager(app, pathname, cfgfile, guikey):
    guiitem = diasemi_gui_sdbgcfglist[guikey]
    
    frontman = guiitem[0](cfgfile)
    frontman.SetWinTitle(guiitem[1])
    frontman.SetIsSoCID(1)
    frontman.SetSocIDFile(guiitem[5])

    oldcfg = frontman.gettextfield()

    frontman.setsection(guiitem[2])

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty(guiitem[3], frontman)
    qmlpath = guiitem[4] % pathname
    
    engine.addImportPath( pathname )
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)
    app.exec_()

    newcfg = frontman.gettextfield()

    socid = frontman.m_socid

    del engine
    del frontman

    if( oldcfg != newcfg ):
        return True, socid

    return False, socid


##########################################################

fc9kimagelist = [ 'UEBOOT', 'RTOS.cache', 'RTOS.sram', 'Comp.combined', 'Comp.PTIM', 'Comp.RaLIB', 'RMA' ]

##########################################################

def diasemi_gui_imgmanager(app, pathname, cfgfile, dbglst):
    sfdpman = front_sfdpman()
    sfdpman.setpath(pathname)
    
    sfdplist = QStringListModel()
    sfdplist.setStringList( sfdpman.gensfdplist() )

    manobj = manconfig(cfgfile)
    bootmodel = manobj.gettpmbootmodel( )
    imagemodel = manobj.gettpmimagemodel( )

    dbgitems = dbglst.split(':')
    dbgmode  = ''
    dbgrma   = ''
    for idx, item in enumerate(dbgitems):
        if( idx == 0 ):
            dbgmode = item
        elif( idx == 1 ):
            dbgrma   = item

    for imageitem in fc9kimagelist:

        if( (bootmodel == 'SRAM Boot') and (imageitem == 'UEBOOT') ):
            continue
        if( (bootmodel == 'SRAM Boot') and (imageitem == 'RTOS.cache') ):
            continue
        if( (bootmodel == 'Cache Boot') and (imageitem == 'RTOS.sram') ):
            continue

        if( (imagemodel == 'Split Image') and (imageitem == 'Comp.combined') ):
            continue
        if( (imagemodel == 'Combined Image') and (imageitem == 'Comp.PTIM') ):
            continue
        if( (imagemodel == 'Combined Image') and (imageitem == 'Comp.RaLIB') ):
            continue

        if( (dbgrma == 'RMA') and (dbgrma != imageitem) ):
            continue
        
        imgman = front_imgman(cfgfile)
        imgman.SetWinTitle('Dialog Security Tool :: %s' % imageitem)
        
        imgman.setsection(imageitem)


        if( (imageitem == 'UEBOOT') ):
            imgman.forcedbgcert(dbgmode)
        elif( (imageitem == 'RTOS.sram') ):
            imgman.forcedbgcert(dbgmode)
        elif( imageitem == 'RMA' ):
            imgman.forcedbgcert('')
        else:
            imgman.forcedbgcert('')
        
        imgman.setsfdpindex( sfdpman.findsfdplist(imgman.getsfdp()) )
        
        engine = QQmlApplicationEngine()
        engine.rootContext().setContextProperty("imgman", imgman)
        engine.rootContext().setContextProperty("sfdpman", sfdplist)
        qmlpath = '%s/imgman.qml' % pathname

        engine.addImportPath( pathname )
        engine.load(QUrl.fromLocalFile(qmlpath))

        if not engine.rootObjects():
            sys.exit(-1)
        app.exec_()

        del engine
        del imgman

    del sfdplist
    del sfdpman
    del manobj

##########################################################

def diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, title):
    conman = front_console()
    conman.SetWinTitle(title)
 
    logfilename = getcurrlogfilename(cfgfile)

    if logfilename != "" :
        fptr = open(logfilename, "a")
        conman.SetLogFile(fptr)


    #engine = QQmlApplicationEngine()
    #engine.rootContext().setContextProperty("conman", conman)

    #qmlpath = '%s/console.qml' % pathname

    #engine.addImportPath( pathname )           
    #engine.load(QUrl.fromLocalFile(qmlpath))

    #if not engine.rootObjects():
    #    sys.exit(-1)

    #conman.SetEngine(engine.rootObjects()[0])
    conman.SetBatch(batchlist)

    #conman.ClickRun()
    conman.NonGUIRun()

    #app.exec_()

    #del engine
    del conman
 
    if logfilename != "" :
        fptr.close()


def diasemi_gui_imagebuild(app, pathname, cfgfile, batchlist):
    
    diasemi_gui_batchbuild(app, pathname, cfgfile, batchlist, 'Dialog Image Build')

##########################################################

class front_keyman(QObject):
    cfgfile = ""
    textChanged = pyqtSignal(str)
    
    def __init__(self, cfgfile, parent=None):
        QObject.__init__(self, parent)
        self.m_text = ""
        self.cfgfile = cfgfile

        self.manobj = manconfig(self.cfgfile)
        self.manobj.parse()
        self.manobj.update()

        self.m_click = ""

    def SetWinTitle(self, title):
        self.m_title = title
    
    def getwintitle(self):
        return self.m_title

    on_wintitle = pyqtSignal(str)
    wintitle = pyqtProperty(str, getwintitle, notify=on_wintitle)    


    @pyqtProperty(str, notify=textChanged)
    def text(self):
        self.m_text = ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('role') + ':' + self.manobj.getconfig('step') )
        return self.m_text
        
    @text.setter
    def text(self, text):
        if( (text != '') and (self.m_text != text) ):
            self.m_text = text

    def gettext(self):
        return self.m_text
		
    def gettextfield(self):
        return ( self.manobj.getconfig('rule') + ':' + self.manobj.getconfig('own') + ':' + self.manobj.getconfig('role') + ':' + self.manobj.getconfig('step') )

    on_textfield = pyqtSignal()
    textfield = pyqtProperty(str, gettextfield, notify=on_textfield)

    def getsboot(self):
        return self.manobj.gettpmsboot_int()

    on_sboot = pyqtSignal()
    sboot = pyqtProperty(int, getsboot, notify=on_sboot)

    def getsdebug(self):
        return self.manobj.gettpmsdebug_int()

    on_sdebug = pyqtSignal()
    sdebug = pyqtProperty(int, getsdebug, notify=on_sdebug)

    on_runclick = pyqtSignal()
    
    @pyqtProperty(str, notify=on_runclick)
    def runclick(self):
        return self.m_click

    @runclick.setter
    def runclick(self, text):
        self.m_click = text

    def getrunclick(self):
        return self.m_click
    

def diasemi_gui_key_manager(app, pathname, cfgfile):
    
    keyman = front_keyman(cfgfile)
    keyman.SetWinTitle('Dialog Security Tool : Key Management')

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("keyman", keyman)

    qmlpath = '%s/keyman.qml' % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)

    app.exec_()

    actrule = keyman.getrunclick()
    
    del engine
    del keyman

    return actrule

##########################################################

def diasemi_gui_messagebox(app, pathname, title):
    msgman = front_msgman()
    msgman.SetWinTitle(title)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("msgman", msgman)

    qmlpath = '%s/message.qml' % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)
    app.exec_()

    cleanoption = msgman.gettext()

    del engine
    del msgman

    return cleanoption

##########################################################

def diasemi_gui_warnbox(app, pathname, title, detail):
    warnman = front_msgman()
    warnman.SetWinTitle(title)
    warnman.SetWinDetail(detail)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("warnman", warnman)

    qmlpath = '%s/warnman.qml' % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)
    app.exec_()

    cleanoption = warnman.gettext()

    del engine
    del warnman

    return cleanoption

##########################################################

def diasemi_gui_removesecrets(app, pathname, title, detail):
    rmovman = front_rmovman()
    rmovman.SetWinTitle(title)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("rmovman", rmovman)

    qmlpath = '%s/rmvman.qml' % pathname

    engine.addImportPath( pathname )           
    engine.load(QUrl.fromLocalFile(qmlpath))

    if not engine.rootObjects():
        sys.exit(-1)
    app.exec_()

    rmovoption = rmovman.gettext()
    
    del engine
    del rmovman

    return rmovoption
