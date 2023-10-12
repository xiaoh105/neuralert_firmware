/**
 * \addtogroup HALayer
 * \{
 * \addtogroup FDMA
 * \{
 * \brief Fast DMA that supports a memory-to-memory transfer.
 */

/**
 ****************************************************************************************
 *
 * @file fdma.h
 *
 * @brief Fast DMA Driver
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


#ifndef __fdma_h__
#define __fdma_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"
#include "oal.h"

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------

typedef 	struct __fdma_handler__		FDMA_HANDLER_TYPE; 	// deferred
typedef 	struct __fdma_ctrlregmap__ 	FDMA_CTRLREG_MAP;	// deferred
typedef 	struct __fdma_llimap__		FDMA_LLI_MAP;		// deferred
typedef		struct __fdma_tasklmap__	FDMA_TASK_MAP;		// deferred
typedef 	struct __fdma_swllimap__	FDMA_SWLLI_MAP;		// deferred
typedef 	struct __fdma_swtskmap__	FDMA_SWTSK_MAP;		// deferred

//---------------------------------------------------------
//	HAL :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief FDMA ID
 */
typedef enum	{
	FDMA_UNIT_0	= 0,	/**< FDMA */
	FDMA_UNIT_MAX
} FDMA_UNIT_LIST;

typedef enum	{
	FDMA_SET_DEBUG = 0,
	FDMA_SET_CLOCK ,
	FDMA_SET_MAX
} FDMA_IOCTL_LIST;

//---------------------------------------------------------
//	HAL :: Offset
//---------------------------------------------------------

// DMA
#define	ERROR_FDMA_FULL		-1

// Channel
#define	FDMA_CHAN_MAX		4
#define FDMA_CHAN_MASK		0x0f

// Offset
#define	DA16X_FDMA_CTRLREG_BASE	(0x0000)
#define	DA16X_FDMA_CHANREG_BASE	(0x0100)


//---------------------------------------------------------
//	HAL :: Bit Field Definition
//---------------------------------------------------------

/**
 * \brief Channel Configuration - attributes of the destnation and source port. \n
 *	Attributes include an addressing mode, AHB port, and transfer data width.
 */
#define	FDMA_CHAN_CONFIG(dai,dpt,dhs, sai,spt,shs) 	(		\
	  ((dai&0x01)<<31) | ((dpt&0x01)<<30) | ((dhs&0x03)<<28) |	\
	  ((sai&0x01)<<27) | ((spt&0x01)<<26) | ((shs&0x03)<<24)   )

/**
 * \brief Channel Attribute - address increment
 */
#define	FDMA_CHAN_AINCR			(1)
/**
 * \brief Channel Attribute - AHB port #0
 */
#define	FDMA_CHAN_AHB0			(0)
/**
 * \brief Channel Attribute - AHB port #1
 */
#define	FDMA_CHAN_AHB1			(1)
/**
 * \brief Channel Attribute - 1-Byte data width 
 */
#define	FDMA_CHAN_HSIZE_1B		(0)
/**
 * \brief Channel Attribute - 2-Byte data width 
 */
#define	FDMA_CHAN_HSIZE_2B		(1)
/**
 * \brief Channel Attribute - 4-Byte data width 
 */
#define	FDMA_CHAN_HSIZE_4B		(2)
/**
 * \brief Channel Configuration - Burst Length
 */
#define	FDMA_CHAN_BURST(x)		(((x)&0x0f)<<20) /* (1<<0) to (1<<15) */

#define	FDMA_CHAN_TTLMASK		0x000fffff
/**
 * \brief Channel Configuration - Total Length
 */
#define	FDMA_CHAN_TOTALEN(x)		(((x)-1)&FDMA_CHAN_TTLMASK)
#define	FDMA_CHAN_GET_TOTALEN(x)	(((x)&FDMA_CHAN_TTLMASK)+1)


/**
 * \brief Channel Control - Priority and Idle cycle
 */
#define	FDMA_CHAN_CONTROL(prt,idl) 	( (((prt)&0x07)<<21)|(((idl)&0x0f)<<16) )
/**
 * \brief Channel Control - Enable a linked list operation 
 */
#define	FDMA_CHAN_LLIEN			(1<<27)
/**
 * \brief Channel Control - Enable an interrupt
 */
#define	FDMA_CHAN_IRQEN			(1<<20)
/**
 * \brief Channel Control - Priority and Idle cycle
 */
#define	FDMA_CHAN_PARAM(x)		((x)&0x0ffff)


//---------------------------------------------------------
//	HAL :: Interrupt
//---------------------------------------------------------
//
//	Interrupt Register
//

//---------------------------------------------------------
//	HAL :: Structure
//---------------------------------------------------------

struct __fdma_ctrlregmap__
{
	volatile UINT32 	ReqHOLD;
	volatile UINT32 	ReqIRQ;
	volatile UINT32 	ReqOP;
	volatile UINT32 	ReqCH;

	volatile UINT32 	Status;
	volatile UINT32 	CHLLISize;
	volatile UINT32 	_reserved1;
	volatile UINT32 	_reserved2;

	volatile UINT32 	History[4];
};

/**
 * \brief Structure of Linked-List Item (LLI)
 */
struct __fdma_llimap__
{
	volatile UINT32 	Config;
	volatile UINT32 	Control;
	volatile UINT32 	SrcAddr;
	volatile UINT32 	DstAddr;
};

struct __fdma_tasklmap__
{
	volatile FDMA_LLI_MAP	LLI;
	volatile UINT32 	LLIbase;
	volatile UINT32 	LLInum;
	volatile UINT32 	LLIindex;
	volatile UINT32		_reserved;
	volatile UINT32		LLIstatus;
};

struct __fdma_swllimap__
{
	FDMA_LLI_MAP		lli;
	USR_CALLBACK		callback;
	VOID			*param;
};

struct __fdma_swtskmap__
{
	HANDLE			fdma;
	UINT16			id;
	UINT16			max;
	UINT32 			Config;
	UINT32 			Control;
	UINT16			swptr;
	UINT16			hwptr;
	FDMA_SWLLI_MAP		*lli;
	FDMA_SWTSK_MAP		*nxt;
};

/**
 * \brief Structure of FDMA Driver
 */
struct	__fdma_handler__
{
	UINT32			dev_addr;		/**< Physical address for identification */
	// Device-dependant
	UINT32			instance;		/**< Number of times that a driver with the same ID is created */
	UINT32			statistics;		/**< Usage counter */
	// Register Map
	FDMA_CTRLREG_MAP	*control;	/**< Control part of a register map */
	FDMA_TASK_MAP		*hwtask[FDMA_CHAN_MAX];	/**< LLI configuration part of a register map */
	UINT32			hwindex;		/**< Current HW index */
	UINT32			swindex;		/**< Current SW index */

	UINT32			swchannum;		/**< Number of registered channels */
	FDMA_SWTSK_MAP		*swchanhead;	/**< data pointer for SW LLI */
	FDMA_SWTSK_MAP		*swchantail;	/**< data pointer for SW LLI */

	// Sync b/w HISR & RW-Func
	SemaphoreHandle_t   mutex;
	EventGroupHandle_t	event;
	UINT32			mode;			/**< running mode (TRUE is RTOS) */
};

//---------------------------------------------------------
//	HAL :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create FDMA driver
 *
 * \param [in] dev_type		id
 *
 * \return	HANDLE			pointer to the driver instance
 *
 */
extern HANDLE	FDMA_CREATE( UINT32 dev_id );

/**
 * \brief Initialize FDMA driver
 *
 * \param [in] handler		driver handler
 * \param [in] mode			running mode (TRUE : RTOS)
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_INIT(HANDLE handler, UINT32 mode);

/**
 * \brief Create Channel handler
 *
 * \param [in] handler		driver handler
 * \param [in] chan_id		channel id
 * \param [in] control		channel control
 * \param [in] config		channel configuration
 * \param [in] maxlli		maximum number of LLIs
 *
 * \return	HANDLE			pointer to the channel handler
 *
 */
extern HANDLE	FDMA_OBTAIN_CHANNEL(HANDLE handler, UINT32 chan_id,
				UINT32 control, UINT32 config, UINT32 maxlli);

/**
 * \brief Reconfigure Channel
 *
 * \param [in] channel		channel handler
 * \param [in] control		channel control
 * \param [in] config		channel configuration
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_CHANGE_CHANNEL(HANDLE channel,
				UINT32 control, UINT32 config);

/**
 * \brief Mutex Lock for Thread Synchronization
 *
 * \param [in] channel		channel handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_LOCK_CHANNEL(HANDLE channel);

/**
 * \brief Mutex Unlock for Thread Synchronization
 *
 * \param [in] channel		channel handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_UNLOCK_CHANNEL(HANDLE channel);

/**
 * \brief Insert New item into the Linked List of Channel
 *
 * \param [in] channel		channel handler
 * \param [in] dst			destination address
 * \param [in] src			source address
 * \param [in] dlen			data length
 * \param [in] callbk		callback funciton
 * \param [in] param		parameters of callback
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_SCATTER_CHANNEL(HANDLE channel,
				VOID *dst, VOID *src, UINT32 dlen
				, UINT32 callbk, UINT32 param);

/**
 * \brief Start DMA Transter 
 *
 * \param [in] channel		channel handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_START_CHANNEL(HANDLE channel);

/**
 * \brief Stop DMA Transter 
 *
 * \param [in] channel		channel handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int 	FDMA_STOP_CHANNEL(HANDLE channel);

/**
 * \brief Check DMA Status
 *
 * \param [in] channel		channel handler
 *
 * \return	TRUE,			DMA is idle.
 * \return	FALSE, 			DMA is busy.
 *
 */
extern int 	FDMA_CHECK_CHANNEL(HANDLE channel);

/**
 * \brief Release Channel handler
 *
 * \param [in] handler		driver handler
 * \param [in] channel		channel handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_RELEASE_CHANNEL(HANDLE handler, HANDLE channel );

/**
 * \brief Print DMA Status
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int 	FDMA_PRINT_STATUS(HANDLE handler);

/**
 * \brief Close FDMA driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	FDMA_CLOSE(HANDLE handler);

//---------------------------------------------------------
//	HAL :: DEVICE-Interface
//---------------------------------------------------------

/**
 * \brief Batch function to initialize FDMA at boot
 *
 * \param [in] mode			running mode
 *
 */
extern void	FDMA_SYS_INIT(UINT32 mode);

/**
 * \brief Batch function to de-initialize FDMA
 *
 */
extern void	FDMA_SYS_DEINIT(void);

/**
 * \brief Get Current FDMA handler
 *
 * \return	driver handler
 *
 */
extern HANDLE	FDMA_SYS_GET_HANDLER(void);

//---------------------------------------------------------
//	HAL :: NONOS-Interface
//---------------------------------------------------------

extern void	ROM_FDMA_COPY(UINT32 mode, UINT32 *dest, UINT32 *src, UINT32 size);

#endif /* __fdma_h__ */
/**
 * \}
 * \}
 */
