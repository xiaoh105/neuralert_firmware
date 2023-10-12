/**
 ****************************************************************************************
 *
 * @file environ.h
 *
 * @brief NVRAM Driver
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#ifndef __environ_h__
#define __environ_h__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "target.h"
#include "common_def.h"

extern void init_environ(void);
extern void check_environ(void);

extern void da16x_environ_lock(UINT32 flag);
extern char *da16x_getenv(UINT32 device, char *rootname, char *envname);
extern int da16x_setenv_flag(UINT32 flag, UINT32 device, char *rootname, char *envname, char *value);
extern int da16x_putenv_flag(UINT32 flag, UINT32 device, char *rootname, char *envname);
extern int da16x_unsetenv_flag(UINT32 flag, UINT32 device, char *rootname, char *envname);
extern int da16x_clearenv_flag(UINT32 flag, UINT32 device, char *rootname);
extern int da16x_printenv(UINT32 device, char *rootname);

#define	da16x_setenv(...)		da16x_setenv_flag(TRUE, __VA_ARGS__ )
#define	da16x_putenv(...)		da16x_putenv_flag(TRUE, __VA_ARGS__ )
#define	da16x_unsetenv(...)		da16x_unsetenv_flag(TRUE, __VA_ARGS__ )
#define	da16x_clearenv(...)		da16x_clearenv_flag(TRUE, __VA_ARGS__ )

#define	da16x_setenv_temp(...)		da16x_setenv_flag(FALSE, __VA_ARGS__ )
#define	da16x_putenv_temp(...)		da16x_putenv_flag(FALSE, __VA_ARGS__ )
#define	da16x_unsetenv_temp(...)		da16x_unsetenv_flag(FALSE, __VA_ARGS__ )
#define	da16x_clearenv_temp(...)		da16x_clearenv_flag(FALSE, __VA_ARGS__ )

extern int da16x_saveenv(UINT32 device);

extern int da16x_setenv_bin(UINT32 device, char *rootname, char *envname, void *value, UINT32 len);
extern void *da16x_getenv_bin(UINT32 device, char *rootname, char *envname, UINT32 *len);

#endif /* __environ_h__ */

/* EOF */
