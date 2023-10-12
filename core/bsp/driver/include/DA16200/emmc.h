/**
 * \addtogroup HALayer
 * \{
 * \addtogroup SDIO_HOST
 * \{
 * \brief Driver for SDIO 2.0 host.
 */

/**
 ****************************************************************************************
 *
 * @file emmc.h
 *
 * @brief Driver for SDIO 2.0 host. it can access SD card, MMC card and SDIO device.
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

#ifndef __emmc_h__
#define __emmc_h__

#include "hal.h"
#include "sd.h"
#include "sdio.h"

#define SYS_EMMC_BASE		((EMMC_REG_MAP*)EMMC_BASE_0)
#ifndef HW_REG_AND_WRITE32
#define	HW_REG_AND_WRITE32(x, v)	( *((volatile UINT32   *) &(x)) = *((volatile UINT32   *) &(x)) & (v) )
#endif
#ifndef HW_REG_OR_WRITE32
#define	HW_REG_OR_WRITE32(x, v)		( *((volatile UINT32   *) &(x)) = *((volatile UINT32   *) &(x)) | (v) )
#endif
#ifndef HW_REG_WRITE32
#define HW_REG_WRITE32(x, v) 		( *((volatile UINT32   *) &(x)) = (v) )
#endif
#ifndef HW_REG_READ32
#define	HW_REG_READ32(x)			( *((volatile UINT32   *) &(x)) )
#endif
//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
#define CARD_READY					0x80000000
#define BLOCK_SIZE					512
//#define MMC_DELAY					200
#define MMC_RETRY_COUNT				100


#define DIALOG_MMC_GO_IDLE_ST			0
#define DIALOG_MMC_SEND_OP_COND			1
#define DIALOG_MMC_ALL_SEND_CID			2
#define DIALOG_MMC_SET_REL_ADDR			3
#define DIALOG_MMC_SWITCH				6
#define DIALOG_MMC_SND_EXT_CSD			8
#define DIALOG_MMC_SEL_CARD				7
#define DIALOG_MMC_SEND_CSD				9
#define DIALOG_MMC_SEND_CID				10
#define DIALOG_MMC_RD_BLK				18
#define DIALOG_MMC_STOP_TRANSM			12
#define DIALOG_MMC_RD_SINGLE_BLK		17
#define DIALOG_MMC_SET_BLK_CNT			23
#define DIALOG_MMC_WR_BLK				24
#define DIALOG_MMC_WR_MBLOCK			25

#define SDIO_CCCR_POWER					0x12
#define ERR_SEND_OP_COND			0x00000200


#define E_CSD_PART_ATTR				156		/* R/W */
#define E_CSD_PART_SUPP				160		/* Read Only */
#define E_CSD_ER_GRP_DEF			175		/* R/W */
#define E_CSD_PART_CFG				179		/* R/W */
#define E_CSD_ERASED_MEM_CONT		181		/* Read Only */
#define E_CSD_BUS_WIDTH				183		/* R/W */
#define E_CSD_HS_TIMING				185		/* R/W */
#define E_CSD_STRUCTURE				194		/* Read Only */
#define E_CSD_REV					192		/* Read Only */
#define E_CSD_CD_TYPE				196		/* Read Only */
#define E_CSD_PART_SWCH_TM			199		/* Read Only */
#define E_CSD_SEC_CNT				212		/* Read Only, 4 bytes */
#define E_CSD_S_A_TIMEOUT			217		/* Read Only */
#define E_CSD_HC_WP_GRP_SIZE		221		/* Read Only */
#define E_CSD_REL_WR_SEC_C			222		/* Read Only */
#define E_CSD_ER_TO_MULT			223		/* Read Only */
#define E_CSD_HC_ERASE_GRP_SIZE		224		/* Read Only */
#define E_CSD_BOOT_MULT				226		/* Read Only */
#define E_CSD_SEC_TRIM_MULT			229		/* Read Only */
#define E_CSD_SEC_ERASE_MULT		230		/* Read Only */
#define E_CSD_SEC_FT_SUPP			231		/* Read Only */
#define E_CSD_TRIM_MULT				232		/* Read Only */

#define E_CSD_CD_TYPE_26			(1 << 0)	/* 26MHz Read : Card Type */
#define E_CSD_CD_TYPE_52			(1 << 1)	/* 52MHz Read : Card Type */
#define E_CSD_CD_TYPE_DDR_1_8V 		(1 << 2)	/* 52MHz Read : DDR I/O mode @1.8V or 3V */
#define E_CSD_CD_TYPE_DDR_1_2V 		(1 << 3)	/* 52MHz Read : DDR I/O mode @1.2V */
#define E_CSD_CD_TYPE_MASK			0xF

#define E_CSD_CD_TYPE_DDR_52       (E_CSD_CD_TYPE_DDR_1_8V | E_CSD_CD_TYPE_DDR_1_2V)

#define E_CSD_CMD_SET_NORMAL		(1 << 0)

#define MMC_SWITCH_MODE_WRITE_BYTE		0x03


#define GET_CMD_RESP_TYPE(x)	((x == DIALOG_MMC_ALL_SEND_CID) ? 3 : 		\
								(x == DIALOG_MMC_SEND_CSD) ? 	3 : 		\
								(x == DIALOG_MMC_SEND_CID) ? 	3 : 		\
								(x == DIALOG_SD_APP_OP_COND) ? 1: 			\
								(x == DIALOG_SD_SND_IF_COND) ? 2 :    \
								(x == DIALOG_APP_CMD) ? 2: 2)			\

/* error */
#define ERR_NONE					0x00000000
#define ERR_RES						0x00000001
#define ERR_TIME_OUT				0x00000002
#define ERR_SINGLE_READ				0x00000004
#define ERR_MULTI_READ				0x00000008
#define ERR_BLOCK_READ				0x00000010
#define ERR_SINGLE_WRITE			0x00000020
#define ERR_MULTI_WRITE				0x00000040
#define ERR_CSD						0x00000080
#define ERR_CID						0x00000100
#define ERR_SEND_OP_COND			0x00000200
#define ERR_UNKNOWN_OCR				0x00000400
#define ERR_SD_IF_COND				0x00000800
#define ERR_MMC_INIT				0x00001000
#define ERR_UNKNOWN_CONTROL			0x00002000
#define ERR_HANDLE					0x00004000
#define ERR_CARD_NOT_EXIST			0x00008000
#define ERR_CARD_NOT_READY			0x00010000
#define ERR_WRITE_BLOCK_CRC			0x00020000
#define ERR_READ_BLOCK_CRC			0x00040000
#define ERR_SD_SETUP				0x00080000
#define ERR_EMMC_SETUP				0x00100000
#define ERR_SCR						0x00200000
#define ERR_SSR						0x00400000
#define ERR_EXT_CSD					0x00800000
#define ERR_NON_SUPPORT_VOLTAGE		0x01000000
#define ERR_INVAL					0x02000000
#define ERR_SDIO_SETUP				0x04000000
#define ERR_SDIO_COMBO_NOT_SUPPORT	0x08000000
#define ERR_SDIO_MEM				0x10000000
#define ERR_SDIO_TIME_OUT			0x20000000
#define ERR_SDIO_IO					0x40000000
#define ERR_SDIO_RANGE				0x80000000

/**
 * \brief Card type.
 * It is stored in card_type after EMMC_INIT().
 */
typedef enum _CARD_TYPE_ {
	UNKNOWN_TYPE,			/**< Unknown device */
	SDIO_CARD,				/**< SDIO device */
	SD_CARD_1_1,			/**< SD card version 1.1*/
	SD_CARD,				/**< SD card */
	MMC_CARD,				/**< MMC */
	CARD_MAX
} CARD_TYPE;

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief Driver type
 *
 */
typedef enum {
	EMMC_INTERRUPT_DRIVEN,	/**< Interrupt driven */
	EMMC_POLLING			/**< Polling */
} EMMC_DRV_TYPE;

/**
 * \brief Clock output type
 *
 */
typedef enum {
	EMMC_CLK_ALWAYS_ON,		/**< Clock always on */
	EMMC_CLK_RW_ON			/**< Clock on only while read & write */
} EMMC_CLK_TYPE;

/**
 * \brief SDIO control IOCTL cmd
 *
 */
typedef enum	{
	EMMC_GET_DEVREG = 1,	/**< Get device register */
	EMMC_SET_BLK_SIZE,		/**< Set the block size */
	EMMC_GET_BLK_SIZE,		/**< Get the block size */

	EMMC_GET_BUS_STATUS,	/**< Get the bus status */

	EMMC_ENUM_MAX
} EMMC_DEVICE_IOCTL_LIST;

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------
/**
 * \brief MMC_CSD structure
 *
 */
typedef struct mmc_csd {
	UINT8		mmc_struct;		/**< Structure */
	UINT8		mmca_vsn;
	UINT16		command_class;
	UINT16		clks_for_tacc;
	UINT32		ns_for_tacc;
	UINT32		c_sz;
	UINT32		factor_for_r2w;
	UINT32		max_dtr;
	UINT32		sz_for_erase;		/**< erase_size In sectors */
	UINT32		rd_bits_for_blk;
	UINT32		wr_bits_for_blk;
	UINT32		capa_size;
	UINT32		rd_part:1,
				rd_misalign_flag:1,
				wr_part:1,
				wr_misalign_flag:1;
}_MMC_CSD;

/**
 * \brief MMC_EXT_CSD structure
 *
 */
typedef struct mmc_ext_csd {
	UINT8		rev;
	UINT8		erase_group_def;
	UINT8		sec_feature_support;
	UINT8		rel_sectors;
	UINT8		rel_param;
	UINT8		part_config;
	UINT8		rst_n_function;
	UINT8		cache_ctrl;
	UINT8		max_packed_writes;
	UINT8		max_packed_reads;
	UINT8		packed_event_en;
	UINT32		part_time;		/* Units: ms */
	UINT32		sa_timeout;		/* Units: 100ns */
	UINT32		generic_cmd6_time;	/* Units: 10ms */
	UINT32      power_off_longtime;     /* Units: ms */
	UINT32		hs_max_dtr;
	UINT32		sectors;
	UINT32		card_type;
	UINT32		hc_erase_size;		/* In sectors */
	UINT32		hc_erase_timeout;	/* In milliseconds */
	UINT32		sec_trim_mult;	/* Secure trim multiplier  */
	UINT32		sec_erase_mult;	/* Secure erase multiplier */
	UINT32		trim_timeout;		/* In milliseconds */
	UINT8		enhanced_area_en;	/* enable bit */
	unsigned long long	enhanced_area_offset;	/* Units: Byte */
	UINT32		enhanced_area_size;	/* Units: KB */
	UINT32		boot_size;		/* in bytes */
	UINT32		cache_size;		/* Units: KB */
	UINT8		hpi_en;			/* HPI enablebit */
	UINT8		hpi;			/* HPI support bit */
	UINT32		hpi_cmd;		/* cmd used as HPI */
	UINT8		raw_partition_support;	/* 160 */
	UINT8		raw_erased_mem_count;	/* 181 */
	UINT8		raw_ext_csd_structure;	/* 194 */
	UINT8		raw_card_type;		/* 196 */
	UINT8		out_of_int_time;	/* 198 */
	UINT8		raw_s_a_timeout;		/* 217 */
	UINT8		raw_hc_erase_gap_size;	/* 221 */
	UINT8		raw_erase_timeout_mult;	/* 223 */
	UINT8		raw_hc_erase_grp_size;	/* 224 */
	UINT8		raw_sec_trim_mult;	/* 229 */
	UINT8		raw_sec_erase_mult;	/* 230 */
	UINT8		raw_sec_feature_support;/* 231 */
	UINT8		raw_trim_mult;		/* 232 */
	UINT8		raw_sectors[4];		/* 212 - 4 bytes */

	UINT32      feature_support;
#define MMC_DISCARD_FEATURE	BIT(0)                  /* CMD38 feature */
}_MMC_EXT_CSD;


/*
 It is differnt struct from SD. but the contents are same.
*/
/**
 * \brief SD_CID struct
 *
 * It is used for SD card and MMC
 */
typedef struct mmc_cid {
	UINT32		manfid;			/**< Manufacturer ID */
	INT8		prod_name[8];	/**< Product name */
	UINT32		serial;			/**< Serial number */
	UINT16		oemid;			/**< OEM ID */
	UINT16		year;			/**< Year */
	UINT8		hwrev;			/**< HW revision */
	UINT8		fwrev;			/**< FW revision */
	UINT8		month;			/**< Month */
}_MMC_CID;

//
//	Register MAP
//
/**
 * \brief HW register map
 *
 * Base Address 0x50010000
 */
typedef struct	__emmc_reg_map__
{
	volatile UINT32	HIF_CTL_00;						// 0x00 host interface control
	volatile UINT32	HIF_EVNT_CTL;					// 0x04 event and status
	volatile UINT32	HIF_INT_CTL;	 				// 0x08 interrupt control & status
	volatile UINT32	HIF_CLK_CTL_CNT;	 			// 0x0C host clock control count
	volatile UINT32	HIF_CMD_ARG; 					// 0x10
	volatile UINT32	HIF_CMD_IDX; 					// 0x14
	volatile UINT32	HIF_CMD_ARGQ;					// 0x18
	volatile UINT32	HIF_CMD_IDXQ; 					// 0x1C
	volatile UINT32	HIF_PAD_CTL;					// 0x20
	volatile UINT32	HIF_BLK_LG;						// 0x24
	volatile UINT32	HIF_BLK_CNT;					// 0x28
	volatile UINT32	Reserved0;	 					// 0x2C
	volatile UINT32	HIF_RSP_TMO_CNT;				// 0x30
	volatile UINT32	HIF_RD_TMO_CNT;					// 0x34
	volatile UINT32	HIF_WB_TMO_CNT;					// 0x38
	volatile UINT32	HIF_RSP_CIX_ST;					// 0x3C
	volatile UINT32	HIF_RSP_ARG_0; 					// 0x40
	volatile UINT32	HIF_RSP_ARG_1;					// 0x44
	volatile UINT32	HIF_RSP_ARG_2;					// 0x48
	volatile UINT32	HIF_RSP_ARG_3; 					// 0x4C
	volatile UINT32	HIF_AHB_SA;						// 0x50
	volatile UINT32	HIF_AHB_EA;						// 0x54
	volatile UINT32	Reserved1[2];					// 0x58 ~ 0x5c
	volatile UINT32	HIF_BUS_ST;						// 0x60 bus status
	volatile UINT32	HIF_SM_ST;						// 0x64 state machine
	volatile UINT32	HIF_XTR_CNT;					// 0x68
	volatile UINT32 HIF_ERR_CNT;					// 0x6c
	//volatile UINT32 Reserved1[4];					// 0x70 ~ 0x7f
} EMMC_REG_MAP;

//
//	Driver Structure
//
/**
 * \brief Driver Structure
 *
 *
 */
typedef struct	__emmc_device_handler__
{
	UINT32				dev_addr;  					// Unique Address
	// Device-dependant
	EMMC_REG_MAP		*regmap;

	CARD_TYPE			card_type;					/**< Card type */
	UINT32				ocr;						/**< OCR value */
	UINT32  			rca;
	UINT32				type;
	UINT32				raw_cid[4];
	UINT32				raw_csd[4];
	UINT32				raw_scr[2];
	UINT32				raw_ssr[16];
	UINT32				use_spi;
	UINT32				clock;
	UINT32 				max_blk_size;
	UINT32				cur_blk_size;

	UINT32				bus_width;
	UINT32				erased_byte;
	_MMC_CSD			csd;
	_MMC_EXT_CSD		ext_csd;
	_MMC_CID			cid;
	_SD_SCR				scr;
	_SD_SSR				ssr;
	_SD_SWITCH_CAPS		caps;
	_SDIO_CCCR			cccr;
	_SDIO_CIS			cis;
	UINT8				*pext_csd;
	UINT8				*psdio_cis;
	UINT32				sdio_num_info;
	UINT8				**psdio_info;

	UINT32 				emmc_queue_buf;
	QueueHandle_t			emmc_queue;
	SemaphoreHandle_t 		emmc_sema;
} EMMC_HANDLER_TYPE;

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 ****************************************************************************************
 * @brief Sets the driver behavior between interrupt or polling. default EMMC_INTERRUPT_DRIVEN. (normally no need to call.)
 * @param[in] type Behavior type. EMMC_INTERRUPT_DRIVEN or EMMC_POLLING.
 * @return EMMC_DRV_TYPE.
 * @sa EMMC_DRV_TYPE
 ****************************************************************************************
 */
extern EMMC_DRV_TYPE EMMC_SET_DRV_TYPE(EMMC_DRV_TYPE type);

/**
 ****************************************************************************************
 * @brief Sets the delay while device initialize. default 0 ms delay. (normally no need to call.)
 * @param[in] delay delay value unit ms.
 * @return delay value.
 ****************************************************************************************
 */
extern int EMMC_SET_DELAY(int delay);
extern int EMMC_SET_DEBUG(int debug_on);

/**
 ****************************************************************************************
 * @brief Sets the Clock output bevavior between Always or ReadWrite only. default value is EMMC_CLK_ALWAYS_ON (normally no need to call.)
 * @param[in] Clock output bevavior EMMC_CLK_ALWAYS_ON or EMMC_CLK_RW_ON.
 * @return Clock output type.
 * @sa EMMC_CLK_TYPE
 ****************************************************************************************
 */
extern EMMC_CLK_TYPE EMMC_SET_CLK_ALWAYS_ON(EMMC_CLK_TYPE type);

/**
 ****************************************************************************************
 * @brief Function create handle. If memory allocation failed, it returns NULL.
 * @return Handle value if succeeded, NULL else.
 ****************************************************************************************
 */
extern HANDLE EMMC_CREATE( void );
extern int EMMC_DETECT(void);

/**
 ****************************************************************************************
 * @brief Initialize the SD/eMMC or SDIO card. If the function returns ERR_NONE, the card information is saved in the handle
 * @param[in] Device handle.
 * @return ERR_NONE if successed, ERR_MMC_INIT else.
 * @note example
 * @code{.c}
	#define EMMC_CLK_DIV_VAL		0x0b	// output clock = bus clock / (2^emmc_clk_div_val)
	DA16X_CLOCK_SCGATE->Off_SysC_HIF = 0;
	DA16200_SYSCLOCK->CLK_DIV_EMMC = EMMC_CLK_DIV_VAL;
	DA16200_SYSCLOCK->CLK_EN_SDeMMC = 0x01;		// clock enable

	// setting pin mux
	// GPIO[9] - mSDeMMCIO_D0,  GPIO[8] - mSDeMMCIO_D1
	_da16x_io_pinmux(PIN_EMUX, EMUX_SDm);
	// GPIO[5] - mSDeMMCIO_CLK,  GPIO[4] - mSDeMMCIO_CMD
	_da16x_io_pinmux(PIN_CMUX, CMUX_SDm);
	// GPIO[7] - mSDeMMCIO_D2,  GPIO[6] - mSDeMMCIO_D3
	_da16x_io_pinmux(PIN_DMUX, DMUX_SDm);

	if (_emmc == NULL)
		_emmc = EMMC_CREATE();

	err = EMMC_INIT(_emmc);
	if (err)
	{
		PRINTF("emmc_init fail\n");
	}
	else
	{
		if (_emmc->card_type)
		{
			PRINTF("%s is inserted\n",
				   ((_emmc->card_type == SD_CARD) ? "SD card" :
					(_emmc->card_type == SD_CARD_1_1) ? "SD_CARD 1.1" :
					(_emmc->card_type == SDIO_CARD) ?
					"SDIO_CARD" : "MMC card"));
		}
		// set the clock = bus clock / 2^1  -> ex) BUS 80Mhz, SD clock 40Mhz
		DA16200_SYSCLOCK->CLK_DIV_EMMC = 1;
	}
 * @endcode
 ****************************************************************************************
 */
extern int EMMC_INIT (HANDLE handler);

extern void EMMC_SEND_CMD(HANDLE handler, UINT32 cmd, UINT32 cmd_arg);
extern void EMMC_SEND_CMD_RES(HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp);
extern int EMMC_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 ****************************************************************************************
 * @brief Data read from the device.
 * @param[in] handler Handle.
 * @param[in] dev_addr Address value to read.
 * @param[in] p_data Pointer of the buffer.
 * @param[in] block_count Block count for read.
 * @return If succeeded return ERR_NONE, if failed return error case.
 ****************************************************************************************
 */
extern int EMMC_READ(HANDLE handler, UINT32 dev_addr, VOID *p_data, UINT32 block_count);

/**
 ****************************************************************************************
 * @brief Data write to the device.
 * @param[in] handler Handle.
 * @param[in] dev_addr Address value to write.
 * @param[in] p_data Pointer of the buffer.
 * @param[in] block_count Block count for write.
 * @return If succeeded return ERR_NONE, if failed return error case.
 ****************************************************************************************
 */
extern int EMMC_WRITE(HANDLE handler, UINT32 dev_addr, VOID *p_data, UINT32 block_count);
extern int EMMC_ERASE(HANDLE handler, UINT32 start_addr, UINT32 block_count);

/**
 ****************************************************************************************
 * @brief Free the device handler.
 * @param[in] handler Handle.
 * @return If succeeded return ERR_NONE.
 ****************************************************************************************
 */
extern int EMMC_CLOSE(HANDLE handler);

/**
 ****************************************************************************************
 * @brief Enable the SDIO function.
 * @param[in] handler Handle.
 * @param[in] func_num Function number want to enable.
 * @return If succeeded return ERR_NONE.
 ****************************************************************************************
 */
extern int SDIO_ENABLE_FUNC(HANDLE handler, UINT32 func_num);

/**
 ****************************************************************************************
 * @brief Disable the SDIO function.
 * @param[in] handler Handle.
 * @param[in] func_num Function number want to disable.
 * @return If succeeded return ERR_NONE.
 ****************************************************************************************
 */
extern int SDIO_DISABLE_FUNC(HANDLE handler, UINT32 func_num);

/**
 ****************************************************************************************
 * @brief Sets the block size of SDIO device.
 * @param[in] handler Handle.
 * @param[in] func_num Function number.
 * @param[in] blk_size block size.
 * @return If succeeded return ERR_NONE.
 ****************************************************************************************
 */
extern int SDIO_SET_BLOCK_SIZE(HANDLE handler, UINT32 func_num, UINT32 blk_size);

/**
 ****************************************************************************************
 * @brief Read data from SDIO by using CMD52.
 * @param[in] handler Handle.
 * @param[in] func_num Function number.
 * @param[in] addr Address of data.
 * @param[in] data Pointer of buffer.
 * @return If succeeded return ERR_NONE.
 ****************************************************************************************
 */
extern int SDIO_READ_BYTE(HANDLE handler, UINT32 func_num, UINT32 addr, UINT8 *data);

/**
 ****************************************************************************************
 * @brief Write data to SDIO by using CMD52.
 * @param[in] handler Handle.
 * @param[in] func_num Function number.
 * @param[in] addr Address of data.
 * @param[in] data Byte data to write.
 * @return If succeeded return ERR_NONE.
 ****************************************************************************************
 */
extern int SDIO_WRITE_BYTE(HANDLE handler, UINT32 func_num, UINT32 addr, UINT8 data);

/**
 ****************************************************************************************
 * @brief Read data from SDIO by using CMD53.
 * @param[in] handler Handle.
 * @param[in] func_num Function number.
 * @param[in] addr Address.
 * @param[in] incr_addr Increase address option (1: address increase, 0: address fix).
 * @param[in] data Pointer of buffer.
 * @param[in] count Count of blocks.
 * @param[in] blksz Block size.
 * @return If succeeded return ERR_NONE, if failed return error case.
 ****************************************************************************************
 */
extern int SDIO_READ_BURST(HANDLE handler, UINT32 func_num, UINT32 addr, UINT32 incr_addr, UINT8 *data, UINT32 count, UINT32 blksz);

/**
 ****************************************************************************************
 * @brief Write data to SDIO by using CMD53.
 * @param[in] handler Handle.
 * @param[in] func_num Function number.
 * @param[in] addr Address.
 * @param[in] incr_addr Increase address option (1: address increase, 0: address fix).
 * @param[in] data Pointer of buffer.
 * @param[in] count Count of blocks.
 * @param[in] blksz Block size.
 * @return If succeeded return ERR_NONE, if failed return error case.
 ****************************************************************************************
 */
extern int SDIO_WRITE_BURST(HANDLE handler, UINT32 func_num, UINT32 addr, UINT32 incr_addr, UINT8 *data, UINT32 count, UINT32 blksz);

#endif	/*__uart_h__*/

