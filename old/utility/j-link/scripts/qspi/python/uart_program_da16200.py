# Copyright (C) 2022-2023 Renesas Electronics Corporation and/or its affiliates. All rights reserved.
# This computer program includes confidential and proprietary information
# belonging to Renesas Electronics Corporation and/or its affiliates.

import os
import math
import time
import logging
import serial
import serial.tools.list_ports as sp
from enum import Enum
from ymodem.Modem import Modem
import sys
import threading
import platform
import datetime
from sys import exit


class Return_Status(Enum):
    OK = 0
    NOK = -1


class Mode(Enum):
    CONSOLE = 0
    COMMAND = 1
    DLOAD = 2
    EMODE = 3


class Status(Enum):
    INIT = 1
    OPEN = 2
    RESET = 3
    MROM = 4
    OTP_LOCK_START = 5
    OTP_LOCK_OK = 6
    FLASH_ID_START = 7
    FLASH_ID_END = 8
    RAM_ID_START = 9
    RAM_ID_END = 10
    START = 11
    READY = 12
    DOING = 13
    DONE = 14
    FAIL = 15


class Download_info:
    enable_debug = False
    exit_thread = False
    mode = Mode.CONSOLE
    now_status = Status.INIT
    flash_id = -1
    ram_id = -1
    port_name = ''
    serial_port = None
    log_f = None
    boot_fullpath = ''
    log_path = ''
    command_list = []
    address_list_for_eclipse = []


def strip_end(text, suffix):
    if suffix and text.endswith(suffix):
        return text[:-len(suffix)]
    return text

def eclipse_print_workaround(out_string, end, end_string):
    if end:
        print(out_string, end=end_string)
    else:
        print(out_string)
    sys.stdout.flush()


def serial_read_thread(download_info):
    download_info.serial_port.timeout = 0.5

    while not download_info.exit_thread:
        read_data = download_info.serial_port.readline()
        if read_data == b'' or read_data.decode('utf-8') == '\n':
            continue

        try:
            read_data.decode('utf-8')
        except:
            continue

        if "\n" in read_data.decode('utf-8'):
            have_return = True
        else:
            have_return = False

        rdata = strip_end(read_data.decode('utf-8'), '\r\n')
        rdata = strip_end(rdata, '\n')

        if download_info.enable_debug or download_info.mode == Mode.CONSOLE:
            print_data = str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]) + ':' + rdata
            if have_return == True:
                eclipse_print_workaround(print_data, False, '')
            else:
                eclipse_print_workaround(print_data, True, " ")

            if download_info.log_f is not None:
                download_info.log_f.write('\n' + print_data)

        if download_info.now_status == Status.RESET:
            if 'http://www.fci.co.kr' in rdata:
                download_info.serial_port.write("\n\n".encode('utf-8'))
            elif '[MROM]' in rdata:
                download_info.now_status = Status.MROM
        elif download_info.now_status == Status.START:
            if 'Load Y-Modem' in rdata:
                download_info.now_status = Status.READY
        elif download_info.now_status == Status.DOING:
            if '## Total Size' in rdata:
                download_info.now_status = Status.DONE
        elif download_info.now_status == Status.FLASH_ID_START:
            if str(rdata).__contains__('SFLASH'):
                data_split = str(rdata).split(':')
                download_info.flash_id = data_split[1]
                download_info.now_status = Status.FLASH_ID_END
        elif download_info.now_status == Status.RAM_ID_START:
            if str(rdata).__contains__('[00F80040]'):
                data_split = str(rdata).split(' ')
                if (data_split[5] + data_split[4] + data_split[3] + data_split[2]) != "50444653":
                    download_info.ram_id = "ffffff"
                else:
                    download_info.ram_id = data_split[9] + data_split[8] + data_split[7] + data_split[6]
                download_info.now_status = Status.RAM_ID_END
        elif download_info.now_status == Status.OTP_LOCK_START:
            if str(rdata).__contains__('[0x40103FFC]'):
                if str(rdata).__contains__('0x00000001'):
                    download_info.now_status = Status.OTP_LOCK_OK

class TaskProgressBar():
    global g_progress_num

    def __init__(self):
        self.bar_width = 50
        self.last_task_name = ""
        self.current_task_start_time = -1

    def show(self, task_index, task_name, total, success, failed):
        if task_name != self.last_task_name:
            if self.last_task_name != "":
                eclipse_print_workaround('\n', True, '')
            self.last_task_name = task_name

        progress =  (success * 100 / total)
        ttime = datetime.datetime.now().timestamp() - download_info.stime_download
        if progress > (self.pre_progress + 2) or progress == 0 or progress == 100:
            sys.stdout.flush()
            eclipse_print_workaround(f'\rDownload file {g_progress_num}: {task_name} '
                                     f': {progress:.2f}% : {ttime:.2f}s', True, '')
            self.pre_progress = progress


def arg_parser():
    if (not os.name == 'posix') and (platform.win32_ver()[0] == '8' or platform.win32_ver()[0] == '10'):
        flag_osname = 'Windows'
    else:
        flag_osname = 'Linux'

    eclipse_print_workaround('>>> Running on %s' % flag_osname, False, '')

    retdict = {}

    if "--type" in sys.argv:
        retdict['type'] = sys.argv[sys.argv.index("--type") + 1]

    if "--cmd" in sys.argv:
        retdict['cmd'] = sys.argv[sys.argv.index("--cmd") + 1]

    if "--exe" in sys.argv:
        retdict['exe'] = sys.argv[sys.argv.index("--exe") + 1]
    else:
        if flag_osname == 'win':
            retdict['exe'] = 'JLink.exe'
        else:
            retdict['exe'] = 'JLinkExe'

    if "--jlink_path" in sys.argv:
        retdict['jlink_path'] = sys.argv[sys.argv.index("--jlink_path") + 1]

    if "--proj" in sys.argv:
        retdict['proj'] = sys.argv[sys.argv.index("--proj") + 1]

    if "--script" in sys.argv:
        retdict['script'] = sys.argv[sys.argv.index("--script") + 1]

    if "--log" in sys.argv:
        retdict['log'] = sys.argv[sys.argv.index("--log") + 1]
    else:
        retdict['log'] = './jlink.log'

    if "--cfg" in sys.argv:
        retdict['cfg'] = sys.argv[sys.argv.index("--cfg") + 1]
    else:
        retdict['cfg'] = './SFLASHList.cfg'

    if "--img" in sys.argv:
        retdict['img'] = sys.argv[sys.argv.index("--img") + 1]
    else:
        retdict['img'] = './../../img'

    if "--defaultble" in sys.argv:
        retdict['defaultble'] = sys.argv[sys.argv.index("--defaultble") + 1]
    else:
        retdict['defaultble'] = 'DA14531_1'

    if "--ble" in sys.argv:
        retdict['ble'] = sys.argv[sys.argv.index("--ble") + 1]

    return retdict


def select_com_port(download_info):
    cmd = ''
    ports = []
    retry = 0

    while cmd == '' or int(cmd) >= len(ports):
        ports = list(serial.tools.list_ports.comports())
        connected = []
        num = 0
        ports.sort()
        for p in ports:
            eclipse_print_workaround(str(num) + '. ' + str(p), False, '')
            num += 1

        if num == 0:
            eclipse_print_workaround('No valid port. Please check connection!', False, '')
            finish_program(download_info)

        eclipse_print_workaround('Please enter number in the list of your COM port and press enter.', False, '')
        eclipse_print_workaround('--> ', True, '')
        input_string = input()
        cmd = strip_end(input_string, '\r\n')

        if cmd.isdigit():
            if cmd == '' or int(cmd) >= len(ports):
                retry += 1
                if retry > 3:
                    finish_program(download_info)
                eclipse_print_workaround('Please enter correct number!', False, '')
        else:
            eclipse_print_workaround('Only number is allowed!', False, '')
            cmd = ''

    eclipse_print_workaround('Selected COM port =  ' + str(ports[int(cmd)]), False, '')

    return str(ports[int(cmd)])


def help_print(download_info):
    eclipse_print_workaround('(python) uart_program_da16200.exe(py) [-h] [-l log_file_path]'
                             ' [-i [port_name] address image_path] [-f setting_file_path]', False, '')
    eclipse_print_workaround('1. Console mode : (python) uart_program_da16200.exe(py)', False, '')
    eclipse_print_workaround('2. Console mode and save logs : (python) uart_program_da16200.exe(py) '
                             '-l log.txt', False, '')
    eclipse_print_workaround('3. Inline download mode 1: (python) uart_program_da16200.exe(py)'
                             ' -i 0 d:\img\DA16600_FBOOT-GEN01-01-922f1e27d_W25Q32JW.img', False, '')
    eclipse_print_workaround('4. Inline download mode 2: (python) uart_program_da16200.exe(py) '
                             '-i COM80 0 d:\img\DA16600_FBOOT-GEN01-01-922f1e27d_W25Q32JW.img', False, '')
    eclipse_print_workaround('5. Download mode with settings in a file  '
                             ': (python) uart_program_da16200.exe(py) -f uart_all_settings.txt', False, '')
    eclipse_print_workaround('6. Enter emergency boot mode  : (python) uart_program_da16200.exe(py) -e', False, '')
    eclipse_print_workaround('Program can be finished by Ctrl+C anytime.', False, '')
    eclipse_print_workaround('## Commands in console mode ##', False, '')
    eclipse_print_workaround('1. ''dload'': Run download mode in MROM.', False, '')
    eclipse_print_workaround('2. ''emode'': Run emergency mode.', False, '')
    eclipse_print_workaround('3. ''exit'': Finish program.', False, '')

    return Return_Status.NOK

def print_file_list():
    num_file = 0
    files = []
    for f in os.listdir('./'):
        if os.path.isfile(os.path.join('./', f)) and os.path.splitext(f)[1] == '.img':
            eclipse_print_workaround(str(num_file) + '. ' + f, False, '')
            num_file += 1
            files.append(f)
    return files


def get_address_filepath(download_info):
    eclipse_print_workaround('Please enter address (0 ~ 3FF000) and press enter: ', True, '')
    address = input()
    num_retry = 0
    while (address == '' or int(strip_end(address,'\r\n'), 16) < 0 or int(strip_end(address,'\r\n'),
                                                                             16) > 0x3FF000):
        eclipse_print_workaround('Out of range. Please enter address (0 ~ 3FF000) and press enter: ', True, '')
        num_retry += 1
        if num_retry == 3:
            eclipse_print_workaround("Please check address. Exit!", False, '')
            return Return_Status.NOK
        else:
            address = input()

    filepath = ''
    list_files = print_file_list()
    num_retry = 0
    if len(list_files) == 0:
        while filepath == '':
            eclipse_print_workaround('Please enter file name (or full path) and press enter: ', True, '')
            input_string = input()
            filepath = strip_end(input_string, '\r\n')
    else:
        while filepath == '':
            eclipse_print_workaround('Please enter file name or number in list, and press enter: ', True, '')
            input_string = input()
            filepath = strip_end(input_string, '\r\n')
            is_int = True
            try:
                int(filepath)
            except ValueError:
                is_int = False

            if is_int == True:
                if int(filepath) >= 0 and int(filepath) < len(list_files):
                    filepath = list_files[int(filepath)]
                else:
                    eclipse_print_workaround(f"Entered Incorrect numbers : {filepath}", False, '')
                    filepath = ''
                    list_files = print_file_list()

            num_retry += 1
            if num_retry == 3:
                eclipse_print_workaround(f"Please check entered numbers. Exit! : {filepath}", False, '')
                return Return_Status.NOK

    if not os.path.exists(strip_end(filepath, '\r\n')):
        eclipse_print_workaround(f"Not existed file : {filepath}! Please check the file is existed. ", False, '')
        return Return_Status.NOK

    download_info.command_list.clear()
    download_info.command_list.append(download_info.port_name)
    download_info.command_list.append(address + " " + filepath)
    if address == "0":
        download_info.boot_fullpath = filepath

    return Return_Status.OK


def run_reboot_command():
    download_info.serial_port.write("\n\n\n".encode('utf-8'))
    time.sleep(0.1)
    download_info.serial_port.write("reboot\n".encode('utf-8'))

def run_console_mode(download_info):
    while True:
        input_string = sys.stdin.readline()
        if strip_end(input_string, '\n') == 'exit':
            return
        elif strip_end(input_string, '\n') == 'dload':
            close_console_mode(download_info)
            ret = get_address_filepath(download_info)
            if ret == Return_Status.OK:
                ret = run_download(download_info, False, False)

                if ret == Return_Status.OK:
                    yes_no = 'yn'
                    while yes_no != 'n':
                        while yes_no != 'y' and yes_no != 'n':
                            eclipse_print_workaround("Will you continue to download? y or n: ", True, '')
                            input_string = input()
                            yes_no = strip_end(input_string, '\n')

                        if yes_no == 'n':
                            break

                        ret = get_address_filepath(download_info)

                        if ret == Return_Status.OK:
                            ret = run_download(download_info, False, False)
                            yes_no = 'yn'
                        else:
                            break

            download_info.mode = Mode.CONSOLE
            open_console_mode(download_info, False)
            #run_reboot_command()

        elif strip_end(input_string, '\n') == 'emode':
            close_console_mode(download_info)
            run_emode(download_info)
            open_console_mode(download_info, False)
        else:
            download_info.serial_port.write((input_string).encode('utf-8'))


def check_MROM_state(download_info):
    max_wait_number = 0
    eclipse_print_workaround('Checking device is in MROM...', False, '')
    download_info.serial_port.write("\n\n".encode('utf-8'))
    while download_info.now_status != Status.MROM:
        time.sleep(0.1)
        max_wait_number += 1
        if max_wait_number > 4:
            break


def lock_otp_in_MROM(download_info):
    # OTP LOCK start
    download_info.now_status = Status.OTP_LOCK_START
    eclipse_print_workaround('Locking OTP...', False, '')
    retry = 0
    while True:
        retry = retry + 1
        download_info.serial_port.write("lwr 40120000 34000000\n".encode('utf-8'))
        time.sleep(0.1)
        download_info.serial_port.write("lwr 40103ffc 04000000\n".encode('utf-8'))
        time.sleep(0.1)
        download_info.serial_port.write("lwr 40120000 38000000\n".encode('utf-8'))
        time.sleep(0.1)
        #Test
        download_info.serial_port.write("lrd 40103ffc\n".encode('utf-8'))
        time.sleep(0.1)
        if download_info.now_status == Status.OTP_LOCK_OK:
            break
        if retry > 5:
            eclipse_print_workaround('Failed locking OTP!!', False, '')
            break

def prepare_download(download_info, reset):
    eclipse_print_workaround(str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3])
                             + " Preparing for download...", False, '')
    # Enter MROM state for download
    download_info.now_status = Status.RESET
    check_MROM_state(download_info)
    max_wait_number = 0

    if reset == True:
        if download_info.now_status == Status.RESET:
            eclipse_print_workaround('Resetting device...', False, '')
            download_info.serial_port.write("\n\nreset\n".encode('utf-8'))
            max_wait_number = 0
            no_response_loop = 0
            while download_info.now_status != Status.MROM:
                max_wait_number += 1
                if max_wait_number > 50:
                    eclipse_print_workaround('Trying to Reset device...', False, '')
                    download_info.serial_port.write("\n\nreset\n".encode('utf-8'))
                    max_wait_number = 0
                    no_response_loop += 1
                    if no_response_loop > 5:
                        eclipse_print_workaround('No response from device. Please check the power supply'
                                                 ', connections or DA16200 is under DPM sleep.', False, '')
                        finish_program(download_info)
                time.sleep(0.1)
    else:
        if download_info.now_status != Status.MROM:
            eclipse_print_workaround("Device is not in MROM! Please confirm the device is in MROM.", False,
                                     '')
            return Return_Status.NOK

    lock_otp_in_MROM(download_info)

    if download_info.now_status != Status.OTP_LOCK_OK:
        return Return_Status.NOK
    else:
        return check_flash_id_boot_ram(download_info)


def check_flash_id_boot_ram(download_info):
    boot_id = ''
    download_info.flash_id = ''
    download_info.ram_id = ''

    eclipse_print_workaround('Checking flash ID...', False, '')
    download_info.now_status = Status.FLASH_ID_START
    download_info.serial_port.write("sflash info\n".encode('utf-8'))
    while download_info.now_status != Status.FLASH_ID_END:
        time.sleep(0.1)

    download_info.now_status = Status.RAM_ID_START
    download_info.serial_port.write("brd f80040 8\n".encode('utf-8'))
    while download_info.now_status != Status.RAM_ID_END:
        time.sleep(0.1)

    if download_info.boot_fullpath != '':
        eclipse_print_workaround('Read ' + download_info.boot_fullpath, False, '')
        f = open(download_info.boot_fullpath, 'rb')
        data_bytes = f.read(96)
        f.close()
        boot_id = data_bytes[87].to_bytes(1, 'little').hex() + data_bytes[86].to_bytes(1, 'little').hex() + data_bytes[
            85].to_bytes(1, 'little').hex() + data_bytes[84].to_bytes(1, 'little').hex()

    if download_info.boot_fullpath == '' and download_info.ram_id == 'ffffff':
        eclipse_print_workaround('No SFDP information in RAM. Need to download bootloader!', False, '')
        return Return_Status.NOK

    elif download_info.boot_fullpath != '' and download_info.flash_id != boot_id:
        sys.stdout.flush()
        eclipse_print_workaround('Not matched flash id between flash and bootloader!', False, '')
        eclipse_print_workaround('flash id in flash : ' + download_info.flash_id, False, '')
        eclipse_print_workaround('flash id in bootloader : ' + boot_id, False, '')
        return Return_Status.NOK

    return Return_Status.OK


def run_ymodem(download_info):
    global g_progress_num

    eclipse_print_workaround(str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3])
                             + " Ready for download.", False, '')

    def sender_read(size, timeout=3):
        download_info.serial_port.timeout = timeout
        return download_info.serial_port.read(size) or None

    def sender_write(data, timeout=0):
        download_info.serial_port.writeTimeout = timeout
        size = download_info.serial_port.write(data)
        download_info.serial_port.flush()
        return size

    sender = Modem(sender_read, sender_write)
    g_progress_num = 1
    progress_bar = TaskProgressBar()

    loop = 0
    ret = False

    while loop < (len(download_info.command_list) - 1):
        cmd = download_info.command_list[loop + 1].split()

        if cmd[0] == '' or cmd[1] == '':
            eclipse_print_workaround('Need address or file path!', False, '')
            finish_program(download_info)

        if not cmd[0].__contains__('#'):
            download_info.now_status = Status.START
            if cmd[0] == '0':
                download_info.serial_port.write(('loady ' + cmd[0] + ' 1000\n').encode('utf-8'))
            else:
                download_info.serial_port.write(('loady ' + cmd[0] + ' 1000 bin\n').encode('utf-8'))

            time.sleep(1)
            download_info.serial_port.read_all()
            file_path = os.path.abspath(cmd[1])

            download_info.now_status = Status.DOING
            progress_bar.pre_progress = 0
            download_info.stime_download = datetime.datetime.now().timestamp()
            ret = sender.send([file_path], timeout=30, callback=progress_bar.show)
            g_progress_num += 1
        else:
            ret = True

        if not ret:
            break
        loop += 1

    if not ret:
        download_info.now_status = Status.FAIL
        eclipse_print_workaround("\n" + str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3])
                                 + " Failed to download!", False, '')
    else:
        download_info.now_status = Status.DONE
        eclipse_print_workaround("\n" + str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3])
                                 + " Done successfully.", False, '')


def parser_eclipse_argument(download_info):
    download_info.port_name = select_com_port(download_info)
    download_info.command_list.append(download_info.port_name)

    cmd_args = arg_parser()
    image_path = cmd_args['img']

    if not os.path.isdir(image_path):
        eclipse_print_workaround('Image Folder is not exist!', False, '')
        return Return_Status.NOK

    all_images = []
    ble_images = []

    for f in os.listdir(image_path):
        if os.path.isfile(os.path.join(image_path, f)) and os.path.splitext(f)[1] == '.img':
            all_images.append('/' + f)

    if os.path.exists(image_path + "/DA14531_1/"):
        for f in os.listdir(image_path + "/DA14531_1/"):
            if os.path.isfile(os.path.join(image_path + "/DA14531_1/", f)) and os.path.splitext(f)[1] == '.img':
                all_images.append("/DA14531_1/" + f)

    if os.path.exists(image_path + "/DA14531_2/"):
        for f in os.listdir(image_path + "/DA14531_2/"):
            if os.path.isfile(os.path.join(image_path + "/DA14531_2/", f)) and os.path.splitext(f)[1] == '.img':
                all_images.append("/DA14531_2/" + f)

    image_boot = ''
    image_rtos = ''
    image_ble = ''
    select_ble = ''

    if cmd_args['type'] == 'boot_rtos_ble' or cmd_args['type'] == 'ble':
        while select_ble == '' or int(select_ble) >= ble_images.__len__():
            image_num = 0
            num = 0
            for p in all_images:
                if str(all_images[image_num]).__contains__("da14531"):
                    ble_images.append(all_images[image_num])
                    eclipse_print_workaround(str(num) + '. ' + str(p), False, '')
                    num += 1
                image_num += 1

            if num == 0:
                eclipse_print_workaround("There is no image for DA14531!", False, '')
                return Return_Status.NOK

            eclipse_print_workaround('Select BLE image: ', True, '')
            input_string = input()
            select_ble = strip_end(input_string, '\r\n')

        image_ble = ble_images[int(select_ble)]

    if cmd_args['type'] == 'boot':
        for image in all_images:
            if image.__contains__('FBOOT'):
                image_boot = image
        if image_boot == '':
            eclipse_print_workaround("No BOOT image. Exit!", False, '')
            return Return_Status.NOK

    elif cmd_args['type'] == 'rtos':
        for image in all_images:
            if image.__contains__('FRTOS'):
                image_rtos = image
        if image_rtos == '':
            eclipse_print_workaround("No RTOS image. Exit!", False, '')
            return Return_Status.NOK

    elif cmd_args['type'] == 'boot_rtos' or cmd_args['type'] == 'boot_rtos_ble':
        for image in all_images:
            if image.__contains__('FBOOT'):
                image_boot = image
            if image.__contains__('FRTOS'):
                image_rtos = image
        if image_boot == '':
            eclipse_print_workaround("No BOOT image. Exit!", False, '')
            return Return_Status.NOK
        if image_rtos == '':
            eclipse_print_workaround("No RTOS image. Exit!", False, '')
            return Return_Status.NOK

    if parser_command_from_address_settings_file(download_info) == Return_Status.NOK:
        return Return_Status.NOK

    if image_boot != '':
        address = -1
        download_info.boot_fullpath = image_path + image_boot
        for cmd_line in download_info.address_list_for_eclipse:
            cmds = cmd_line.split(' ')
            if cmds[0] == 'BOOT':
                address = strip_end(cmds[1], '\n')
                break

        if address == -1:
            eclipse_print_workaround("BOOT image address is not existed in the settings file.", False, '')
            return Return_Status.NOK

        download_info.command_list.append(address + " " + image_path + image_boot)

    if image_rtos != '':
        address = -1
        for cmd_line in download_info.address_list_for_eclipse:
            cmds = cmd_line.split(' ')
            if cmds[0] == 'RTOS':
                address = strip_end(cmds[1], '\n')
                break

        if address == -1:
            eclipse_print_workaround("RTOS image address is not existed in the settings file.", False, '')
            return Return_Status.NOK

        download_info.command_list.append(address + " " + image_path + image_rtos)

    if image_ble != '':
        address = -1
        for cmd_line in download_info.address_list_for_eclipse:
            cmds = cmd_line.split(' ')
            if cmds[0] == 'BLE':
                address = strip_end(cmds[1], '\n')
                break

        if address == -1:
            eclipse_print_workaround("BLE image address is not existed in the settings file.", False, '')
            return Return_Status.NOK

        download_info.command_list.append(address + " " + image_path + image_ble)

    return Return_Status.OK


def parser_command_from_all_settings_file(download_info):
    fp = ''
    if len(sys.argv) == 2:
        file_name = 'uart_all_settings.txt'
    else:
        file_name = str(sys.argv[2])

    fp = open(file_name, 'r')
    if fp == '':
        eclipse_print_workaround(f"Not existed file : {file_name}! Please check the file is existed. ", False, '')
        return Return_Status.NOK

    download_info.command_list = fp.readlines()
    fp.close()

    if len(download_info.command_list) == 0:
        eclipse_print_workaround('No command line for download!', False, '')
        return Return_Status.NOK

    for cmd in download_info.command_list:
        cmds = cmd.split(' ')
        if cmds[0] == '0':
            download_info.boot_fullpath = strip_end(cmds[1], '\n')
            break

    return Return_Status.OK


def parser_command_from_address_settings_file(download_info):
    fp = ''
    file_name = 'uart_address_settings.txt'

    fp = open(file_name, 'r')
    if fp == '':
        eclipse_print_workaround(f"Not existed file : {file_name}! Please check the file is existed. ", False, '')
        return Return_Status.NOK

    download_info.address_list_for_eclipse = fp.readlines()
    fp.close()

    if len(download_info.address_list_for_eclipse) == 0:
        eclipse_print_workaround('No information about address!', False, '')
        return Return_Status.NOK

    return Return_Status.OK


def parser_console_mode(download_info):
    download_info.port_name = select_com_port(download_info)
    download_info.command_list.append(download_info.port_name)
    if len(sys.argv) == 3:
        download_info.log_path = sys.argv[2]

    return Return_Status.OK


def parser_inline_command(arg_num, download_info):
    if arg_num == 4:
        download_info.port_name = select_com_port(download_info)
        download_info.command_list.append(download_info.port_name)
        file_path = sys.argv[3]
        download_info.command_list.append(sys.argv[2] + " " + sys.argv[3])
        if sys.argv[2] == "0":
            download_info.boot_fullpath = str(sys.argv[3])
    else:
        download_info.command_list.append(sys.argv[2])  # port name
        file_path = sys.argv[4]
        download_info.command_list.append(sys.argv[3] + " " + sys.argv[4])
        if sys.argv[3] == "0":
            download_info.boot_fullpath = str(sys.argv[4])

    if not os.path.exists(strip_end(file_path, '\r\n')):
        eclipse_print_workaround(f"Not exist file : {file_path}! Please check the file is existed. ", False, '')
        return Return_Status.NOK
    else:
        return Return_Status.OK


def finish_program(download_info):
    close_console_mode(download_info)
    exit()


def print_out_serial_open_error(e):
    eclipse_print_workaround(e, False, '')
    eclipse_print_workaround('Failed to open COM port.', False, '')
    eclipse_print_workaround('Please check below things.', False, '')
    eclipse_print_workaround(' 1. HW connection of COM port.', False, '')
    eclipse_print_workaround(' 2. pyserial package is installed.', False, '')
    eclipse_print_workaround(' 3. Port is already opened by other operation.', False, '')
    eclipse_print_workaround(' 4. Terminate all launches in Eclipse.', False, '')
    eclipse_print_workaround('    (Console->Click button \"Remove Terminate all launches\" '
                             'and \"Terminate\")', False, '')


def open_console_mode(download_info, hide_mode):
    if not hide_mode:
        eclipse_print_workaround('Entering console mode...', False, '')
    try:
        download_info.serial_port = serial.Serial(
            port=download_info.port_name,
            baudrate='230400',
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS)
    except Exception as e:
        print_out_serial_open_error(e)
        finish_program(download_info)

    download_info.serial_port.flush()
    download_info.now_status = Status.OPEN
    download_info.exit_thread = False

    if download_info.log_path != '':
        try:
            log_f = open(download_info.log_path, 'a')

        except:
            eclipse_print_workaround('Can not open log file:' + download_info.log_path, False, '')
            finish_program(download_info)

    p_read_thread = threading.Thread(target=serial_read_thread, args=(download_info,))
    p_read_thread.start()
    if not hide_mode:
        eclipse_print_workaround('Ready for console mode. Input anything.', False, '')


def close_console_mode(download_info):
    download_info.exit_thread = True
    time.sleep(1)
    if download_info.serial_port is not None:
        download_info.serial_port.close()
    if download_info.log_f is not None:
        download_info.log_f.close()


def run_ymodem_download(download_info, reboot):
    try:
        download_info.serial_port = serial.Serial(
            port=download_info.port_name,
            baudrate='230400',
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS)
    except Exception as e:
        print_out_serial_open_error(e)
        return

    run_ymodem(download_info)

    if reboot:
        run_reboot_command()

    download_info.serial_port.close()


def run_download(download_info, need_mrom, reboot):
    eclipse_print_workaround('Entering download mode...', False, '')

    open_console_mode(download_info, True)
    ret = prepare_download(download_info, need_mrom)
    close_console_mode(download_info)

    if ret == Return_Status.OK:
        run_ymodem_download(download_info, reboot)

    return ret

def run_emode(download_info):
    eclipse_print_workaround('Entering emergency mode...', False, '')
    res = ''
    while res != 'Y':
        eclipse_print_workaround('Confirm turn off device. Then enter Y and press Enter.', False, '')
        res = input()
    eclipse_print_workaround('Turn on device within 5 sec.', False, '')
    send_esc_key(download_info)


def send_esc_key(download_info):
    try:
        download_info.serial_port = serial.Serial(
            port=download_info.port_name,
            baudrate='230400',
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=5)
    except Exception as e:
        print_out_serial_open_error(e)
        return

    now_time = datetime.datetime.now().timestamp()
    pre_time = now_time
    start_time = pre_time
    while True:
        now_time = datetime.datetime.now().timestamp()
        if (now_time - start_time) > 6:
            break
        if (now_time - pre_time) > 0.0005:
            pre_time = now_time
            download_info.serial_port.write("\x1B".encode('utf-8'))

    download_info.serial_port.close()


if __name__ == '__main__':

    download_info = Download_info()

    try:
        eclipse_print_workaround('uart_program_da16200 Version 1.0.6', False, '')

        if download_info.enable_debug:
            logging.basicConfig(level=logging.DEBUG, format='%(message)s')
        else:
            logging.basicConfig(level=logging.ERROR, format='%(message)s')

        # Console Mode
        if len(sys.argv) == 1 or (len(sys.argv) == 3 and sys.argv[1] == "-l"):
            download_info.mode = Mode.CONSOLE
            ret = parser_console_mode(download_info)
        # Inline command Mode : port, address and full file path
        elif sys.argv[1] == "-i" and (len(sys.argv) == 4 or len(sys.argv) == 5):
            ret = parser_inline_command(len(sys.argv), download_info)
            download_info.mode = Mode.DLOAD
        # File mode : settings from file
        elif sys.argv[1] == "-f" and (len(sys.argv) == 3 or len(sys.argv) == 2):
            ret = parser_command_from_all_settings_file(download_info)
            download_info.mode = Mode.DLOAD
        # Print Help
        elif len(sys.argv) == 2 and sys.argv[1] == "-h":
            ret = help_print(download_info)
        # Run Mode in Eclipse
        elif sys.argv[1] == "--type":
            ret = parser_eclipse_argument(download_info)
            download_info.mode = Mode.DLOAD
        # Run for emergency boot
        elif sys.argv[1] == "-e":
            download_info.port_name = select_com_port(download_info)
            download_info.command_list.append(download_info.port_name)
            download_info.mode = Mode.EMODE
            ret = Return_Status.OK
        else:
            eclipse_print_workaround("Please check input parameters! : " + str(sys.argv), False, '')
            ret = help_print(download_info)

        if ret == Return_Status.NOK:
            finish_program(download_info)

        # Start download process
        download_info.now_status = Status.INIT
        strip_string = strip_end(download_info.command_list[0], '\n')
        cmd = strip_string.split(' ')
        download_info.port_name = cmd[0]

        if download_info.mode == Mode.CONSOLE:
            open_console_mode(download_info, False)
            run_console_mode(download_info)
            close_console_mode(download_info)
            eclipse_print_workaround("Terminated console mode.", False, '')
        elif download_info.mode == Mode.EMODE:
            run_emode(download_info)
            open_console_mode(download_info, False)
            download_info.mode = Mode.CONSOLE
            run_console_mode(download_info)
            close_console_mode(download_info)
        else:
            run_download(download_info, True, True)
            finish_program(download_info)

    except KeyboardInterrupt:
        finish_program(download_info)
