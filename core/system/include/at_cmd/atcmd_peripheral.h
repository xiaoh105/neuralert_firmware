/**
 ****************************************************************************************
 *
 * @file atcmd_peripheral.h
 *
 * @brief Header file - AT-CMD main controller
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

#ifndef	__ATCMD_PERIPHERAL_H__
#define	__ATCMD_PERIPHERAL_H__

#include "da16x_types.h"

#if defined ( BUILD_OPT_DA16200_ROMAC )
	#define GET_DPMP()   (dpmp_base_addr)
#else
	#define GET_DPMP()   ((struct dpm_param *) (DA16X_RTM_VAR_BASE))
#endif	// BUILD_OPT_DA16200_ROMAC

//
// Extern global functions
//
extern void set_wlaninit(UINT flag);
extern UINT get_wlaninit(void);
extern void phy_rc_rf_reg_write(uint16_t addr, uint16_t value);
extern void phy_rc_rf_reg_read(uint16_t addr, uint16_t *value);

extern UINT32	flash_image_read(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize);
extern void	flash_image_close(HANDLE handler);
extern void da16x_environ_lock(UINT32 flag);
extern char *ramlib_get_logo(void);

int	atcmd_rftest(int argc, char *argv[]);
int	atcmd_rftx(int argc, char *argv[]);
int	atcmd_rfrxtest(int argc, char *argv[]);
int	atcmd_rfrxstop(int argc, char *argv[]);
int	atcmd_rfcwttest(int argc, char *argv[]);
int	atcmd_rf_per(int argc, char *argv[]);
int	atcmd_rf_cont(int argc, char *argv[]);
int	atcmd_rf_chan(int argc, char *argv[]);
int atcmd_autocal(int argc, char *argv[]);

/*
 ****************************************************************************************
 * @brief RF Test AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 ****************************************************************************************
 */
int	atcmd_testmode(int argc, char *argv[]);

#if defined	(__SUPPORT_PERI_CMD__)
int atcmd_system(int argc, char *argv[]);
int atcmd_spiconf(int argc, char *argv[]);
int atcmd_uotp(int argc, char *argv[]);
int atcmd_get_uversion(int argc, char *argv[]);
int atcmd_set_TXPower(int argc, char *argv[]);
int atcmd_flash_dump(int argc, char *argv[]);
int atcmd_gpio_test(int argc, char *argv[]);
int atcmd_sleep_ms(int argc, char *argv[]);
int atcmd_xtal(int argc, char *argv[]);
#endif // (__SUPPORT_PERI_CMD__)

#endif	// __ATCMD_PERIPHERAL_H__

/* EOF */
