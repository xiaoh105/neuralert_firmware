set TOOLNAME=".\certtool\da16secutool.exe"
rem set TOOLNAME=py -3.8 ".\certtool_py\da16secutool.py"

%TOOLNAME% -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg -tpm ./cmtpmcfg

REM Add Version Info.
if "%1"=="" (
	pushd "%~dp0\..\util"
	cmd /C add_version.bat %~dp0\.. 2 3 DA16xxx_ueboot
	pushd "%~dp0\..\SBOOT"
) else (
	pushd "%~dp0\..\util"
	cmd /C add_version.bat %1 %2 %3 %4
	pushd "%~dp0\..\SBOOT"
)

REM Image Generate 
%TOOLNAME% -mod secuboot -cfg ./cmconfig/da16xtpmconfig.cfg

rem pause