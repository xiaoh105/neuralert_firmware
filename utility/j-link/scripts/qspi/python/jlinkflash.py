# Copyright (C) 2022-2023 Renesas Electronics Corporation and/or its affiliates. All rights reserved.
# This computer program includes confidential and proprietary information belonging to Renesas Electronics Corporation and/or its affiliates. 

from ast import arguments
import sys
import os
import re
import subprocess
import shutil
import threading
import platform
import datetime
import tkinter
from sys import exit

##########################################################

def  arg_parser():
        
        if (not os.name == 'posix') and (platform.win32_ver()[0] == '8' or platform.win32_ver()[0] == '10') :
                flag_osname = 'win'
        else:
                flag_osname = 'linux'

        print('>>> OS:%s' % flag_osname)

        retdict = {}
        if "--cmd" in sys.argv:
                retdict['cmd'] = sys.argv[sys.argv.index("--cmd") + 1]

        if "--exe" in sys.argv:
                retdict['exe'] = sys.argv[sys.argv.index("--exe") + 1]
        else:
                if flag_osname == 'win' :
                        retdict['exe'] = 'JLink.exe'    #### default
                else:
                        retdict['exe'] = 'JLinkExe'     #### default

        if "--jlink_path" in sys.argv:
                retdict['jlink_path'] = sys.argv[sys.argv.index("--jlink_path") + 1]

        if "--proj" in sys.argv:
                retdict['proj'] = sys.argv[sys.argv.index("--proj") + 1]

        if "--script" in sys.argv:
                retdict['script'] = sys.argv[sys.argv.index("--script") + 1]

        if "--log" in sys.argv:
                retdict['log'] = sys.argv[sys.argv.index("--log") + 1]
        else:
                retdict['log'] = './jlink.log'          #### default

        if "--cfg" in sys.argv:
                retdict['cfg'] = sys.argv[sys.argv.index("--cfg") + 1]
        else:
                retdict['cfg'] = './SFLASHList.cfg'     #### default


        if "--img" in sys.argv:
                retdict['img'] = sys.argv[sys.argv.index("--img") + 1]
        else:
                retdict['img'] = './../../img'          #### default

        if "--defaultble" in sys.argv:
                retdict['defaultble'] = sys.argv[sys.argv.index("--defaultble") + 1]
        else:
                retdict['defaultble'] = 'DA14531_1'     #### default

        if "--ble" in sys.argv:
                retdict['ble'] = sys.argv[sys.argv.index("--ble") + 1]

        return retdict

##########################################################

def    TkMessageBox(mbtitle, mtext, listitems):
        ws = tkinter.Tk()
        ws.title(mbtitle)
        ws.geometry('240x260')
        #ws.config(bg='#446644')

        global tkmb_selecteditem
        tkmb_selecteditem = ''

        def showSelected(event):
                global tkmb_selecteditem
                index = lb.curselection()
                tkmb_selecteditem = lb.get(index)
                show.config(text=tkmb_selecteditem)

        def showClosed():
                global tkmb_selecteditem
                if lb.get(tkinter.ANCHOR) != '' and tkmb_selecteditem != '':
                        ws.destroy()

        mlabel = tkinter.Label(ws, text=mtext)
        mlabel.pack()

        lb = tkinter.Listbox(ws, width=30)
        lb.pack()
        for idx, item in enumerate(listitems):
                lb.insert(idx, item)
        lb.bind("<<ListboxSelect>>", showSelected)

        tkinter.Button(ws, text='SELECT !', command=showClosed).pack(pady=20)
        
        show = tkinter.Label(ws)
        show.pack()

        ws.mainloop()

        #if tkmb_selecteditem != '' :
        #        print( 'TkMessageBox:', tkmb_selecteditem )

        return tkmb_selecteditem

##########################################################
# main
##########################################################

if __name__ == "__main__":

        print( '.......................................................................................................................')
        print( '.. DA16x SFLASH Download ..... %s' % datetime.datetime.now())
        print( '.......................................................................................................................')

        jflashargs = arg_parser()

        cfgfilename = jflashargs['cfg']

        sflash_list = {}
        ELFfilename = ''
                                                
        if os.path.isfile(os.path.join(jflashargs['jlink_path'], jflashargs['exe'])) != True :
                print( "%s is not found. Please check it!" % os.path.join(jflashargs['jlink_path'], jflashargs['exe']))
                exit(-1)

        with  open(cfgfilename) as cfgfp :
                lines = cfgfp.readlines()
                
                for rdline in lines :
                        if( rdline[0] != ';'):
                                items = rdline.split(',')
                                items_model = items[0].strip()
                                items_man   = items[1].strip()
                                items_den   = items[2].strip()
                                sflash_list[items_model] = [items_man, items_den]
                        else:
                                items = rdline.split(' ')
                                ELFfilename = items[0][1:]

        ######## WIFI part

        imagepath = jflashargs['img']

        if os.path.isdir(imagepath) != True :
                print('Image Folder is not exist.')
                exit(-1)

        imgfiles = [f for f in os.listdir(imagepath) if os.path.isfile(os.path.join(imagepath, f)) and os.path.splitext(f)[1] == '.img']

        flag_da16200_freertosboot = False
        flag_da16200_freertosrtos = False
        flag_da16600_freertosboot = False
        flag_da16600_freertosrtos = False

        sflashmodel = ''

        #### check SFLASH 
        boot_image_count = 0
        for imgfile in imgfiles:
                print('\t%s' % imgfile)
                if imgfile.startswith("DA16200_FBOOT") == True :
                        boot_image_count = boot_image_count + 1
                elif imgfile.startswith("DA16600_FBOOT") == True :
                        boot_image_count = boot_image_count + 1

        if boot_image_count != 1 :
                sflashmodel = TkMessageBox('SFLASH.List', 'No bootable image found.\nPlease select Target SFLASH !!', sflash_list)
                print('No bootable image found, this is a manually seclected model. : %s' % sflashmodel )

        #### convert IMAGE
        for imgfile in imgfiles:
                if imgfile.startswith("DA16200_FBOOT") == True :
                        flag_da16200_freertosboot = True
                        tokens = re.split("[-_.]+", imgfile)

                        if sflashmodel == '' or sflashmodel == tokens[-2] :
                                sflashmodel = tokens[-2]
                                shutil.copyfile(os.path.join(imagepath, imgfile), './FBOOT.bin' )

                elif imgfile.startswith("DA16200_FRTOS") == True :
                        flag_da16200_freertosrtos = True
                        shutil.copyfile(os.path.join(imagepath, imgfile), './FRTOS.bin' )
                elif imgfile.startswith("DA16600_FBOOT") == True :
                        flag_da16600_freertosboot = True
                        tokens = re.split("[-_.]+", imgfile)

                        if sflashmodel == '' or sflashmodel == tokens[-2] :
                                sflashmodel = tokens[-2]
                                shutil.copyfile(os.path.join(imagepath, imgfile), './FBOOT.bin' )

                elif imgfile.startswith("DA16600_FRTOS") == True :
                        flag_da16600_freertosrtos = True
                        shutil.copyfile(os.path.join(imagepath, imgfile), './FRTOS.bin' )

        ######## BLE part
        #### convert BLE

        if 'ble' in jflashargs :
                blepath = os.path.join(imagepath, jflashargs['ble'])
                if os.path.isdir(blepath) == True :
                        blefiles = [f for f in os.listdir(blepath) if os.path.isfile(os.path.join(blepath, f)) and os.path.splitext(f)[1] == '.img']
                else:
                        blefiles = []
        elif 'defaultble' in jflashargs :
                blepath = os.path.join(imagepath, jflashargs['defaultble'])
                if os.path.isdir(blepath) == True :
                        blefiles = [f for f in os.listdir(blepath) if os.path.isfile(os.path.join(blepath, f)) and os.path.splitext(f)[1] == '.img']
                else:
                        blefiles = []
        else:
                blefiles = []

        flag_ble_imge = False

        for blefile in blefiles:
                print('\t%s' % blefile)
                shutil.copyfile(os.path.join(blepath, blefile), './BLE.bin' )
                flag_ble_imge = True

        ######## JLINK

        if jflashargs['cmd'] == 'program_all':
                if flag_ble_imge == True :
                        jflashargs['cmd'] =  'program_16600'
                else:
                        jflashargs['cmd'] =  'program_16200'


        flag_jlink_run = True
        if jflashargs['cmd'].startswith('program_ble') == True:
                if flag_ble_imge == True :
                        flag_jlink_run = True
                else:
                        print('>>> BLE image not found.' )
                        flag_jlink_run = False

        elif jflashargs['cmd'].startswith('program_boot') == True:
                if flag_da16200_freertosboot == True or flag_da16600_freertosboot == True :
                        flag_jlink_run = True
                else:
                        print('>>> BOOT image not found.' )
                        flag_jlink_run = False

        elif jflashargs['cmd'].startswith('program_rtos') == True:
                if flag_da16200_freertosrtos == True or flag_da16600_freertosrtos == True :
                        flag_jlink_run = True
                else:
                        print('>>> RTOS image not found.' )
                        flag_jlink_run = False

        elif jflashargs['cmd'].startswith('program_16200') == True or jflashargs['cmd'].startswith('program_16600') == True :
                if flag_da16200_freertosboot == True and flag_da16200_freertosrtos == True :
                        flag_jlink_run = True
                elif flag_da16600_freertosboot == True and flag_da16600_freertosrtos == True :
                        flag_jlink_run = True
                else:
                        print('>>> Downloadable images (BOOT and RTOS) not found.' )
                        flag_jlink_run = False

        elif jflashargs['cmd'].startswith('program') == True:
                if flag_da16200_freertosboot == True or flag_da16200_freertosrtos == True :
                        flag_jlink_run = True
                elif flag_da16600_freertosboot == True or flag_da16600_freertosrtos == True :
                        flag_jlink_run = True
                elif flag_ble_imge == True :
                        flag_jlink_run = True
                else:
                        print('>>> Downloadable images not found.' )
                        flag_jlink_run = False


        if flag_jlink_run == True:
                str_density = ''
        
                if sflashmodel == '' :
                        print('>>> SFLASH model not found. Default 2MB' )
                        str_density = '2MB'

                elif sflash_list.get(sflashmodel) != None :
                        print('>>> %s : %s' % (sflashmodel, sflash_list[sflashmodel][1]) )
                        str_density = sflash_list[sflashmodel][1]
                else:
                        print('>>> %s : %s' % (sflashmodel, 'not supported') )

                if str_density != '' :
                        JlinkCommand = '"%s/%s" -Device %s -Log "%s" -CommandFile "%s.jlink"' % (
                                                        jflashargs['jlink_path']
                                                        , jflashargs['exe']
                                                        , ELFfilename
                                                        , jflashargs['log']
                                                        , jflashargs['cmd']
                                        )

                        print(JlinkCommand)

                        jlinkprocess = None
                        def jlinkrun():
                                jlinkprocess = subprocess.Popen(JlinkCommand
                                                , stdout=subprocess.PIPE
                                                , stderr=subprocess.PIPE
                                                , cwd=os.getcwd()
                                                , universal_newlines=None
                                                , shell=True
                                                )
                                logout, logerr = jlinkprocess.communicate()
                                print( logout.decode().replace('\r\n', '\n') )
                                print( logerr.decode().replace('\r\n', '\n') )

                        jthread = threading.Thread(target=jlinkrun)
                        jthread.start()
                        jthread.join(timeout=(60*6)) # 6 min

                        if jthread.is_alive():
                                print( '"%s/%s" is terminated due to hang-up' % (jflashargs['jlink_path'], jflashargs['exe']) )                 
                                jlinkprocess.terminate()
                                jthread.join()



        if os.path.isfile('./FBOOT.bin'):
                os.remove('./FBOOT.bin')
        if os.path.isfile('./FRTOS.bin'):
                os.remove('./FRTOS.bin')
        if os.path.isfile('./BLE.bin'):
                os.remove('./BLE.bin')

        exit(0)
