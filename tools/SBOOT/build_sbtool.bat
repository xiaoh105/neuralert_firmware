del .\certtool\ /S

pyInstaller --clean --hidden-import PyQt5.sip --hidden-import PyQt5.QtQuick ^
				--hidden-import common --hidden-import common_cert_lib ^
				--add-data .\certtool_py\*.qml;.\ ^
				--onefile ^
				--paths ./certtool_py/common:./certtool_py/common_cert_lib ^
				--icon .\certtool_py\ieframe_31063.ico ^
				--distpath .\certtool ^
				./certtool_py/da16secutool.py				


rem py -3.8 -m PyInstaller --clean ^
rem 			--hidden-import common --hidden-import common_cert_lib ^
rem 			--onefile --uac-admin ^
rem 			--paths ./certtool_py/common:./certtool_py/common_cert_lib ^
rem 			--icon .\certtool_py\ieframe_31063.ico ^
rem 			--distpath .\certtool ^
rem 		./certtool_py/da16test.py
	

copy .\certtool_py\*.qml  .\certtool\
copy .\certtool_py\*.exe  .\certtool\
copy .\certtool_py\*.dll  .\certtool\
copy  .\certtool_py\proj.cfg .\certtool\
	
pause