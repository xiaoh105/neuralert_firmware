#!/bin/bash
####################################
# Do not edit with Windows editor! #
####################################
export LANG=C
# *========================================
# * build
# *========================================

called_path=${0%/*}
stripped=${called_path#[^/]*}
script_path=`pwd`/$stripped

vcs_repository="NULL"
vcs_revision=0
version_dir=$script_path/../../tools/version
filename=$version_dir/genfiles/buildtime.h

rm -f "$filename"

pushd  "$script_path"

if [ $4 == "DA16xxx_ueboot" ]; then
	vcs_revision=`cat "$version_dir/genfiles/ueboot_svn_rev.txt"`
	vcs_revision=`echo ${vcs_revision%}`
else
	vcs_revision=`cat "$version_dir/genfiles/rtos_svn_rev.txt"`
	vcs_revision=`echo ${vcs_revision%}`
fi


datetime=`date "+%Y%m%d%H%M"`
builddate=`date "+%Y%m%d"`
buildtime=`date "+%H%M%S"`
buildtime_dec=`date "+%H%M%S"`

echo "/**"> "$filename"
echo " ****************************************************************************************">> "$filename"
echo " *">> "$filename"
echo " * @buildtime.h">> "$filename"
echo " *">> "$filename"
echo " * @brief Build Time Generation">> "$filename"
echo " *">> "$filename"
echo " * Copyright (c) 2016-2023 Renesas Electronics. All rights reserved.">> "$filename"
echo " *">> "$filename"
echo " * This software ("Software") is owned by Renesas Electronics.">> "$filename"
echo " *">> "$filename"
echo " * By using this Software you agree that Renesas Electronics retains all">> "$filename"
echo " * intellectual property and proprietary rights in and to this Software and any">> "$filename"
echo " * use, reproduction, disclosure or distribution of the Software without express">> "$filename"
echo " * written permission or a license agreement from Renesas Electronics is">> "$filename"
echo " * strictly prohibited. This Software is solely for use on or in conjunction">> "$filename"
echo " * with Renesas Electronics products.">> "$filename"
echo " *">> "$filename"
echo " * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE">> "$filename"
echo " * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR">> "$filename"
echo " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,">> "$filename"
echo " * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE">> "$filename"
echo " * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL">> "$filename"
echo " * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,">> "$filename"
echo " * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF">> "$filename"
echo " * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER">> "$filename"
echo " * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE">> "$filename"
echo " * OF THE SOFTWARE.">> "$filename"
echo " *">> "$filename"
echo " ****************************************************************************************">> "$filename"
echo " */">> "$filename"
echo >> "$filename"
echo "#ifndef __buildtime_h__">>	"$filename"
echo "#define __buildtime_h__">>	"$filename"
echo >> "$filename"
echo "#define REV_BUILDFULLTIME			$datetime">> "$filename"
echo "#define REV_BUILDDATE				$builddate">> "$filename"
echo "#define REV_BUILDTIME				$buildtime_dec">> "$filename"
echo "#define REV_BUILDDATE_HEX			0x$builddate">> "$filename"
echo "#define REV_BUILDTIME_HEX			0x00$buildtime">> "$filename"
echo "#define SVN_REPOSITORY			\"$vcs_repository\"">> "$filename"
echo "#define SVN_REVISION_NUMBER		$vcs_revision">> "$filename"
echo "#define SVN_REVISION_NUMBER_HEX	0x$vcs_revision">> "$filename"
echo >> "$filename"
echo "#endif /*__buildtime_h__*/">> "$filename"
