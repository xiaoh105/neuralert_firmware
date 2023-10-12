/**
 ****************************************************************************************
 *
 * @file mem.ld
 *
 * @brief linker script for DA16200
 *
 * Copyright (c) 2016-2019 Dialog Semiconductor. All rights reserved.
 * 
 * This software ("Software") is owned by Dialog Semiconductor.
 * 
 * By using this Software you agree that Dialog Semiconductor retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Dialog Semiconductor is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Dialog Semiconductor products.
 * 
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * DIALOG SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * 
 ****************************************************************************************
 */

MEMORY
{
	CACHE (rx)    : ORIGIN = 0x00100000, LENGTH = 2048K
	
	SYSRAM0 (rw)  : ORIGIN = 0x00080000, LENGTH = 0x400
	SYSRAM1 (rw)  : ORIGIN = 0x00080400, LENGTH = 0x200
	CSTACK (rw)   : ORIGIN = 0x00080600, LENGTH = 0x800
	OOPS (rw)	  : ORIGIN = 0x00080E00, LENGTH = 0x20
	RAM0 (xrw)    : ORIGIN = 0x00080E20, LENGTH = 0x21E0
	RAMLIB (xrw)  : ORIGIN = 0x00083000, LENGTH = 0x4000
	RAMIPD (xrw)  : ORIGIN = 0x00087000, LENGTH = 0x4800
	RAM1 (xrw)    : ORIGIN = 0x0008B800, LENGTH = 0x74800
	
	FLASH  (rx)   : ORIGIN = 0x00000000, LENGTH = 1792K
}

__SRAM_segment_end__ 	   = ORIGIN(RAM1) + LENGTH(RAM1);

__region_SYSRAM0_start__   = ORIGIN(SYSRAM0);
__region_SYSRAM0_end__     = ORIGIN(SYSRAM0) + LENGTH(SYSRAM0);
__region_SYSRAM1_start__   = ORIGIN(SYSRAM1);
__region_SYSRAM1_end__     = ORIGIN(SYSRAM1) + LENGTH(SYSRAM1);
__region_RAMLIB_start__	   = ORIGIN(RAMLIB);
__region_RAMLIB_end__	   = ORIGIN(RAMLIB) + LENGTH(RAMLIB);
__region_RAMIPD_start__	   = ORIGIN(RAMIPD);
__region_RAMIPD_end__	   = ORIGIN(RAMIPD) + LENGTH(RAMIPD);
__region_RAM0_start__ 	   = ORIGIN(RAM0);
__region_RAM0_end__	   	   = ORIGIN(RAM0) + LENGTH(RAM0);

__stack = ORIGIN(CSTACK);

