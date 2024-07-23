rem *========================================
rem * build
rem *========================================
set vcs_repository="NULL"
set vcs_revision=0
set version_dir=%~dp0..\..\tools\version
set filename="%version_dir%\genfiles\buildtime.h"

if not exist "%version_dir%\genfiles" (
  mkdir "%version_dir%\genfiles"
)

if exist %filename% (
  del  /Q %filename%
)

PUSHD  "%~dp0"

if "%4"=="DA16xxx_ueboot" (
	set /p vcs_revision=<"%version_dir%\genfiles\ueboot_svn_rev.txt"
) else (
	set /p vcs_revision=<"%version_dir%\genfiles\rtos_svn_rev.txt"
)

for /f %%a in ('WMIC OS GET LocalDateTime ^| find "."') do set locdate=%%a

if %time:~0,2%  LEQ 9 (
	set datetime=%locdate:~0,4%%locdate:~4,2%%locdate:~6,2%0%time:~1,1%%time:~3,2%
) else (
	set datetime=%locdate:~0,4%%locdate:~4,2%%locdate:~6,2%%time:~0,2%%time:~3,2%
)

set builddate=%locdate:~0,4%%locdate:~4,2%%locdate:~6,2%

if %time:~0,2%  LEQ 9 (
	if %time:~0,2%  EQU 0 (
		set buildtime=0%time:~1,1%%time:~3,2%%time:~6,2%
		set buildtime_dec=%time:~3,2%%time:~6,2%
	) else (
		set buildtime=0%time:~1,1%%time:~3,2%%time:~6,2%
		set buildtime_dec=%time:~1,1%%time:~3,2%%time:~6,2%
	)
) else (
	set buildtime=%time:~0,2%%time:~3,2%%time:~6,2%
	set buildtime_dec=%time:~0,2%%time:~3,2%%time:~6,2%
)

echo /**> %filename%
echo  ****************************************************************************************>> %filename%
echo  *>> %filename%
echo  * @buildtime.h>> %filename%
echo  *>> %filename%
echo  * @brief Build Time Generation>> %filename%
echo  *>> %filename%
echo  * Copyright (c) 2016-2023 Renesas Electronics. All rights reserved.>> %filename%
echo  *>> %filename%
echo  * This software ("Software") is owned by Renesas Electronics.>> %filename%
echo  *>> %filename%
echo  * By using this Software you agree that Renesas Electronics retains all>> %filename%
echo  * intellectual property and proprietary rights in and to this Software and any>> %filename%
echo  * use, reproduction, disclosure or distribution of the Software without express>> %filename%
echo  * written permission or a license agreement from Renesas Electronics is>> %filename%
echo  * strictly prohibited. This Software is solely for use on or in conjunction>> %filename%
echo  * with Renesas Electronics products.>> %filename%
echo  *>> %filename%
echo  * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE>> %filename%
echo  * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR>> %filename%
echo  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,>> %filename%
echo  * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE>> %filename%
echo  * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL>> %filename%
echo  * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,>> %filename%
echo  * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF>> %filename%
echo  * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER>> %filename%
echo  * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE>> %filename%
echo  * OF THE SOFTWARE.>> %filename%
echo  *>> %filename%
echo  ****************************************************************************************>> %filename%
echo  */>> %filename%
(echo.)>> %filename%
echo #ifndef __buildtime_h__>>	%filename%
echo #define __buildtime_h__>>	%filename%
(echo.)>> %filename%
echo #define REV_BUILDFULLTIME			%datetime%>> %filename%
echo #define REV_BUILDDATE				%builddate%>> %filename%
echo #define REV_BUILDTIME				%buildtime_dec%>> %filename%
echo #define REV_BUILDDATE_HEX			0x%builddate%>> %filename%
echo #define REV_BUILDTIME_HEX			0x00%buildtime%>> %filename%
echo #define SVN_REPOSITORY				%vcs_repository%>> %filename%
echo #define SVN_REVISION_NUMBER		%vcs_revision%>> %filename%
echo #define SVN_REVISION_NUMBER_HEX	0x%vcs_revision%>> %filename%
(echo.)>> %filename%
echo #endif /*__buildtime_h__*/>> %filename%
