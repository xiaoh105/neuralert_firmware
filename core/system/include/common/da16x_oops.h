/**
 ****************************************************************************************
 *
 * @file da16x_oops.h
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


#ifndef	__da16x_oops_h__
#define	__da16x_oops_h__

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#define	OOPS_MARK		0x4F4F5053 /*OOPS*/
#define OOPS_LEN_MASK		0x00000FFF
#define OOPS_MOD_SHFT		12
#define	OOPS_STACK_TAG		((UINT32 *)(DA16X_SRAM_BASE|0x00E00))
#define	OOPS_STACK_CTXT		((UINT32 *)(DA16X_SRAM_BASE|0x00E10))
#define	OOPS_STACK_PICS		19
#define	OOPS_STACK_SICS		(7+6)

typedef 	struct _oops_control_	OOPS_CONTROL_TYPE;
typedef 	struct _oops_taginfo_	OOPS_TAG_TYPE;

/******************************************************************************
 *
 *
 *
 ******************************************************************************/

struct	_oops_control_ {
	UINT8	active  : 1;	// Enable bit
	UINT8	boot    : 1;	// Boot or Prompt
	UINT8	stop	: 1;	// Overwirte or Stop if current num is reached at the Max.
	UINT8	dump	: 1;	// Display a dump on the console
	UINT8	maxnum	: 4;	// Max Items : 2^N Sectors
	UINT8	secsize : 7;	// Sector Size : 2^N Bytes
	UINT8	save	: 1;	// Save mode

	UINT16	offset  : 15;	// Sector Offset
	UINT16	memtype : 1;	// SFLASH or NOR
};

struct	_oops_taginfo_ {
	UINT32	tag;
	UINT16	mark;		// Application Mark
	UINT16	length;		// Dump Length
	UINT32	rtc[2];		// RTC Time Stamp
};

/******************************************************************************
 *
 *
 *
 ******************************************************************************/

extern void	da16x_oops_init(VOID *oopsctrl, OOPS_CONTROL_TYPE *info);
extern void	da16x_oops_clear(VOID *oopsctrl);
extern UINT32	da16x_oops_save(VOID *oopsctrl, OOPS_TAG_TYPE *oopstag, UINT16 mark);
extern void	da16x_oops_view(VOID *oopsctrl);

#endif	/*__da16x_oops_h__*/

/* EOF */
