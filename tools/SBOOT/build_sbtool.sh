#!/bin/bash
rm -rf ./certtool

pyinstaller --clean --hidden-import PyQt5.sip --hidden-import PyQt5.QtQuick \
				--hidden-import common --hidden-import common_cert_lib \
				--add-data './certtool_py/*.qml':'./'	\
				-F \
				--paths ./certtool_py/common:./certtool_py/common_cert_lib \
				--icon ./certtool_py/ieframe_31063.ico \
				--distpath ./certtool \
				./certtool_py/da16secutool.py				


#pyinstaller --onedir --clean --hidden-import common_utils \
#				-F \
#				--paths ./cert_tool/common_utils \
#				--icon ./cert_tool/ieframe_31063.ico \
#				--distpath ./certtool \
#				./cert_tool/da16secuact.py				

#pyinstaller --onedir --clean --hidden-import common_utils \
#				-F \
#				--paths ./cert_tool/common_utils \
#				--icon ./cert_tool/ieframe_31063.ico \
#				--distpath ./certtool \
#				./cert_tool/da16test.py

#mkdir ./certtool
cp ./certtool_py/*.qml  ./certtool/
cp ./certtool_py/*.exe  ./certtool/
cp ./certtool_py/*.dll  ./certtool/
cp  ./certtool_py/proj.cfg ./certtool/
