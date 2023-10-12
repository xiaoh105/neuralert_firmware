@echo off
rem *========================================
rem * mk_version
rem *========================================
rem
rem PROJ_DIR		= %1
rem TOOLKIT_DIR		= %2
rem TARGET_DIR		= %3
rem TARGET_BNAME	= %4
rem
rem *========================================

set filename="NULL"


PUSHD  "%~dp0"

set version_dir="%~dp0"..\..\tools\version

set /p vendor=<%version_dir%\1st_vendor.h
set /p major=<%version_dir%\2nd_major_num.h
set /p customer=<%version_dir%\3rd_customer_build_num.h

rem For RTOS
set filename=%version_dir%\genfiles\rtos_version_str.txt
set /p svn_revision=<%version_dir%\genfiles\rtos_svn_rev.txt

if exist %filename% (
  del  /Q %filename%
)

echo %vendor%-%major%-%svn_revision%-%customer%>%filename%

REM pause