#!/bin/bash
####################################
# Do not edit with Windows editor! #
####################################
export LANG=C
# *========================================
# * mk_version
# *========================================
#
# PROJ_DIR		= %1
# TOOLKIT_DIR		= %2
# TARGET_DIR		= %3
# TARGET_BNAME		= %4
#
# *========================================

vcs_revision=0
filename="NULL"

called_path=${0%/*}
stripped=${called_path#[^/]*}
script_path=`pwd`/$stripped

pushd  "$script_path"

version_dir=${script_path}/../../tools/version

vendor=`cat "$version_dir/1st_vendor.h"`
vendor=`echo ${vendor%}`
major=`cat "$version_dir/2nd_major_num.h"`
major=`echo ${major%}`
customer=`cat "$version_dir/3rd_customer_build_num.h"`
customer=`echo ${customer%}`

# For RTOS
filename=$version_dir/genfiles/rtos_version_str.txt
vcs_revision=`cat "$version_dir/genfiles/rtos_svn_rev.txt"`
vcs_revision=`echo ${vcs_revision%}`

rm -f "$filename"

echo ${vendor}-${major}-${vcs_revision}-${customer}>"${filename}"
