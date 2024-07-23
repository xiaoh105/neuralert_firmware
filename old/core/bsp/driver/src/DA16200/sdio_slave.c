/**
 ****************************************************************************************
 *
 * @file sdio_slave.c
 *
 * @brief SDIO Slave Driver
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


#include "sdio_slave.h"
#include "hal.h"

#include "da16200_map.h"

// Definition of the SD core config structure
typedef struct __sd_config_struct{
	UINT8* pCis0Data;		// Pointer to the CIS0 data
	UINT8* pCis1Data;		// Pointer to the CIS1 data
	UINT8* pCsaData;		// Pointer to the CSA data

}SD_CFG_STR;


#define SD_CIA_VALUE 0x6007112		// old 0x6007112 csa enable 0x6017112

#define SMS_SDIO_BASE	SDIO_SLAVE_BASE_0

// for compilation purpose
#define SD_TX_BIT				1
#define SD_RX_BUFF_A_BIT		1
#define SD_RX_BUFF_B_BIT		2


#define AHB_BRIDGE				0x50010044
// Definitions for SD core registers
typedef struct	__sdio_slave_reg_map__
{
	volatile UINT32	reserved0;
	volatile UINT32	reserved1;
	volatile UINT32	cis_fn0_addr;
	volatile UINT32	cis_fn1_addr;
	volatile UINT32	csa_addr;
	volatile UINT32	read_addr;
	volatile UINT32	write_addr;
	volatile UINT32	ahb_transfer_cnt;		// transfer counter
	volatile UINT32	sdio_transfer_cnt;		// transfer counter
	volatile UINT32 cia_register;
	volatile UINT32 program_register;
	volatile UINT32 interrupt_status;
	volatile UINT32 interrupt_enable;
	volatile UINT32	ocr_register;
	volatile UINT32	reserved2;
	volatile UINT32	ahb_burst_size;
	volatile UINT32	ahb_reset_register;
} SDIO_SLAVE_REG_MAP;

// Definitions for Program Register fields
#define PROG_REG_FUNCTION_RDY		0x0001
#define PROG_REG_FUN1_DATA_RDY		0x0002
#define PROG_REG_SCSI				0x0004
#define PROG_REG_SDC				0x0008
#define PROG_REG_SMB				0x0010
#define PROG_REG_SRW				0x0020
#define PROG_REG_SBS				0x0040
#define PROG_REG_S4MI				0x0080
#define PROG_REG_LSC				0x0100
#define PROG_REG_4BLS				0x0200
#define PROG_REG_CARD_RDY			0x0400

// Definitions for Interrupt Enable/Status Register fields
#define INT_EN_REG_WR_TRAN_OVER		0x00000001
#define INT_EN_REG_RD_TRAN_OVER		0x00000002
#define INT_EN_REG_RD_ERR			0x00000004

#define INT_STATUS_CLEAR_ALL		0xFFFFFFF8

// Definitions for OCR Register fields
#define OCR_REG_VOLTAGE_RANGE		0x00FF8000	// Default value

// Definitions for the CIA register fields
#define CIA_REG_CCCR_REV			0x00000001	// Default value
#define CIA_REG_SDIO_REV			0x00000010	// Default value
#define CIA_REG_SD_FORMAT_REV		0x00000000	// Default value
#define CIA_REG_IO_DEV_CODE1		0x00007000	// Default value
#define CIA_REG_CSA_SUPPORT1		0x00010000	// Default value
#define CIA_REG_EXT_IO_DEV_CODE1	0x00000000	// Default value
#define CIA_REG_SPS					0x02000000	// Default value
#define CIA_REG_SHS					0x04000000	// Default value
#define CIA_REG_RFU					0x00000000	// Default value


// Defines for Success and Abort
#define ERR_SDIO_NONE					0x0
#define ERR_SDIO_RX_BUF				0x00000001	// rx buffer malloc fail
#define ERR_SDIO_TX_BUF				0x00000002	// tx buffer malloc fail
//
//	Driver Structure
//
typedef struct	__sdio_slave_handler__
{
	UINT32				dev_addr;  					// Unique Address
	// Device-dependant
	SDIO_SLAVE_REG_MAP	*regmap;
	SD_CFG_STR 			SdCfg;
	//void 				(*rx_callback)(UINT32 status);

} SDIO_SLAVE_HANDLER;

#define SYS_SDIO_SLAVE_BASE_0		((SDIO_SLAVE_REG_MAP*)SDIO_SLAVE_BASE_0)

#ifndef HW_REG_AND_WRITE32
#define	HW_REG_AND_WRITE32(x, v)	( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) & (v) )
#endif
#ifndef HW_REG_OR_WRITE32
#define	HW_REG_OR_WRITE32(x, v)		( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) | (v) )
#endif
#ifndef HW_REG_WRITE32
#define HW_REG_WRITE32(x, v) 		( *((volatile unsigned int   *) &(x)) = (v) )
#endif
#ifndef HW_REG_READ32
#define	HW_REG_READ32(x)			( *((volatile unsigned int   *) &(x)) )
#endif
/*
*	static variable
*/
// Using default values supplied by Arasan for CIS0 and CIS1
const UINT8 CIS0_Data[] = { 0x21,	// TPL_CODE_CISTPL_FUNCID
						   0x02,	// Link to next tuple
						   0x0C,	// Card function code
						   0x00,	// Not used

						   0x22,	// TPL_CODE_CISTPL_FUNCE
						   0x04,	// Link to next tuple
						   0x00,	// Extended data
						   0x00,	// Only block size function 0 can support (2048 = 0x0800)
						   0x08,	// Together with previous byte
						   0x32,	// Transfer rate (25 Mbit/sec = 0x32) 50Mbit/sec = 0x5A


						   0x20,	// TPL_CODE_CISTPL_MANFID
						   0x04,	// Link to next tuple
						   0x96,	// SDIO manufacturer code 0x0296
						   0x02,	// Used with previous byte
						   0x47,	// Part number/revision number OEM ID = 0x5347
						   0x53,	// Used with previous byte

						   0xFF };	// End of tuple chain
const UINT8 CIS1_Data[] = {0x21,	// TPL_CODE_CISTPL_FUNCID
						   0x02,	// Link to next tuple
						   0x0C,	// Card function type
						   0x00,	// Not used

						   0x22,	// TPL_CODE_CISTPL_FUNCE
						   0x2A,	// Link to next tuple		42byte?

						   0x01,	// Type of extended data
						   0x01,	// Wakeup support
						   0x20,	// X.Y revision
						   0x00,	// No serial number
						   0x00,	// No serial number
						   0x00,	// No serial number
						   0x00,	// No serial number
						   0x00,	// Size of the CSA space available for this function in bytes (0)
						   0x00,	// Used with previous
						   0x02,	// Used with previous

						   0x00,	// Used with previous
						   0x03,	// CSA property: Bit 0 - 0 implies R/W capability in CSA // Bit 1 - 0 implies the Host may reformat the file system
						   0x00,	// Maximum block size (256 bytes)		// was 0x80
						   0x08,	// Used with previous
						   0x00,	// OCR value of the function
						   0x80,	// Used with previous					// was 0x01 ->0x08
						   0xFF,	// Used with previous					// was 0xff
						   0x00,	// Used with previous
						   0x08,	// Minimum power required by this function (8 mA)
						   0x0a,    // ADDED => Average power required by this function when operating (10 mA)

						   0x0f,    // ADDED => Maximum power required by this function when operating (15 mA)
						   0x01,	// Stand by is not supported
						   0x01,	// Used with previous
						   0x01,	// Used with previous
						   0x00,	// Minimum BW
						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,

						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,
						   0x00,

						   0x00,
						   0x00,

						   0x91,		// TPL_CODE_CISTPL_SDIO_STD
						   0x03,		// TPL_LINK
						   0x07,		// SDIO Standard I/O device type
						   0x00,		// Format and type of data

						   0x00,
						   0xFF, };	// End of tuple
const UINT8 CSA_DATA[] = {0x01, 0x02, 0x03};


const SDIO_SLAVE_HANDLER	sdio_slave = {
	.dev_addr = (UINT32)SYS_SDIO_SLAVE_BASE_0,
	.regmap = (SDIO_SLAVE_REG_MAP *)(SYS_SDIO_SLAVE_BASE_0),

	.SdCfg = {
		.pCis0Data = (UINT8*)CIS0_Data,
		.pCis1Data = (UINT8*)CIS1_Data,
		.pCsaData =  (UINT8*)SYS_SDIO_SLAVE_BASE_0, //CSA_DATA,
	},
};

static UINT32 sys_ocr;
static UINT8  *g_buffer;

void 				(*rx_callback)(UINT32 status);
#define SDIO_RESET			0x500802D0
#define SDIO_RESET_PIN		0x00000040
#define SERIAL_SLAVE_TYPE	0x5000122c
#define CMD_ADDR			0x50080254
#define AT_CMD_ADDR			0x50080260
#define	SLAVE_INT_STATUS_CMD			(0x1 << 15)
#define	SLAVE_INT_STATUS_ATCMD			(0x1 << 14)
#define	SLAVE_INT_STATUS_END			(0x1 << 13)
void _sdio_slave_interrupt(void)
{
	//DRV_PRINTF("sdio_slave interrupt 0x%08x\n",*((volatile unsigned int   *)(sdio_slave.regmap->interrupt_status)) );
	HW_REG_AND_WRITE32(sdio_slave.regmap->interrupt_status, (UINT32)INT_STATUS_CLEAR_ALL);
	//DRV_PRINTF("sdio_slave interrupt 0x%08x\n",*((volatile unsigned int   *)(AHB_BRIDGE)) );
	if (rx_callback != NULL)
	{

#if 0//jason190601
	switch(*((volatile unsigned int   *)(AHB_BRIDGE)))
	{
		case CMD_ADDR:
			rx_callback(SLAVE_INT_STATUS_CMD);
			break;
		case AT_CMD_ADDR:
			rx_callback(SLAVE_INT_STATUS_ATCMD);
			break;
		default:
			rx_callback(SLAVE_INT_STATUS_END);
			break;
	}
#else
	rx_callback(*((volatile unsigned int   *)(AHB_BRIDGE)));
#endif
	}
}

static void	_tx_specific_sdio_slave_interrupt(void)
{
	INTR_CNTXT_CALL(_sdio_slave_interrupt);
}

/*
*	static functions
*/
void sdio_set_func_mode(UINT8 mode)
{
	if (mode == TRUE)
	{
		HW_REG_WRITE32(sdio_slave.regmap->interrupt_enable,0x0011);
		HW_REG_OR_WRITE32(sdio_slave.regmap->program_register, (UINT32)PROG_REG_FUNCTION_RDY);

		HW_REG_WRITE32(sdio_slave.regmap->interrupt_enable, (UINT32)(INT_EN_REG_WR_TRAN_OVER |
			  					     							  INT_EN_REG_RD_TRAN_OVER |
									 							  INT_EN_REG_RD_ERR));
		HW_REG_AND_WRITE32(sdio_slave.regmap->interrupt_status, (UINT32)INT_STATUS_CLEAR_ALL);
	}
	else
	{
		HW_REG_AND_WRITE32(sdio_slave.regmap->program_register, (~PROG_REG_FUNCTION_RDY));
	}
}

/*
*	extern functions
*/
UINT32 SDIO_SLAVE_INIT(void)
{
	UINT32 temp;

	/* clock gating on */
	DA16X_CLOCK_SCGATE->Off_SSI_SDIO = 0;
	HW_REG_AND_WRITE32(*((volatile unsigned int   *)(SDIO_RESET)), ~SDIO_RESET_PIN);
	HW_REG_OR_WRITE32(*((volatile unsigned int   *)(SDIO_RESET)), SDIO_RESET_PIN);

	// removed
	//HW_REG_WRITE32(*((volatile unsigned int   *)(SERIAL_SLAVE_TYPE)), 1);

	// set ready bit
	HW_REG_OR_WRITE32(sdio_slave.regmap->program_register, PROG_REG_CARD_RDY);

	if (g_buffer != NULL)
	{
		memcpy(g_buffer, CIS0_Data, sizeof(CIS0_Data));
		memcpy(g_buffer + sizeof(CIS0_Data) + (4-(sizeof(CIS0_Data)%4)), CIS1_Data, sizeof(CIS1_Data));
		//memcpy(buffer + sizeof(CIS0_Data) + sizeof(CIS1_Data), SYS_SDIO_SLAVE_BASE_0, sizeof(SYS_SDIO_SLAVE_BASE_0));
		HW_REG_WRITE32(sdio_slave.regmap->cis_fn0_addr, (UINT32)g_buffer);
		HW_REG_WRITE32(sdio_slave.regmap->cis_fn1_addr, (UINT32)(g_buffer + sizeof(CIS0_Data) + (4-(sizeof(CIS0_Data)%4))));
		HW_REG_WRITE32(sdio_slave.regmap->csa_addr, (UINT32)(SYS_SDIO_SLAVE_BASE_0));
	}
	else
	{
		// Program the CIS function 0 register with the starting address of CIS Fn0 memory Location
		HW_REG_WRITE32(sdio_slave.regmap->cis_fn0_addr, (UINT32)sdio_slave.SdCfg.pCis0Data);
		// Program the CIS function 1 register with the starting address of CIS Fn1 memory Location
		HW_REG_WRITE32(sdio_slave.regmap->cis_fn1_addr, (UINT32)sdio_slave.SdCfg.pCis1Data);
		// Program the CSA register with the starting address of CSA memory Location
		HW_REG_WRITE32(sdio_slave.regmap->csa_addr, (UINT32)sdio_slave.SdCfg.pCsaData);
	}
	// Program the DMA write register with the starting address of DMA Location where the SD Host has to write data for a function1 write data transfer
	//HW_REG_WRITE32(sdio_slave.regmap->write_addr, (UINT32)rx_buffer);

	// Program the interrupt enable register -
	// enable write_over, read_over and read_err
	_sys_nvic_write(SDIO_Slave_IRQn, (void *)_tx_specific_sdio_slave_interrupt, 0x7);

	HW_REG_AND_WRITE32(sdio_slave.regmap->interrupt_status, (UINT32)INT_STATUS_CLEAR_ALL);
	HW_REG_WRITE32(sdio_slave.regmap->interrupt_enable, (UINT32)(INT_EN_REG_WR_TRAN_OVER | INT_EN_REG_RD_TRAN_OVER ));
/*
	HW_REG_WRITE32(sdio_slave.regmap->interrupt_enable, (UINT32)(INT_EN_REG_WR_TRAN_OVER |
			  					     							  INT_EN_REG_RD_TRAN_OVER |
									 							  INT_EN_REG_RD_ERR));
*/

	// Program the OCR register
	if (sys_ocr == 0)
		HW_REG_WRITE32(sdio_slave.regmap->ocr_register, (UINT32)OCR_REG_VOLTAGE_RANGE);
	else
		HW_REG_WRITE32(sdio_slave.regmap->ocr_register, (UINT32)sys_ocr);

	// Program the Program register
	temp = 0;
	temp = PROG_REG_SCSI | PROG_REG_SDC | PROG_REG_SMB | PROG_REG_SRW | PROG_REG_SBS | PROG_REG_S4MI | PROG_REG_FUNCTION_RDY;
	HW_REG_WRITE32(sdio_slave.regmap->program_register,temp);

	// Program the CIA register // Temp - default from Arasan reference;  Changed bit 16 to 0 (CSA not supported)
	HW_REG_WRITE32(sdio_slave.regmap->cia_register, SD_CIA_VALUE);

	// removed
	//HW_REG_WRITE32(*((volatile unsigned int   *)(SERIAL_SLAVE_TYPE)), 1);

	return ERR_SDIO_NONE;
}

void SDIO_SLAVE_TX(UINT8 *data, UINT32 length)
{
	DA16X_UNUSED_ARG(data);
	DA16X_UNUSED_ARG(length);
	HW_REG_OR_WRITE32(sdio_slave.regmap->program_register, PROG_REG_FUN1_DATA_RDY);
}

UINT32 SDIO_SLAVE_CALLBACK_REGISTER(void (* p_rx_callback_func)(UINT32 status))
{
	rx_callback = p_rx_callback_func;
	return ERR_SDIO_NONE;
}
UINT32 SDIO_SLAVE_CALLBACK_DEREGISTER(void)
{
	rx_callback = NULL;
	return ERR_SDIO_NONE;
}

UINT32 SDIO_SLAVE_SET_OCR(UINT32 ocr)
{
	sys_ocr = ocr;
	return sys_ocr;
}

UINT32 SDIO_SlAVE_GET_OCR( void )
{
	if (sys_ocr == 0)
		return OCR_REG_VOLTAGE_RANGE;
	else
		return sys_ocr;
}

void SDIO_SLAVE_SET_BUFFER( UINT8 *buffer)
{
	g_buffer = buffer;
}

UINT32 SDIO_SLAVE_DEINIT( void )
{
	/* clock gating off */
	//DA16X_CLKGATE_OFF(clkoff_sdio);

	return ERR_SDIO_NONE;
}
