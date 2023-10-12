/**
 ****************************************************************************************
 *
 * @file da16x_oops.c
 *
 * @brief OOPS management
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

#include "clib.h"
#include "errno.h"

#include "hal.h"
#include "library/library.h"
#include "driver.h"


/******************************************************************************
 *
 *	MACRO
 *
 ******************************************************************************/

#undef	SUPPORT_OOPS_POWERDOWN

#define	OOPS_GET_LEN(x)	((x)&OOPS_LEN_MASK)

#ifdef	BUILD_OPT_DA16200_FPGA

static void da16x_oops_nor_erase(UINT32 addr, UINT32 length);
static void da16x_oops_nor_save(UINT32 mode
		, UINT32 startaddr, UINT32 sector, UINT32 limitaddr
		, VOID *data, UINT32 length);
static void da16x_oops_nor_view(UINT32 startaddr, UINT32 limitaddr);


#endif	//BUILD_OPT_DA16200_FPGA

#ifdef  SUPPORT_SFLASH_DEVICE
static HANDLE da16x_oops_sflash_create(void);
static void da16x_oops_sflash_close(HANDLE sflash);

static void da16x_oops_sflash_erase(UINT32 addr, UINT32 length);
static void da16x_oops_sflash_save(UINT32 mode
			, UINT32 startaddr, UINT32 sector, UINT32 limitaddr
			, VOID *data, UINT32 length);
static void da16x_oops_sflash_view(UINT32 startaddr, UINT32 limitaddr);
#endif  //SUPPORT_SFLASH_DEVICE

static void dispaly_oops_dump(OOPS_TAG_TYPE *oopstag);

/******************************************************************************
 *  da16x_oops_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_oops_init(VOID *oopsctrl, OOPS_CONTROL_TYPE *info)
{
	OOPS_CONTROL_TYPE	*oops;

	oops = (OOPS_CONTROL_TYPE *)oopsctrl;
	DRV_MEMCPY( oops, info, sizeof(OOPS_CONTROL_TYPE) );
}

/******************************************************************************
 *  da16x_oops_clear ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_oops_clear(VOID *oopsctrl)
{
	OOPS_CONTROL_TYPE	*oops;

	oops = (OOPS_CONTROL_TYPE *)oopsctrl;

	if( oops->active == 1 && oops->save == 1 ){
		UINT32	address;
		UINT32	roomspace;

		address = (UINT32)((1<<(oops->secsize)) * (oops->offset));
		roomspace = (UINT32)((1<<(oops->secsize)) * (1<<(oops->maxnum)));

#ifdef	BUILD_OPT_DA16200_FPGA
		if( oops->memtype == 1 ){
			da16x_oops_nor_erase(address, roomspace);
		}else
#endif	//BUILD_OPT_DA16200_FPGA
		{
#ifdef	SUPPORT_SFLASH_DEVICE
			da16x_oops_sflash_erase(address, roomspace);
#endif	//SUPPORT_SFLASH_DEVICE
		}
	}

}

/******************************************************************************
 *  da16x_oops_save ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_oops_save(VOID *oopsctrl, OOPS_TAG_TYPE *oopstag, UINT16 mark)
{
	OOPS_CONTROL_TYPE	*oops;

	oops = (OOPS_CONTROL_TYPE *)oopsctrl;

	if( oops->active == 1 ){
		UINT32	address;
		UINT32	roomspace;
		unsigned long long rtcstamp;

		address = (UINT32)((1<<(oops->secsize)) * (oops->offset));
		roomspace = (UINT32)((1<<(oops->secsize)) * (1<<(oops->maxnum)));

		oopstag->mark = mark ;

		if( ((mark & 0x0FF0) == 0x00E0) ){
			oopstag->length = 0;
		}

		// PRINTF("OOPS : %x, %x\n", address, roomspace );
		// PRINTF("OOPS : %x\n", (1<<(oops->secsize)) );
		rtcstamp = RTC_GET_COUNTER( );
		DRV_MEMCPY( &(oopstag->rtc[0]), &rtcstamp, sizeof(unsigned long long) );

		if( oops->save == 1 && (oopstag->tag == OOPS_MARK) ){
#ifdef	BUILD_OPT_DA16200_FPGA
			if( oops->memtype == 1 ){
				da16x_oops_nor_save(
					((oops->stop == 1 )? TRUE : FALSE)
					, address
					, (1<<(oops->secsize))
					, (address+roomspace)
					, (UINT32 *)oopstag
					, (OOPS_GET_LEN(oopstag->length) + sizeof(OOPS_TAG_TYPE))
					);
			}else
#endif	//BUILD_OPT_DA16200_FPGA
			{
#ifdef	SUPPORT_SFLASH_DEVICE
				da16x_oops_sflash_save(
					((oops->stop == 1 )? TRUE : FALSE)
					, address
					, (UINT32)(1<<(oops->secsize))
					, (address+roomspace)
					, (UINT32 *)oopstag
					, (OOPS_GET_LEN(oopstag->length) + sizeof(OOPS_TAG_TYPE))
					);
#endif	//SUPPORT_SFLASH_DEVICE
			}
		}

		if( oops->dump == 1 ){
			dispaly_oops_dump( oopstag );
		}

		return ((oops->boot == 1)? TRUE : FALSE) ;
	}

	return FALSE;
}

/******************************************************************************
 *  da16x_oops_view ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_oops_view(VOID *oopsctrl)
{
	OOPS_CONTROL_TYPE	*oops;

	oops = (OOPS_CONTROL_TYPE *)oopsctrl;

	if( oops->active == 1 ){
		UINT32	address;
		UINT32	roomspace;

		address = (UINT32)((1<<(oops->secsize)) * (oops->offset));
		roomspace = (UINT32)((1<<(oops->secsize)) * (1<<(oops->maxnum)));


		if( oops->save == 1 ){
#ifdef	BUILD_OPT_DA16200_FPGA
			if( oops->memtype == 1 ){
				da16x_oops_nor_view(
					address
					, (address+roomspace)
					);
			}else
#endif	//BUILD_OPT_DA16200_FPGA
			{
#ifdef	SUPPORT_SFLASH_DEVICE
				da16x_oops_sflash_view(
					address
					, (address+roomspace)
					);
#endif //SUPPORT_SFLASH_DEVICE
			}
		}
	}
}

/******************************************************************************
 *  dispaly_oops_dump ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void	dispaly_oops_dump(OOPS_TAG_TYPE *oopstag)
{
	UINT32	*oopscontext ;

	const char *exception_title[]={ "Hard", "MPU", "BUS", "Usage", "Stack", "Wdog" };
	const char *reg_msg_format = "\t%s :%08x%s";
	const char *regdelimiter[]= {
		"R0 ", "R1 ",
		"R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ",
		"R8 ", "R9 ", "R10", "R11", "R12", "SP ",
		"LR ", "PC ", "PSR", "EXC"
	};
	const char *faultdelimiter[]= {
		"SHCSR", "CFSR ", "HFSR ", "DFSR ", "MMFAR", "BFAR ", "AFSR "
	};
	UINT32	i, dumpaddr;

	oopscontext = (UINT32 *) ( ((UINT32)oopstag) + sizeof(OOPS_TAG_TYPE) );


	if( oopstag->tag != OOPS_MARK ){
		return ;
	}

	DRV_PRINTF("[[OOPS Dump : %04x]]\r\n", oopstag->mark);
	if( (oopstag->mark & 0x0FF0) == 0x00E0 ){
		i = 5;
	}else{
		i = ((oopstag->length>>OOPS_MOD_SHFT)&0x00F);
	}
	DRV_PRINTF("[[%s Fault]]\n", exception_title[i]);
	DRV_PRINTF("--RTC Time : %08x.%08x\r\n", oopstag->rtc[1], oopstag->rtc[0] );

	if( OOPS_GET_LEN(oopstag->length) == 0 ){
		// WatchDog Reset
		return;
	}

	DRV_PRINTF("\r\nRegister-Dump\r\n");
	for(i = 0; i < 18; i++){
		DRV_PRINTF(reg_msg_format
				, regdelimiter[i], oopscontext[i]
				, ((i%4==3)? "\r\n": ","));
	}

	DRV_PRINTF("\r\nFault Status\r\n");
	for(i=0; i < 7; i++){
		DRV_PRINTF(reg_msg_format
				, faultdelimiter[i], oopscontext[OOPS_STACK_PICS+48+i]
				, ((i%3==2)? "\r\n": ","));
	}

	// reserved area : OOPS_STACK_SICS
	DRV_PRINTF("\r\nDA16X SysInfo\n");
	for( i= 7; i < OOPS_STACK_SICS; i++ ){
		DRV_PRINTF("\tSICS[%d] = %08x\n"
				, (i-7), oopscontext[OOPS_STACK_PICS+48+i]);
	}

	DRV_PRINTF("\r\nStack");
	DRV_PRINTF("\r\nStack-Dump (%d)", oopscontext[18]);
	dumpaddr = ((UINT32)&(oopscontext[OOPS_STACK_PICS]));

	for(i=0; i<oopscontext[18]; i++) {
		if( dumpaddr <= DA16X_SRAM_END ){
			if((i % 8) == 0){
				DRV_PRINTF("\r\n[0x%08x] : "
					, (oopscontext[13] + (i*sizeof(UINT32)))
					);
			}
			DRV_PRINTF("%08lX ", *((UINT32 *)dumpaddr) );

			dumpaddr = dumpaddr + sizeof(UINT32);
		}else{
			break;
		}
	}
	DRV_PRINTF("\r\n");

	if( (OOPS_GET_LEN(oopstag->length) - sizeof(OOPS_TAG_TYPE)) > (sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS)) ){
		UINT32 max ;
		max = OOPS_GET_LEN(oopstag->length) >> 2;

		//////////////////////////////////////////
		//thread : sizeof(UINT32)*(OOPS_STACK_PICS+48+7+11+40)
		//////////////////////////////////////////
		if( max == (OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+32) ){
			DRV_PRINTF("\nThread: %s\r\n", (char *)(&(oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0])) );
			DRV_PRINTF("\t stack ptr : %p\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] );
			DRV_PRINTF("\t stack base: %p\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+6] );
			DRV_PRINTF("\t stack end : %p\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] );
			DRV_PRINTF("\t stack high: %p\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+8] );
			DRV_PRINTF("\t max usage : %p\r\n",
					(oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] - oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] + 1)
				);
			DRV_PRINTF("\t suspend   : %08x\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+9] );

			DRV_PRINTF("\r\nThread Stack (%d)", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10]);

			dumpaddr = ((UINT32)&(oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11]));

			for(i=0; i<oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10]; i++) {
				if( dumpaddr <= DA16X_SRAM_END ){
					if((i % 8) == 0){
						DRV_PRINTF("\r\n[0x%08x] : "
							, (oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] + (i*sizeof(UINT32)))
							);
					}
					DRV_PRINTF("%08lX ", *((UINT32 *)dumpaddr) );

					dumpaddr = dumpaddr + sizeof(UINT32);
				}else{
					break;
				}
			}
		}
		else if( max == (OOPS_STACK_PICS+48+OOPS_STACK_SICS+2) ){
			DRV_PRINTF("\nISR? : %08x\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+1] );
			DRV_PRINTF("\t nested count: %d\r\n", oopscontext[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0] );
		}
	}

	DRV_PRINTF("\r\n");
}

/******************************************************************************
 *  fc9000_nor_erase & save ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	BUILD_OPT_DA16200_FPGA

static void da16x_oops_nor_erase(UINT32 addr, UINT32 length)
{
	HANDLE	nor;
	UINT32	ioctldata[2];

	nor = NOR_CREATE(NOR_UNIT_0);

	if(nor != NULL){

		NOR_INIT(nor);

		//ioctldata[0] = addr;
		//ioctldata[1] = length;
		//NOR_IOCTL(nor, NOR_SET_UNLOCK, ioctldata);

		ioctldata[0] = addr;
		ioctldata[1] = length;
		NOR_IOCTL(nor, NOR_SET_ERASE, ioctldata);

		//ioctldata[0] = addr;
		//ioctldata[1] = length;
		//NOR_IOCTL(nor, NOR_SET_LOCK, ioctldata);

		NOR_CLOSE(nor);
	}
}

static void da16x_oops_nor_save(UINT32 stop
		, UINT32 startaddr, UINT32 sector, UINT32 limitaddr
		, VOID *data, UINT32 length)
{
	HANDLE	nor;
	UINT32	ioctldata[2], addr;
	OOPS_TAG_TYPE oopstag;

	nor = NOR_CREATE(NOR_UNIT_0);

	if(nor == NULL){
		return;
	}

	NOR_INIT(nor);

	addr = startaddr;

	do{
		NOR_READ(nor, addr, &oopstag, sizeof(OOPS_TAG_TYPE));

		if( oopstag.tag == OOPS_MARK ){
			addr = addr + OOPS_GET_LEN(oopstag.length) + sizeof(OOPS_TAG_TYPE);
		}else{
			break;
		}
	}while(1);

	if( limitaddr < (addr+length) ){
		if( stop == TRUE ){ // Stop
			NOR_CLOSE(nor);
			return ;
		}
		addr =  startaddr;
	}

	//ioctldata[0] = (addr & ~(sector-1));
	//ioctldata[1] = length + (addr & (sector-1));
	//NOR_IOCTL(nor, NOR_SET_UNLOCK, ioctldata);

	if( oopstag.tag != 0xFFFFFFFF ){
		ioctldata[0] = (addr & ~(sector-1));
		ioctldata[1] = length + (addr & (sector-1));
		NOR_IOCTL(nor, NOR_SET_ERASE, ioctldata);
	}

	NOR_WRITE(nor, addr, data, length);

	//ioctldata[0] = (addr & ~(sector-1));
	//ioctldata[1] = length + (addr & (sector-1));
	//NOR_IOCTL(nor, NOR_SET_LOCK, ioctldata);

	NOR_CLOSE(nor);
}

static void da16x_oops_nor_view(UINT32 startaddr, UINT32 limitaddr)
{
	HANDLE	nor;
	UINT32	addr;
	OOPS_TAG_TYPE oopstag;

	nor = NOR_CREATE(NOR_UNIT_0);

	if(nor == NULL){
		return;
	}

	NOR_INIT(nor);

	addr = startaddr;

	do{
		NOR_READ(nor, addr, &oopstag, sizeof(OOPS_TAG_TYPE));

		if( oopstag.tag == OOPS_MARK ){
			UINT32	*dump;

			dump = (UINT32 *)HAL_MALLOC((OOPS_GET_LEN(oopstag.length) + sizeof(OOPS_TAG_TYPE)));
			NOR_READ(nor, addr, dump, (OOPS_GET_LEN(oopstag.length) + sizeof(OOPS_TAG_TYPE)) );
			dispaly_oops_dump((OOPS_TAG_TYPE *)dump);
			HAL_FREE(dump);

			addr = addr + OOPS_GET_LEN(oopstag.length) + sizeof(OOPS_TAG_TYPE);
		}else{
			break;
		}

		if( limitaddr < (addr) ){
			break;
		}

	}while(1);

	NOR_CLOSE(nor);
}


#endif	//BUILD_OPT_DA16200_FPGA

#ifdef	SUPPORT_SFLASH_DEVICE
/******************************************************************************
 *  fc9000_sflash_erase & save ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static HANDLE da16x_oops_sflash_create(void)
{
	HANDLE  sflash;
	UINT32	ioctldata[7];

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);

	if( sflash != NULL ){
		// setup speed
		ioctldata[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(sflash
				, SFLASH_BUS_MAXSPEED, ioctldata);

		// setup bussel
		ioctldata[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(sflash
				, SFLASH_SET_BUSSEL, ioctldata);

		// setup parameters
		if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
			SFLASH_IOCTL(sflash
				, SFLASH_SET_INFO, ioctldata);

			if( da16x_boot_get_offset(BOOT_IDX_PWRCFG) != 0xFFFFFFFF ){
				OTP_TAG_TYPE otp_flag;

				ioctldata[0] = da16x_boot_get_offset(BOOT_IDX_PWRCFG);
				//otp_flag = *((OTP_TAG_TYPE *)&(ioctldata[0]));
				DRV_MEMCPY(((OTP_TAG_TYPE *)&(ioctldata[0])), &otp_flag, sizeof(OTP_TAG_TYPE));

				if( otp_flag.tag == 1 ){
					if( otp_flag.sflash == 1 ){ /* External */
						if( otp_flag.dio == 1 ){
							da16x_io_volt33(TRUE);
						}else{
							da16x_io_volt33(FALSE);
						}
						da16x_io_pinmux(PIN_GMUX, GMUX_QSPI);
					}
				}
			}

			if( SFLASH_INIT(sflash) == TRUE ){
				ioctldata[0] = SFLASH_BUS_111;
				SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, ioctldata);
			}else{
				SFLASH_CLOSE(sflash);
				sflash = NULL;
			}
		}
		else
		{
			SFLASH_CLOSE(sflash);
			sflash = NULL;
		}

	}

	return sflash;
}

static void da16x_oops_sflash_close(HANDLE sflash)
{
	if( sflash != NULL ){
#ifdef	SUPPORT_OOPS_POWERDOWN
		UINT32	ioctldata[1];

		ioctldata[0] = 0;
		SFLASH_IOCTL(sflash, SFLASH_CMD_POWERDOWN, ioctldata);
#endif	//SUPPORT_OOPS_POWERDOWN

		SFLASH_CLOSE(sflash);
	}
}

static void da16x_oops_sflash_erase(UINT32 addr, UINT32 length)
{
	HANDLE  sflash;
	UINT32	ioctldata[2];

	sflash = da16x_oops_sflash_create();

	if( sflash != NULL ){
		ioctldata[0] = addr ;
		ioctldata[1] = length  ;
		SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);

		ioctldata[0] = addr ;
		ioctldata[1] = length  ;
		SFLASH_IOCTL(sflash, SFLASH_CMD_ERASE, ioctldata);

		ioctldata[0] = addr ;
		ioctldata[1] = length  ;
		SFLASH_IOCTL(sflash, SFLASH_SET_LOCK, ioctldata);

		da16x_oops_sflash_close(sflash);
	}
}

static void da16x_oops_sflash_save(UINT32 stop
		, UINT32 startaddr, UINT32 sector, UINT32 limitaddr
		, VOID *data, UINT32 length)
{
	HANDLE  	sflash;
	OOPS_TAG_TYPE   oopstag[1];
	UINT32		ioctldata[6], addr;

	sflash = da16x_oops_sflash_create();

	if( sflash == NULL ){
		return;
	}

	addr = startaddr;

	do{
		SFLASH_READ(sflash, addr, oopstag, sizeof(OOPS_TAG_TYPE));

		if( oopstag->tag == OOPS_MARK ){
			addr = addr + OOPS_GET_LEN(oopstag->length) + sizeof(OOPS_TAG_TYPE);
		}else{
			break;
		}
	}while(1);

	if( limitaddr < (addr+length) ){
		if( stop == TRUE ){ // Stop
			da16x_oops_sflash_close(sflash);
			return ;
		}
		addr =  startaddr;
	}

	ioctldata[0] = (addr & ~(sector-1));
	ioctldata[1] = length + (addr & (sector-1));
	SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);

	if( oopstag->tag != 0xFFFFFFFF ){
		ioctldata[0] = (addr & ~(sector-1));
		ioctldata[1] = length + (addr & (sector-1));
		SFLASH_IOCTL(sflash, SFLASH_CMD_ERASE, ioctldata);
	}

	SFLASH_WRITE(sflash, addr, data, length);

	ioctldata[0] = (addr & ~(sector-1));
	ioctldata[1] = length + (addr & (sector-1));
	SFLASH_IOCTL(sflash, SFLASH_SET_LOCK, ioctldata);

	da16x_oops_sflash_close(sflash);
}

static void da16x_oops_sflash_view(UINT32 startaddr, UINT32 limitaddr)
{
	HANDLE  	sflash;
	OOPS_TAG_TYPE   oopstag[1];
	UINT32		addr;

	sflash = da16x_oops_sflash_create();

	if( sflash == NULL ){
		return;
	}

	addr = startaddr;

	do{
		SFLASH_READ(sflash, addr, oopstag, sizeof(OOPS_TAG_TYPE));

		if( oopstag->tag == OOPS_MARK ){
			UINT32	*dump;

			dump = (UINT32 *)HAL_MALLOC((OOPS_GET_LEN(oopstag->length) + sizeof(OOPS_TAG_TYPE)));
			SFLASH_READ(sflash, addr, dump, (OOPS_GET_LEN(oopstag->length) + sizeof(OOPS_TAG_TYPE)) );
			dispaly_oops_dump((OOPS_TAG_TYPE *)dump);
			HAL_FREE(dump);

			addr = addr + OOPS_GET_LEN(oopstag->length) + sizeof(OOPS_TAG_TYPE);
		}else{
			break;
		}

		if( limitaddr < (addr) ){
			break;
		}
	}while(1);

	da16x_oops_sflash_close(sflash);
}

#endif	/*SUPPORT_SFLASH_DEVICE	*/

/* EOF */
