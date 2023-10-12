/**
 ****************************************************************************************
 *
 * @file command_mem.c
 *
 * @brief Memory console commands
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

#include "sdk_type.h"


#include "stdio.h"
#include "string.h"
#include "clib.h"

#include "library.h"
#include "da16x_system.h"

#include "command.h"
#include "environ.h"
#include "command_mem.h"

#define	SUPPORT_SFLASH_COPY

//-----------------------------------------------------------------------
// Command MEM-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_mem_list[] = {
  { "MEM",		CMD_MY_NODE,	cmd_mem_list,	NULL,	"MEMORY & BUS Verification "					},		// Head

  { "-------",	CMD_FUNC_NODE,	NULL,	NULL,			"--------------------------------"				},

  { "brd",		CMD_FUNC_NODE,	NULL,	&cmd_mem_read,		"byte read  [addr] [length]"				},
  { "bwr",		CMD_FUNC_NODE,	NULL,	&cmd_mem_write,		"byte write [addr] [data] [length]"			},
  { "wrd",		CMD_FUNC_NODE,	NULL,	&cmd_mem_wread,		"word read  [addr] [length]"				},
  { "wwr",		CMD_FUNC_NODE,	NULL,	&cmd_mem_wwrite,	"word write [addr] [data] [length]"			},
  { "lrd",		CMD_FUNC_NODE,	NULL,	&cmd_mem_lread,		"long read  [addr] [length]"				},
  { "lwr",		CMD_FUNC_NODE,	NULL,	&cmd_mem_lwrite,	"long write [addr] [data] [length]"			},

  { "memcpy",	CMD_FUNC_NODE,	NULL,	&cmd_mem_memcpy,	"memcpy  [dst addr] [src addr] [length]"	},
  { "memset",	CMD_FUNC_NODE,	NULL,	&cmd_mem_memcpy,	"memset  [dst addr] [value] [length]"		},
  { "memtest",	CMD_FUNC_NODE,	NULL,	&cmd_mem_memcpy,	"memtest [dst addr] [value] [length]"		},

#ifdef	SUPPORT_NOR_DEVICE
  { "nor",		CMD_FUNC_NODE,	NULL,	&cmd_nor_func,		"nor command  [op] [start addr] [length]"	},
#endif	//SUPPORT_NOR_DEVICE
#ifdef	SUPPORT_SFLASH_DEVICE
  { "sflash",	CMD_FUNC_NODE,	NULL,	&cmd_sflash_func,	"sflash command  [op] [start] [length]"		},
#endif	//SUPPORT_SFLASH_DEVICE
#ifdef	SUPPORT_SFLASH_COPY
  { "ffcopy",	CMD_FUNC_NODE,	NULL,	&cmd_sfcopy_func,	"fast sflash image copy [start] [dst] [length]"},
#endif	//SUPPORT_SFLASH_COPY

  { NULL, 	CMD_NULL_NODE,	NULL,	NULL,			NULL }						// Tail
};


//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------

#define MEM_BYTE_READ(addr, data)		*data = *((volatile UCHAR *)(addr))
#define MEM_WORD_READ(addr, data)		*data = *((volatile USHORT *)(addr))
#define MEM_LONG_READ(addr, data)		*data = *((volatile UINT *)(addr))
#define MEM_BYTE_WRITE(addr, data)		*((volatile UCHAR *)(addr)) = data
#define MEM_WORD_WRITE(addr, data)		*((volatile USHORT *)(addr)) = data
#define MEM_LONG_WRITE(addr, data)		*((volatile UINT *)(addr)) = data

#define	putascii(x) 	(x<32) ? ('.') : ( (x>126) ? ('.') : x )

//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------

/******************************************************************************
 *  cmd_mem_read ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_mem_read(int argc, char *argv[])
{
	UINT32  addr;
	UINT32  length;
	UINT8   data[16];
	UINT32  i, t;

	if(argc == 2) {
		addr = htoi(argv[1]);
		MEM_BYTE_READ(addr, &(data[0]));

		PRINTF("[0x%08lX] : 0x%02X\n", addr, data[0]);
	} else if(argc == 3) {
		addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		for(i=0; i<length; i++)
		{
			t = (i % 16);
			MEM_BYTE_READ((addr+i), &(data[t]));

			if( t == 15 || (i+1) == length ){
				cmd_dump_print((addr+i-t), data
					, ((t+1)*sizeof(UINT8)), sizeof(UINT8) );
			}
		}
		PRINTF("\n");
	}
}

/******************************************************************************
 *  cmd_mem_write ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_mem_write(int argc, char *argv[])
{
	UINT32  dest_addr;
	UINT8   dest_data;
	UINT32  length;
	UINT32  i;


	if(argc == 3) {
		dest_addr = htoi(argv[1]);
		dest_data = (UINT8)htoi(argv[2]);

		MEM_BYTE_WRITE(dest_addr, dest_data);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]);
		dest_data = (UINT8)htoi(argv[2]);
		length    = htoi(argv[3]);

		for(i=0; i<length; i++){
			MEM_BYTE_WRITE(dest_addr+i, dest_data);
		}
	}
}

/******************************************************************************
 *  cmd_mem_wread ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_mem_wread(int argc, char *argv[])
{
	UINT32 length;
	UINT32 dest_addr, target_addr;
	UINT16 data[16];
	UINT32 i, t;

	if(argc == 2) {
		dest_addr = htoi(argv[1]);
		MEM_WORD_READ(dest_addr, &(data[0]));
		PRINTF("[0x%08lX] : 0x%04X\n", dest_addr, data[0]);
	} else if(argc == 3) {
		dest_addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		length = length >> 1;
		for(i=0; i<length; i++)
		{
			t = i % 16;
			target_addr = dest_addr + (i<<1);
			MEM_WORD_READ(target_addr, &(data[t]));

			if( t == 15 || (i+1) == length ){
				cmd_dump_print((dest_addr+(i<<1)-(t<<1)), (UCHAR *)data
					, ((t+1)*sizeof(UINT16)), sizeof(UINT16) );
			}
		}
		PRINTF("\n");
	}

}

/******************************************************************************
 *  cmd_mem_wwrite ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_mem_wwrite(int argc, char *argv[])
{
	UINT32  dest_addr;
	UINT16  dest_data;
	UINT32  length;
	UINT32  i;

	if(argc == 3) {
		dest_addr = htoi(argv[1]) & 0xfffffffe;
		dest_data = (UINT16)htoi(argv[2]);

		MEM_WORD_WRITE(dest_addr, dest_data);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]) & 0xfffffffe;
		dest_data = (UINT16)htoi(argv[2]);
		length    = htoi(argv[3]);

		for(i=0; i<length; i+=2) {
			MEM_WORD_WRITE(dest_addr+i, dest_data);
		}
	}
}


/******************************************************************************
 *  cmd_mem_lread ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_mem_lread(int argc, char *argv[])
{
	UINT32  length;
	UINT32  dest_addr, target_addr;
	UINT32  data[16];
	UINT32  i, t;

	if(argc == 2) {
		dest_addr = htoi(argv[1]);
		MEM_LONG_READ(dest_addr, &(data[0]));
		PRINTF("[0x%08lX] : 0x%08lX\n", dest_addr, data[0]);
	} else if(argc == 3) {
		dest_addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		length = length >> 2;
		for(i=0; i<length; i++) {
			t = i % 16;
			target_addr = dest_addr + (i<<2);
			MEM_LONG_READ(target_addr, &(data[t]));

			if( t == 15 || (i+1) == length ){
				cmd_dump_print((dest_addr+(i<<2)-(t<<2)), (UCHAR *)data
					, ((t+1)*sizeof(UINT32)), sizeof(UINT32) );
			}
		}
		PRINTF("\n");
	}
}

/******************************************************************************
 *  cmd_mem_lwrite ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_mem_lwrite(int argc, char *argv[])
{
	UINT32  dest_addr;
	UINT32  dest_data;
	UINT32  length;
	UINT32  i;

	if(argc == 3) {
		dest_addr = htoi(argv[1]) & 0xfffffffc;
		dest_data = htoi(argv[2]);

		MEM_LONG_WRITE(dest_addr, dest_data);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]) & 0xfffffffc;
		dest_data = htoi(argv[2]);
		length    = htoi(argv[3]);

		for(i=0; i<length; i+=4) {
			MEM_LONG_WRITE(dest_addr+i, dest_data);
		}
	}
}

/******************************************************************************
 *  cmd_mem_memcpy ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	puthexa(x) 	( ((x)<10) ? (char)((x)+'0') : (char)((x)-10+'a') )

static int cmddata_dump_compare(UINT8 *src, UINT8 *dst, UINT32 len)
{
	int	status;
	UINT32  index;

	{
		UINT32	tlen;
		UINT8	*tsrc, *tdst;

		asm volatile ( "nop       \n" );

		tsrc = src ;
		tdst = dst ;
		tlen = len ;

		do{
			tsrc ++;
			tdst ++;
			tlen --;
			if( tlen == 0 ) break;

			if( *tsrc != *tdst ) break;

		}while(1);

		if( tlen != 0 ){
			ASIC_DBG_TRIGGER(((UINT32)tsrc));
			ASIC_DBG_TRIGGER(((UINT32)tdst));
			PRINTF("error %p, %p ============================================\n", tsrc, tdst);
			status = 1;
		}else{
			status = 0;
                }

	}

	if( (index = DRIVER_MEMCMP(src, dst, len)) != 0 ){
		PRINTF("[[[[[[ 2 mismatched  %d \n", index);
	}

	PRINTF("[[[[[[ src %p : dst %p = %x \n", src, dst, status);

	return status ;
}

void cmd_mem_memcpy(int argc, char *argv[])
{
	UINT32  dst_addr;
	UINT32  src_addr;
	UINT32  length;

	if(argc == 4) {
		dst_addr = htoi(argv[1]);
		src_addr = htoi(argv[2]);
		length = htoi(argv[3]);

		if( DRIVER_STRCMP(argv[0], "memtest") == 0 ){
			dst_addr = (UINT32)APP_MALLOC(length);
			src_addr = (UINT32)APP_MALLOC(length);


			PRINTF(" memtest %x %x (%x)\n", dst_addr, src_addr , length);

			DRIVER_MEMCPY( (void *)dst_addr, (void *)src_addr, length);
			//if( DRIVER_MEMCMP( (void *)dst_addr, (void *)src_addr, length) != 0 ){
			//	PRINTF(" memtest1 error !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			//}
			if( cmddata_dump_compare( (void *)dst_addr, (void *)src_addr, length) != 0 ){
				PRINTF(" memtest2 error !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
			if( cmddata_dump_compare( (void *)dst_addr, (void *)src_addr, length) != 0 ){
				PRINTF(" memtest3 error !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}

			APP_FREE((void *)dst_addr);
			APP_FREE((void *)src_addr);

		}else
		if( DRIVER_STRCMP(argv[0], "memset") == 0 ){
			DRIVER_MEMSET( (void *)dst_addr, (int)src_addr, length);
		}else{
			DRIVER_MEMCPY( (void *)dst_addr, (void *)src_addr, length);
		}
	}
}

/******************************************************************************
 *  cmd_nor_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_nor_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

#ifdef	SUPPORT_NOR_DEVICE
	static UINT8	*mem_read_data;

	UINT32	ioctldata[3], pagesize, i ;
	HANDLE	nor;
	UINT32	dest_addr;
	UINT32	dest_len;

	if( argc >= 3 ){
		dest_addr = htoi(argv[2]);
	}
	if( argc >= 4 ){
		dest_len  = htoi(argv[3]);
	}

	// Initialize
	mem_read_data = APP_MALLOC(512);

	nor = NOR_CREATE(NOR_UNIT_0);

	if(nor != NULL){

		NOR_INIT(nor);

#ifdef	BUILD_OPT_DA16200_ROMALL
		ioctldata[0] = TRUE;
		NOR_IOCTL(nor, NOR_SET_BOOTMODE, ioctldata);
#endif	//BUILD_OPT_DA16200_ROMALL

		pagesize = 0;
		NOR_IOCTL(nor, NOR_GET_SIZE, &pagesize);

		if( DRIVER_STRCMP(argv[1], "reset") == 0){
			ioctldata[0] = 0;
			NOR_IOCTL(nor, NOR_SET_RESET, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[1], "info") == 0){
			NOR_IOCTL(nor, NOR_GET_INFO, NULL);
		}else
		if( DRIVER_STRCMP(argv[1], "unlock") == 0){
			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			NOR_IOCTL(nor, NOR_SET_UNLOCK, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[1], "lock") == 0){
			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			NOR_IOCTL(nor, NOR_SET_LOCK, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[1], "verify") == 0){
			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			NOR_IOCTL(nor, NOR_GET_VERIFY, ioctldata);
			if( ioctldata[2] == TRUE){
				PRINTF("%lx-%lx Locked\n", dest_addr, dest_len);
			}else{
				PRINTF("%lx-%lx Unlocked\n", dest_addr, dest_len);
			}
		}else
		if( DRIVER_STRCMP(argv[1], "erase") == 0){
			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			NOR_IOCTL(nor, NOR_SET_ERASE, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[1], "read") == 0){

			do{
				NOR_READ(nor, dest_addr, mem_read_data, ((dest_len > 512) ? 512 : dest_len) );
				cmd_dump_print(dest_addr, mem_read_data, ((dest_len > 512) ? 512 : dest_len), 0);
				dest_addr += 512;

				if(dest_len < 512){
					dest_len = 0;
				}else{
					dest_len -= 512;
				}
			}while(dest_len > 0);

		}else
		if( DRIVER_STRCMP(argv[1], "write") == 0){

			for(i=0; i < (argc-3) ; i++){
				mem_read_data[i] = htoi(argv[4+i]);
			}
			for(   ; i < dest_len; i++){
				mem_read_data[i] = i % 256;
			}
			NOR_WRITE(nor, dest_addr, mem_read_data, dest_len);

		}else{
			PRINTF("usage : %s [reset|unlock|lock|verify|erase|read|write] [start addr] [length] \n", argv[0]);
		}

		NOR_CLOSE(nor);
	}

	APP_FREE(mem_read_data);

#endif	//SUPPORT_NOR_DEVICE
}

/******************************************************************************
 *  cmd_sflash_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#undef	TEST_SFLASH_POWERDOWN

#define	TEST_SFLASH_MAX_BUFF	512

#define SUPPORT_SFLASH_TEST
#ifdef	SUPPORT_SFLASH_TEST
extern int make_message(char *title, char *buf, size_t buflen);
extern int  make_binary(char *message, UINT8 **perdata, UINT32 swap);
#endif	//SUPPORT_SFLASH_TEST

void cmd_sflash_func(int argc, char *argv[])
{
#ifdef SUPPORT_SFLASH_DEVICE
	static UINT8	*mem_read_data;

	UINT32	ioctldata[7], pagesize, i , nxt ;
	HANDLE	sflash;
	UINT32	dest_addr = 0;
	UINT32	dest_len = 0;
	UINT32	tbusmode, busmode, speed, bussel;

	if( *argv[1] == 'q' ){
		tbusmode = ctoi(&(argv[1][1]));
		switch(tbusmode){
		case	112:	tbusmode = SFLASH_BUS_112; break;
		case	122:	tbusmode = SFLASH_BUS_122; break;
		case	222:	tbusmode = SFLASH_BUS_222; break;
		case	114:	tbusmode = SFLASH_BUS_114; break;
		case	144:	tbusmode = SFLASH_BUS_144; break;
		case	444:	tbusmode = SFLASH_BUS_444; break;
		case	011:	tbusmode = SFLASH_BUS_011; break;
		case	001:	tbusmode = SFLASH_BUS_001; break;
		case	022:	tbusmode = SFLASH_BUS_022; break;
		case	002:	tbusmode = SFLASH_BUS_002; break;
		case	044:	tbusmode = SFLASH_BUS_044; break;
		case	014:	tbusmode = SFLASH_BUS_014; break;
		case	004:	tbusmode = SFLASH_BUS_004; break;
		default:	tbusmode = SFLASH_BUS_111; break;
		}
		nxt = 2;
	}else{
		tbusmode = SFLASH_BUS_111;
		nxt = 1;
	}

	if( *argv[nxt] == 's' ){
		speed = ctoi(&(argv[nxt][1]));
		nxt += 1;
	}else{
		speed = 30; // FPGA Test
	}

	if( *argv[nxt] == 'c' ){ // bussel
		bussel = ctoi(&(argv[nxt][1]));
		nxt += 1;
	}else{
		bussel = da16x_sflash_get_bussel();
	}

	if( (UINT32)argc >= (nxt+2) ){
		dest_addr = htoi(argv[(nxt+1)]);
	}
	if( (UINT32)argc >= (nxt+3) ){
		dest_len  = htoi(argv[(nxt+2)]);
	}

	// Initialize
	mem_read_data = APP_MALLOC(TEST_SFLASH_MAX_BUFF);

	da16x_environ_lock(TRUE);

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);

	if(sflash != NULL){

		if( speed == 0 ){
			ioctldata[0] = da16x_sflash_get_maxspeed();
		}else{
			speed = (speed > 100 ) ? 100 : speed ;
			ioctldata[0] = speed * MHz ;
		}
		SFLASH_IOCTL(sflash, SFLASH_BUS_MAXSPEED, ioctldata);

		// setup bussel
		ioctldata[0] = bussel;
		SFLASH_IOCTL(sflash, SFLASH_SET_BUSSEL, ioctldata);

		//Cache Issue: ioctldata[0] = TRUE;	/* qspi */
		//Cache Issue: SFLASH_IOCTL(sflash, SFLASH_BUS_INTERFACE, ioctldata);

		if( SFLASH_INIT(sflash) == TRUE ){
			// to prevent a reinitialization
			if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
				SFLASH_IOCTL(sflash, SFLASH_SET_INFO, ioctldata);
			}
		}

		SFLASH_IOCTL(sflash, SFLASH_CMD_WAKEUP, ioctldata);
		if( ioctldata[0] > 0 ){
			ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
			//OAL_MSLEEP( ioctldata[0] );
			{
				portTickType xFlashRate, xLastFlashTime;
				xFlashRate = ioctldata[0]/portTICK_RATE_MS;
				xLastFlashTime = xTaskGetTickCount();
				vTaskDelayUntil( &xLastFlashTime, xFlashRate );				
			}
		}

		busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);

		pagesize = 0;
		SFLASH_IOCTL(sflash, SFLASH_GET_SIZE, &pagesize);

		if( DRIVER_STRCMP(argv[nxt], "reset") == 0){
			ioctldata[0] = 0;
			SFLASH_IOCTL(sflash, SFLASH_SET_RESET, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[nxt], "info") == 0){
			ioctldata[0] = 0;
			if( SFLASH_IOCTL(sflash, SFLASH_GET_INFO, ioctldata) == TRUE ){
				PRINTF("SFLASH:%08x\n", ioctldata[1]);
				PRINTF("Density:%08x\n", ((ioctldata[2] != 0) ? ((UINT32 *)ioctldata[2])[1] : 0) );
			}
		}else
		if( DRIVER_STRCMP(argv[nxt], "unlock") == 0){
			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[nxt], "lock") == 0){
			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			SFLASH_IOCTL(sflash, SFLASH_SET_LOCK, ioctldata);
		}else
		if( DRIVER_STRCMP(argv[nxt], "ce") == 0){
			if( dest_len != 0xFFFFFFFF ){
				tbusmode = tbusmode | SFLASH_BUS_3BADDR ;
				SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &tbusmode);

				ioctldata[0] = dest_addr;
				ioctldata[1] = dest_len;
				SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);
			}
			SFLASH_IOCTL(sflash, SFLASH_CMD_CHIPERASE, NULL);
		}else
		if( DRIVER_STRCMP(argv[nxt], "erase") == 0 ||
			DRIVER_STRCMP(argv[nxt], "4erase") == 0){

			if( DRIVER_STRCMP(argv[nxt], "4erase") == 0){
				tbusmode = tbusmode | SFLASH_BUS_4BADDR ;
			}else{
				tbusmode = tbusmode | SFLASH_BUS_3BADDR ;
			}

			SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &tbusmode);

			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);

			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			SFLASH_IOCTL(sflash, SFLASH_CMD_ERASE, ioctldata);

		}else
		if( DRIVER_STRCMP(argv[nxt], "read") == 0 ||
			DRIVER_STRCMP(argv[nxt], "4read") == 0){

			if( DRIVER_STRCMP(argv[nxt], "4read") == 0){
				tbusmode = tbusmode | SFLASH_BUS_4BADDR ;
			}else{
				tbusmode = tbusmode | SFLASH_BUS_3BADDR ;
			}

			SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &tbusmode);

			do{
				SFLASH_READ(sflash, dest_addr, mem_read_data, ((dest_len > TEST_SFLASH_MAX_BUFF) ? TEST_SFLASH_MAX_BUFF : dest_len) );
				cmd_dump_print(dest_addr, mem_read_data, ((dest_len > TEST_SFLASH_MAX_BUFF) ? TEST_SFLASH_MAX_BUFF : dest_len), 0);
				dest_addr += TEST_SFLASH_MAX_BUFF;

				if(dest_len < TEST_SFLASH_MAX_BUFF){
					dest_len = 0;
				}else{
					dest_len -= TEST_SFLASH_MAX_BUFF;
				}
			}while(dest_len > 0);

		}else
		if( DRIVER_STRCMP(argv[nxt], "write") == 0 ||
			DRIVER_STRCMP(argv[nxt], "4write") == 0){


			for(i=0; (int)i < (argc-((int)nxt + 2)) ; i++){
				mem_read_data[i] = (UINT8)htoi(argv[nxt+3+i]);
			}
			for(   ; i < dest_len; i++){
				if( i == TEST_SFLASH_MAX_BUFF ){
					break;
				}
				mem_read_data[i] = i % 256;
			}

			if( DRIVER_STRCMP(argv[nxt], "4write") == 0){
				tbusmode = tbusmode | SFLASH_BUS_4BADDR ;
				busmode = SFLASH_BUS_4BADDR | SFLASH_BUS_111;
			}else{
				tbusmode = tbusmode | SFLASH_BUS_3BADDR ;
				busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
			}

			// DYB lock issue
			SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);

			ioctldata[0] = dest_addr;
			ioctldata[1] = dest_len;
			SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);

			SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &tbusmode);

			do{
				SFLASH_WRITE(sflash, dest_addr, mem_read_data, ((dest_len > TEST_SFLASH_MAX_BUFF) ? TEST_SFLASH_MAX_BUFF : dest_len) );
				if(dest_len < TEST_SFLASH_MAX_BUFF){
					dest_len = 0;
				}else{
					dest_len -= TEST_SFLASH_MAX_BUFF;
				}
			}while(dest_len > 0);

#ifdef	SUPPORT_SFLASH_TEST
		}else
		if( DRIVER_STRCMP(argv[nxt], "test") == 0){
			int	len = 0;
			UINT8	*snddata = NULL;
			UINT8	*rcvdata = NULL;
			UINT32	rcvlen = 0;
			char	*buffer = NULL;
			size_t	buflen = (1024 * 4) + 2;
			int	msglen = 0;

			while(1){
				// mode =================================
				busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
				SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);
				// data =================================

				buffer = APP_MALLOC(buflen);
				if (buffer) {
					msglen = make_message("SND", buffer, buflen);
					if (msglen > 0) {
						len = make_binary(buffer, &snddata, FALSE);
					}

					APP_FREE(buffer);
					buffer = NULL;
				}

				if( snddata == NULL ){
					break;
				}

				ioctldata[0] = (UINT32)snddata;
				ioctldata[1] = (UINT32)len;

				buffer = APP_MALLOC(buflen);
				if (buffer) {
					msglen = make_message("RCV", buffer, buflen);

					for( len = 0; buffer[len] != '\0'; len++ ){
						if (buffer[len] == ' ' || buffer[len] == '\r' || buffer[len] == '\n'){
							buffer[len] = '\0';
							break;
						}
					}

					PRINTF("rcv(%s):\n", buffer);
					rcvlen = htoi(buffer);
					APP_FREE(buffer);
					buffer = NULL;
				}

				if( rcvlen > 0 ){
					rcvdata = (UINT8 *)APP_MALLOC(rcvlen);
				}

				ioctldata[2] = (UINT32)rcvdata;
				ioctldata[3] = rcvlen;

				SFLASH_IOCTL(sflash, SFLASH_SET_VENDOR, ioctldata);

				PRINTF("SNDdata(%d):\n", ioctldata[1]);
				DRV_DBG_DUMP(0, snddata, (UINT16)ioctldata[1]);
				PRINTF("RCVdata(%d):\n", ioctldata[3]);
				DRV_DBG_DUMP(0, rcvdata, (UINT16)ioctldata[3]);

				if( snddata != NULL ){
					APP_FREE(snddata);
				}
				if( rcvdata != NULL ){
					APP_FREE(rcvdata);
				}
			}
		}else
		if (DRIVER_STRCMP(argv[nxt], "wrap") == 0)
		{
			SFLASH_HANDLER_TYPE *xfctest;
			UINT32	busctrl[2], toctrl[2];
			UINT8	txdata[4];

			busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
			SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);
			xfctest = (SFLASH_HANDLER_TYPE *)sflash;

			ioctldata[0] = TRUE;
			ioctldata[1] = portMAX_DELAY; //OAL_SUSPEND;
			ioctldata[2] = XFC_CSEL_0;
			XFC_IOCTL( xfctest->bushdlr, XFC_SET_LOCK, (VOID *)ioctldata);

			XFC_IOCTL( xfctest->bushdlr, XFC_SET_DMAMODE, NULL);

			ioctldata[0] = XFC_CSEL_0;
			ioctldata[1] = 4; // 4-wire
			XFC_IOCTL(  xfctest->bushdlr, XFC_SET_WIRE, (VOID *)ioctldata);


			busctrl[0] = XFC_SPI_TIMEOUT_EN
				| XFC_SPI_PHASE_CMD_1BYTE
				| XFC_SPI_PHASE_ADDR_3BYTE
				| XFC_SET_SPI_DUMMY_CYCLE(0);

			busctrl[1] = XFC_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,XFC_BUS_SPI) // CMD
							, SPI_BUS_TYPE(0,0,XFC_BUS_SPI)
							, SPI_BUS_TYPE(0,0,XFC_BUS_SPI)
							, SPI_BUS_TYPE(1,0,XFC_BUS_SPI) // DATA
						);

			toctrl[0] = 3; // Unit (8^K us)
			toctrl[1] = 5; // (Count+1) * Unit

			txdata[0] = (UINT8)htoi(argv[nxt+1]);

			PRINTF("WRAP:C0.%02x\n", txdata[0]);

			XFC_IOCTL( xfctest->bushdlr, XFC_SET_BUSCONTROL, &busctrl );

			XFC_IOCTL( xfctest->bushdlr, XFC_SET_TIMEOUT, &toctrl );  // about 3 msec
			XFC_SFLASH_TRANSMIT(xfctest->bushdlr, 0xC0, 0x00, 0x00, txdata, 1, NULL, 0);

			busctrl[0] = 0; // Unit (8^K us)
			busctrl[1] = 0; // (Count+1) * Unit
			// Zero-wait : 1 usec
			XFC_SFLASH_TRANSMIT(xfctest->bushdlr, 0x05, 0x00, 0x00, NULL, 0, txdata, 1);
			cmd_dump_print(0, txdata, 1, 0);

			ioctldata[0] = FALSE;
			ioctldata[1] = portMAX_DELAY; //OAL_SUSPEND;
			ioctldata[2] = XFC_CSEL_0;
			XFC_IOCTL( xfctest->bushdlr, XFC_SET_LOCK, (VOID *)ioctldata);

			busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_144;
			SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);

			SFLASH_READ(sflash, 0x300008, mem_read_data, 256 );
			cmd_dump_print(0x300008, mem_read_data, 256, 0);

#endif	//SUPPORT_SFLASH_TEST
		}else{
			PRINTF("usage : %s [reset|unlock|lock|verify|erase|read|write] [start addr] [length] \n", argv[0]);
		}

		busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_144;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);

#ifdef	TEST_SFLASH_POWERDOWN
		SFLASH_IOCTL(sflash, SFLASH_CMD_POWERDOWN, ioctldata);
		if( ioctldata[0] > 0 ){
			ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
			//OAL_MSLEEP( ioctldata[0] );
			{
				portTickType xFlashRate, xLastFlashTime;
				xFlashRate = ioctldata[0]/portTICK_RATE_MS;
				xLastFlashTime = xTaskGetTickCount();
				vTaskDelayUntil( &xLastFlashTime, xFlashRate );				
			}
		}
#endif	//TEST_SFLASH_POWERDOWN

		SFLASH_CLOSE(sflash);
	}

	da16x_environ_lock(FALSE);

	APP_FREE(mem_read_data);

#endif	//SUPPORT_SFLASH_DEVICE
}

/******************************************************************************
 *  cmd_sfcopy_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	SFCOPY_BULKSIZE	(4*KBYTE)
#define	SFCOPY_TICKSTEP	(32*KBYTE)

UINT32	cmd_sfcopy_get_sflashmode(char *token)
{
	UINT32 mode, addr;

	if( token[1] == 'a' || token[1] == 'A' ){
		mode = htoi(&(token[2]));
		addr = SFLASH_BUS_4BADDR;
	}else{
		mode = htoi(&(token[1]));
		addr = SFLASH_BUS_3BADDR;
	}

	switch(mode){
	case	112:	mode = SFLASH_BUS_112; break;
	case	122:	mode = SFLASH_BUS_122; break;
	case	222:	mode = SFLASH_BUS_222; break;
	case	114:	mode = SFLASH_BUS_114; break;
	case	144:	mode = SFLASH_BUS_144; break;
	case	444:	mode = SFLASH_BUS_444; break;
	case	011:	mode = SFLASH_BUS_011; break;
	case	001:	mode = SFLASH_BUS_001; break;
	case	022:	mode = SFLASH_BUS_022; break;
	case	002:	mode = SFLASH_BUS_002; break;
	case	044:	mode = SFLASH_BUS_044; break;
	case	014:	mode = SFLASH_BUS_014; break;
	case	004:	mode = SFLASH_BUS_004; break;
	default:	mode = SFLASH_BUS_111; break;
	}
	return (addr|mode);
}

void cmd_sfcopy_func(int argc, char *argv[])
{
#ifdef	SUPPORT_SFLASH_COPY
	HANDLE	handler;
	UINT32	srcaddr, dstaddr, loadaddr, epaddr;
	UINT32	length, status, offset;
	UINT8	*imgbuffer;

	if( argc < 4 ){
		return ;
	}

	srcaddr = htoi(argv[1]) & (~(SFCOPY_BULKSIZE-1));
	dstaddr = htoi(argv[2]) & (~(SFCOPY_BULKSIZE-1));
	length  = htoi(argv[3]);

	imgbuffer = (UINT8 *)APP_MALLOC(SFCOPY_BULKSIZE);
	if( imgbuffer == NULL ){
		PRINTF("SFLASH Image buffer error\n");
		return ;
	}

	handler = flash_image_open((sizeof(UINT32)*8), srcaddr, (VOID *)&da16x_environ_lock);

	// CRC Check for Src Image Header
	status = flash_image_check(handler, IMAGE_HEADER_TYPE_MULTI, srcaddr);

	if( status > 0 ){
		PRINTF("SFLASH Image Copy %08x to %08x, size %d\n"
			, srcaddr, dstaddr, length);

		// CRC Check for Src Image Data
		if( (status = flash_image_test(handler, srcaddr, &loadaddr, &epaddr)) == 0 ){
			PRINTF("SFLASH Src Image Data Error\n");
		}else{
			PRINTF("SFLASH Src Image Data OK [len:%d]\n", status);
		}
	}else{
		PRINTF("SFLASH Src Wrong Image\n");
	}

	//TEST: flash_image_force(handler, 0x2);

	if( status > 0 ){
		length = (length < status ) ? status : length ;

		flash_image_erase(handler, dstaddr, length);

		for(offset = 0; offset < length; offset += SFCOPY_BULKSIZE ){
			status = flash_image_read(handler, (srcaddr+offset), imgbuffer, SFCOPY_BULKSIZE );
			if( status > 0 ){
				if((offset % (SFCOPY_TICKSTEP/SFCOPY_BULKSIZE)) == 0){
					PUTC('.');
				}
				flash_image_write(handler, (dstaddr+offset), imgbuffer, SFCOPY_BULKSIZE );
			}
		}

		// CRC Check for Dst Image Header & Data
		status = flash_image_check(handler, IMAGE_HEADER_TYPE_MULTI, dstaddr);

		if( status > 0 ){
			if( flash_image_test(handler, dstaddr, &loadaddr, &epaddr) == 0 ){
				PRINTF("\nSFLASH Dst Image Data Error\n");
			}else{
				PRINTF("\nSFLASH Dst Image Data OK\n");
			}
		}else{
			PRINTF("\nSFLASH Dst Wrong Image\n");
		}
	}

	//TEST: flash_image_force(handler, 0x0);

	APP_FREE(imgbuffer);

	flash_image_close(handler);

#endif	/*SUPPORT_SFLASH_COPY*/
}

/******************************************************************************
 *  cmd_setsfl_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_setsfl_func(int argc, char *argv[])
{
#ifdef	SUPPORT_SFLASH_DEVICE
	UINT32 cselmode;

	if( argc < 2 ){
		return ;
	}

	cselmode = htoi(argv[1]);

	switch(cselmode){
	case SFLASH_CSEL_0:
		da16x_sflash_set_bussel(SFLASH_CSEL_0);
		break;
	case SFLASH_CSEL_1:
		da16x_sflash_set_bussel(SFLASH_CSEL_1);
		break;
	default:
		break;
	}

#endif	/*SUPPORT_SFLASH_DEVICE*/
}

/* EOF */
