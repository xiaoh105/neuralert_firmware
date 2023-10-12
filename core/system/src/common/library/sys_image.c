/**
 ****************************************************************************************
 *
 * @file sys_image.c
 *
 * @brief Image management
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "driver.h"
#include "library/library.h"

//------------------------------------------------------------
//
//------------------------------------------------------------

#undef	SUPPORT_LOADING_TIME
#undef	SUPPORT_SFLASH_SPEEDUP
#undef	SUPPORT_SFLASH_POWERDOWN

//------------------------------------------------------------
//
//------------------------------------------------------------

#define LOGICAL_PAGE_SIZE 		sizeof(image_header_t)

#define	BOOT_TICK_SCALE			100

#define SFLASH_LOGICAL_PAGE_SIZE 	(sizeof(image_header_t)+ (sizeof(UINT32)*3) + da16x_sflash_get_parasize())

#define	IMAGE_SFLASH_READ_MODE		(SFLASH_BUS_144) // TEST
#define	IMAGE_SFLASH_SAFEREAD_MODE	(SFLASH_BUS_111)
#define	IMAGE_SFLASH_SAFE_MODE		(SFLASH_BUS_111)
#define IMAGE_SFLASH_SAFEWRITE_MODE	(SFLASH_BUS_111)

#define	SFLASH_TEST_SIZE	(sizeof(UINT8)*256)

#define	XFC_DMA_PORT		0x1A

#define IMG_INFO_PRINT(...)	//PRINTF( __VA_ARGS__ )
#define IMG_LOAD_PRINT(...)	PRINTF( __VA_ARGS__ )
#define SYS_IMG_RS485_INFO_PRINT(...)	PRINTF( __VA_ARGS__ )

//------------------------------------------------------------
//
//------------------------------------------------------------

#define	SFDP_TABLE_SIZE		0x40
#define	SFDP_TABLE_MAGIC	0x50444653
#define	SFDP_RETMEM_SFLG	11
#define	SFDP_RETMEM_ICRC	12
#define	SFDP_RETMEM_ISIZ	13
#define	SFDP_RETMEM_ICTL	14

// DA16X secure format
typedef union {
	UINT8	bulk[164]; // New SFDP 164, Old SFDP 180
	struct	{
		UINT8	sfdp[64];
		UINT8	extra[28];
		UINT8	cmdlst[44];
		UINT8	delay[16];
		UINT8	_dummy[12];
	} sfdptab;
} PSEUDO_SFDP_CHUNK_TYPE;

typedef		struct	{
	UINT32	magic;
	UINT32	devid;
	UINT16	offset;
	UINT16	length;
	PSEUDO_SFDP_CHUNK_TYPE chunk;
} PSEUDO_SFDP_TABLE_TYPE;

typedef		struct {
	UINT32	length;
	UINT32	crc;
}	CERT_INFO_TYPE;

typedef		struct	{
	UINT16	check_flag	;
	UINT16	check_boot	;
	HANDLE	boot_flash	;
	UINT8	*flash_imghdr	;
	VOID	*retmem_sfdp	;
	CERT_INFO_TYPE	*certchain;
#ifdef	SUPPORT_LOADING_TIME
	UINT32	timestamp[5];	// 0: init, 1: hdr, 2: sfdp, 3: data, 4: deinit
#endif	//SUPPORT_LOADING_TIME
	VOID	(*lockfunc)(UINT32);
	UINT32	load_offset;

	UINT32	erase_size;
	UINT32	erase_offset;
	UINT32	erase_mode;

	UINT32	ioctldata[8];

} DA16X_FLASH_BOOT_TYPE;

#define	SFLASH_BOOT_LOCKER(sboot,mode)				\
		if(sboot->lockfunc != NULL ){			\
			sboot->lockfunc(mode);			\
		}

/******************************************************************************
 *  nor_image_load
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 nor_image_load(UINT32 mode, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
#ifdef	BUILD_OPT_DA16200_FPGA

	UINT32	crc32value;
	image_header_t	   *imghdr;
	UINT8	*writeregion, *readregion ;
	UINT32  fcicrc, ih_size;
	UINT32 bkupnorype, bkupintr;

	imghdr = (image_header_t *)(UINT8 *)DRIVER_MALLOC(LOGICAL_PAGE_SIZE);

	////////////////////////////////////////////////////////
	// Read First Block :: Image Header
	readregion = (UINT8 *) imghdr_offset;
	////////////////////////////////////////////////////////
	CHECK_INTERRUPTS(bkupintr);
	PREVENT_INTERRUPTS(1);
	bkupnorype = DA16XFPGA_GET_NORTYPE();
	if( (bkupnorype == DA16X_NORTYPE_CACHE) ){
		da16x_cache_disable();
	}
	DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // FLASH access
	////////////////////////////////////////////////////////

	DRIVER_MEMCPY( imghdr, (void *)readregion, sizeof(image_header_t) );

	////////////////////////////////////////////////////////
	DA16XFPGA_SET_NORTYPE(bkupnorype);	// FLASH access
	if( (bkupnorype == DA16X_NORTYPE_CACHE) ){
		da16x_cache_enable(FALSE);
	}
	PREVENT_INTERRUPTS(bkupintr);
	////////////////////////////////////////////////////////

	// Check
	if(da16xfunc_ntohl(imghdr->ih_magic) != (IH_MAGIC)){

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C2C)|0xA);

		PRINTF(" [%s] Wrong magic code: %x - %x [%p]\n",
								__func__,
								da16xfunc_ntohl(imghdr->ih_magic), (IH_MAGIC),
								readregion );
		DRIVER_FREE(imghdr);
		return 0;
	}

	if( (mode & NOR_BOOT_CHECK) == NOR_BOOT_CHECK )
	{
		UINT32 bkup_hcrc;

		bkup_hcrc  = da16xfunc_ntohl( imghdr->ih_hcrc );
		imghdr->ih_hcrc = 0;

		fcicrc = da16x_hwcrc32(sizeof(UINT32), (void *)imghdr, sizeof(image_header_t), (~0));

		if( bkup_hcrc != fcicrc ){
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C2C)|0xB);

			PRINTF("Image Header, CRC Failed (%08x - %08x) [%08x]!!\n"
				, bkup_hcrc, fcicrc, da16x_btm_get_timestamp() );

			DRIVER_FREE(imghdr);

			return 0;
		}else{
			IMG_INFO_PRINT("Image Header, CRC Completed (%08x) [%08x]!!\n"
				, bkup_hcrc, da16x_btm_get_timestamp() );
		}
	}

	// SWAP
	imghdr->ih_magic = da16xfunc_ntohl( imghdr->ih_magic );
	imghdr->ih_hcrc  = da16xfunc_ntohl( imghdr->ih_hcrc );
	imghdr->ih_time  = da16xfunc_ntohl( imghdr->ih_time );
	imghdr->ih_size  = da16xfunc_ntohl( imghdr->ih_size );
	imghdr->ih_load  = da16xfunc_ntohl( imghdr->ih_load );
	imghdr->ih_ep	 = da16xfunc_ntohl( imghdr->ih_ep );
	imghdr->ih_dcrc  = da16xfunc_ntohl( imghdr->ih_dcrc );

	*jmp_addr  = imghdr->ih_ep ;
	*load_addr = imghdr->ih_load;

	if( (mode & NOR_BOOT_HEADER) == NOR_BOOT_HEADER ){
		ih_size = imghdr->ih_size;
		DRIVER_FREE(imghdr);
		return  ih_size;
	}

	// Copy Image
	if( (DA16200FPGA_SYSCON->_reserved1 == 0xDA16200AC) && (*load_addr <= DA16X_ROM_END) ){
		// ROM Patch
		writeregion = (UINT8 *) (*load_addr) ;
	}else
	if( ((mode & NOR_BOOT_RUN) == 0) || (*load_addr <= DA16X_ROM_END) || ((*load_addr >= DA16X_CACHE_BASE) && (*load_addr < DA16X_CACHE_END)) ){
		writeregion = (UINT8 *) 0xFFFFFFFF;
	}else{
		writeregion = (UINT8 *) (*load_addr) ;
	}
	//readregion = &(readregion[sizeof(image_header_t)]) ;
	readregion = (UINT8 *)(imghdr_offset + sizeof(image_header_t));

	IMG_INFO_PRINT("Image Data, CRC Check Start (%p, %p, %p) [%08x]\n"
		, (UINT8 *)(imghdr->ih_load), writeregion, readregion
		, da16x_btm_get_timestamp() );

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C2C)|0x00);

#ifdef	SUPPORT_LOADING_TIME
	da16x_btm_control(TRUE);
#endif	//SUPPORT_LOADING_TIME

	crc32value = da16x_hwcopycrc32( (CRC_CTRL_CACHE|sizeof(UINT32)), (UINT8 *)writeregion, (UINT8 *)readregion, imghdr->ih_size, (~0));

#ifdef	SUPPORT_LOADING_TIME
	da16x_btm_control(FALSE);
#endif	//SUPPORT_LOADING_TIME

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C2C)|0x01);

	fcicrc = crc32value;

	if( imghdr->ih_dcrc != fcicrc ){
		ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C2C)|0xC);

		PRINTF("Image Data, CRC Failed (%08x - %08x, %08x, %d)"
#ifdef	SUPPORT_LOADING_TIME
			"  [%08x]"
#endif	//SUPPORT_LOADING_TIME
			"!!\n"
			, imghdr->ih_dcrc
			, fcicrc
			, (imghdr->ih_ep)
			, (imghdr->ih_size)
#ifdef	SUPPORT_LOADING_TIME
			, da16x_btm_get_timestamp()
#endif	//SUPPORT_LOADING_TIME
			);
		return 0;
	}else{
		IMG_INFO_PRINT("Image Data, CRC Completed (%08x)"
#ifdef	SUPPORT_LOADING_TIME
			"  [%08x]"
#endif	//SUPPORT_LOADING_TIME
			"!!\n"
			, imghdr->ih_dcrc
#ifdef	SUPPORT_LOADING_TIME
			, da16x_btm_get_timestamp()
#endif	//SUPPORT_LOADING_TIME
			);
	}

	*jmp_addr  = imghdr->ih_ep ;
	*load_addr = imghdr->ih_load;

	ih_size = imghdr->ih_size;
	DRIVER_FREE(imghdr);

	ASIC_DBG_TRIGGER(*jmp_addr);
	ASIC_DBG_TRIGGER(*load_addr);

	return  ih_size;

#else	//BUILD_OPT_DA16200_FPGA
	DA16X_UNUSED_ARG(mode);
	DA16X_UNUSED_ARG(imghdr_offset);
	DA16X_UNUSED_ARG(load_addr);
	DA16X_UNUSED_ARG(jmp_addr);

	ASIC_DBG_TRIGGER(0xC2CE2202);
	return 0;
#endif	//BUILD_OPT_DA16200_FPGA
}


#ifdef	SUPPORT_XYMODEM

/******************************************************************************
 *  xymodem_ram_store
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 xymodem_ram_store (UINT32 mode, UINT32 offset, UINT32 bufsiz)
{
	connection_info_t info;
	int  readerr = 0;
	char *databuf = (char *)offset;
	int  len = 0;

	info.chan = 0;
	if((mode & 0x7f) == TRUE){
		info.mode = xyzModem_ymodem;
		PRINTF("\nLoad Y-Modem (Load Offset:%lx)\n", offset);
	}else{
		info.mode = xyzModem_xmodem;
		PRINTF("\nLoad X-Modem (Load Offset:%lx)\n", offset);
	}

	OAL_MSLEEP( 200/*ms*/ );

	readerr = 0;
	xyzModem_stream_open(&info, &readerr);
	if (readerr) {
		PRINTF("xymodem: readerr=%x\n", readerr);
	}else{
		len = xyzModem_stream_read(databuf, bufsiz, &readerr);
	}

	xyzModem_stream_close(&readerr);
	if( readerr != 0 && readerr != xyzModem_timeout ){
		// TODO: da16x_set_error( EPROTO );
	}

	OAL_MSLEEP( 200/*ms*/ );
	PRINTF("## Total Size      = 0x%08x = %d Bytes\n", len, len);

	return len;

}

/******************************************************************************
 *  xymodem_download_thread
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	DATA_QUE_NUM	8
#define XYM_QUE_MIN	2
#define XYM_THD_STACK	1024
#define RS485_UART_SW_QUESIZE 2048

typedef	 struct {
	UINT32 len;
	UINT8  *buf;
} xym_que_info;

typedef	 struct {
	UINT32 len;
	UINT8  *buf;
} rs485_que_info;

typedef	 struct {
	OAL_THREAD_TYPE thread;
	UINT8		stack[XYM_THD_STACK];
	UINT32		stksiz;
	OAL_QUEUE 	queue;
	xym_que_info 	quebuf[DATA_QUE_NUM];
	UINT8		*data[DATA_QUE_NUM];
	UINT32		bufsiz;
	UINT32		bufnum;
	UINT32 		queIndex;
	OAL_SEMAPHORE	*mutex;
	connection_info_t info;
	int 		readerr;
	int		size;
	int		fsync;
} XYMODEM_DOWN_INFO_TYPE;

typedef	 struct {
	OAL_THREAD_TYPE thread;
	UINT8		stack[XYM_THD_STACK];
	UINT32		stksiz;
	OAL_QUEUE 	queue;
	rs485_que_info 	quebuf[DATA_QUE_NUM];
	UINT8		*data[DATA_QUE_NUM];
	UINT32		bufsiz;
	UINT32		bufnum;
	UINT32 		queIndex;
	OAL_SEMAPHORE	*mutex;
	connection_info_t info;
	int 		readerr;
	int		size;
	int		fsync;
} RS485_DOWN_INFO_TYPE;

static  void	thread_xymodem_download(ULONG thread_input)
{
	XYMODEM_DOWN_INFO_TYPE	*thread;
	UINT32	queidx, len;
	xym_que_info que_info;

	thread = (XYMODEM_DOWN_INFO_TYPE *)thread_input;
	queidx = 0;

	thread->readerr = 0;

	xyzModem_stream_open(&(thread->info), &(thread->readerr));

	if (thread->readerr) {
		PRINTF("xymodem: readerr=%x\n", thread->readerr);
	}
	else
	{
		while (1) {

			que_info.buf = thread->data[queidx];

			if( thread->mutex != NULL ){
				OAL_OBTAIN_SEMAPHORE( thread->mutex, OAL_SUSPEND);
			}

			len = xyzModem_stream_read( (char *)(que_info.buf)
						, thread->bufsiz, &(thread->readerr));

			if( thread->mutex != NULL ){
				OAL_RELEASE_SEMAPHORE( thread->mutex);
			}

			if( len > 0 ){
				que_info.len = len;
				thread->size = thread->size + len;

				//DRV_SPRINTF((char *)(que_info.buf), "(i:%d,%d, s:%6d,l:%2d,b:%p)"
				//	, queidx , thread->bufnum
				//	, thread->size, que_info.len, que_info.buf);

//				PRINTF("len: 0x%x, 0x%x,0x%x\n", que_info.len, que_info.buf[0], que_info.buf[1]);


				if( OAL_SEND_TO_QUEUE( &(thread->queue)
						, &(que_info)
						, (sizeof(xym_que_info)/sizeof(UINT32))
						, OAL_SUSPEND) != OAL_SUCCESS ){
					break;
				}

				queidx = (queidx+1) % (thread->bufnum);
			}

			if (!len || thread->readerr){
				break;
			}
		}
	}

	que_info.len = 0;
	que_info.buf = NULL;

	OAL_SEND_TO_QUEUE( &(thread->queue)
			, &(que_info)
			, (sizeof(xym_que_info)/sizeof(UINT32))
			, OAL_SUSPEND);

	// wait until NOR accessing is finished
	while( thread->fsync != 0x1 ){
		OAL_MSLEEP(10);
	}

	xyzModem_stream_close(&(thread->readerr));

	// send a signal to complete all processes
	thread->fsync = 0x3;
}

UNSIGNED rs485QueueInfoGet(ULONG thread_input) {
	RS485_DOWN_INFO_TYPE	*thread;
	UNSIGNED available;
	CHAR name[10];

	thread = (RS485_DOWN_INFO_TYPE	*)thread_input;
	OAL_QUEUE_INFORMATION(&(thread->queue), name,
								  NULL, NULL,
								  &available, NULL,
								  NULL, NULL,
								  NULL, NULL,
								  NULL);
	return available;
}

void sendQueue(ULONG thread_input, uint8_t * pkt, uint16_t len) {
	RS485_DOWN_INFO_TYPE	*thread;
	rs485_que_info que_info;
	CHAR name[10];
	UNSIGNED available;
	UINT status;
	UINT32	timestamp;

	thread = (RS485_DOWN_INFO_TYPE *)thread_input;
	que_info.buf = thread->data[thread->queIndex];

	/*
	 * FIXME: mutex 처리 필요한지 확인할 것!!
	 */
	if( thread->mutex != NULL ){
		OAL_OBTAIN_SEMAPHORE( thread->mutex, OAL_SUSPEND);
	}

	DRV_MEMCPY((char *)(que_info.buf), pkt, len);

	if( thread->mutex != NULL ){
		OAL_RELEASE_SEMAPHORE( thread->mutex);
	}

	if( len > 0 ) {
		que_info.len = len;
		thread->size = thread->size + len;

		status = OAL_SEND_TO_QUEUE( &(thread->queue)
				, &(que_info)
				, (sizeof(rs485_que_info)/sizeof(UINT32))
				, OAL_SUSPEND);
		if( status != OAL_SUCCESS ){
			return;
		}
		thread->queIndex = (thread->queIndex+1) % (thread->bufnum);

	} else {
		que_info.len = 0;
		que_info.buf = NULL;

		OAL_SEND_TO_QUEUE( &(thread->queue)
				, &(que_info)
				, (sizeof(rs485_que_info)/sizeof(UINT32))
				, OAL_SUSPEND);

		// wait until NOR accessing is finished
		while( thread->fsync != 0x1 ){
			OAL_MSLEEP(10);
		}

		// send a signal to complete all processes
		thread->fsync = 0x3;
	}
}

static HANDLE uart_initialize(void)
{
	volatile INT32 ret;
	static UINT32 flag = 0;
	UINT32 clock = 80000000;
	UINT32 baud = 1152000, fifo_en = 1,
		   parity = 0, parity_en = 0,
		   hwflow = 0, swflow = 0,
		   word_access = 0, DMA = 0, rw_word = 0,
		   rs485Enable = 1;
	UINT32 temp;
	HANDLE uart1;

	uart1 = UART_CREATE(UART_UNIT_1);
	UART_IOCTL(uart1, UART_SET_CLOCK, &clock);

	UART_IOCTL(uart1, UART_SET_BAUDRATE, &baud);

	temp = UART_WORD_LENGTH(8) | UART_FIFO_ENABLE(fifo_en) | UART_PARITY_ENABLE(parity_en) | UART_EVEN_PARITY(parity) /*parity*/ ;
	UART_IOCTL(uart1, UART_SET_LINECTRL, &temp);

	temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(hwflow);
	UART_IOCTL(uart1, UART_SET_CONTROL, &temp);

	temp = UART_RX_INT_LEVEL(UART_ONEEIGHTH) |UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);
	UART_IOCTL(uart1, UART_SET_FIFO_INT_LEVEL, &temp);

	temp = DMA;
	UART_IOCTL(uart1, UART_SET_USE_DMA, &temp);

	if (fifo_en)
		temp = UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT | UART_INTBIT_TIMEOUT |UART_INTBIT_ERROR | UART_INTBIT_FRAME
				| UART_INTBIT_PARITY | UART_INTBIT_BREAK | UART_INTBIT_OVERRUN ;
	else
		temp = UART_INTBIT_RECEIVE;

	UART_IOCTL(uart1, UART_SET_INT, &temp);

    temp = swflow;
    UART_IOCTL(uart1, UART_SET_SW_FLOW_CONTROL, &temp);

    temp = word_access;
    UART_IOCTL(uart1, UART_SET_WORD_ACCESS, &temp);

    temp = rw_word;
    UART_IOCTL(uart1, UART_SET_RW_WORD, &temp);

    temp = rs485Enable;
    UART_IOCTL(uart1, UART_SET_RS485, &temp);

    temp = RS485_UART_SW_QUESIZE;
    UART_IOCTL(uart1, UART_SET_SW_RX_QUESIZE, &temp);

	UART_INIT(uart1);
	return uart1;
}

static const rs485_primitive_type rs485_primitives = {
		&sendQueue,
		&rs485QueueInfoGet
};

static void thread_rs485_download(ULONG thread_input)
{
	RS485_receive(thread_input);
}

static XYMODEM_DOWN_INFO_TYPE *create_thread_xymodem(UINT32 mode, UINT32 offset, UINT32 bufsiz, UINT32 bufnum, UINT32 flagmutex)
{
	XYMODEM_DOWN_INFO_TYPE *thread;
	UINT32	i;

	thread = (XYMODEM_DOWN_INFO_TYPE *)HAL_MALLOC(sizeof(XYMODEM_DOWN_INFO_TYPE));

	if(thread == NULL ){
		return NULL;
	}

	DRV_MEMSET(thread, 0, sizeof(XYMODEM_DOWN_INFO_TYPE));
	thread->fsync = 0x0;

	thread->bufsiz = bufsiz;
	thread->bufnum = (bufnum > DATA_QUE_NUM) ? DATA_QUE_NUM : bufnum;
	for( i = 0; i < thread->bufnum; i++ ){
		thread->data[i] = (UINT8 *)(offset + (i * bufsiz));
	}

	thread->info.chan = 0;
	if((mode & 0x0f) == TRUE){
		thread->info.mode = xyzModem_ymodem;
	}else{
		thread->info.mode = xyzModem_xmodem;
	}

	if( OAL_CREATE_QUEUE(&(thread->queue),
				"@xymque",
				(thread->quebuf),
				((sizeof(xym_que_info))*(thread->bufnum - XYM_QUE_MIN)),
				OAL_FIXED_SIZE,
				(sizeof(xym_que_info)/sizeof(UINT32)), /* 4 BYTE */
				OAL_SUSPEND) != OAL_SUCCESS ){
		HAL_FREE(thread);
		return NULL;
	}

	if( flagmutex == TRUE ){
		thread->mutex = (OAL_SEMAPHORE *)HAL_MALLOC(sizeof(OAL_SEMAPHORE));
		OAL_CREATE_SEMAPHORE( thread->mutex, "@xymtx", 1, OAL_SUSPEND);
	}

	if( OAL_CREATE_THREAD( &(thread->thread)
		, "@xymthd"
		, &thread_xymodem_download
		, (ULONG)(thread)
		, &(thread->stack[0])
		, (XYM_THD_STACK)
		, OAL_PRI_APP(1)
		, 0
		, OAL_PRI_APP(1)
		, OAL_START ) != OAL_SUCCESS ){

		HAL_FREE(thread);
		return NULL;
	}

	return thread;
}

static RS485_DOWN_INFO_TYPE *create_thread_rs485(UINT32 mode, UINT32 offset, UINT32 bufsiz, UINT32 bufnum, UINT32 flagmutex)
{
	RS485_DOWN_INFO_TYPE *thread;
	UINT32	i;

	thread = (RS485_DOWN_INFO_TYPE *)HAL_MALLOC(sizeof(RS485_DOWN_INFO_TYPE));

	if(thread == NULL ){
		return NULL;
	}

	DRV_MEMSET(thread, 0, sizeof(RS485_DOWN_INFO_TYPE));
	thread->fsync = 0x0;

	thread->bufsiz = bufsiz;
	thread->bufnum = (bufnum > DATA_QUE_NUM) ? DATA_QUE_NUM : bufnum;
	for( i = 0; i < thread->bufnum; i++ ){
		thread->data[i] = (UINT8 *)(offset + (i * bufsiz));
	}

	if( OAL_CREATE_QUEUE(&(thread->queue),
				"@rs485que",
				(thread->quebuf),
				((sizeof(rs485_que_info))*(thread->bufnum - XYM_QUE_MIN)),
				OAL_FIXED_SIZE,
				(sizeof(rs485_que_info)/sizeof(UINT32)), /* 4 BYTE */
				OAL_SUSPEND) != OAL_SUCCESS ){
		HAL_FREE(thread);
		return NULL;
	}

	if( flagmutex == TRUE ){
		thread->mutex = (OAL_SEMAPHORE *)HAL_MALLOC(sizeof(OAL_SEMAPHORE));
		OAL_CREATE_SEMAPHORE( thread->mutex, "@rs485tx", 1, OAL_SUSPEND);
	}

	if( OAL_CREATE_THREAD( &(thread->thread)
		, "@rs485thd"
		, &thread_rs485_download
		, (ULONG)(thread)
		, &(thread->stack[0])
		, (XYM_THD_STACK)
		, OAL_PRI_APP(1)
		, 0
		, OAL_PRI_APP(1)
		, OAL_START ) != OAL_SUCCESS ){
		HAL_FREE(thread);
		return NULL;
	}

	return thread;
}

static int delete_thread_xymodem(XYMODEM_DOWN_INFO_TYPE *thread)
{
	int size;

	// send a signal that NOR accessing is done.
	thread->fsync = 0x01;

	// wait until receiving done signal.
	while(thread->fsync != 0x03){
		OAL_MSLEEP(10);
	}

	while( OAL_DELETE_THREAD(&(thread->thread)) != OAL_SUCCESS){
		OAL_THREAD_RELINGISH();
	}

	if( thread->mutex != NULL ){
		OAL_DELETE_SEMAPHORE( thread->mutex );
		HAL_FREE( thread->mutex );
	}

	OAL_DELETE_QUEUE(&(thread->queue));

	size = thread->size;

	HAL_FREE(thread);

	return size;
}

static int delete_thread_rs485(RS485_DOWN_INFO_TYPE *thread)
{
	int size;

	// send a signal that NOR accessing is done.
	thread->fsync = 0x01;

	// wait until receiving done signal.
	while(thread->fsync != 0x03){
		OAL_MSLEEP(10);
	}

	while( OAL_DELETE_THREAD(&(thread->thread)) != OAL_SUCCESS){
		OAL_THREAD_RELINGISH();
	}

	if( thread->mutex != NULL ){
		OAL_DELETE_SEMAPHORE( thread->mutex );
		HAL_FREE( thread->mutex );
	}

	OAL_DELETE_QUEUE(&(thread->queue));

	size = thread->size;

	HAL_FREE(thread);

	return size;
}

/******************************************************************************
 *  xymodem_nor_store
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 xymodem_nor_store(UINT32 mode, UINT32 offset, UINT32 bufsiz, UINT32 bufnum, UINT32 secsize, UINT32 storeaddr)
{
#if	defined(BUILD_OPT_DA16200_FPGA) && defined(SUPPORT_NOR_DEVICE)

	HANDLE	device ;
	UINT32	ioctldata[3], first, mutex;
	int len, size ;
	XYMODEM_DOWN_INFO_TYPE *thread;

	if( bufnum <= (XYM_QUE_MIN) ){
		return 0;
	}

	device = NOR_CREATE(NOR_UNIT_0);
	if(device == NULL){
		return 0;
	}

	// Initialize
	NOR_INIT(device);

	if((mode & 0x80000000) != 0 ){
		mutex = TRUE;
	}else{
		mutex = FALSE;
	}

	ioctldata[0] = TRUE;
	NOR_IOCTL(device, NOR_SET_BOOTMODE, ioctldata);

	if((mode & 0x0f) == TRUE){
		PRINTF("\nLoad Y-Modem (Load Offset:%lx)\n", offset);
	}else{
		PRINTF("\nLoad X-Modem (Load Offset:%lx)\n", offset);
	}

	OAL_MSLEEP( 100/*ms*/ );

	size = 0;
	thread = create_thread_xymodem(mode, offset, bufsiz, bufnum, mutex);

	if( thread != NULL ){
		xym_que_info que_info;
		int	partsiz, queidx;
		first = 1;
		queidx = 0;

		do{
			OAL_RECEIVE_FROM_QUEUE( &(thread->queue), &que_info
						, (sizeof(xym_que_info)/sizeof(UINT32))
						, NULL, OAL_SUSPEND);
			len = que_info.len;

			if(len == 0){
				break;
			}

			while(len > 0 ){
				//DRV_SPRINTF((char *)&(que_info.buf[0x30]), "<i:%4d, s:%08X,l:%2d,b:%p>"
				//	, queidx , storeaddr
				//	, que_info.len, que_info.buf);

				if( thread->mutex != NULL ){
					OAL_OBTAIN_SEMAPHORE( thread->mutex, OAL_SUSPEND);
				}

				if( (storeaddr/secsize) != ((storeaddr+len)/secsize) ){
					partsiz = secsize - (storeaddr % secsize);
				}else{
					partsiz = len ;
				}
				if((mode & 0x40) == 0 ){
					if( first == 1 ){
						ioctldata[0] = storeaddr - (storeaddr % bufsiz);
						ioctldata[1] = secsize  ;
						NOR_IOCTL(device, NOR_SET_UNLOCK, ioctldata);

						ioctldata[0] = storeaddr - (storeaddr % bufsiz);
						ioctldata[1] = secsize  ;
						NOR_IOCTL(device, NOR_SET_ERASE, ioctldata);

						first = 0;
					}else
					if( (storeaddr % secsize) == 0 ){
						ioctldata[0] = storeaddr ;
						ioctldata[1] = secsize  ;
						NOR_IOCTL(device, NOR_SET_UNLOCK, ioctldata);

						ioctldata[0] = storeaddr ;
						ioctldata[1] = secsize  ;
						NOR_IOCTL(device, NOR_SET_ERASE, ioctldata);
					}
				}

				NOR_WRITE(device, storeaddr, (VOID *)(que_info.buf), partsiz);

				storeaddr = storeaddr + partsiz ;
				len = len - partsiz ;

				if( thread->mutex != NULL ){
					OAL_RELEASE_SEMAPHORE( thread->mutex );
				}

			}

			queidx++;

		}while(1);

		size = delete_thread_xymodem(thread);
	}

	//OAL_MSLEEP( 100/*ms*/ );
	PRINTF("## Total Size      = 0x%08x = %d Bytes\n", size, size);

	NOR_CLOSE(device);

	return size;

#else	//defined(BUILD_OPT_DA16200_FPGA) && defined(SUPPORT_NOR_DEVICE)

	return 0;

#endif	//defined(BUILD_OPT_DA16200_FPGA) && defined(SUPPORT_NOR_DEVICE)
}

/******************************************************************************
 *  xymodem_sflash_store
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32 da16x_xyzModem_sflash_boot_check(HANDLE handler, UINT8 *ram_addr);

UINT32 xymodem_sflash_store(UINT32 mode, UINT32 offset, UINT32 bufsiz, UINT32 bufnum, UINT32 secsize, UINT32 storeaddr, VOID *locker)
{
#ifdef	SUPPORT_SFLASH_DEVICE

	DA16X_FLASH_BOOT_TYPE *da16x_sflash;
	UINT32	ioctldata[4], first, mutex;
	int len, size ;
	XYMODEM_DOWN_INFO_TYPE *thread;

	if( bufnum <= (XYM_QUE_MIN) ){
		return 0;
	}

	da16x_sflash = (DA16X_FLASH_BOOT_TYPE *)flash_image_open((sizeof(UINT32)*8), BOOT_OFFSET_GET(offset), locker);

	if( da16x_sflash == NULL ){
		return 0;
	}

	ioctldata[0] = SFLASH_BUS_111;
	SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

	if( (mode & 0x80000000) != 0 ){
		mutex = TRUE;
	}else{
		ioctldata[0] = TRUE;
		SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_BOOTMODE, ioctldata);
		mutex = FALSE;
	}

	flash_image_force(da16x_sflash, 0x2);

	da16x_sflash_set_image( 0, 0, 0); // reset

	// Initialize
	if((mode & 0x0f) == TRUE){
		PRINTF("\nLoad Y-Modem (Load Offset:%lx)\n", offset);
	}else{
		PRINTF("\nLoad X-Modem (Load Offset:%lx)\n", offset);
	}

	OAL_MSLEEP( 100/*ms*/ );

	size = 0;
	thread = create_thread_xymodem(mode, offset, bufsiz, bufnum, mutex);

	if( thread != NULL ){
		xym_que_info que_info;
		int	partsiz, queidx;
		first = 1;
		queidx = 0;

		do{
			OAL_RECEIVE_FROM_QUEUE( &(thread->queue), &que_info
						, (sizeof(xym_que_info)/sizeof(UINT32))
						, NULL, OAL_SUSPEND);
			len = que_info.len;

			if(len == 0){
				break;
			}

			if( (first == 1) && ((mode & 0x80) == 0x00) ){
				// mode 8Xh means a binary format.
				// Update Pseudo SFDP
				if( (mode & 0x0100) == 0x00 ){
					// Old format !!
					IMG_LOAD_PRINT("\x18\x18\nWrong Image\n");
					len = 0; // stop !!
				}else{
					if( da16x_xyzModem_sflash_boot_check(da16x_sflash , que_info.buf) == FALSE ){
						IMG_LOAD_PRINT("\x18\x18\nWrong Image\n");
						len = 0; // stop !!
					}
				}
			}

			while(len > 0 ){
				//DRV_SPRINTF((char *)&(que_info.buf[0x30]), "<i:%4d, s:%08X,l:%2d,b:%p>"
				//	, queidx , storeaddr
				//	, que_info.len, que_info.buf);

				if( thread->mutex != NULL ){
					OAL_OBTAIN_SEMAPHORE( thread->mutex, OAL_SUSPEND);
				}

				if( (storeaddr/secsize) != ((storeaddr+len)/secsize) ){
					partsiz = secsize - (storeaddr % secsize);
				}else{
					partsiz = len ;
				}
				if((mode & 0x40) == 0 ){
					if( first == 1 ){
						ioctldata[0] = storeaddr - (storeaddr % bufsiz);
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_UNLOCK, ioctldata);

						ioctldata[0] = storeaddr - (storeaddr % bufsiz);
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_CMD_ERASE, ioctldata);

						first = 0;
					}else
					if( (storeaddr % secsize) == 0 ){
						ioctldata[0] = storeaddr ;
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_UNLOCK, ioctldata);

						ioctldata[0] = storeaddr ;
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_CMD_ERASE, ioctldata);
					}
				}


				SFLASH_WRITE(da16x_sflash->boot_flash, storeaddr, (que_info.buf), partsiz);

				storeaddr = storeaddr + partsiz ;
				len = len - partsiz ;

				if( thread->mutex != NULL ){
					OAL_RELEASE_SEMAPHORE( thread->mutex );
				}
			}

			queidx++;

		}while(1);

		size = delete_thread_xymodem(thread);
	}

	//OAL_MSLEEP( 100/*ms*/ );
	PRINTF("## Total Size      = 0x%08x = %d Bytes\n", size, size);

#ifdef	SUPPORT_LOADING_TIME
	da16x_sflash->timestamp[0] = 0;
#endif	//SUPPORT_LOADING_TIME

	ioctldata[0] = FALSE;
	SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_BOOTMODE, ioctldata);

	flash_image_force(da16x_sflash, 0x0);

	flash_image_close(da16x_sflash);

	return size;

#else	//SUPPORT_SFLASH_DEVICE

	return 0;

#endif	//SUPPORT_SFLASH_DEVICE
}


UINT32 rs485_sflash_store(UINT32 mode, UINT32 offset, UINT32 bufsiz, UINT32 bufnum, UINT32 secsize, UINT32 storeaddr, VOID *locker, UINT8 uid)
{
#ifdef	SUPPORT_SFLASH_DEVICE

	DA16X_FLASH_BOOT_TYPE *da16x_sflash;
	UINT32	ioctldata[4], first, mutex;
	int len, size ;
	RS485_DOWN_INFO_TYPE *thread;
	CHAR name[10];
	UNSIGNED available;
	UINT32	timestamp;
	HANDLE uart1;

	if( bufnum <= (XYM_QUE_MIN) ){
		return 0;
	}

	da16x_sflash = (DA16X_FLASH_BOOT_TYPE *)flash_image_open((sizeof(UINT32)*8), BOOT_OFFSET_GET(offset), locker);

	if( da16x_sflash == NULL ){
		return 0;
	}

	ioctldata[0] = SFLASH_BUS_111;
	SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

	if( (mode & 0x80000000) != 0 ){
		mutex = TRUE;
	}else{
		ioctldata[0] = TRUE;
		SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_BOOTMODE, ioctldata);
		mutex = FALSE;
	}


	flash_image_force(da16x_sflash, 0x2);
	da16x_sflash_set_image( 0, 0, 0); // reset

	// Initialize
	PRINTF("\nLoad RS485 (Load Offset:%lx)\n", offset);
	OAL_MSLEEP( 100/*ms*/ );

	uart1 = uart_initialize();
	RS485_Create(uid, uart1, &rs485_primitives);

	size = 0;
	thread = create_thread_rs485(mode, offset, bufsiz, bufnum, mutex);

	if( thread != NULL ){
		rs485_que_info que_info;
		int	partsiz, queidx;
		first = 1;
		queidx = 0;

		do{
			OAL_RECEIVE_FROM_QUEUE( &(thread->queue), &que_info
						, (sizeof(rs485_que_info)/sizeof(UINT32))
						, NULL, OAL_SUSPEND);
			len = que_info.len;

			if(len == 0){
				SYS_IMG_RS485_INFO_PRINT("Info: image download success.\n");
				break;
			}

			if (len < 0) {
				SYS_IMG_RS485_INFO_PRINT("Info: image download failed.\n");
				continue;
			}

			OAL_QUEUE_INFORMATION(&(thread->queue), name,
										  NULL, NULL,
										  &available, NULL,
										  NULL, NULL,
										  NULL, NULL,
										  NULL);

			if( (first == 1) && ((mode & 0x80) == 0x00) ){
				// mode 8Xh means a binary format.
				// Update Pseudo SFDP
				if( (mode & 0x0100) == 0x00 ){
					// Old format !!
					IMG_LOAD_PRINT("\x18\x18\nWrong Image - 1\n");
					len = 0; // stop !!
				}else{
					if( da16x_xyzModem_sflash_boot_check(da16x_sflash , que_info.buf) == FALSE ){
						IMG_LOAD_PRINT("\x18\x18\nWrong Image - 2\n");
						len = 0; // stop !!
					}
				}
			}

			while(len > 0 ){
				if( thread->mutex != NULL ){
					OAL_OBTAIN_SEMAPHORE( thread->mutex, OAL_SUSPEND);
				}



				if( (storeaddr/secsize) != ((storeaddr+len)/secsize) ){
					partsiz = secsize - (storeaddr % secsize);
				}else{
					partsiz = len ;
				}
				if((mode & 0x40) == 0 ){
					if( first == 1 ){
						ioctldata[0] = storeaddr - (storeaddr % bufsiz);
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_UNLOCK, ioctldata);
						ioctldata[0] = storeaddr - (storeaddr % bufsiz);
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_CMD_ERASE, ioctldata);
						first = 0;
					}else
					if( (storeaddr % secsize) == 0 ){
						ioctldata[0] = storeaddr ;
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_UNLOCK, ioctldata);
						ioctldata[0] = storeaddr ;
						ioctldata[1] = secsize  ;
						SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_CMD_ERASE, ioctldata);
					}

				}
				SFLASH_WRITE(da16x_sflash->boot_flash, storeaddr, (que_info.buf), partsiz);
				storeaddr = storeaddr + partsiz ;
				len = len - partsiz ;

				if( thread->mutex != NULL ){
					OAL_RELEASE_SEMAPHORE( thread->mutex );
				}
			}
			len = -1;

			queidx++;

		}while(1);

		size = delete_thread_rs485(thread);
	}

	//OAL_MSLEEP( 100/*ms*/ );
	PRINTF("## Total Size      = 0x%08x = %d Bytes\n", size, size);
#ifdef	SUPPORT_LOADING_TIME
	da16x_sflash->timestamp[0] = 0;
#endif	//SUPPORT_LOADING_TIME

	RS485_Destroy();

	ioctldata[0] = FALSE;
	SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_BOOTMODE, ioctldata);

	flash_image_force(da16x_sflash, 0x0);

	flash_image_close(da16x_sflash);

	return size;

#else	//SUPPORT_SFLASH_DEVICE

	return 0;

#endif	//SUPPORT_SFLASH_DEVICE
}


/******************************************************************************
 *  da16x_xyzModem_sflash_boot_check
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32 da16x_xyzModem_sflash_boot_check(HANDLE handler, UINT8 *ram_addr)
{
#ifdef	SUPPORT_SFLASH_DEVICE
	DA16X_FLASH_BOOT_TYPE *da16x_sflash;
	DA16X_IMGHEADER_TYPE *imghdr;
	UINT32	*ioctldata;

	UINT32 image_crctemp;
	UINT32 ih_hcrc;
	UINT32 *readregion;
	PSEUDO_SFDP_TABLE_TYPE *pseudosfdp;

	if( handler == NULL ){
		return 0;
	}

	da16x_sflash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = (UINT32 *)&(da16x_sflash->ioctldata[0]);

	// Read First Block :: Image Header
	readregion = (UINT32 *) ram_addr;
	imghdr = (DA16X_IMGHEADER_TYPE *) ram_addr;

	ih_hcrc = imghdr->hcrc;
	imghdr->hcrc = 0;

	image_crctemp = da16x_hwcrc32( sizeof(UINT32)
			, ram_addr, sizeof(DA16X_IMGHEADER_TYPE), (~0) );

	// Check
	if((imghdr->magic) != (DA16X_IH_MAGIC)){
		return FALSE;
	}

	imghdr->hcrc = ih_hcrc;
	if( (imghdr->hcrc) != image_crctemp ){
		return FALSE;
	}

	// MPW Issue :: mismatch jedec id !!
	if( imghdr->sfdpcrc != 0 ){
		readregion = (UINT32 *)&(ram_addr[sizeof(DA16X_IMGHEADER_TYPE)]) ;
		pseudosfdp = (PSEUDO_SFDP_TABLE_TYPE *) &(readregion[0]);

		if( (pseudosfdp->magic) != SFDP_TABLE_MAGIC ){
			return FALSE;
		}

		ioctldata[0] = 0;
		SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_GET_INFO, ioctldata);

		if( pseudosfdp->devid != ioctldata[1] ){
			return FALSE;
		}

		// Setup SFDP with RETMEM
		if( pseudosfdp->offset != 0 ){
			PSEUDO_SFDP_TABLE_TYPE *retmem_sfdp;
			// append
			DRV_MEMCPY((da16x_sflash->retmem_sfdp)
				, (VOID *)(ioctldata[2]), ioctldata[3]);

			retmem_sfdp = (PSEUDO_SFDP_TABLE_TYPE *)da16x_sflash->retmem_sfdp;

			DRV_MEMCPY(&(retmem_sfdp->chunk.sfdptab.sfdp[pseudosfdp->offset])
				, (VOID *)(pseudosfdp->chunk.bulk[0])
				, ioctldata[pseudosfdp->length]);

		}else{
			// overwrite
			DRV_MEMCPY((da16x_sflash->retmem_sfdp)
				, pseudosfdp, sizeof(PSEUDO_SFDP_TABLE_TYPE));
		}
	}

	if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
		SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_SET_INFO, ioctldata);
	}

	// Check Bus mode
	ioctldata[0] = 0;
	SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_GET_INFO, ioctldata);

	//if( ioctldata[3] == SFDP_TABLE_SIZE ){
	//	ioctldata[0] = SFLASH_BUS_444;
	//	SFLASH_IOCTL(da16x_sflash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);
	//}

	return TRUE;
#else	//SUPPORT_SFLASH_DEVICE
	return 0;
#endif	//SUPPORT_SFLASH_DEVICE
}

#endif	/*SUPPORT_XYMODEM*/

/******************************************************************************
 *  da16x_sflash_reset_parameters
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_sflash_reset_parameters(void)
{
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_SFLG] = 0 ;
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICRC] = 0 ;
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ISIZ] = 0 ;
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL] = 0 ;
	((UINT32 *)RETMEM_SFLASH_BASE)[0] = 0 ;
}

/******************************************************************************
 *  da16x_sflash_get_parasize
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_sflash_get_parasize(void)
{
	return sizeof(PSEUDO_SFDP_TABLE_TYPE);
}

/******************************************************************************
 *  da16x_sflash_get_maxspeed
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32  da16x_sflash_get_maxspeed(void)
{
	UINT32 maxspeed;
	maxspeed = ((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL];
	maxspeed = (maxspeed >> 24) & 0x0ff;
	maxspeed = (maxspeed == 0) ? (40 * MHz) : (maxspeed * MHz) ; // safe
	return maxspeed;
}

/******************************************************************************
 *  da16x_sflash_set_maxspeed
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_sflash_set_maxspeed(UINT32 spiclock)
{
	spiclock = spiclock / MHz ;

	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL]
		= (((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL] & (0x00FFFFFF))
		  | ( spiclock << 24 ) ;
}

/******************************************************************************
 *  da16x_sflash_get_bussel
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32  da16x_sflash_get_bussel(void)
{
	UINT32 bussel;
	bussel = ((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL];
	bussel = (bussel >> 16) & 0x0ff;
	return bussel;
}

/******************************************************************************
 *  da16x_sflash_set_bussel
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_sflash_set_bussel(UINT32 bussel)
{
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL]
		= (((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL] & (0xFF00FFFF))
		  | ( (bussel & 0x0FF) << 16 ) ;
}

/******************************************************************************
 *  da16x_sflash_setup_parameter
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_sflash_setup_parameter(UINT32 *parameters)
{
	PSEUDO_SFDP_TABLE_TYPE *sfdp_table;

	sfdp_table = (PSEUDO_SFDP_TABLE_TYPE *)&(((UINT32 *)RETMEM_SFLASH_BASE)[0]);
	if( sfdp_table->magic == SFDP_TABLE_MAGIC ){
		if( parameters != NULL ){
			parameters[0] = sfdp_table->devid;
			parameters[1] = 64;
			parameters[2] = (UINT32)&(sfdp_table->chunk.sfdptab.sfdp[0]);
			parameters[3] = (UINT32)NULL;
			parameters[4] = (UINT32)&(sfdp_table->chunk.sfdptab.extra[0]);

			if( sfdp_table->chunk.sfdptab.cmdlst[0] == 0x00 ){
				parameters[5] = (UINT32)NULL;
			}else{
				parameters[5] = (UINT32)&(sfdp_table->chunk.sfdptab.cmdlst[0]);
			}

			// MPW3 CellDelay
			if( sfdp_table->chunk.sfdptab.delay[0] == 0x00 ){
				parameters[6] = (UINT32)NULL;
			}else{
				parameters[6] = (UINT32)&(sfdp_table->chunk.sfdptab.delay[0]);
			}
		}
		return TRUE;
	}
	return FALSE;
}


/******************************************************************************
 *  da16x_sflash_set_image
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_sflash_set_image(UINT32 ipoly, UINT32 ioffset, UINT32 isize)
{
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICRC] = ipoly ;
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ISIZ] = isize ;
	((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL]
		= (((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL] & (UINT32)(~0x0FFFF))
		  | ( ioffset & 0x0FFFF ) ;
}

/******************************************************************************
 *  da16x_sflash_get_image
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_sflash_get_image(UINT32 *ipoly, UINT32 *ioffset, UINT32 *isize)
{
	*ipoly   = ((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICRC];
	*isize   = ((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ISIZ];
	*ioffset = (((UINT32 *)RETMEM_BOOTMODE_BASE)[SFDP_RETMEM_ICTL]) & 0x0FFFF ;
	return ((*ioffset == 0)? FALSE : TRUE);
}


/* EOF */
