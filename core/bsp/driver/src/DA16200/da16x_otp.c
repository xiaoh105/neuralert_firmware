/**
 ****************************************************************************************
 *
 * @file da16x_otp.c
 *
 * @brief DA16200 Generic feature - User application entry point
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
#include "da16x_otp.h"

/******************************************************************************
 *
 *  Local Definitions & Declaration
 *
 ******************************************************************************/

#undef SUPPORT_OTP_CLOCKGATING
//---------------------------------------------------------
//
//---------------------------------------------------------

#define MEM_BYTE_READ(addr, data)	*(data) = *((volatile UINT8 *)(addr))
#define MEM_WORD_READ(addr, data)	*(data) = *((volatile UINT16 *)(addr))
#define MEM_LONG_READ(addr, data)	*(data) = *((volatile UINT32 *)(addr))

#define MEM_BYTE_WRITE(addr, data)	*((volatile UINT8 *)(addr)) = (data)
#define MEM_WORD_WRITE(addr, data)	*((volatile UINT16 *)(addr)) = (data)
#define MEM_LONG_WRITE(addr, data)	*((volatile UINT32 *)(addr)) = (data)
#define REG_PL_WR(addr, value) (*(volatile uint32_t *)(addr)) = (value)
#define REG_PL_RD(addr)        (*(volatile uint32_t *)(addr))

#ifdef	SUPPORT_OTP_CLOCKGATING
#define  OTP_CLKGATE_ON()         {                             \
				unsigned int div;    		\
								\
				DA16200_CLKGATE_ON(clkoff_otp);  \
				NOP_EXECUTION();                \
				NOP_EXECUTION();                \
				NOP_EXECUTION();                \
				NOP_EXECUTION();                \
				NOP_EXECUTION();                \
				div  = SYSGET_OTP_DOWNSYNC();	\
				SYSSET_OTP_DOWNSYNC(div);       \
				DA16200_PERI_RESET(rst_otp);	\
			}

#define	OTP_CLKGATE_OFF()	DA16200_CLKGATE_OFF(clkoff_otp)
#else	//SUPPORT_OTP_CLOCKGATING
#define	OTP_CLKGATE_ON()
#define	OTP_CLKGATE_OFF()
#endif	//SUPPORT_OTP_CLOCKGATING

//---------------------------------------------------------
//
//---------------------------------------------------------
static void otp_delay(UINT32 udelay)
{
	int i, nop_cycle, loop_cnt;
	unsigned int sysclock;
	_sys_clock_read( &sysclock, sizeof(UINT32));	// 50000000
	nop_cycle = (1000000000 / (sysclock));
	loop_cnt = ((udelay * 1000) / nop_cycle);
	for (i = 0; i <loop_cnt; i++)
	{
		asm volatile (	"nop       \n");
	}
}

int otp_write(UINT32 addr, UINT32 data)
{
//#ifdef BUILD_OPT_DA16200_ASIC
#if 1
	int retry,i;
	UINT32 rdata, cmd = 0;
	for(retry=0; retry<NP2; retry++)
	{
		OTP_ASIC_TRIGGER(addr);
		OTP_ASIC_TRIGGER( (((data<<1) | 0x01) << 26) );
		MEM_LONG_WRITE(addr, (((data<<1) | 0x01) << 26));
		PRINT_OTP("OTP\twrite addr = 0x%x data 0x%x\n", addr, ((data<<1) | 0x01));

		for(i=0; i<BUSY_WAIT_TIME_OUT; i++)//busy wait for done signal
		{
			//OAL_USLEEP(1);
			//WAIT_FOR_INTR();
			otp_delay(1);			// 10->2
			MEM_LONG_READ(OTP_ADDR_CMD, &cmd);
			if((cmd&0x10)==0) {					/*************/
			    OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb120));
                break;
            }
		}
		if(i==BUSY_WAIT_TIME_OUT)//check for time out
		{
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb130));
			PRINT_OTP("OTP\tbusy wait TIME out \n");
			return OTP_ERROR_BUSY_WAIT_TIMEOUT;
		}
		else
		{
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb140));
			MEM_LONG_READ(addr, &rdata);
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb170));
			PRINT_OTP("OTP\tread data %x\n", rdata);
		}

		if((rdata & (0x01 << data)) == (UINT32)(0x01<<data))
		{
		  	OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb150));
			//1bit write success
			PRINT_OTP("OTP\tread addr = 0x%x\n", addr);//(((addr-OTP_ADDR_BASE)/8)*8)+OTP_ADDR_BASE);
			PRINT_OTP("OTP\tSuccess\n");
			return OTP_OK;
		}
		else
		{
			//PRINT_OTP("OTP\tcompare fail R%x D%x\n", rdata, data);
		}
	}
	if(retry==NP2)
	{
		OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb160));
		PRINT_OTP("OTP\tNP2 fail\n");
		return OTP_ERROR_NP2;
	}
#endif //BUILD_OPT_DA16200_ASIC

        return OTP_ERROR_UNKNOWN;
}

int _sys_otp_ioctl(int cmd, void *data)
{
#ifdef BUILD_OPT_DA16200_ASIC
	switch(cmd){
		case OTP_IOCTL_CMD_WRITE:
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb011));
			OTP_ASIC_TRIGGER(*((UINT32 *)data));
			MEM_LONG_WRITE(OTP_ADDR_CMD, ((*((UINT32 *)data)) <<26));
			PRINT_OTP("OTP\tcmd data = 0x%x\n", *((UINT32 *)data));
			break;
		case OTP_IOCTL_CMD_READ:
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(0xb012));
			OTP_ASIC_TRIGGER(*((UINT32 *)data));
			MEM_LONG_READ(OTP_ADDR_CMD,((UINT32 *)data));
			PRINT_OTP("OTP\tcmd data = 0x%x\n", *((UINT32 *)data));
			break;
	}
#endif //BUILD_OPT_DA16200_ASIC

	return OTP_OK;
}



/******************************************************************************
 *  _sys_otp_read ( )
 *
 *  Purpose :
 *  Input   :   len, length of string buffer
 *  Output  :   buf, string data
 *  Return  :   string length
 ******************************************************************************/

int _sys_otp_read(UINT32 addr, UINT32 *buf)
{
	MEM_LONG_READ(addr, buf);
    return OTP_OK;
}

int _sys_otp_write(UINT32 addr, UINT32 buf)
{
	UINT32 i;
	int status;
	for(i=0; i<32; i++)
	{
		if((buf & (0x01<<i)) == (UINT32)(0x01<<i) )
		//do write
		{
			status=otp_write(addr, i);
			if(status!=OTP_OK)
				return status;
		}
#ifdef OTP_DEBUG_ON
//else skip 0 data
		else{
			PRINT_OTP("OTP\t\t\tWRITE :0\n");
		}
#endif
	}
    return OTP_OK;
}

/******************************************************************************
 *  otp_mem_create
 *
 *  Purpose :   wrapper
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#define	DA16200_OTPROM_ADDRESS(x) 	(OTP_ADDR_BASE|(x<<2))

void	otp_mem_create(void)
{
	UINT32 cmd;

	OTP_CLKGATE_ON();

	cmd = OTP_CMD_WAKE_UP;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);
	//otp_delay(10); // 1us
	cmd = OTP_CMD_CEB;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);
	otp_delay(15); // 1us

}

/******************************************************************************
 *  otp_mem_read
 *
 *  Purpose :   wrapper
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	otp_mem_read(UINT32 offset, UINT32 *data)
{
	UINT32 cmd;
	int	status = 0;

	//Read Phase
	cmd = OTP_CMD_READ;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

	status = _sys_otp_read(DA16200_OTPROM_ADDRESS(offset), data);

	return status;
}

/******************************************************************************
 *  otp_mem_write
 *
 *  Purpose :   wrapper
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	otp_mem_write(UINT32 offset, UINT32 data)
{
	UINT32 cmd;
	int	status;

	//Read Phase
	cmd = OTP_CMD_WRITE;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

	status = _sys_otp_write(DA16200_OTPROM_ADDRESS(offset), data);

	return status;
}

int  otp_mem_lock_read(UINT32 offset, UINT32 *data)
{
	UINT32 cmd;
	int	status, i;

	cmd = OTP_CMD_SPARE_R;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

	// check busy
	for(i=0; i<BUSY_WAIT_TIME_OUT; i++)//busy wait for done signal
	{
		otp_delay(1);			// 10->2
		MEM_LONG_READ(OTP_ADDR_CMD, &cmd);
		if((cmd&0x10)==0) {
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(OTP_CMD_SPARE_R | 0x100));
        	break;
        }
	}


	status = _sys_otp_read(DA16200_OTPROM_ADDRESS(offset), data);

	return status;
}

int  otp_mem_lock_write(UINT32 offset, UINT32 data)
{
	UINT32 cmd;
	int	status, i;

	cmd = OTP_CMD_SPARE_W;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

	// check busy
	for(i=0; i<BUSY_WAIT_TIME_OUT; i++)//busy wait for done signal
	{
		otp_delay(1);			// 10->2
		MEM_LONG_READ(OTP_ADDR_CMD, &cmd);
		if((cmd&0x10)==0) {
			OTP_ASIC_TRIGGER(MODE_HAL_STEP(OTP_CMD_SPARE_W | 0x100));
        	break;
        }
	}

	status = _sys_otp_write(DA16200_OTPROM_ADDRESS(offset), data);

	return status;
}

/******************************************************************************
 *  otp_mem_close
 *
 *  Purpose :   wrapper
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	otp_mem_close(void)
{
#if 0
	UINT32 cmd;

	// OTP Signal control start
	cmd  = OTP_CMD_WAKE_UP;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);
//	otp_delay(10); // 1us
	cmd = OTP_CMD_SLEEP;
	_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);
//	otp_delay(10); // 1us
#endif
	OTP_CLKGATE_OFF();
}

/******************************************************************************
 *  debug_otp_mem_write
 *
 *  Purpose :   patch
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void debug_otp_delay(UINT32 udelay)
{
	int i, nop_cycle, loop_cnt;
	unsigned int sysclock;
	_sys_clock_read( &sysclock, sizeof(UINT32)); // 50000000
	nop_cycle = (1000000000 / (sysclock));
	loop_cnt = ((udelay * 1000) / nop_cycle);
	for (i = 0; i <loop_cnt; i++)
	{
		asm volatile ("nop       \n");
	}
}

static int debug_otp_write(UINT32 addr, UINT32 data)
{
	int retry,i;
	UINT32 rdata, cmd = 0;
	for(retry=0; retry<NP2; retry++)
	{
	     MEM_LONG_WRITE(addr, (((data<<1) | 0x01) << 26));

	     for(i=0; i<BUSY_WAIT_TIME_OUT; i++)//busy wait for done signal
	     {
	            debug_otp_delay(1);            // 10->2
	            MEM_LONG_READ(OTP_ADDR_CMD, &cmd);
	            if((cmd&0x10)==0)
				{ /*************/
					break;
				}
	     }
	     if(i==BUSY_WAIT_TIME_OUT)//check for time out
	     {
	            return OTP_ERROR_BUSY_WAIT_TIMEOUT;
	     }
	     else
	     {
	            MEM_LONG_READ(addr, &rdata);
	     }

	     if((rdata & (0x01 << data)) == (UINT32)(0x01<<data))
	     {
	            //1bit write success
	            return OTP_OK;
	     }
	     else
	     {
	            //PRINT_OTP("OTP\tcompare fail R%x D%x\n", rdata, data);
	     }
	}
	if(retry==NP2)
	{
	         return OTP_ERROR_NP2;
	}

	return OTP_ERROR_UNKNOWN;
}

static int debug_sys_otp_write(UINT32 addr, UINT32 buf)
{
	UINT32 i;
	INT32 status;
	UINT32 errorcount;

	errorcount = 0;
	for(i=0; i<32; i++)
	{
	     if((buf & (0x01<<i)) == (UINT32)(0x01<<i) )
	     //do write
	     {
	            status=debug_otp_write(addr, i);
				if(status == OTP_ERROR_NP2){
					errorcount ++;
				}else
	            if(status!=OTP_OK){
					return status;
	            }
	     }
	}
	return ((errorcount == 0 ) ? OTP_OK : OTP_ERROR_NP2);
}

static int debug_sys_otp_ioctl(int cmd, void *data)
{
	switch(cmd){
		case OTP_IOCTL_CMD_WRITE:
			MEM_LONG_WRITE(OTP_ADDR_CMD, ((*((UINT32 *)data)) <<26));
			break;
		case OTP_IOCTL_CMD_READ:
		    MEM_LONG_READ(OTP_ADDR_CMD,((UINT32 *)data));
		    break;
	}

	return OTP_OK;
}

int debug_otp_mem_write(UINT32 offset, UINT32 data)
{
	UINT32 cmd;
	int        status;

	//Read Phase
	cmd = OTP_CMD_WRITE;
	debug_sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

	status = debug_sys_otp_write(DA16200_OTPROM_ADDRESS(offset), data);

	return status;
}

