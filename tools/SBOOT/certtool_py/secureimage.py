#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
from struct import *

class ImageMake:

    crc_table = [
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
      0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
      0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
      0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
      0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
      0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
      0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
      0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
      0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
      0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
      0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
      0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
      0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
      0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
      0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
      0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
      0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
      0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
      0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
      0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
      0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
      0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
      0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
      0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
      0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
      0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
      0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
      0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
      0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
      0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
      0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
      0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
      0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
      0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
      0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
      0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
      0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
      0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
      0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
      0x2d02ef8d
    ]

#    typedef struct {
# 	unsigned int	magic;
# 	unsigned int	hcrc;
# 	unsigned int	sfdpcrc;
# 	unsigned int	version;
# 
# 	unsigned char	name[40];
# 	unsigned int	datsiz;
# 	unsigned int	datcrc;
# 
# 	unsigned int	dbgkey    :  1;
# 	unsigned int	keysize   : 15;
# 	unsigned int	keynum    :  8;
# 	unsigned int	loadscheme:  4;
# 	unsigned int	encrscheme:  4;
# 	unsigned int	ccrc;
# 	unsigned int	csize;
# 	unsigned int	cpoint;
#    } ImageHeader_t;

    #ImageHeader = struct("!<IIII40sIIIIII")

#   typedef struct {
#   	unsigned int length;
#   	unsigned int crc;
#   	unsigned int hash;
#   	unsigned int offset;
#   	unsigned char *data;
#   } ImageComponent_t;

    #ImageComponent = struct("!<IIIIp")

#   typedef struct {
#   	unsigned sfdplen;
#   	unsigned char *sfdp;
#   	unsigned int certnum;
#   	ImageComponent_t *certchain;
#   	ImageComponent_t dbgcert;
#   	unsigned int crcon;
#   	unsigned int ctntnum;
#   	ImageComponent_t *ctntchain;
#   } ImageBodyInfo_t;

    sfdpcrc = 0
    sfdplen = 0
    sfdp = bytes()

    logfname = str()

    #########################################################
    def __init__(self, logfname):
        self.logfname = logfname
        return

    def LOG_PRINT(self, *kwargs):
        #print(kwargs[1], end='')
        logfile = open(self.logfname, "at")
        logfile.write((kwargs[0]))
        logfile.close()   
        return     

    #########################################################
    def crc32(self, crc, buf, len):
        i = 0
        crc = crc ^ 0xffffffff

        while (len > 0 ):
            crc = self.crc_table[(crc ^ (buf[i])) & 0xff] ^ (crc >> 8)
            i   = i + 1
            len = len - 1
            
        return (crc ^ 0xffffffff)

    #########################################################
    def swap32(self, x):
        return (((x << 24) & 0xFF000000) |
                ((x <<  8) & 0x00FF0000) |
                ((x >>  8) & 0x0000FF00) |
                ((x >> 24) & 0x000000FF))   

    #########################################################
    def LoadSFDPfile(self, sfdpfname):
        status = True

        if( len(sfdpfname) == 0 ):
            self.LOG_PRINT("LoadSFDPfile : none\r\n")
            return False
        
        if os.path.isfile(sfdpfname) == False :
            return False
        
        try:
            fobj = open(sfdpfname, 'rb')
            self.sfdp = fobj.read( )
            fobj.close()
        except IOError as e:
            self.LOG_PRINT("LoadSFDPfile : error\r\n");
            status = False

        if( status == True ):
            self.LOG_PRINT("LoadSFDPfile : open - %s\r\n" % sfdpfname);
            
            self.sfdplen = len(self.sfdp)

            appendsiz = ( int((self.sfdplen + 15)/16) * 16 ) - self.sfdplen

            if(appendsiz != 0) :
                self.sfdp += b'\xff' * appendsiz            

            self.LOG_PRINT("LoadSFDPfile : ReadFile - %d \r\n" % self.sfdplen);

            length = len(self.sfdp)
            self.sfdplen = length
            
            self.sfdpcrc = self.crc32(0, self.sfdp, length)
            
            self.LOG_PRINT("LoadSFDPfile : crc32 - %08x\r\n" % self.sfdpcrc);
 
        return status


    #########################################################
    def LoadCertfile(self, keylevel, index, certfname):
        status = True

        certdata = bytes()
        data = bytes()
        certhash = 0
        offset   = 0
        certcrc  = 0

        if (keylevel == 0) or (keylevel <= index) :
            self.LOG_PRINT("LoadCertfile : cert is not exists : %d , %d\r\n" % (keylevel, index) )
            return certdata

        self.LOG_PRINT("LoadCertfile : create certchain [%d]\r\n" % index)

        if os.path.isfile(certfname) == False :
            self.LOG_PRINT("LoadCertfile : error\r\n")
            return certdata

        try:
            fobj = open(certfname, 'rb')
            data  = fobj.read( )
            fobj.close()
        except IOError as e:
            self.LOG_PRINT("LoadCertfile : error\r\n")
            status = False

        if( status == True ):
            self.LOG_PRINT("LoadCertfile : open - %d, %s\r\n" % (index, certfname))
            
            length = len(data)
            appendsiz = ( int((length + 15)/16) * 16 ) - length

            self.LOG_PRINT("LoadCertfile : ReadFile - %d (%d)\r\n" %( length, index) )

            if(appendsiz != 0) :
                data += b'\xff' * appendsiz

            newlength = len(data)
            certcrc = self.crc32(certcrc, data, newlength)

            self.LOG_PRINT("LoadCertfile : crc32 - %08x (%d)\r\n" %( certcrc, index) )
           
            certdata += length.to_bytes(4, 'little')
            certdata += certcrc.to_bytes(4, 'little')
            certdata += certhash.to_bytes(4, 'little')
            certdata += offset.to_bytes(4, 'little')
            certdata += data
            
            #print(certdata)

        return certdata

    #########################################################
    def LoadDbgCertfile(self, dbgkfname, dbgfname):
        status = True

        certdata = bytes()
        datak = bytes()
        data = bytes()
        newdata = bytes()
        certhash = 0
        offset   = 0
        certcrc  = 0

        if len(dbgkfname) == 0 :
            self.LOG_PRINT("LoadDbg.Key.Certfile : none\r\n")
            return certdata

        self.LOG_PRINT("LoadDbg.Key.Certfile : open - %s\r\n" % dbgkfname)

	
        if os.path.isfile(dbgkfname) == False :
            #print("LoadDbg.Key.Certfile : error" % dbgkfname)
            return certdata

        try:
            fobj = open(dbgkfname, 'rb')
            datak  = fobj.read( )
            fobj.close()
        except IOError as e:
            self.LOG_PRINT("LoadDbg.Key.Certfile : error\r\n")
            status = False

        if( status == True ):
            length = len(datak)
            appendsiz = ( int((length + 15)/16) * 16 ) - length

            newdata += datak
            
            if(appendsiz != 0) :
                datak += b'\xff' * appendsiz

            self.LOG_PRINT("LoadDbg.Key.Certfile : ReadFile - %d\r\n" % length)
            offset = length

            length = len(datak)
            certcrc = 0
            certcrc = self.crc32(certcrc, datak, length)
            
            self.LOG_PRINT("LoadDbg.Key.Certfile : crc32 - %08x\r\n" % certcrc)
            

        if len(dbgfname) == 0 :
            self.LOG_PRINT("LoadDbgCertfile : none\r\n")
            return certdata
        
        if os.path.isfile(dbgfname) == False :
            #print("cann't open the DbgCert file, %s" % dbgfname)
            return certdata

        self.LOG_PRINT("LoadDbgCertfile : open - %s\r\n" % dbgfname)

        try:
            fobj = open(dbgfname, 'rb')
            data  = fobj.read( )
            fobj.close()
        except IOError as e:
            self.LOG_PRINT("LoadDbgCertfile : error\r\n")
            status = False

        if( status == True ):
            length = len(data)
            appendsiz = ( int((length + 15)/16) * 16 ) - length

            self.LOG_PRINT("LoadDbgCertfile : ReadFile - [offset:%x], %d\r\n" % ( offset, length) )

            newdata += data
                

        if( status == True ):
            newlength = len(newdata)

            appendsiz = ( int((newlength + 15)/16) * 16 ) - newlength
            if(appendsiz != 0) :
                newdata += b'\xff' * appendsiz
                
            certcrc = self.crc32(0, newdata, (newlength+appendsiz) )
 
            self.LOG_PRINT("LoadDbgCertfile : crc32 - %08x [len %d]\r\n" % ( certcrc, (offset + length)) )
            
            certdata += (offset + length).to_bytes(4, 'little')
            certdata += certcrc.to_bytes(4, 'little')
            certdata += certhash.to_bytes(4, 'little')
            certdata += offset.to_bytes(4, 'little')
            certdata += newdata

            #print(certdata)
            
        return certdata

    #########################################################
    def LoadContentfile(self, index, offset, ctxtfname):
        status = True

        ctxtdata = bytes()
        data = bytes()
        ctxthash = 0
        ctxtcrc  = 0

        self.LOG_PRINT("LoadContentfile : create SWComp [%d]\r\n" % index)

        self.LOG_PRINT("LoadContentfile : open - %d, %s\r\n" % (index, ctxtfname))

        if os.path.isfile(ctxtfname) == False :
            print("cann't open the content file, %s" % ctxtfname)
            return ctxtdata

        try:
            fobj = open(ctxtfname, 'rb')
            data  = fobj.read( )
            fobj.close()
        except IOError as e:
            status = False

        if( status == True ):
            length = len(data)
            appendsiz = ( int((length + 15)/16) * 16 ) - length

            self.LOG_PRINT("LoadContentfile : ReadFile - %d (%d)\r\n" % (length, index) )

            if(appendsiz != 0) :
                data += b'\xff' * appendsiz

            length = len(data)
            ctxtcrc = self.crc32(ctxtcrc, data, length)
            
            self.LOG_PRINT("LoadContentfile : crc32 - %08x (%d)\r\n" % (ctxtcrc, index) )
           
            ctxtdata += length.to_bytes(4, 'little')
            ctxtdata += ctxtcrc.to_bytes(4, 'little')
            ctxtdata += ctxthash.to_bytes(4, 'little')
            ctxtdata += offset.to_bytes(4, 'little')
            ctxtdata += data
            
            #print(ctxtdata)

        return ctxtdata    

class GenSecureImage:
    out = str()
    err = str()

    inifname = str( )
    logname  = str( )
    config = configparser.ConfigParser()
    config.optionxform = str
    
    srcfname = str( )
    imgfname = str( )

    #########################################################
    def __init__(self, ininame):
        self.inifname = ininame
        return


    def get_console(self):
        return self.out, self.err


    def LOG_PRINT(self, *kwargs):
        ##print(kwargs[0], end='')
        #self.out += (kwargs[0] + '\n')

        logfile = open(self.logname, "at")
        logfile.write((kwargs[0] ))
        logfile.close()

    #########################################################
    def parse(self):
        status = True

        if os.path.isfile(self.inifname) == False :
            self.out += ("cann't open the ini file, %s\n" % self.inifname)
            return False
        
        try:
            fobj = open(self.inifname, 'r')
            self.config.read_file(fobj)
            fobj.close()
        except IOError as e:
            status = False

        if status == True :
            status = False

            if( (self.config.has_section('IMAGE') == True) ):
                self.imgfname = self.get_option('IMAGE')
                self.imgfname = self.imgfname.replace('\\\\', '\\' )
                self.imgfname = self.imgfname.replace('"', '' )
                status = True
                
            if( self.config.has_option('IMAGE', 'CERT1') == True):
                status = True
            if( self.config.has_option('IMAGE', 'TITLE') == True):
                status = True
            if( self.config.has_option('IMAGE', 'KEYLEVEL') == True):
                status = True
            if( self.config.has_option('IMAGE', 'KEYSIZE') == True):
                status = True
            if( self.config.has_option('IMAGE', 'KEYLOAD') == True):
                status = True
            if( self.config.has_option('IMAGE', 'KEYENC') == True):
                status = True
            if( self.config.has_option('IMAGE', 'COMP1') == True):
                status = True
            if( self.config.has_option('IMAGE', 'CRCON') == True):
                status = True
            if( self.config.has_option('IMAGE', 'OFFSET') == True):
                status = True
            if( self.config.has_option('IMAGE', 'COMP1ADDR') == True):
                status = True
            if( self.config.has_option('IMAGE', 'FLASH') == True):
                status = True
            if( self.config.has_option('IMAGE', 'VERSION') == True):
                status = True
        
        return status
            
    #########################################################
    def get_option(self, keyname):
        if( self.config.has_option('IMAGE', keyname) == True):
            return self.config.get('IMAGE', keyname)

        return str()

    #########################################################
    def set_imagename(self, imgname):
        self.imgfname = imgname.replace('\\\\', '\\' )
        self.imgfname = self.imgfname.replace('"', '' )
        return   

    #########################################################
    def set_logname(self, logname):
        self.logname  = logname

        logfile = open(self.logname, "wt")
        logfile.write('')
        logfile.close()
        return 

    def set_logclose(self):
        return

    #########################################################
    def SaveImageFile(self):

        gimgmake = ImageMake(self.logname)

        datasize = 80
        datacrc  = 0
        certchainindex = 0
        ccert = [bytes(), bytes(), bytes()]
        ctxt = [bytes(), bytes(), bytes()]
        ctxtfill = [bytes(), bytes(), bytes()]

        if self.get_option('SFDP') != '' :
            gimgmake.LoadSFDPfile(self.get_option('SFDP'))
            datasize += gimgmake.sfdplen
            #print("[1]foffset_wr - %d\r\n" % datasize)

        keylevel = self.get_option('KEYLEVEL')

        for idx in range(int(keylevel)) :
            certkeyname = 'CERT%d' % (idx+1)
        
            if self.get_option(certkeyname) != '' :
                #print(self.get_option(certkeyname))
                ccert[idx] = gimgmake.LoadCertfile(int(keylevel), (idx), self.get_option(certkeyname))
                ccert_size = int.from_bytes(ccert[idx][0:4], "little")
                ccert_crc  = int.from_bytes(ccert[idx][4:8], "little")

                datacrc    = gimgmake.crc32(datacrc, ccert_size.to_bytes(4,'little'), 4)
                datacrc    = gimgmake.crc32(datacrc, ccert_crc.to_bytes(4,'little'), 4)
                datasize  += 8
                #print("[2]foffset_wr - %d\r\n" % datasize)
                #print("datacrc - %08x\r\n" % datacrc)

                certchainindex += 1


        if self.get_option('DBGCERT') != '' :
            dcert = gimgmake.LoadDbgCertfile(self.get_option('DBGCERTK'), self.get_option('DBGCERT'))    
            dcert_size = int.from_bytes(dcert[0:4], "little")
            dcert_crc  = int.from_bytes(dcert[4:8], "little")

            datacrc    = gimgmake.crc32(datacrc, dcert_size.to_bytes(4,'little'), 4)
            datacrc    = gimgmake.crc32(datacrc, dcert_crc.to_bytes(4,'little'), 4)
            datasize  += 8

            #print("[3]foffset_wr - %d\r\n" % datasize)
            #print("datacrc - %08x\r\n" % datacrc)

            certchainindex += 1

        if  (certchainindex & 1) != 0 :
            zerofill = 0
            datacrc = gimgmake.crc32(datacrc, zerofill.to_bytes(4,'little'), 4)
            datacrc = gimgmake.crc32(datacrc, zerofill.to_bytes(4,'little'), 4)
            datasize  += 8
            #print("[4]foffset_wr - %d\r\n" % datasize)
            #print("datacrc - %08x\r\n" % datacrc)

        for idx in range(int(keylevel)) :
            ccertsize = int.from_bytes(ccert[idx][0:4], "little")
            ccert_size = ( int((ccertsize + 15)/16) * 16 )
            datacrc = gimgmake.crc32(datacrc, ccert[idx][16:], ccert_size)
            datasize  += ccert_size
            #print("[5]foffset_wr - %d\r\n" % datasize)
            #print("datacrc - %08x\r\n" % datacrc)

        if self.get_option('DBGCERT') != '' :
            dcert_size = int.from_bytes(dcert[0:4], "little")
            dcert_size = ( int((dcert_size + 15)/16) * 16 )
            datacrc = gimgmake.crc32(datacrc, dcert[16:], dcert_size)
            datasize  += dcert_size
            #print("[6]foffset_wr - %d\r\n" % datasize)
            #print("datacrc - %08x\r\n" % datacrc)

            m_imgheader_dbgkey = 1
        else:
            m_imgheader_dbgkey = 0


        m_imgheader_offset = int(self.get_option('FLASH'))
        m_imgheader_offset = m_imgheader_offset & int(0x0ffffff)

        m_imgbody_ctntnum = 0
        m_imgheader_ccrc  = 0
        m_imgheader_csize = 0

        for idx in range(3) :
            ctxtname = 'COMP%d' % (idx+1)
            ctxtaddr = 'COMP%dADDR' % (idx+1)
            #print(ctxtname)
            if self.get_option(ctxtname) != '' :
                ctxt[idx] = gimgmake.LoadContentfile((idx), int(self.get_option(ctxtaddr), 16), self.get_option(ctxtname))

                content_ccrc  = int.from_bytes(ctxt[idx][4:8], "little")
                content_csize = int.from_bytes(ctxt[idx][0:4], "little")
                if idx == 0 :
                    m_imgheader_ccrc  = content_ccrc
                    m_imgheader_csize = content_csize

                ctxt_offset = int.from_bytes(ctxt[idx][12:16], "little")

                if datasize < (ctxt_offset - m_imgheader_offset ) :
                    ctxtfill[idx] = b'\xff' * ((ctxt_offset-m_imgheader_offset)-datasize)
                    datacrc   = gimgmake.crc32(datacrc, ctxtfill[idx], ((ctxt_offset-m_imgheader_offset)-datasize))
                    datasize  = (ctxt_offset-m_imgheader_offset)
                    #print("[7]foffset_wr - %d\r\n" % datasize)
                    #print("datacrc - %08x\r\n" % datacrc)
                else:
                    ctxtfill[idx] = bytes()

                datacrc   = gimgmake.crc32(datacrc, ctxt[idx][16:], content_csize)
                datasize += content_csize
                #print("[8]foffset_wr - %d\r\n" % datasize)
                #print("datacrc - %08x\r\n" % datacrc)
                m_imgbody_ctntnum += 1

        m_imgheader_datcrc = datacrc
        m_imgheader_datsiz = datasize - ( 80 + gimgmake.sfdplen )
	
        self.LOG_PRINT("\r\n\r\nSaveImageFile : FLASH Address - %08x\r\n" % ( m_imgheader_offset ) )
        self.LOG_PRINT("\r\n\r\nSaveImageFile : open - %s\r\n" % self.imgfname)


        m_imgheader_magic = 0x4B394346
        m_imgheader_hcrc = 0
        if self.get_option('VERSION') != '' :
            m_imgheader_version = int( self.get_option('VERSION') , 16 )
        else:
            m_imgheader_version = 0
        m_imgheader_keysize = int( self.get_option('KEYSIZE') ) / 8
        m_imgheader_keynum  = int( self.get_option('KEYLEVEL') )

        loadlist = [ "LOAD_AND_VERIFY", "VERIFY_ONLY_IN_FLASH", "VERIFY_ONLY_IN_MEM", "LOAD_ONLY" ]
        encrlist = [ "NO_IMAGE_ENC", "ICV_CODE_ENC", "OEM_CODE_ENC" ]

        m_imgheader_loadscheme = loadlist.index( self.get_option('KEYLOAD') )
        m_imgheader_encrscheme = encrlist.index( self.get_option('KEYENC') )

        crconlist = [ "OFF", "ON" ]
        
        m_imgheader_crcon  = crconlist.index( self.get_option('CRCON') )
        if( self.get_option('OFFSET') != '' ):
            m_imgheader_cpoint = int( self.get_option('OFFSET'), 16 )
        else:
            m_imgheader_cpoint = 0

        if( m_imgheader_crcon != 1 ):
            self.LOG_PRINT("CRC-OFF : cpoint zero\r\n")
            m_imgheader_cpoint = 0

        m_title = self.get_option('TITLE')
        m_imgheader_title = bytes(m_title, 'ascii')
        m_imgheader_title += b'\x00' * (40 - len(m_title))

        #print(m_imgheader_title)

        m_imgheader_combi =   int(m_imgheader_dbgkey)
        m_imgheader_combi |= (int(m_imgheader_keysize)<<1)
        m_imgheader_combi |= (int(m_imgheader_keynum)<<16)
        m_imgheader_combi |= (int(m_imgheader_loadscheme)<<24)
        m_imgheader_combi |= (int(m_imgheader_encrscheme)<<28)
                            
        m_imgheader = pack('<IIII40sIIIIII'
             , m_imgheader_magic, m_imgheader_hcrc, gimgmake.sfdpcrc, m_imgheader_version
             , m_imgheader_title, m_imgheader_datsiz, m_imgheader_datcrc
             , m_imgheader_combi, m_imgheader_ccrc, m_imgheader_csize, m_imgheader_cpoint
             )

        m_imgheader_hcrc = gimgmake.crc32(0, m_imgheader, 80)

        m_imgheader = pack('<IIII40sIIIIII'
             , m_imgheader_magic, m_imgheader_hcrc, gimgmake.sfdpcrc, m_imgheader_version
             , m_imgheader_title, m_imgheader_datsiz, m_imgheader_datcrc
             , m_imgheader_combi, m_imgheader_ccrc, m_imgheader_csize, m_imgheader_cpoint
             )


        self.LOG_PRINT("TITLE  : \'%s\'\r\n" % m_imgheader_title.decode('ASCII'))
        self.LOG_PRINT("TITLE  : \'%s\'\r\n" % m_imgheader_title)
        self.LOG_PRINT("MAGIC  : %08x\r\n" % m_imgheader_magic)
        self.LOG_PRINT("HCRC   : %08x\r\n" % m_imgheader_hcrc)
        self.LOG_PRINT("SCRC   : %08x\r\n" % gimgmake.sfdpcrc)
        self.LOG_PRINT("VER    : %08x\r\n" % m_imgheader_version)

        self.LOG_PRINT("D.CRC  : %08x\r\n" % m_imgheader_datcrc)
        self.LOG_PRINT("D.SIZE : %08x\r\n" % m_imgheader_datsiz)

        self.LOG_PRINT("keysize    : %d\r\n" % m_imgheader_keysize)
        self.LOG_PRINT("keynum     : %d\r\n" % m_imgheader_keynum)

        self.LOG_PRINT("loadscheme : %d\r\n" % m_imgheader_loadscheme)
        self.LOG_PRINT("encrscheme : %d\r\n" % m_imgheader_encrscheme)

        self.LOG_PRINT("CCRC   : %08x\r\n" % m_imgheader_ccrc)
        self.LOG_PRINT("Csize  : %d\r\n" % m_imgheader_csize)
        self.LOG_PRINT("CPoint : %08x\r\n" % m_imgheader_cpoint)     

        ### PASS-2 #####################################

        self.out += ("WBfile is %s \n" % self.imgfname)
        
        imagefile = open( self.imgfname , 'wb')
        foffset_wr = 80
        imagefile.write(m_imgheader)
        if gimgmake.sfdp != 0 :
            imagefile.write(gimgmake.sfdp)
            self.LOG_PRINT("Write SFDP (%d)\r\n" % gimgmake.sfdplen)
            foffset_wr += gimgmake.sfdplen

        keylevel = self.get_option('KEYLEVEL')

        for idx in range(int(keylevel)) :
            certkeyname = 'CERT%d' % (idx+1)
        
            if self.get_option(certkeyname) != '' :
                imagefile.write(ccert[idx][0:4]) # length
                imagefile.write(ccert[idx][4:8]) # crc
                ccert_size = int.from_bytes(ccert[idx][0:4], "little")
                self.LOG_PRINT("Write [%08x] DBG-CERT-INFO(%d)\r\n" % (foffset_wr, ccert_size) )
                foffset_wr  += 8

        if self.get_option('DBGCERT') != '' :
            imagefile.write(dcert[0:4]) # length
            imagefile.write(dcert[4:8]) # crc
            dcert_size = int.from_bytes(dcert[0:4], "little")
            self.LOG_PRINT("Write [%08x] DBG-CERT-INFO(%d)\r\n" % (foffset_wr, dcert_size) )
            foffset_wr  += 8

        if  (certchainindex & 1) != 0 :
            zerofill = 0
            imagefile.write(zerofill.to_bytes(4,'little'))
            imagefile.write(zerofill.to_bytes(4,'little'))
            self.LOG_PRINT("Write [%08x] CERT-Alignment\r\n" % foffset_wr)
            foffset_wr  += 8

        self.LOG_PRINT("CertChain : %d\r\n" % int(keylevel))

        for idx in range(int(keylevel)) :
            ccertsize = int.from_bytes(ccert[idx][0:4], "little")
            ccert_size = ( int((ccertsize + 15)/16) * 16 )
            imagefile.write(ccert[idx][16:])
            self.LOG_PRINT("Write [%08x] %dth CERT(%d)\r\n" % (foffset_wr, (idx+1), ccert_size) )
            foffset_wr  += ccert_size

        self.LOG_PRINT("DbgCertChain : %d\r\n" % m_imgheader_dbgkey)

        if self.get_option('DBGCERT') != '' :
            dcert_size = int.from_bytes(dcert[0:4], "little")
            dcert_size = ( int((dcert_size + 15)/16) * 16 )
            imagefile.write(dcert[16:])
            self.LOG_PRINT("Write [%08x] DBG-CERT(%d)\r\n" % (foffset_wr, dcert_size) )
            foffset_wr  += dcert_size

        self.LOG_PRINT("ContentChain : %d\r\n" % m_imgbody_ctntnum)    

        for idx in range(3) :
            ctxtname = 'COMP%d' % (idx+1)
            ctxtaddr = 'COMP%dADDR' % (idx+1)
            #print(ctxtname)
            if self.get_option(ctxtname) != '' :
  
                content_ccrc  = int.from_bytes(ctxt[idx][4:8], "little")
                content_csize = int.from_bytes(ctxt[idx][0:4], "little")

                ctxt_offset = int.from_bytes(ctxt[idx][12:16], "little")

                if foffset_wr < (ctxt_offset - m_imgheader_offset ) :
                    self.LOG_PRINT("Fill up [%d] : %08x \r\n" % (idx, ((ctxt_offset-m_imgheader_offset)-foffset_wr)) )
                    imagefile.write(ctxtfill[idx])
                else:
                    self.LOG_PRINT("Illegal Offset [%d] : real offset %08x, target %08x\r\n" % (idx, foffset_wr, (ctxt_offset-m_imgheader_offset)) )

                imagefile.write(ctxt[idx][16:])
                self.LOG_PRINT("Write [%08x]  %dth CONTENT(%d)\r\n"  % (foffset_wr, (idx+1), content_csize) )
                foffset_wr += content_csize
               
                
        imagefile.close()        
       
#########################################################

def mkSecureImage(sysArgsList):
    out = str() 
    err = str() 

    if sys.version_info<(3,0,0):
        err += ("Error: You need python 3.0 or later to run this script")
        err += "\r\n"
        err += (sys.version_info)
        err += "\r\n"
        return out, err #exit(1)

    if len(sysArgsList) < 3 :
        err += ("Error: %s ini-filename image-filename log-filename" % sysArgsList[0] )
        err += "\r\n"
        return out, err #exit(1)

    if len(sys.argv) < 4 :
        logfname = sysArgsList[1] + '.log'
    else:
        logfname = sysArgsList[3]

    gsecuimg = GenSecureImage(sysArgsList[1])

    gsecuimg.set_logname( logfname )

    if gsecuimg.parse() != True:
        err += ("Error: error found in INI file")
        err += "\r\n"
        return out, err #exit(1)

    #if sys.argv[2] != 'CONFIGNAME' :
    #    gsecuimg.set_imagename( sysArgsList[2] )

    gsecuimg.SaveImageFile( )

    pout, perr = gsecuimg.get_console()

    del gsecuimg

    return pout, perr

#############################
if __name__ == "__main__":
    out, err = mkSecureImage(sys.argv)
    print( out )
    print( err )
