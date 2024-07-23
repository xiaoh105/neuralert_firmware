#!/bin/bash
chmod 755 *.sh
chmod 755 ../../utility/cfg_generator/da16x_gencfg
chmod 666 ../../utility/cfg_generator/da16x_gencfg.py
chmod 755 ../../utility/j-link/scripts/qspi/linux/jlinkflash
chmod 755 ../../utility/j-link/scripts/qspi/linux/uart_program_da16200
chmod 666 ../../utility/j-link/scripts/qspi/python/*.py

if [ -f bin2c ]; then
	chmod 755 bin2c
fi

chmod 755 ../SBOOT/*.sh
chmod 755 ../SBOOT/certtool/da16secutool
chmod 666 ../SBOOT/certtool_py/*.py
