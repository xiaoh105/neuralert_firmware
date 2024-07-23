uart program tool for DA16200/DA16600 {#uart_program_da16200}
================================

## Overview

`uart_program_da16200` is a console interface tool with functionalities like programming firmware to flash, and viewing console log from DA16200 and controlling DA16200 using console commands.

## Requirement

This tool is made based on python 3.8.10 and it requires the pyserial package.
The prebuild images for Windows and Linux (Ubuntu 20.04LTS) are in win and linux folders.
> Notes:
> * If the access to serial port is denied, permission must be changed using "sudo usermod -a -G dialout $USER"

## Interface

The serial port file name should be as presented by the operating system `COM5` (Windows) or `/dev/ttyUSB0` (Linux).
This serial port should be connected to UART0 of DA16200.

## Usage

uart_program_da16200 can be run with several different modes.

### Download Mode in E2studio or Eclipse

Download the firmware in E2stuio or Eclipse using launch. The settings for address and file name are in uart_address_settings.txt of the same folder.
The setting file has default values as follows.

	BOOT 0
	RTOS 23000
	BLE 3ad000

- BOOT is BOOT binary for DA16200 and 0 is address
- RTOS is main binary for DA16200 and 23000 is address.
- BLE is binary for DA14531 and 3ad000 is address.

The file name uart_address_settings.txt is fixed so do not change the name.


### Download Mode with Inline Arguments

Download the firmware to DA16200 with the below arguments.

	(python) uart_program_da16200.(exe or py) -i [port name] <address> <path and file name of firmware>
	example 1 : uart_program_da16200.exe -i 0 d:\img\DA16600_FBOOT-GEN01-01-922f1e27d_W25Q32JW.img
	example 2 : uart_program_da16200.exe -i 0 DA16600_FBOOT-GEN01-01-922f1e27d_W25Q32JW.img
	example 3 : uart_program_da16200.exe -i 23000 DA16600_FRTOS-GEN01-01-0561372b7c-006529.img
	example 4 : uart_program_da16200.exe -i COM3 23000 DA16600_FRTOS-GEN01-01-0561372b7c-006529.img


### Download Mode with Setting files

Download the firmware to DA16200 with the below file which can set port, address, and file name.
	
    (python) uart_program_da16200.(exe or py) -f <name of file with settings>
	example : uart_program_da16200.exe -f uart_all_settings.txt
	
### Console Mode

The tool can be used for viewing logs from DA16200 and inputting commands for controlling DA16200.
The Console mode can be used as shown in the exmples below.
	
	(python) uart_program_da16200.(exe or py)
	example : uart_program_da16200.exe
	
The logs from DA16200 can be saved to a file with the below usage.
	
	(python) uart_program_da16200.(exe or py) -l <log file name>
	example : uart_program_da16200.exe -l log.txt
	
Several  special commands are supported in this mode.
 
1. `dload` : It can be used for downloading firware image in MROM manually. It is needed to input address and image name in following instruction.	
2. `emode` : When you are having trouble entering MROM by exception case, use this command in the console mode and follow the instruction from the tool.
3. `exit`  : This command allows the tool to be finsiehd from the console mode. 

