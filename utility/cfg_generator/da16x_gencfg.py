# gencfg.py
import os
import sys
import shutil

from sys import exit
from os.path import exists
import tkinter as tk
from tkinter import *
from tkinter import ttk
from tkinter.constants import DISABLED, NORMAL
from tkinter import messagebox

TITLE = "Generate Config"
class Win(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(TITLE)
        # self.attributes("-topmost", True)
        self.protocol('WM_DELETE_WINDOW', self.checkConfig)
        
        combo_with = 300
        combo_height = 230
        screen_with = self.winfo_screenwidth()
        screen_height = self.winfo_screenheight()
        location_x = (screen_with/2) - (combo_with/2)
        location_y = (screen_height/2) - (combo_height/2)
        self.geometry('%dx%d+%d+%d' % (combo_with, combo_height, location_x, location_y))

        #self.geometry("300x220")
        self.resizable(False, False)

        label = ttk.Label(text="Please select a flash memory type")
        label.pack(fill='x', padx=5, pady=5)

        files = []
        fileList = os.listdir("../../tools/SBOOT/SFDP/")
        for file in fileList:
            name = file.split('.', 1)
            files.append(name[0])

        self.sfdp_cb = ttk.Combobox(self, textvariable=tk.StringVar())
        self.sfdp_cb['values'] = files
        self.sfdp_cb['state'] = 'readonly'  # normal
        self.sfdp_cb.pack(fill='x', padx=5, pady=5)
        self.sfdp_cb.bind('<<ComboboxSelected>>', self.sfdp_selected)

        label = ttk.Label(text="Please select the size of the flash memory")
        label.pack(fill='x', padx=5, pady=5)

        #size = ("2MB", "4MB")
        size = ("4MB")
        self.size_cb = ttk.Combobox(self, textvariable=tk.StringVar())
        self.size_cb['values'] = size
        self.size_cb['state'] = DISABLED
        self.size_cb.pack(fill='x', padx=5, pady=5)
        self.size_cb.bind('<<ComboboxSelected>>', self.size_selected)

        label = ttk.Label(text="Please select the partition size to be used")
        label.pack(fill='x', padx=5, pady=5)

        #part_size = ("2MB", "4MB")
        part_size = ("4MB")
        self.part_size_cb = ttk.Combobox(self, textvariable=tk.StringVar())
        self.part_size_cb['values'] = part_size
        self.part_size_cb['state'] = DISABLED
        self.part_size_cb.pack(fill='x', padx=5, pady=5)
        self.part_size_cb.bind('<<ComboboxSelected>>', self.part_size_selected)


        #self.secure_debug=IntVar()
        #check_secure_debug=Checkbutton(self,text="Secure Debug",variable=self.secure_debug)
        #check_secure_debug.pack()

        self.button = ttk.Button(self, text ="Generate", state=DISABLED, command = self.generateConfig)
        self.button.pack(fill='x', padx=5, pady=5)
        pass

    def checkConfig(self, *e):
         file_exists = exists('../../tools/SBOOT/cmconfig/da16xtpmconfig.cfg')
         if file_exists:
            self.destroy()
         else:
            messagebox.showinfo(TITLE, 'Please generate a configuration file. The configuration file is required to build a binary image.')
         
         
    def sfdp_selected(self, *e):
        self.selected_sfdp = self.sfdp_cb.get()
        self.size_cb['state'] = 'readonly'
        self.part_size_cb['state'] = 'readonly'
        self.size_cb.current(0)
        self.selected_size = self.size_cb.get()
        self.part_size_cb.current(0)
        self.selected_part_size = self.part_size_cb.get()
        self.button["state"] = NORMAL
        pass

    def size_selected(self, *e):
        self.selected_size = self.size_cb.get()
        self.part_size_cb['state'] = 'readonly'
        pass

    def part_size_selected(self, *e):
        self.selected_part_size = self.part_size_cb.get()
        self.button["state"] = NORMAL
        pass

    def generateConfig(self, *e):
        # FreeRTOS
        try:
            shutil.copy('../../tools/SBOOT/cmconfig/da16xtpmconfig.cfg.' + self.selected_sfdp + '-' + self.selected_size , '../../tools/SBOOT/cmconfig/da16xtpmconfig.cfg')
        except OSError as why:
            print('Failed to generate config: ', why)
            messagebox.showerror('Error', why)
            exit(0)

        try:
            shutil.copy('../../core/bsp/ldscripts/mem.ld.' + self.selected_part_size, '../../core/bsp/ldscripts/mem.ld')
        except OSError as why:
            print('Failed to generate config: ', why)
            messagebox.showerror('Error', why)
            exit(0)

        messagebox.showinfo(TITLE, 'Completed.')
        self.destroy()
        pass
        
def help():
	print("")
	print("ex)python gencfg.py <sfdp file name> <real size (2M or 4M)> < partition size (2M or 4M)>")
	print("ex)python gencfg.py AT25SL321 4M 4M")
	print("")
	fileList = os.listdir("../../tools/SBOOT/SFDP/")
	print("=== Avaliable flash list ===")
	print(fileList)

def main():
    if len(sys.argv) == 1:
        Win().mainloop()
        exit(0)

    if len(sys.argv) < 4:
	    help()
    else:
        flash = sys.argv[1]
        real_size = sys.argv[2]
        part_size = sys.argv[3]

        file_exists = exists('../../tools/SBOOT/cmconfig/da16xtpmconfig.cfg')
        if file_exists:
            # FreeRTOS
            try:
                shutil.copy('../../tools/SBOOT/cmconfig/da16xtpmconfig.cfg.' + flash + '-' + real_size + 'B', '../../tools/SBOOT/cmconfig/da16xtpmconfig.cfg')
            except OSError as why:
                print('Failed to generate config: ', why)
                exit(0)

            try:
                shutil.copy('./../core/bsp/ldscripts/mem.ld.' + part_size + 'B', './../core/bsp/ldscripts/mem.ld')
            except OSError as why:
                print('Failed to generate config: ', why)
                exit(0)
        else:
            try:
                shutil.copy('../../tools/SBOOT/image/DA16xxx_ueboot.bin.' + part_size +'B', '../../tools/SBOOT/image/DA16xxx_ueboot.bin')
            except OSError as why:
                print('Failed to generate config: ', why)
                exit(0)
            try:
                shutil.copy('../../tools/SBOOT/cmconfig/fc9ktpmconfig.cfg.' + flash + '-' + real_size + 'B', '../../tools/SBOOT/cmconfig/fc9ktpmconfig.cfg')
            except OSError as why:
                print('Failed to generate config: ', why)
                exit(0)
            try:
                shutil.copy('./ldscripts/DA16xxx_rtos_cache.icf.' + part_size +'B', './ldscripts/DA16xxx_rtos_cache.icf')
            except OSError as why:
                print('Failed to generate config: ', why)
                exit(0)
            try:
                shutil.copy('./macros/da16200_asic_cache.mac.' + part_size +'B', './macros/da16200_asic_cache.mac')
            except OSError as why:
                print('Failed to generate config: ', why)
                exit(0)

        print("Completed.")
        
    pass

if __name__ == "__main__":
    # execute only if run as a script
    main()

# End of file