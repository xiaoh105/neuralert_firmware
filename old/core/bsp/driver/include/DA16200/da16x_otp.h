/**
 * \addtogroup HALayer
 * \{
 * \addtogroup OTP_DRIVER
 * \{
 * \brief Driver for OTP(one time programmable) memory.
 */

/**
 ****************************************************************************************
 *
 * @file da16x_otp.h
 *
 * @brief Definition of API for the OTP Controller driver.
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

#ifndef _da16x_otp_h__
#define _da16x_otp_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"
#define OTP_ADDR_BASE			0x40102000
#define OTP_ADDR_DEBUG_BASE		0x40122000
#define OTP_ADDR_CMD			0x40120000	// Same cmd address normal mode and debug


#define OTP_IOCTL_CMD_WRITE	0x00
#define OTP_IOCTL_CMD_READ 	0x01
#define OTP_IOCTL_SET_CLOCK 0x02

#define OTP_CMD_LOCK	0x0F
#define OTP_CMD_SLEEP	0x00
#define OTP_CMD_WAKE_UP	0x01		// close
#define OTP_CMD_CEB		0x02		// start
#define OTP_CMD_RST		0x03
#define OTP_CMD_WRITE	0x07
#define OTP_CMD_READ	0x0B
#define OTP_CMD_TEST_1	0x09	    // blank check
#define OTP_CMD_TEST_2	0x0A		// test dec
#define OTP_CMD_TEST_3	0x0C		// test write
#define OTP_CMD_SPARE_W	0X0D
#define OTP_CMD_SPARE_R 0x0E

#define NP2	20
#define BUSY_WAIT_TIME_OUT 100000

/** @ Return values
 *  Return value of the OTP controller
 */
///@{
#define OTP_OK						0x00 /*!< Return value if successed.*/
#define OTP_ERROR_BUSY_WAIT_TIMEOUT	0x01 /*!< Return value Time out error */
#define OTP_ERROR_NP2				0x02 /*!< Return value Cannot write */
#define OTP_ERROR_UNKNOWN			0x03 /*!< Return value error unknown */
///@}

#undef OTP_DEBUG_ON

#ifdef OTP_DEBUG_ON
#define PRINT_OTP(...) 				DBGT_Printf(__VA_ARGS__)
#define OTP_ASIC_TRIGGER(x)			ASIC_DBG_TRIGGER(x)
#else
#define PRINT_OTP(...)
#define OTP_ASIC_TRIGGER(x)			ASIC_DBG_TRIGGER(x)
#endif

int	_sys_otp_ioctl(int cmd, void *data);
int	_sys_otp_read(unsigned int addr, unsigned int *buf);
int	_sys_otp_write(unsigned int addr, unsigned int buf);

/******************************************************************************
 *  otp wrapper
 ******************************************************************************/
/**
 ****************************************************************************************
 * @brief Initialize OTP HW
 *
 * @note Before call this function, it need otp_clock enable <br>
 * @note example <br>
 * @code{.c}
 *	FC9100_SYSCLOCK->PLL_CLK_EN_4_PHY = 1;
 *  FC9100_SYSCLOCK->CLK_EN_PHYBUS = 1;
 *	extern void	DA16X_SecureBoot_OTPLock(unsigned int mode);
 *	DA16X_SecureBoot_OTPLock(1); 						// unlock
 *	#define CLK_GATING_OTP 		0x50006048
 *	MEM_BYTE_WRITE(CLK_GATING_OTP, 0x00);
 *  otp_mem_create();
 * @endcode
 ****************************************************************************************
 */
extern void	otp_mem_create(void);

/**
 ****************************************************************************************
 * @brief Read OTP memory
 * @param[in] offset OTP memory offset(0x000 ~ 0x7FF)
 * @param[out] data Pointer of buffer
 * @return OTP_OK if successed
 * @note The DA16200 have 8k byte OTP memory <br>
 * @note Offset range 0 to 0x7FF <br>
 *   each offset store 32bit data.
 * @warning Offset 0 to 0x2C used for secure purpose. so it may not accessible. <br>
 * @warning Please refer the OTP_MAP document.
 ****************************************************************************************
 */
extern int  otp_mem_read(UINT32 offset, UINT32 *data);

/**
 ****************************************************************************************
 * @brief Write OTP memory
 * @param[in] offset OTP memory offset(0x000 ~ 0x7ff)
 * @param[in] data to write
 * @return OTP_OK if successed
 * @warning Offset 0 to 0x2c used for secure purpose. Do not write any data within.
 * @warning Please refer the OTP_MAP document.
 ****************************************************************************************
 */
extern int	otp_mem_write(UINT32 offset, UINT32 data);

/**
 ****************************************************************************************
 * @brief Read lock status value
 * @param[in] offset lock status offset always (0xFFF)
 * @param[out] data Pointer of lock status
 * @return OTP_OK if successed
 * @note The OTP memory can be locked. <br>
 * @note Each lock bit can lock range ~ 0x40 <br>
 * example <br>
 * lock status value 0x00000002, it means that offset 0x40~0x7F OTP memory locked. <br>
 * lock bit 0 lock offset 0 ~ 0x3F <br>
 * lock bit 1 lock offset 0x40 ~ 0x7F <br>
 * lock bit 2 lock offset 0x80 ~ 0xBF <br>
 * .... <br>
 * lock bit 15 lock offset 0x3C0 ~ 0x3FF <br>
 * .... <br>
 * lock bit 29 lock offset 0x740 ~ 0x77F <br>
 * lock bit 30 lock offset 0x780 ~ 0x7BF <br>
 * lock bit 31 lock offset 0x7C0 ~ 0x7FF <br>
 ****************************************************************************************
 */
extern int  otp_mem_lock_read(unsigned int offset, unsigned int *data);

/**
 ****************************************************************************************
 * @brief Write lock status value
 * @param[in] offset lock status offset always (0xFFF)
 * @param[in] data lock status value
 * @return OTP_OK if successed
 * @note The OTP memory can be locked. <br>
 * Each lock bit can lock range ~ 0x40 <br>
 * example <br>
 * lock status value 0x00000002, it means that offset 0x40~0x7F OTP memory locked. <br>
 * otp_mem_lock_write(0xFFF, 0x00000002); <br>
 * lock bit 0 lock offset 0 ~ 0x3F <br>
 * lock bit 1 lock offset 0x40 ~ 0x7F <br>
 * lock bit 2 lock offset 0x80 ~ 0xBF <br>
 * .... <br>
 * lock bit 15 lock offset 0x3C0 ~ 0x3FF <br>
 * .... <br>
 * lock bit 29 lock offset 0x740 ~ 0x77F <br>
 * lock bit 30 lock offset 0x780 ~ 0x7BF <br>
 * lock bit 31 lock offset 0x7C0 ~ 0x7FF <br>
 ****************************************************************************************
 */
extern int  otp_mem_lock_write(unsigned int offset, unsigned int data);

/**
 ****************************************************************************************
 * @brief Close OTP HW
 *
 ****************************************************************************************
 */
extern void	otp_mem_close(void);
#endif	/*_da16x_otp_h__*/
