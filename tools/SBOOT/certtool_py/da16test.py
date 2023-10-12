#!/bin/python3

import sys
import os
import datetime
import string
from string import whitespace
import configparser
import struct

from fc9ksecuutil import *

##########################################################



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

    print("CFG file is %s" % man_cfgfile )

    manobj = manconfig(man_cfgfile)
    manobj.parse()

    print( 'UEBOOT:'   , manobj.getrawconfig('UEBOOT', 'SFDP') )
    print( 'RTOS.sram:', manobj.getrawconfig('RTOS.sram', 'SFDP') )
	
    ueboot_sfdp = manobj.getrawconfig('UEBOOT', 'SFDP')
    sram_sfdp = manobj.getrawconfig('RTOS.sram', 'SFDP')

    for filename in os.listdir(('./SFDP')):
        fname, fext = os.path.splitext(filename)
        if( fext == '.bin' ):
            print( fname )

            submanobj = manconfig(man_cfgfile)
            submanobj.parse()
            
            submanobj.setrawconfig('UEBOOT', 'SFDP', fname)
            submanobj.setrawconfig('RTOS.sram', 'SFDP', fname)

            submanobj.update()
            del submanobj

            pythoncmd = "./certtool/da16secutool.exe -mod secuboot -cfg %s  -tpm ./cmtpmcfg" % man_cfgfile
                
            p = subprocess.Popen(pythoncmd
                         , stdout=subprocess.PIPE
                         , stderr=subprocess.PIPE
                        )

            out, err = p.communicate()

            print( pythoncmd, fname )
            
            
    manobj.setrawconfig('UEBOOT', 'SFDP', ueboot_sfdp)
    manobj.setrawconfig('RTOS.sram', 'SFDP', sram_sfdp)       
    
    manobj.update()
	
	
