 /**
 ****************************************************************************************
 *
 * @file nvedit.c
 *
 * @brief System Monitor
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

#include "hal.h"
#include "oal.h"
#include "sal.h"
#include "driver.h"
#include "library.h"

#include "da16x_system.h"
#include "nvedit.h"
#include "common_def.h"

#include "cal.h"
#include "crypto_primitives.h"

#undef	NVEDIT_WBUF_SUPPORT
#ifdef SFLASH_NVEDIT_DEBUGGING
#define	SUPPORT_NVEDIT_DEBUG
#else
#undef	SUPPORT_NVEDIT_DEBUG
#endif
#undef	SUPPORT_SLOW_WAKEUP_ISSUE
#undef SUPPORT_CHECK_CRC_16
#define SUPPORT_CHECK_CRC
//---------------------------------------------------------
// NOTICE: Wrong Wrap-Around Read Issue
//
// When accessing a SFLASH Burst Read by an unaligned length
// during the POR booting process,
// some SFLASHs are set to the wrong Wrap-Around mode.
// For preventing this malfunction of some models,
// it is necessary to manage to read access
// by 16 byte aligned length.
//---------------------------------------------------------
#define	SUPPORT_EON_BURSTREAD_ISSUE
//---------------------------------------------------------
// NOTICE: Quad Program Support Issue
//
// DA16200 can support 'Quad Program' to automatically detect
// using the SFDP.  But some SFLASH manufacturers recommend
// that it is less effective than 'Single Program', because
// the page program time is much greater than the time that
// it take to clock-in the data.
// According to manufacrurer's guide, DA16200 SDK suggests
// 'Single Program' in your application.
//---------------------------------------------------------
#define	NVEDIT_SFLASH_WRITE_MODE	(SFLASH_BUS_111)
#define	NVEDIT_SFLASH_SAFE_MODE		(SFLASH_BUS_111)
#define	NVEDIT_SFLASH_READ_MODE		(SFLASH_BUS_144)
#define	NVEDIT_SFLASH_SAFEREAD_MODE	(SFLASH_BUS_112)

//---------------------------------------------------------
//
//---------------------------------------------------------

#ifdef	NVEDIT_WBUF_SUPPORT
	#define	NVEDIT_WBUF_MALLOC(x)	APP_MALLOC(x)
	#define	NVEDIT_WBUF_FREE(x)	APP_FREE(x)
	#define	NVEDIT_WORBUF_MAX	nvedit->poolsiz
	#define NVEDIT_WBUF_INIT(x)	if(x != NULL){ ((UINT32 *)(x))[0] = 0; }
#else
	#define	NVEDIT_WBUF_MALLOC(x)	nvedit->mempool
	#define	NVEDIT_WBUF_FREE(x)
	#define	NVEDIT_WORBUF_MAX	nvedit->poolsiz
	#define NVEDIT_WBUF_INIT(x)	if(x != NULL){ ((UINT32 *)(x))[0] = 0; }
#endif //NVEDIT_WBUF_SUPPORT

#ifdef	SUPPORT_NVEDIT_DEBUG
#define	NVEDIT_DBG_PRINTF(...)	if(nvedit->debug == TRUE) { Printf( __VA_ARGS__ ); }
#define NVEDIT_DBGITM_PRINTF(...)	{ Printf( __VA_ARGS__ ); }
#define NVEDIT_ERR_PRINTF(...)		{ Printf( __VA_ARGS__ ); }
#define	NVEDIT_DRV_MALLOC(...)	nvedit_malloc( __func__, __LINE__, __VA_ARGS__ )
#define	NVEDIT_DRV_FREE(...)	nvedit_free( __func__, __LINE__, __VA_ARGS__ )
#else	//SUPPORT_NVEDIT_DEBUG
#define	NVEDIT_DBG_PRINTF(...)
#define NVEDIT_DBGITM_PRINTF(...)
#define NVEDIT_ERR_PRINTF(...)		{ Printf( __VA_ARGS__ ); }
#ifdef	BUILD_OPT_VSIM
#define	NVEDIT_DRV_MALLOC(...)	nvedit_vsim_malloc(__VA_ARGS__)
#else	//BUILD_OPT_VSIM
#define	NVEDIT_DRV_MALLOC(...)	APP_MALLOC(__VA_ARGS__)
#endif	//BUILD_OPT_VSIM
#define	NVEDIT_DRV_FREE(...)	APP_FREE(__VA_ARGS__ )
#endif	//SUPPORT_NVEDIT_DEBUG
#define	NVEDIT_STRNCPY(...)	nvedit_strncpy(__VA_ARGS__ ) // IAR Issue

#define	NVEDIT_ALIGN_SIZE(x)	(((x)+3)&(~0x3))

#define	NVEDIT_SFLASH_DWIDTH	(sizeof(UINT32)*8)

#define	NVEDIT_ASSET_ID		0x4E56524D
#define	NVEDIT_ASSET_NONCE	0x0A0A0A00	// default 0

#ifdef SUPPORT_CHECK_CRC_16
#define	swap16(x)	(((x) & 0xff) << 8) | (((x) & 0xff00) >> 8)
static uint16_t nvedit_swcrc16(uint8_t *data, uint16_t length);
#endif /* SUPPORT_CHECK_CRC_16 */

#ifdef SUPPORT_CHECK_CRC
extern int	SYS_NVEDIT_IOCTL(UINT32 cmd , VOID *data );
#endif /*SUPPORT_CHECK_CRC*/
//---------------------------------------------------------
//
//---------------------------------------------------------
#ifdef	SUPPORT_NVRAM_SFLASH
#undef	SUPPORT_SLR_PWRMGMT	/* Power Down mode	*/
#define	SUPPORT_SLR_PWRWKUP

static void	MSLEEP(unsigned int	msec)
{
	portTickType xFlashRate, xLastFlashTime;

	if((msec/portTICK_RATE_MS) == 0)
	{
		xFlashRate = 1;
	}
	else
	{
		xFlashRate = msec/portTICK_RATE_MS;
	}
	xLastFlashTime = xTaskGetTickCount();
	vTaskDelayUntil( &xLastFlashTime, xFlashRate );
}

static int nvedit_sflash_create(NVEDIT_HANDLER_TYPE *nvedit
					, UINT32 dev_unit, UINT32 *ioctldata);

#define	NVEDIT_MGMT_SFLASH_CREATE(nv,dev,iodat)	nvedit_sflash_create(nv, dev, iodat)

#if	defined(SUPPORT_SLR_PWRMGMT)
#define	NVEDIT_MGMT_SFLASH_CLOSE(nvedit)	{	\
		UINT32 sfiodata ;			\
		sfiodata = NVEDIT_SFLASH_SAFE_MODE;	\
		SFLASH_IOCTL(nvedit->sflash_device, SFLASH_BUS_CONTROL, &sfiodata);\
		sfiodata = 0;				\
		SFLASH_IOCTL(nvedit->sflash_device, SFLASH_CMD_POWERDOWN, &sfiodata);\
		sfiodata = sfiodata / 1000 ; 		\
		if( sfiodata > 0 ){			\
			MSLEEP(sfiodata);			\
		}					\
	}
#else	//SUPPORT_SLR_PWRMGMT
#if	defined(BUILD_OPT_DA16200_CACHEXIP) || defined(BUILD_OPT_DA16200_UEBOOT)
#define	NVEDIT_MGMT_SFLASH_CLOSE(nvedit)	{			\
		/* for CACHE: SFLASH_CLOSE(nvedit->sflash_device) */;	\
		/* for CACHE: nvedit->sflash_device = NULL 	*/;	\
	}
#else	//defined(BUILD_OPT_DA16200_CACHEXIP) || defined(BUILD_OPT_DA16200_UEBOOT)
#define	NVEDIT_MGMT_SFLASH_CLOSE(nvedit)	{			\
		SFLASH_CLOSE(nvedit->sflash_device) ;			\
		nvedit->sflash_device = NULL 		;			\
	}
#endif	//defined(BUILD_OPT_DA16200_CACHEXIP) || defined(BUILD_OPT_DA16200_UEBOOT)
#endif	//SUPPORT_SLR_PWRMGMT
#endif	//SUPPORT_NVRAM_SFLASH

//---------------------------------------------------------
//
//---------------------------------------------------------

static void nvedit_strncpy(char *dst, char *src, int len);

static	NVEDIT_TREE_ITEM *nvedit_node_make_func(UINT32 available, char *name, UINT32 datsiz, UINT32 hash);
static	void nvedit_node_free_func(NVEDIT_TREE_ITEM *node);
static	int nvedit_node_fill_func(NVEDIT_TREE_ITEM *node, VOID *data);

static	NVEDIT_TREE_ITEM *nvedit_tree_init_func(UINT32 available, NVEDIT_TREE_ITEM **node);
static	NVEDIT_TREE_ITEM *nvedit_tree_add_func(UINT32 available, NVEDIT_TREE_ITEM *parent, char *name, UINT32 datsiz, UINT32 hash);
static	int nvedit_tree_del_func(NVEDIT_TREE_ITEM *node);
static	NVEDIT_TREE_ITEM *nvedit_tree_find_name_func(char *token, NVEDIT_TREE_ITEM *parent, char *name);
static	NVEDIT_TREE_ITEM *nvedit_tree_find_node_func(NVEDIT_HANDLER_TYPE *nvedit, NVEDIT_TREE_ITEM *node);
static	int nvedit_tree_print_func(NVEDIT_HANDLER_TYPE *nvedit, NVEDIT_TREE_ITEM *parent);

static	UINT8	*nvedit_pack_string(UINT8 *buffer, char *string);
static	UINT8	*nvedit_pack_string_data(UINT8 *buffer, char *string, UINT32 len);
static	UINT8	*nvedit_pack_uint8(UINT8 *buffer, UINT8 data);
static	UINT8	*nvedit_pack_uint16(UINT8 *buffer, UINT16 data);
static	UINT8	*nvedit_pack_uint32(UINT8 *buffer, UINT32 data);
static	UINT8	*nvedit_pack_var(UINT8 *buffer, void *data, UINT32 len);
static	int 	nvedit_tree_pack_func(NVEDIT_HANDLER_TYPE	*nvedit, NVEDIT_TREE_ITEM *parent, UINT8 *buffer);

static	UINT8	*nvedit_unpack_string(char *string, UINT8 *buffer, UINT32 len);
static	UINT8	*nvedit_unpack_uint8(UINT8 *data, UINT8 *buffer);
static	UINT8	*nvedit_unpack_uint16(UINT16 *data, UINT8 *buffer);
static	UINT8	*nvedit_unpack_uint32( UINT32 *data, UINT8 *buffer);
static	UINT8	*nvedit_unpack_var(void *data, UINT8 *buffer, UINT32 len);
static	int 	nvedit_tree_unpack_func(NVEDIT_HANDLER_TYPE	*nvedit, NVEDIT_TREE_ITEM *parent);

static	UINT32	nvedit_check_room(NVEDIT_HANDLER_TYPE *nvedit);
static	void	nvedit_tree_lenupdate(UINT32 flag, NVEDIT_TREE_ITEM *node, int offset);
static	int nvedit_change_base_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_base );
static	int nvedit_get_base_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 *dev_base, UINT32 *dev_count );
static	int nvedit_tree_load_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_unit );
static	int nvedit_tree_parse_func(NVEDIT_HANDLER_TYPE	*nvedit);
static	int nvedit_tree_save_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_unit , UINT32 mode);
static	int nvedit_tree_erase_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_unit );

#ifndef	 SUPPORT_NVEDIT_FULL
static	int nvedit_tree_litefind_func(char *token, NVEDIT_HANDLER_TYPE *nvedit, char *name, UINT32 *datlen, VOID *data, VOID *pnode );
static	int nvedit_tree_liteupdate_func(NVEDIT_HANDLER_TYPE *nvedit, VOID *pnode, VOID *data );
static	int nvedit_ioctl_cmd_litefind(NVEDIT_HANDLER_TYPE *nvedit, UINT32 *data);
static	int nvedit_ioctl_cmd_liteupdate(NVEDIT_HANDLER_TYPE *nvedit, UINT32 *data);
#endif	//!SUPPORT_NVEDIT_FULL

//---------------------------------------------------------
//
//---------------------------------------------------------

static int		_nvedit_instance[NVEDIT_MAX];
static void		*_mempool_buffer[NVEDIT_MAX];

#define	NVEDIT_INSTANCE		nvedit->dev_unit

#define	NVEDIT_NOR_BASE		nvedit->nor_base
#define	NVEDIT_SFLASH_BASE	nvedit->sflash_base
#define	NVEDIT_RAM_BASE		nvedit->ram_base

#ifdef	SUPPORT_NVEDIT_DEBUG
/******************************************************************************
 *   nvedit_name_delimiter( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	BASE_FOLDER_NAME	"src"
#define	BASE_FOLDER_LEN		3

static char *nvedit_name_delimiter(char *name)
{
	char	*delimiter ;
	delimiter = name ;
	while(*delimiter != '\0') {
		if( *delimiter == '\\') {
			delimiter++;
			if(DRIVER_STRNCMP(BASE_FOLDER_NAME, delimiter,(BASE_FOLDER_LEN)) == 0 ){
				return (delimiter+BASE_FOLDER_LEN+1) ;
			}
		} else{
			delimiter++	;
		}
	}
	return NULL ;
}

/******************************************************************************
 *   nvedit_malloc( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void	*nvedit_malloc(char *name, int line, size_t size)
{
	void	*ptr;
	ptr = APP_MALLOC( size );
	PRINTF("nvedit_malloc-%s:%d-(%p)%d \r\n", nvedit_name_delimiter(name), line, ptr, size );
	return ptr;
}

/******************************************************************************
 *   nvedit_free( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	nvedit_free(char *name, int line, void *ptr)
{
	PRINTF("nvedit_free-%s:%d-%p \r\n", nvedit_name_delimiter(name), line, ptr );
	APP_FREE( ptr );
}
#endif	//SUPPORT_NVEDIT_DEBUG

#ifdef	BUILD_OPT_VSIM
/******************************************************************************
 *   nvedit_vsim_malloc( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void	*nvedit_vsim_malloc(size_t size)
{
	void	*ptr;
	ptr = APP_MALLOC( size );
	if( ptr != NULL ){
		da16x_memset32(ptr, 0, size);
	}
	return ptr;
}

#endif	//BUILD_OPT_VSIM

/******************************************************************************
 *   nvedit_strncpy( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void nvedit_strncpy(char *dst, char *src, int len)
{
	while(len > 0){
		*dst++ = *src++;
		if( *src == '\0') {
			break;
		}
		len--;
	}
	*dst = '\0';
}

#ifdef	SUPPORT_NVRAM_HASH
/******************************************************************************
 *   nvedit hash
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#undef	NVEDIT_HASH_DEBUG
#define	NVEDIT_BACKWARD_COMPATIBILITY
#define	NVEDIT_SUPPORT_CASEINSENSITIVE

#define	NV_HSH_MSK_CSEN		0x0000ffff
#define	NV_HSH_MSK_CINS		0x0fff0000
#define	NV_HSH_SHT_CINS		16
#define	NV_HSH_MSK_FULL		(NV_HSH_MSK_CSEN|NV_HSH_MSK_CINS)

#define	NVEDIT_HASH_KEY_LEN	(sizeof(UINT32))

#define	NVEDIT_HASH_KEY_DECLARE(hash)	UINT32	hash

static	UINT32 nvedit_hash_generate(char *name)
{
	UINT32	csenhash;
#ifdef	NVEDIT_SUPPORT_CASEINSENSITIVE
	UINT32	cinshash;
#endif	//NVEDIT_SUPPORT_CASEINSENSITIVE
#ifdef	NVEDIT_HASH_DEBUG
	char	*token;

	token = name;
#endif	//NVEDIT_HASH_DEBUG
	csenhash = 0;
#ifdef	NVEDIT_SUPPORT_CASEINSENSITIVE
	cinshash = 0;
#endif	//NVEDIT_SUPPORT_CASEINSENSITIVE
	while(*name != '\0'){
		// Case-Sensitive
		csenhash += (UINT32)(*name);

#ifdef	NVEDIT_SUPPORT_CASEINSENSITIVE
		// Case-Insensitive
		if( *name >= 'a' && *name <= 'z' ){
			cinshash += (UINT32)(*name - 32); //(*name - 'a' + 'A');
		}else{
			cinshash += (UINT32)(*name);
		}
#endif	//NVEDIT_SUPPORT_CASEINSENSITIVE

		name++;
	}

#ifdef	NVEDIT_SUPPORT_CASEINSENSITIVE
#ifdef	NVEDIT_HASH_DEBUG
	PRINTF("hash all [%s] = %08x \r\n", token
		, ( NVITEM_HASH_MARK
			|((cinshash<<NV_HSH_SHT_CINS)&NV_HSH_MSK_CINS)
			|(csenhash&NV_HSH_MSK_CSEN) )
		);
#endif	//NVEDIT_HASH_DEBUG

	return ( NVITEM_HASH_MARK
		|((cinshash<<NV_HSH_SHT_CINS)&NV_HSH_MSK_CINS)
		|(csenhash&NV_HSH_MSK_CSEN) );
#else	//NVEDIT_SUPPORT_CASEINSENSITIVE
#ifdef	NVEDIT_HASH_DEBUG
	PRINTF("hash csn [%s] = %08x \r\n", token
		, ( NVITEM_HASH_MARK
			|(csenhash&NV_HSH_MSK_CSEN) )
		);
#endif	//NVEDIT_HASH_DEBUG

	return ( NVITEM_HASH_MARK
		|(csenhash&NV_HSH_MSK_CSEN) );
#endif	//NVEDIT_SUPPORT_CASEINSENSITIVE
}

#ifdef	NVEDIT_HASH_DEBUG
static UINT32	nvedit_hash_compare_csen(NVEDIT_TREE_ITEM *node, UINT32 hash)
{
	PRINTF("CSEN node [%s] = %08x vs %08x \r\n", node->name, node->hash, hash);

	if( node->hash & NVITEM_HASH_MARK ){
		return ((node->hash & NV_HSH_MSK_FULL) == (hash & NV_HSH_MSK_FULL));
	}
	return TRUE;
}

static UINT32	nvedit_hash_compare_cins(NVEDIT_TREE_ITEM *node, UINT32 hash)
{
	PRINTF("CINS node [%s] = %08x vs %08x \r\n", node->name, node->hash, hash);

	if( node->hash & NVITEM_HASH_MARK ){
		return ((node->hash & NV_HSH_MSK_FULL) == (hash & NV_HSH_MSK_FULL));
	}
	return TRUE;
}
#endif	//NVEDIT_HASH_DEBUG

#define	NVEDIT_HASH_KEY_MAKE(node, name, hash)		{	\
		if( hash == 0 ){				\
			node->hash = nvedit_hash_generate(name); \
		}else{						\
			node->hash = hash;			\
		}						\
	}

#define	NVEDIT_HASH_KEY_GEN(token, hash)		{	\
		hash = nvedit_hash_generate(token);		\
	}

#ifdef	NVEDIT_HASH_DEBUG
#define	NVEDIT_HASH_KEY_COMP_CSEN(node, hash)	nvedit_hash_compare_csen(node, hash)

#define	NVEDIT_HASH_KEY_COMP_CINS(node, hash)	nvedit_hash_compare_cins(node, hash)

#ifdef	NVEDIT_BACKWARD_COMPATIBILITY
#define	NVEDIT_HASH_KEY_PACK_UINT32(buffer, node)	{	\
		PRINTF("HASH skipped [%x] \r\n", node->name);	\
	}
#else	//NVEDIT_BACKWARD_COMPATIBILITY
#define	NVEDIT_HASH_KEY_PACK_UINT32(buffer, node)	{	\
		PRINTF("HASH Packed  [%x] \r\n", node->name);	\
		buffer = nvedit_pack_uint32(buffer, node->hash);\
	}
#endif	//NVEDIT_BACKWARD_COMPATIBILITY

#define	NVEDIT_HASH_KEY_UNPACK_UINT32(buffer, offset, hash) {	\
		nvedit_unpack_uint32( &hash, &(buffer[offset]));\
		if( (hash & NVITEM_HASH_MARK) == 0 ){		\
			PRINTF("node %s - unhashed \r\n",	buffer);\
			hash = 0;				\
			buffer = &(buffer[offset]);		\
		}else{						\
			buffer = &(buffer[offset + NVEDIT_HASH_KEY_LEN]);\
		}						\
	}

#else	//NVEDIT_HASH_DEBUG
#define	NVEDIT_HASH_KEY_COMP_CSEN(node, hash)			\
	( (node->hash & NVITEM_HASH_MARK) 			\
		? ((node->hash & NV_HSH_MSK_FULL) == (hash & NV_HSH_MSK_FULL)) \
		: (TRUE) )

#define	NVEDIT_HASH_KEY_COMP_CINS(node, hash)			\
	( (node->hash & NVITEM_HASH_MARK) 			\
		? ((node->hash & NV_HSH_MSK_CINS) == (hash & NV_HSH_MSK_CINS)) \
		: (TRUE) )

#ifdef	NVEDIT_BACKWARD_COMPATIBILITY
#define	NVEDIT_HASH_KEY_PACK_UINT32(buffer, node)
#else	//NVEDIT_BACKWARD_COMPATIBILITY
#define	NVEDIT_HASH_KEY_PACK_UINT32(buffer, node)	{	\
		buffer = nvedit_pack_uint32(buffer, node->hash);\
	}
#endif	//NVEDIT_BACKWARD_COMPATIBILITY

#define	NVEDIT_HASH_KEY_UNPACK_UINT32(buffer, offset, hash) {	\
		nvedit_unpack_uint32( &hash, &(buffer[offset]));\
		if( (hash & NVITEM_HASH_MARK) == 0 ){		\
			hash = 0;				\
			buffer = &(buffer[offset]);		\
		}else{						\
			buffer = &(buffer[offset + NVEDIT_HASH_KEY_LEN]);\
		}						\
	}
#endif	//NVEDIT_HASH_DEBUG

#define	NVEDIT_HASH_KEY_MOVE(hashkey)			{	\
		hashkey = hashkey + NVEDIT_HASH_KEY_LEN;	\
	}

#define NVEDIT_HASH_KEY_GET(hash)		hash

#else	//SUPPORT_NVRAM_HASH

#define	NVEDIT_HASH_KEY_LEN			(0)
#define	NVEDIT_HASH_KEY_DECLARE(hash)
#define	NVEDIT_HASH_KEY_MAKE(node, name, hash)
#define	NVEDIT_HASH_KEY_GEN(node, hash)
#define	NVEDIT_HASH_KEY_COMP_CSEN(node, hash)	TRUE
#define	NVEDIT_HASH_KEY_COMP_CINS(node, hash)	TRUE
#define	NVEDIT_HASH_KEY_PACK_UINT32(buffer, node)
#define	NVEDIT_HASH_KEY_UNPACK_UINT32(buffer, offset, hash) {	\
		UINT32	temp;					\
		nvedit_unpack_uint32( &temp, &(((UINT8 *)buffer)[offset]));\
		if( (temp & NVITEM_HASH_MARK) == 0 ){		\
			buffer = &(buffer[offset]);		\
		}else{						\
			buffer = &(buffer[offset + NVEDIT_HASH_KEY_LEN]);\
		}						\
	}
#define	NVEDIT_HASH_KEY_MOVE(hashkey)
#define NVEDIT_HASH_KEY_GET(hash)		0

#endif	//SUPPORT_NVRAM_HASH


#ifdef	SUPPORT_NVRAM_HASH
#ifdef	NVEDIT_BACKWARD_COMPATIBILITY
#define	NVITEM_NODE_ADJSIZE			(sizeof(UINT32))
#else	//NVEDIT_BACKWARD_COMPATIBILITY
#define	NVITEM_NODE_ADJSIZE			(sizeof(UINT32)+NVEDIT_HASH_KEY_LEN)
#endif	//NVEDIT_BACKWARD_COMPATIBILITY
#else	//SUPPORT_NVRAM_HASH
#define	NVITEM_NODE_ADJSIZE			(sizeof(UINT32))
#endif	//SUPPORT_NVRAM_HASH

/******************************************************************************
 *   NVEDIT_CREATE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	NVEDIT_CREATE( UINT32 dev_id )
{
	NVEDIT_HANDLER_TYPE	*nvedit;

	// Allocate
	nvedit = (NVEDIT_HANDLER_TYPE *) NVEDIT_DRV_MALLOC(sizeof(NVEDIT_HANDLER_TYPE)) ;
	if (nvedit == NULL) {
		return NULL;
	}

	// Clear
	DRIVER_MEMSET(nvedit, 0, sizeof(NVEDIT_HANDLER_TYPE));

	// Address Mapping
	switch ((NVEDIT_LIST)dev_id) {
		case	NVEDIT_0: /* default */
			nvedit->dev_unit = dev_id ;
			nvedit->instance = (_nvedit_instance[NVEDIT_INSTANCE]) ;
			_nvedit_instance[NVEDIT_INSTANCE] ++;
			//nvedit->debug	 = FALSE;
			nvedit->debug	 = TRUE;		/* TEMPORARY */
			break;

		case	NVEDIT_1: /* custom */
			nvedit->dev_unit = dev_id ;
			nvedit->instance = (_nvedit_instance[NVEDIT_INSTANCE]) ;
			_nvedit_instance[NVEDIT_INSTANCE] ++;
			//nvedit->debug	 = FALSE;
			nvedit->debug	 = TRUE;		/* TEMPORARY */
			break;

		default:
			NVEDIT_DBG_PRINTF("ilegal unit number \r\n");
			NVEDIT_DRV_FREE(nvedit);
			return NULL;
	}

	nvedit->mempool = NULL ;
	nvedit->poolsiz	= NVEDIT_WORBUF_DEFAULT;

	nvedit->nor_base	= DA16X_NVRAM_NOR;
	nvedit->sflash_base	= DA16X_NVRAM_SFLASH;
	nvedit->sflash_foot_base = DA16X_NVRAM_FOOT_PRINT;
	nvedit->ram_base	= DA16X_NVRAM_RAM;
	nvedit->update		= 0; /* Update Count */
	nvedit->find		= NULL;
#ifdef	SUPPORT_NVRAM_SECU
	nvedit->secure		= 0xFFFFFFFF; // not used
#endif	//SUPPORT_NVRAM_SECU

	return (HANDLE) nvedit;
}

/******************************************************************************
 *   NVEDIT_INIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	NVEDIT_INIT (HANDLE handler)
{
	NVEDIT_HANDLER_TYPE	*nvedit;

	if(handler == NULL){
		return FALSE;
	}
	nvedit = (NVEDIT_HANDLER_TYPE *)handler ;

	// Normal Mode
	if(nvedit->instance == 0){
		// Semaphore
#ifdef	NVEDIT_WBUF_SUPPORT
		// Skip
#else
		if( _mempool_buffer[NVEDIT_INSTANCE] == NULL )
		{
			_mempool_buffer[NVEDIT_INSTANCE]
				= NVEDIT_DRV_MALLOC(sizeof(UINT8)* NVEDIT_WORBUF_MAX );
#ifdef	BUILD_OPT_VSIM
			da16x_memset32(_mempool_buffer[NVEDIT_INSTANCE]
					, 0, (sizeof(UINT8)* NVEDIT_WORBUF_MAX));
#endif	//BUILD_OPT_VSIM
		}
		nvedit->mempool = _mempool_buffer[NVEDIT_INSTANCE] ;
#endif //NVEDIT_WBUF_SUPPORT
	}

	return TRUE;
}

/******************************************************************************
 *   NVEDIT_IOCTL( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	NVEDIT_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	NVEDIT_HANDLER_TYPE	*nvedit;
#ifdef	SUPPORT_NVEDIT_FULL
	NVEDIT_TREE_ITEM	*parent, *node;
#endif  //SUPPORT_NVEDIT_FULL
	int	status;

	if(handler == NULL){
		return FALSE;
	}
	nvedit = (NVEDIT_HANDLER_TYPE *)handler ;

	status = FALSE;
	switch(cmd){
#ifdef	SUPPORT_NVEDIT_FULL
		case	NVEDIT_CMD_INIT:
			NVEDIT_DBG_PRINTF(" >>> CMD_INIT \r\n");
			if (nvedit_tree_init_func(nvedit_check_room(nvedit), &(nvedit->root)) != NULL) {
				status = TRUE;
				NVEDIT_DBG_PRINTF(" >>> CMD_INIT(root:0x%x) OK \r\n", nvedit->root);
			}
			else {
				NVEDIT_DBG_PRINTF(" >>> CMD_INIT(%d) Error %d \r\n", cmd);
			}
			break;

		case	NVEDIT_CMD_LOAD:
			NVEDIT_DBG_PRINTF(" >>> CMD_LOAD \r\n");
			if (nvedit->workbuf == NULL) {
				nvedit->workbuf = NVEDIT_WBUF_MALLOC(sizeof(UINT8)* NVEDIT_WORBUF_MAX );
				NVEDIT_WBUF_INIT(nvedit->workbuf);
			}

			if (nvedit->workbuf != NULL) {
				NVEDIT_DBG_PRINTF(" >> Start nvedit_tree_load_func(%d,%d) \r\n", ((UINT32 *)data)[0], ((UINT32 *)data)[1]);
				status = nvedit_tree_load_func(nvedit, ((UINT32 *)data)[0], ((UINT32 *)data)[1]);
				NVEDIT_DBG_PRINTF(" >> End nvedit_tree_load_func(status:%d) \r\n", status);
				if( status == TRUE ){
					status = nvedit_tree_parse_func(nvedit);
				}
			}
			NVEDIT_DBG_PRINTF(" >>> CMD_LOAD(status:%d) \r\n", status);
			break;

		case	NVEDIT_CMD_SAVE:
			if(nvedit->workbuf == NULL){
				nvedit->workbuf = NVEDIT_WBUF_MALLOC(sizeof(UINT8)* NVEDIT_WORBUF_MAX );
				NVEDIT_WBUF_INIT(nvedit->workbuf);
			}
			if(nvedit->workbuf != NULL){
				DRIVER_MEMSET(nvedit->workbuf, 0xFF, (sizeof(UINT8)* NVEDIT_WORBUF_MAX));
				NVEDIT_DBG_PRINTF(" >>> CMD_SAVE(status:%d) \r\n", status);
				status = nvedit_tree_save_func(nvedit , ((UINT32 *)data)[0],  ((UINT32 *)data)[1], TRUE);

				if( status == TRUE ){
					nvedit->update = nvedit->update + 1;
				}
			}
			break;

		case	NVEDIT_CMD_CLEAR:
			if(nvedit->root != NULL){
				if( data == NULL || (VOID *)( ((UINT32 *)data)[0] ) == NULL ){
					/* clear all */
					while(nvedit->root->child != NULL){
						nvedit_tree_del_func( nvedit->root->child );
					}
					nvedit->root->length = NVITEM_NOD_MARK;
				}else{
					//NVEDIT_TREE_ITEM *node ;
					node = (NVEDIT_TREE_ITEM *)( ((UINT32 *)data)[0] );
					while(node->child != NULL){
						nvedit_tree_del_func( node->child );
					}
				}
			}
			if(nvedit->workbuf != NULL){
				NVEDIT_WBUF_FREE(nvedit->workbuf);
				nvedit->workbuf = NULL;
			}
			status = TRUE;
			break;

		case	NVEDIT_CMD_ERASE:
			status = nvedit_tree_erase_func(nvedit, ((UINT32 *)data)[0],  ((UINT32 *)data)[1] );
			if( status == TRUE ){
				nvedit->update = nvedit->update + 1;
			}
			if(nvedit->workbuf != NULL){
				NVEDIT_WBUF_FREE(nvedit->workbuf);
				nvedit->workbuf = NULL;
			}
			break;
#endif //SUPPORT_NVEDIT_FULL

#ifndef	 SUPPORT_NVEDIT_FULL
		/* Simple Access Model ---vvvv ****************/
		case	NVEDIT_CMD_LITEFIND:
			// (UINT32)( ((UINT32 *)data)[0] ), dev type
			// (UINT32)( ((UINT32 *)data)[1] ), dev unit
			// (char *)( ((UINT32 *)data)[2] ), full name
			// (UINT32)( ((UINT32 *)data)[3] ), len pointer
			// (UINT32)( ((UINT32 *)data)[4] ), data
			status = nvedit_ioctl_cmd_litefind(nvedit, (UINT32 *)data);
			break;
		case	NVEDIT_CMD_LITEUPDATE:
			// (UINT32)( ((UINT32 *)data)[0] ), dev type
			// (UINT32)( ((UINT32 *)data)[1] ), dev unit
			// (char *)( ((UINT32 *)data)[2] ), full name
			// (UINT32)( ((UINT32 *)data)[3] ), len pointer
			// (UINT32)( ((UINT32 *)data)[4] ), data
			status = nvedit_ioctl_cmd_liteupdate(nvedit, (UINT32 *)data);
			break;
		/* Simple Access Model ---^^^^ ****************/
#endif	//!SUPPORT_NVEDIT_FULL

#ifdef	SUPPORT_NVEDIT_FULL
		case	NVEDIT_CMD_ROOT:
			// (char *)( ((UINT32 *)data)[0] ), name
			// (UINT32)( ((UINT32 *)data)[1] ), size
			// (VOID *)( ((UINT32 *)data)[2] ), value
			node = nvedit_tree_find_name_func(nvedit->tokenbuffer, nvedit->root, (char *)( ((UINT32 *)data)[0] ) );
			if(node != NULL){
				nvedit->find = node ;
				break;
			}
			node = nvedit_tree_add_func(nvedit_check_room(nvedit), nvedit->root, (char *)( ((UINT32 *)data)[0] ), (UINT32)( ((UINT32 *)data)[1] ), 0 );
			if( (UINT32)( ((UINT32 *)data)[1] ) != 0 ){
				status = nvedit_node_fill_func( node, (VOID *)( ((UINT32 *)data)[2] ) );
			}else{
				status = TRUE;
			}
			break;

		case	NVEDIT_CMD_ADD:
			// (char *)( ((UINT32 *)data)[0] ), parent name
			// (char *)( ((UINT32 *)data)[1] ), child name
			// (UINT32)( ((UINT32 *)data)[2] ), size
			// (VOID *)( ((UINT32 *)data)[3] ), value
			parent = nvedit_tree_find_name_func(nvedit->tokenbuffer,  nvedit->root, (char *)( ((UINT32 *)data)[0] ) );
			if(parent == NULL || ( parent->length & NVITEM_TYPE_MASK ) != NVITEM_NOD_MARK ){
				break;
			}
			node = nvedit_tree_find_name_func(nvedit->tokenbuffer,  parent, (char *)( ((UINT32 *)data)[1] ) );
			if(node != NULL){
				nvedit->find = node ;
				break;
			}
			node = nvedit_tree_add_func(nvedit_check_room(nvedit), parent, (char *)( ((UINT32 *)data)[1] ), (UINT32)( ((UINT32 *)data)[2] ), 0 );
			if( (UINT32)( ((UINT32 *)data)[2] ) != 0 ){
				status = nvedit_node_fill_func( node, (VOID *)( ((UINT32 *)data)[3] ) );
			}else{
				status = TRUE;
			}
			nvedit->find = node ;
			break;

		case	NVEDIT_CMD_DEL:
			// (char *)( ((UINT32 *)data)[0] ), node name
			// (UINT32)( ((UINT32 *)data)[1] ), flag
			node = nvedit_tree_find_name_func(nvedit->tokenbuffer,  nvedit->root, (char *)( ((UINT32 *)data)[0] ) );
			status = nvedit_tree_del_func( node );
			break;

		case	NVEDIT_CMD_FIND:
			// (char *)( ((UINT32 *)data)[0] ), node name
			// (VOID *)( ((UINT32 *)data)[1] ), return node pointer
			// (VOID *)( ((UINT32 *)data)[2] ), return node size
			NVEDIT_DBG_PRINTF(" [%s] NVEDIT_CMD_FIND(root:%s data:%s) \r\n", __func__, nvedit->root->name, (char *)( ((UINT32 *)data)[0]));
			node = nvedit_tree_find_name_func(nvedit->tokenbuffer,  nvedit->root, (char *)( ((UINT32 *)data)[0] ) );
			if (node != NULL) {
				nvedit->find = node ;
				( ((UINT32 *)data)[1] ) = (UINT32)( node ) ;
				( ((UINT32 *)data)[2] ) = (UINT32)( node->length ) ;
				status = TRUE;
				NVEDIT_DBG_PRINTF(" [%s] NVEDIT_CMD_FIND(node->name:%s node->value:%s length:%d) \r\n",
								__func__, node->name, node->value, node->length & NVITEM_LEN_MASK);
			}
			break;

		case	NVEDIT_CMD_PRINT:
			if( data == NULL || (VOID *)( ((UINT32 *)data)[0] ) == NULL ){
				NVEDIT_DBG_PRINTF(" [%s] NVEDIT_CMD_PRINT(root->name:%s root->value:%s root->length:%d) \r\n",
						__func__, nvedit->root->name, nvedit->root->value, nvedit->root->length & NVITEM_LEN_MASK);
				/* print all */
				status = nvedit_tree_print_func( nvedit, nvedit->root );
			}
			else {
				//NVEDIT_TREE_ITEM *node ;
				node = (NVEDIT_TREE_ITEM *)( ((UINT32 *)data)[0] );
				NVEDIT_DBG_PRINTF(" [%s] NVEDIT_CMD_PRINT(node->name:%s node->value:%s node->length:%d) \r\n",
						__func__, node->name, node->value, node->length);
				status = nvedit_tree_print_func( nvedit, node );
			}
			break;
#endif //SUPPORT_NVEDIT_FULL

		case	NVEDIT_CMD_CHANGE_BASE:
			// (VOID *)( ((UINT32 *)data)[0] ), device type
			// (VOID *)( ((UINT32 *)data)[1] ), base address
			status = nvedit_change_base_func(nvedit, ((UINT32 *)data)[0],  ((UINT32 *)data)[1] );
			break;

		case	NVEDIT_CMD_GETCOUNT:
			// (VOID *)( ((UINT32 *)data)[0] ), device type
			// (VOID *)( ((UINT32 *)data)[1] ), return base pointer
			// (VOID *)( ((UINT32 *)data)[2] ), return count pointer
			status = nvedit_get_base_func(nvedit, ((UINT32 *)data)[0],  &(((UINT32 *)data)[1]), &(((UINT32 *)data)[2]) );
			break;

		case	NVEDIT_CMD_RESETCOUNT:
			nvedit->update = 0;
			status = TRUE;
			break;

		case	NVEDIT_CMD_SETSIZE:
			// (VOID *)( ((UINT32 *)data)[0] ), pool size
			if(nvedit->mempool == NULL ){
				nvedit->poolsiz = ((UINT32 *)data)[0];
				status = TRUE;
			}
			break;

#ifdef	SUPPORT_NVRAM_SECU
		case	NVEDIT_CMD_SECURE:
			switch(((UINT32 *)data)[0]){
			case	ASSET_ROOT_KEY:
			case	ASSET_KCP_KEY:
			case	ASSET_KPICV_KEY:
				nvedit->secure = ((UINT32 *)data)[0]; // asset id
				NVEDIT_DBGITM_PRINTF("secure-on: %d\n", nvedit->secure);
				status = TRUE;
				break;
			default:
				status = FALSE;
				break;
			}
			//}
			break;
#endif	//SUPPORT_NVRAM_SECU

		default:
			NVEDIT_DBG_PRINTF("NVEDIT: Unknown %d \r\n", cmd);
			break;
	}

	return status;
}

#ifndef	 SUPPORT_NVEDIT_FULL
/******************************************************************************
 *   nvedit_ioctl_cmd_litefind( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int nvedit_ioctl_cmd_litefind(NVEDIT_HANDLER_TYPE *nvedit, UINT32 *data)
{
	int status;
	NVEDIT_HASH_KEY_DECLARE(hash);

	status = FALSE;
	if(nvedit->workbuf == NULL){
		nvedit->workbuf = NVEDIT_WBUF_MALLOC(sizeof(UINT8)* NVEDIT_WORBUF_MAX );
		NVEDIT_WBUF_INIT(nvedit->workbuf);
	}

	if(nvedit->workbuf != NULL){
		status = nvedit_tree_load_func(nvedit, data[0],  data[1]);

		if(status == TRUE && (data[4]) != NULL){
			status  = nvedit_tree_litefind_func(nvedit->tokenbuffer
						, nvedit, (char *)( data[2] )
						, (UINT32 *)(data[3])
						, (VOID *)(data[4])
						, NULL );
		}
		else if (status == TRUE && (data[4]) == NULL) {
			void *pnode;

			status  = nvedit_tree_litefind_func(nvedit->tokenbuffer
						, nvedit, (char *)( data[2] )
						, (UINT32 *)(data[3])
						, NULL
						, (VOID *)&pnode );

			if( status == TRUE && pnode != NULL ){
				char *sibling = (char *)pnode;
				// extract name
				while(*sibling != '\0'){
					sibling ++;
				}
				// extract "nil"
				sibling ++;
				// extract "hash"
				NVEDIT_HASH_KEY_UNPACK_UINT32(sibling, 0, hash);
				// extract "datlen"
				sibling +=sizeof(UINT32);
				data[4] = (UINT32)sibling;
			}
		}
	}
	return status;
}

/******************************************************************************
 *   nvedit_ioctl_cmd_liteupdate( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int nvedit_ioctl_cmd_liteupdate(NVEDIT_HANDLER_TYPE *nvedit, UINT32 *data)
{
	int status;

	status = FALSE;
	if(nvedit->workbuf == NULL){
		nvedit->workbuf = NVEDIT_WBUF_MALLOC(sizeof(UINT8)* NVEDIT_WORBUF_MAX );
		NVEDIT_WBUF_INIT(nvedit->workbuf);
	}

	if(nvedit->workbuf != NULL){
		void *pnode;
		status = nvedit_tree_load_func(nvedit, data[0],  data[1]);

		if(status == TRUE){
			status  = nvedit_tree_litefind_func(nvedit->tokenbuffer, nvedit, (char *)( data[2] )
						, (UINT32 *)(data[3]), NULL, (VOID *)&pnode );
		}
		if( status == TRUE && pnode != NULL ){
			status  = nvedit_tree_liteupdate_func(nvedit, pnode, (VOID *)(data[4]));
		}
		if(status == TRUE){
			status = nvedit_tree_save_func(nvedit, data[0],  data[1], FALSE);
		}

		if(status == TRUE){
			nvedit->update = nvedit->update + 1;
		}
	}
	return status;
}
#endif	 //!SUPPORT_NVEDIT_FULL

/******************************************************************************
 *   NVEDIT_READ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	NVEDIT_READ(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	NVEDIT_HANDLER_TYPE	*nvedit;
	NVEDIT_TREE_ITEM	*node;
	int	status;

	if(handler == NULL || addr == 0 ){
		return 0;
	}

	nvedit = (NVEDIT_HANDLER_TYPE *)handler ;
	node   = (NVEDIT_TREE_ITEM *)addr;

	if( ( node->length & NVITEM_LEN_MASK ) > p_dlen){
		return 0;
	}


	status = FALSE;
	if( nvedit_tree_find_node_func(nvedit, node) != NULL){
		switch((node->length & NVITEM_TYPE_MASK)){
			case 	NVITEM_VAR_MARK:
				NVEDIT_STRNCPY((char *)p_data, ((char *)node->value), p_dlen );
				status = TRUE;
				break;
			case	NVITEM_NOD_MARK:
				break;
			default:
				switch(node->length){
				case	sizeof(UINT8):
					*((UINT8 *)p_data) = *((UINT8 *)node->value);
					status = TRUE;
					break;
				case	sizeof(UINT16):
					*((UINT16 *)p_data) = *((UINT16 *)node->value);
					status = TRUE;
					break;
				case	sizeof(UINT32):
					*((UINT32 *)p_data) = *((UINT32 *)node->value);
					status = TRUE;
					break;
				default:
					DRIVER_MEMCPY(p_data, node->value, (node->length & NVITEM_LEN_MASK) );
					status = TRUE;
					break;
				}
				break;
		}
	}


	return (status == FALSE) ? 0 : (node->length) ;
}

/******************************************************************************
 *   NVEDIT_WRITE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	NVEDIT_WRITE(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	NVEDIT_HANDLER_TYPE	*nvedit;
	NVEDIT_TREE_ITEM	*node;
	int	status;

	if(handler == NULL || addr == 0 ){
		return 0;
	}

	nvedit = (NVEDIT_HANDLER_TYPE *)handler ;
	node   = (NVEDIT_TREE_ITEM *)addr;

	if( nvedit_tree_find_node_func(nvedit, node) == NULL){
		return 0;
	}

	switch((node->length & NVITEM_TYPE_MASK)){
		case	NVITEM_VAR_MARK:
			if( ( node->length & NVITEM_LEN_MASK ) < p_dlen){
				void	*value;
				int	oldlen;

				NVEDIT_DBG_PRINTF("nv.write:change len(%d > %d) \r\n", (node->length & NVITEM_LEN_MASK), p_dlen);

				value = NVEDIT_DRV_MALLOC(NVEDIT_ALIGN_SIZE((p_dlen+1) & NVITEM_LEN_MASK));
				if( value == NULL ){
					NVEDIT_DBG_PRINTF("nvedit: write alloc error \r\n");
					return 0;
				}

				NVEDIT_DRV_FREE(node->value);
				node->value = value;

				oldlen = (int)(node->length & NVITEM_LEN_MASK);
				node->length = NVITEM_VAR_MARK | ((p_dlen+1) & NVITEM_LEN_MASK);

				// len update
				nvedit_tree_lenupdate(4/*+*/, node, oldlen );
			}
			break;
		case	NVITEM_NOD_MARK:
			return 0;
		default:
			if( ( node->length & NVITEM_LEN_MASK ) < p_dlen){
				return 0;
			}
			break;
	}

	status = nvedit_node_fill_func( node, p_data );


	return (status == FALSE) ? 0 : p_dlen;
}

/******************************************************************************
 *   NVEDIT_CLOSE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	NVEDIT_CLOSE(HANDLE handler)
{
	NVEDIT_HANDLER_TYPE	*nvedit;

	if(handler == NULL){
		return FALSE;
	}
	nvedit = (NVEDIT_HANDLER_TYPE *)handler ;

	nvedit->mempool = NULL ;

#ifdef	SUPPORT_NVRAM_NOR
	if( nvedit->nor_device != NULL )
	{
		NOR_CLOSE(nvedit->nor_device);
	}
#endif	//SUPPORT_NVRAM_NOR
#ifdef	SUPPORT_NVRAM_SFLASH
	if( nvedit->sflash_device != NULL )
	{
		SFLASH_CLOSE(nvedit->sflash_device);
	}
#endif	//SUPPORT_NVRAM_SFLASH

	if ( _nvedit_instance[NVEDIT_INSTANCE] > 0) {
		_nvedit_instance[NVEDIT_INSTANCE] -- ;

		if( _nvedit_instance[NVEDIT_INSTANCE] == 0) {

#ifdef	NVEDIT_WBUF_SUPPORT
			// skip
#else
			NVEDIT_DRV_FREE(_mempool_buffer[NVEDIT_INSTANCE]);
			_mempool_buffer[NVEDIT_INSTANCE] = NULL ;
#endif //NVEDIT_WBUF_SUPPORT

		}
	}

	NVEDIT_DRV_FREE(handler);
	return TRUE;
}

/******************************************************************************
 *   nvedit_node_make_func( )
 *
 *  Purpose :    node function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	NVEDIT_TREE_ITEM *nvedit_node_make_func(UINT32 available, char *name, UINT32 datsiz, UINT32 hash)
{
	NVEDIT_TREE_ITEM *node;
	UINT32 namesiz;

	if( name == NULL ){
		return NULL;
	}

	namesiz = DRIVER_STRLEN(name) ;
	namesiz = namesiz + 1;
	namesiz = ( namesiz > NVITEM_NAME_MAX ) ? (NVITEM_NAME_MAX) : namesiz ;

	if( (namesiz + sizeof(NVEDIT_TREE_ITEM) + (datsiz & NVITEM_LEN_MASK)) > available ){ // overflow
		NVEDIT_DBGITM_PRINTF("make: overflow %d vs %d \r\n"
			, available
			,(namesiz + sizeof(NVEDIT_TREE_ITEM) + (datsiz & NVITEM_LEN_MASK))
			);
		return NULL;
	}

	node = (NVEDIT_TREE_ITEM *)NVEDIT_DRV_MALLOC(sizeof(NVEDIT_TREE_ITEM));

	if( node == NULL ){
		NVEDIT_DBGITM_PRINTF("make: node alloc error \r\n");
		return NULL;
	}

	DRIVER_MEMSET(node, 0x00, sizeof(NVEDIT_TREE_ITEM));

	node->name = (char *) NVEDIT_DRV_MALLOC(NVEDIT_ALIGN_SIZE(namesiz)) ;

	if( node->name == NULL ){
		NVEDIT_DBGITM_PRINTF("make: name alloc error \r\n");
		NVEDIT_DRV_FREE(node);
		return NULL;
	}

	NVEDIT_STRNCPY(node->name, name, namesiz);	// IAR issue

	NVEDIT_HASH_KEY_MAKE(node, name, hash);

	switch((datsiz & NVITEM_TYPE_MASK)){
		case	NVITEM_NOD_MARK:
			node->length = NVITEM_NOD_MARK|(datsiz & NVITEM_LEN_MASK);
			break;

		case	NVITEM_VAR_MARK:
		default:
			node->length = datsiz;
			if(((node->length) & NVITEM_LEN_MASK) > 0){
				node->value = NVEDIT_DRV_MALLOC( NVEDIT_ALIGN_SIZE((node->length) & NVITEM_LEN_MASK) ) ;
				if( node->value == NULL ){
					NVEDIT_DBGITM_PRINTF("make: value alloc error \r\n");
					NVEDIT_DRV_FREE(node->name);
					NVEDIT_DRV_FREE(node);
					return NULL;
				}
				//DRIVER_MEMSET(node->value, 0x00, (datsiz & NVITEM_LEN_MASK));
			}
			break;
	}

	return node;
}

/******************************************************************************
 *   nvedit_node_free_func( )
 *
 *  Purpose :    node function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	void nvedit_node_free_func(NVEDIT_TREE_ITEM *node)
{
	if(node == NULL){
		return ;
	}

	if(node->name != NULL){
		NVEDIT_DBGITM_PRINTF("del %p - \"%s\", %p \r\n", node, node->name, node->value);
		NVEDIT_DRV_FREE(node->name);
	}else{
		NVEDIT_DBGITM_PRINTF("del %p - \"%s\", %p \r\n", node, node->name, node->value);
	}

	if(node->value != NULL){
		NVEDIT_DRV_FREE(node->value);
	}
	NVEDIT_DRV_FREE(node);
}

/******************************************************************************
 *   nvedit_node_fill_func( )
 *
 *  Purpose :    node function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_node_fill_func(NVEDIT_TREE_ITEM *node, VOID *data)
{
	if(node == NULL){
		return FALSE;
	}

	switch((node->length & NVITEM_TYPE_MASK)){
		case	NVITEM_VAR_MARK:
			if( (node->length & NVITEM_LEN_MASK) == 0 || node->value == NULL){
				return FALSE;
			}
			{
				unsigned int realen, oldlen;

				// double check
				realen = DRIVER_STRLEN((char *)data);
				realen = realen + 1;

				if( realen < (node->length & NVITEM_LEN_MASK) ){
					NVEDIT_DBGITM_PRINTF("nv.fill: mismatch %d < %d \r\n", realen, (node->length & NVITEM_LEN_MASK));

					oldlen = (node->length & NVITEM_LEN_MASK);
					node->length = (node->length & NVITEM_TYPE_MASK) | realen ;

					// len update
					nvedit_tree_lenupdate(3/*-*/, node, (int)oldlen );
				}else{
					NVEDIT_DBGITM_PRINTF("nv.fill: mismatch %d >= %d \r\n", realen, (node->length & NVITEM_LEN_MASK));
					realen = (node->length & NVITEM_LEN_MASK);
				}

				NVEDIT_STRNCPY((char *)node->value, (char *)data, realen );
			}
			break;
		case	NVITEM_NOD_MARK:
			break;
		default:
			if( (node->length & NVITEM_LEN_MASK) == 0 || node->value == NULL){
				return FALSE;
			}
			DRIVER_MEMCPY(node->value, data, (node->length & NVITEM_LEN_MASK) );
			break;
	}

	NVEDIT_DBGITM_PRINTF("nv.fill: ok %s [%x] %s \r\n"
			, (node->name), node->length
			, ((node->parent == NULL)? "nil" : node->parent->name)
			);

	return TRUE;
}

/******************************************************************************
 *   nvedit_tree_init_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	NVEDIT_TREE_ITEM *nvedit_tree_init_func(UINT32 available, NVEDIT_TREE_ITEM **node)
{
	NVEDIT_TREE_ITEM	*root;

	if( node == NULL || *node != NULL){
		return NULL;
	}

	if( (root = nvedit_node_make_func(available, "root", NVITEM_NOD_MARK, 0)) != NULL){
		*node = root;	// assign
		NVEDIT_DBGITM_PRINTF(" >> node_make root:0x%x \r\n" , root);
	}

	return root;
}

/******************************************************************************
 *   nvedit_tree_add_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	NVEDIT_TREE_ITEM *nvedit_tree_add_func(UINT32 available, NVEDIT_TREE_ITEM *parent, char *name, UINT32 datsiz, UINT32 hash)
{
	NVEDIT_TREE_ITEM	*child, *sibling;

	if( parent == NULL ){
		return NULL;
	}

	if( (child = nvedit_node_make_func((available&(~NVITEM_HASH_MARK)), name, datsiz, hash)) != NULL){
		if( parent->child == NULL ){
			// first child
			parent->child = child  ;	// link
			child->parent = parent ;	// link
		}else{
			sibling = parent->child;
			while(sibling->next != NULL){
				sibling = sibling->next ;
			}

			sibling->next = child ;		// link
			child->parent = parent ;	// link
		}

		// len update
		if((available&NVITEM_HASH_MARK) == 0 ){
			nvedit_tree_lenupdate(1/*+*/, child, 0);
		}
	}

	return child;
}

/******************************************************************************
 *   nvedit_tree_del_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_del_func(NVEDIT_TREE_ITEM *node)
{
	NVEDIT_TREE_ITEM	*child, *prev, *next, *parent;

	if( node == NULL || node->parent == NULL ){
		return FALSE;
	}

	parent = node->parent ;
	prev   = NULL ;
	next   = node->next ;
	child  = node->child ;

	if(parent->child == node){
		// first child
		parent->child = next ;	// replace
	}
	else{
		// not first child
		for( prev = parent->child; prev != NULL; prev = prev->next ){
			if( prev->next == node ){
				break;
			}
		}

		if(prev != NULL){
			prev->next = next ; // replace
		}
	}

	// len update
	nvedit_tree_lenupdate(2/*-*/, node, 0);

	node->parent = NULL ;	// unlink
	node->next   = NULL ;	// unlink

	while(child != NULL){
		// search childs
		if(child->child != NULL ){
			child = child->child ;
			continue;
		}

		// search first
		if( child->next != NULL ){
			child->parent->child = child->next; // replace
			next = child->next ;
		}else{
			child->parent->child = NULL; // replace
			next = (child->parent == node) ? NULL : child->parent ;
		}

		// free node
		NVEDIT_DBGITM_PRINTF("del subnode %s (nxt %p) \r\n", child->name, next);
		nvedit_node_free_func(child);

		child = next ;
	}

	NVEDIT_DBGITM_PRINTF("del node %s \r\n", child->name);
	nvedit_node_free_func(node);

	return TRUE;
}

/******************************************************************************
 *   nvedit_tree_find_name_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	NVEDIT_TREE_ITEM *nvedit_tree_find_name_func(char *token, NVEDIT_TREE_ITEM *parent, char *name)
{
	NVEDIT_TREE_ITEM	*child, *sibling;
	UINT32	idx , wildcard;
	char	*nameoffset;
	NVEDIT_HASH_KEY_DECLARE(hash);

	if( parent == NULL || name == NULL ){
		return NULL;
	}

	sibling = NULL;
	nameoffset = name ;
	child = parent->child ;
	do{
		// get token
		idx = 0;
		while(*nameoffset != '.' && *nameoffset != '\0'){
			if(idx < NVITEM_NAME_MAX){
				token[idx] = *nameoffset ;
				idx++;
			}
			nameoffset ++;
		}
		if(*nameoffset == '.'){
			nameoffset ++;
		}
		token[idx] = '\0';

		// check wildcard (asterisk)
		wildcard = FALSE;
		if( idx >= 2 && token[idx-1] == '*'){
			idx --;
			token[idx] = '\0';
			wildcard = TRUE;
		}

		// find child
		sibling = NULL;
		NVEDIT_HASH_KEY_GEN(token, hash);

		for( ; child != NULL ; child = child->next ){
			if( wildcard == FALSE
			    || (child->length & NVITEM_TYPE_MASK) == NVITEM_NOD_MARK ){

				NVEDIT_DBGITM_PRINTF("find:node-%s vs %s [%d] \r\n"
						, child->name, token
						, NVEDIT_HASH_KEY_COMP_CSEN(child, hash)
						);

				if((child->name != NULL)
				&& (NVEDIT_HASH_KEY_COMP_CSEN(child, hash) == TRUE)
				&& (DRIVER_STRCMP(child->name, token) == 0)){
					// find token node
					sibling = child ;
					break;
				}
			}else{

				NVEDIT_DBGITM_PRINTF("find:leaf-%s vs %s [%d] \r\n"
						, child->name, token
						, NVEDIT_HASH_KEY_COMP_CSEN(child, hash)
						);

				if((child->name != NULL)
				&& (DRIVER_STRNCMP(child->name, token, idx) == 0)){
					// find token node
					sibling = child ;
					break;
				}
			}
		}

		if(sibling == NULL){
			// not matched
			break;
		}

		child = sibling->child ;

	}while(*nameoffset != '\0');

	return sibling;
}

/******************************************************************************
 *   nvedit_tree_find_node_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	NVEDIT_TREE_ITEM *nvedit_tree_find_node_func(NVEDIT_HANDLER_TYPE *nvedit, NVEDIT_TREE_ITEM *node)
{
	if( nvedit->find == node ){
		return (nvedit->find);
	}
	return NULL;
}

/******************************************************************************
 *   nvedit_tree_print_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_print_func(NVEDIT_HANDLER_TYPE *nvedit, NVEDIT_TREE_ITEM *parent)
{
	NVEDIT_TREE_ITEM	*child;
	UINT32	depth;
	char	*depthmark, keyspace[24];
	int 	delimeterlen = 20;

	if( parent == NULL){
		return FALSE;
	}

	child = parent->child ;

	// scratch pad
	depthmark = (char *)&(nvedit->tokenbuffer[0]);

	depth = 0;
	depthmark[depth] = '\0' ;
	DRIVER_MEMSET(keyspace, '.', delimeterlen);
	keyspace[delimeterlen] = '\0';

	NVEDIT_DBG_PRINTF("Parent %s \r\n", parent->name);

	PRINTF("\nTotal length (%d)\r\n", (parent->length & NVITEM_LEN_MASK) );

	while (child != NULL) {

		if(child->child == NULL){
			int space_len = 0;

			space_len = (int)DRIVER_STRLEN(child->name);
			space_len = (space_len >= delimeterlen) ? 0 : (delimeterlen-space_len);
			keyspace[space_len] = '\0'; // nil

			switch((child->length & NVITEM_TYPE_MASK)){
				case 	NVITEM_VAR_MARK:
					PRINTF("%s%s (STR,%02d) %s ", depthmark, child->name
						, (child->length & NVITEM_LEN_MASK)
						, keyspace );
					PRINTF("%s ", child->value);
					PRINTF("\r\n");
					break;

				case	NVITEM_NOD_MARK:
					PRINTF("\n%s%s (%02d)\r\n"
						, depthmark, child->name
						, (child->length & NVITEM_LEN_MASK) );
					break;
				default:
					switch(child->length){
						case	sizeof(UINT8):
								PRINTF("%s%s (UINT8)  %s %c(h%02x)\r\n"
									, depthmark, child->name, keyspace
									, (char)*((UINT8 *)child->value), *((UINT8 *)child->value));
								break;
						case	sizeof(UINT16):
								PRINTF("%s%s (UINT16) %s %d(h%04x)\r\n"
									, depthmark, child->name, keyspace
									, *((UINT16 *)child->value), *((UINT16 *)child->value));
								break;
						case	sizeof(UINT32):
								PRINTF("%s%s (UINT32) %s %d(h%08x)\r\n"
									, depthmark, child->name, keyspace
									, *((UINT32 *)child->value), *((UINT32 *)child->value));
								break;

						default:
								PRINTF("%s%s (unknown type, h%08x)\r\n"
									, depthmark, child->name, child->length);
								DRV_DBG_DUMP(0, child->value, (child->length & NVITEM_LEN_MASK) );
								break;
					}
					break;
			}

			keyspace[space_len] = '.'; // restore
		}
		else {
			PRINTF("\n%s%s (%02d)\r\n", depthmark, child->name, (child->length & NVITEM_LEN_MASK) );
		}

		if(child->child != NULL){
			child = child->child ;
			depthmark[depth++] = ' ' ;
			depthmark[depth++] = ' ' ;
			depthmark[depth++] = ' ' ;
			depthmark[depth] = '\0' ;
		}
		else if (child->next != NULL) {
			child = child->next ;
		}
		else {
			child = child->parent;

			if( child == parent ) {
				child = NULL;
				break;
			}
			else {
				depth = depth - 3;
				depthmark[depth] = '\0' ;
			}

			// Next & Upper
			while (child != NULL) {
				if (child->next != NULL) {
					child = child->next ;
					break;
				}
				else {
					child = child->parent;
					if ( child == parent ) {
						child = NULL;
						break;
					}
					else {
						depth = depth - 3;
						depthmark[depth] = '\0' ;
					}
				}
			}
		}
	}

	return TRUE;
}

/******************************************************************************
 *   nvedit_pack_string( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_pack_string(UINT8 *buffer, char *string)
{
	while(*string != '\0') {
		*buffer++=(UINT8)(*string++);
	}
	*buffer++='\0';
	return buffer;
}

/******************************************************************************
 *   nvedit_pack_string_data( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_pack_string_data(UINT8 *buffer, char *string, UINT32 len)
{
	while(*string != '\0' && len > 0 ) {
		*buffer++=(UINT8)(*string++);
		len--;
	}
	*buffer++='\0';
	return buffer;
}

/******************************************************************************
 *   nvedit_pack_uint8( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_pack_uint8(UINT8 *buffer, UINT8 data)
{
	*buffer++=(UINT8)(data);
	return buffer;
}

/******************************************************************************
 *   nvedit_pack_uint16( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_pack_uint16(UINT8 *buffer, UINT16 data)
{
	*buffer++=(UINT8)((data >> 8)&0x0FF);
	*buffer++=(UINT8)((data >> 0)&0x0FF);
	return buffer;
}

/******************************************************************************
 *   nvedit_pack_uint32( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_pack_uint32(UINT8 *buffer, UINT32 data)
{
	*buffer++=(UINT8)((data >> 24)&0x0FF);
	*buffer++=(UINT8)((data >> 16)&0x0FF);
	*buffer++=(UINT8)((data >> 8)&0x0FF);
	*buffer++=(UINT8)((data >> 0)&0x0FF);
	return buffer;
}

/******************************************************************************
 *   nvedit_pack_var( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_pack_var(UINT8 *buffer, void *data, UINT32 len)
{
	UINT8 *string;

	string = (UINT8 *)data;

	while(len-- > 0){
		*buffer++=*string++;
	}
	return buffer;
}

/******************************************************************************
 *   nvedit_tree_pack_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_pack_func(NVEDIT_HANDLER_TYPE	*nvedit, NVEDIT_TREE_ITEM *parent, UINT8 *buffer)
{
	DA16X_UNUSED_ARG(nvedit);

	NVEDIT_TREE_ITEM	*child;

	if( parent == NULL || buffer == NULL){
		NVEDIT_DBG_PRINTF("nvedit_tree_pack_func - failed \r\n");
		return FALSE;
	}

	child = parent->child ;

	while(child != NULL){
		buffer = nvedit_pack_string(buffer, child->name);
		NVEDIT_HASH_KEY_PACK_UINT32(buffer, child);
		buffer = nvedit_pack_uint32(buffer, child->length);

		if(child->child == NULL){
			switch((child->length & NVITEM_TYPE_MASK)){
				case 	NVITEM_VAR_MARK:
						buffer = nvedit_pack_string_data(buffer, (char *)(child->value)
									, (child->length & NVITEM_LEN_MASK));
						break;
				case	NVITEM_NOD_MARK:
						break;
				default:
					switch((child->length & NVITEM_LEN_MASK)){
						case	sizeof(UINT8):
								buffer = nvedit_pack_uint8(buffer, *((UINT8 *)(child->value)));
								break;
						case	sizeof(UINT16):
								buffer = nvedit_pack_uint16(buffer, *((UINT16 *)(child->value)));
								break;
						case	sizeof(UINT32):
								buffer = nvedit_pack_uint32(buffer, *((UINT32 *)(child->value)));
								break;
						default:
								buffer = nvedit_pack_var(buffer, child->value
											, ( child->length & NVITEM_LEN_MASK ) );
								break;
					}
					break;
			}
		}

		if(child->child != NULL){
			// Inner
			child = child->child ;
		}else
		if(child->next != NULL){
			child = child->next ;
		}else{
			child = child->parent;

			if( child == parent ){
				// TOP
				child = NULL;
				break;
			}else{
			}

			// Next & Upper
			while(child != NULL){
				if(child->next != NULL){
					child = child->next ;
					break;
				}else{
					child = child->parent;
					if( child == parent ){
						child = NULL;
						break;
					}else{
					}
				}
			}

		}
	}

	return TRUE;
}

/******************************************************************************
 *   nvedit_unpack_string( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_unpack_string(char *string, UINT8 *buffer, UINT32 len)
{
	len ++;

	while((char)*buffer != '\0' && len > 0) {
		*string++ = (char)(*buffer++);
		len--;
	}
	*string ='\0';

	while((char)*buffer != '\0') {
		buffer++;
	}

	buffer++;
	return buffer;
}

/******************************************************************************
 *   nvedit_unpack_uint8( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_unpack_uint8(UINT8 *data, UINT8 *buffer)
{
	NVEDIT_DBGITM_PRINTF("unpack8[%02x] \r\n", *buffer);
	*data = *buffer++;
	return buffer;
}

/******************************************************************************
 *   nvedit_unpack_uint16( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_unpack_uint16(UINT16 *data, UINT8 *buffer)
{
	NVEDIT_DBGITM_PRINTF("unpack16[%02x]", *buffer);
	*data = (((*buffer++) << 8) & 0xFF00);
	NVEDIT_DBGITM_PRINTF("[%02x] \r\n", *buffer);
	*data = *data | (((*buffer++) << 0) & 0x0FF);
	return buffer;
}

/******************************************************************************
 *   nvedit_unpack_uint32( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_unpack_uint32( UINT32 *data, UINT8 *buffer)
{
	NVEDIT_DBGITM_PRINTF("unpack32[%02x]", *buffer);
	*data = (((*buffer++) << 24) & 0xFF000000);
	NVEDIT_DBGITM_PRINTF("[%02x]", *buffer);
	*data = *data | (((*buffer++) << 16) & 0x00FF0000);
	NVEDIT_DBGITM_PRINTF("[%02x]", *buffer);
	*data = *data | (((*buffer++) <<  8) & 0x0000FF00);
	NVEDIT_DBGITM_PRINTF("[%02x] \r\n", *buffer);
	*data = *data | (((*buffer++) <<  0) & 0x000000FF);
	return buffer;
}

/******************************************************************************
 *   nvedit_unpack_var( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT8	*nvedit_unpack_var(void *data, UINT8 *buffer, UINT32 len)
{
	UINT8 *string;

	string = (UINT8 *)data;

	while(len-- > 0){
		*string++ = *buffer++;
	}
	return buffer;
}

/******************************************************************************
 *   nvedit_tree_unpack_func( )
 *
 *  Purpose :    tree function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_unpack_func(NVEDIT_HANDLER_TYPE	*nvedit, NVEDIT_TREE_ITEM *parent)
{
	UINT32	status;
	UINT32	depth, namlen, nodlen;
	UINT32  *depthmark;
	NVEDIT_TREE_ITEM  *parentnode;
	NVEDIT_TREE_ITEM  *child;
	char	*name;
	UINT8	*buffer;
	NVEDIT_HASH_KEY_DECLARE(hash);

	if( parent == NULL || parent->child != NULL ){
		NVEDIT_DBG_PRINTF("unpack - failed \r\n");
		return FALSE;
	}

	// scratch pad
	depthmark = (UINT32 *)&(nvedit->tokenbuffer[0]);

	depth  = 0;
	depthmark[depth] = (parent->length & NVITEM_LEN_MASK);
	parentnode = parent ;
	child  = NULL;

	status = TRUE;
	buffer = &(nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)]);

	do {
		// extract NAME
		name = (char *)(buffer);
		namlen = (name == NULL) ? 1 :  (DRIVER_STRLEN(name) + 1);
		// extract Hash
		NVEDIT_HASH_KEY_UNPACK_UINT32(buffer, namlen, hash);
		// extract LEN
		buffer = nvedit_unpack_uint32( (UINT32 *)&nodlen, buffer );

		if( (name == NULL) || (namlen == 1)  || ((nodlen & NVITEM_LEN_MASK) == NVITEM_LEN_MASK ) )
		{
			NVEDIT_DBG_PRINTF("nv.unpack failed\r\n");
			status  = FALSE;
			break;
		}

		NVEDIT_DBG_PRINTF("node unpack - %s (%d) para %s - (%08x) = room %d \r\n"
				, name, namlen, parentnode->name, nodlen, depthmark[depth]);

		// Make NODE
		child = nvedit_tree_add_func((NVITEM_HASH_MARK|(nvedit->poolsiz)), parentnode, name, nodlen, NVEDIT_HASH_KEY_GET(hash));
		if(child == NULL){
			NVEDIT_DBG_PRINTF("node unpack failed : %s (%d)", name, nodlen);
			status  = FALSE;
			break;
		}

		// Calc Length
		depthmark[depth] = depthmark[depth]
				- ( namlen + NVITEM_NODE_ADJSIZE + (child->length & NVITEM_LEN_MASK) );

		NVEDIT_DBG_PRINTF("node dump - %s [%x] (dep[%d]=%d) %x \r\n"
				, name, child->length, depth, depthmark[depth]
				, (nodlen & NVITEM_TYPE_MASK) );

		// fill up data
		switch((nodlen & NVITEM_TYPE_MASK)){
			case 	NVITEM_VAR_MARK:
				buffer = nvedit_unpack_string( (char *)(child->value), buffer
						, (child->length & NVITEM_LEN_MASK) );	// String Type
				break;
			case 	NVITEM_NOD_MARK:
				depth ++;
				depthmark[depth] = (child->length & NVITEM_LEN_MASK);
				parentnode = child ; // update Parent
				child = NULL;
				break;
			default:
				switch((child->length & NVITEM_LEN_MASK)){
					case	sizeof(UINT8):
						buffer = nvedit_unpack_uint8( (UINT8 *)child->value, buffer );
						break;
					case	sizeof(UINT16):
						buffer = nvedit_unpack_uint16( (UINT16 *)child->value, buffer );
						break;
					case	sizeof(UINT32):
						buffer = nvedit_unpack_uint32( (UINT32 *)child->value, buffer );
						break;
					default:
						buffer = nvedit_unpack_var( child->value, buffer, (nodlen & NVITEM_LEN_MASK) );
						break;
					}
				break;
		}

		while(depth > 0 && depthmark[depth] == 0){
			// Upper
			depth--;

			if( parentnode != NULL ){
				parentnode = parentnode->parent;
			}
		}

		NVEDIT_DBG_PRINTF("papa = %s depthmark[%d] = %d, depthmark[0] = %d\r\n"
				,parentnode->name, depth, depthmark[depth], depthmark[0]);

	} while(depth != 0 || depthmark[0] > 0);

	return status;
}

/******************************************************************************
 *   nvedit_check_room( )
 *
 *  Purpose :    check function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT32	nvedit_check_room(NVEDIT_HANDLER_TYPE *nvedit)
{
	UINT32	room;

	if (nvedit->root != NULL) {
		if (nvedit->poolsiz < (nvedit->root->length & NVITEM_LEN_MASK)) {
			room = 0;
		}
		else {
			room = nvedit->poolsiz - (nvedit->root->length & NVITEM_LEN_MASK);
		}
	}
	else {
		room = nvedit->poolsiz ;
	}

	if( room < ( sizeof(NVEDIT_TREE_ITEM) * 2 ) ){
		room = 0;
	}
	else {
		room = room - ( sizeof(NVEDIT_TREE_ITEM) * 2 ) ;
	}

	return room;
}

/******************************************************************************
 *   nvedit_check_room( )
 *
 *  Purpose :    check function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	void nvedit_tree_lenupdate(UINT32 flag, NVEDIT_TREE_ITEM *node, int offset)
{
	int	len ;
	NVEDIT_TREE_ITEM *parent;

	parent = node->parent ;

	switch(flag){
		case	4:	/* incr */
			len = ( node->length & NVITEM_LEN_MASK ) - offset ;
			break;
		case	3:	/* decr */
			len = offset - ( node->length & NVITEM_LEN_MASK ) ;
			len = -len ;
			break;
		case	2:	/* del */
			len = NVITEM_NODE_ADJSIZE
				+ DRIVER_STRLEN(node->name) + 1
				+ ( node->length & NVITEM_LEN_MASK ) ;
			len = -len ;
			break;
		case	1:	/* add */
			len =  NVITEM_NODE_ADJSIZE
				+ DRIVER_STRLEN(node->name) + 1
				+ ( node->length & NVITEM_LEN_MASK ) ;
			break;
		default:
			return;
	}


	while( parent != NULL ){
		parent->length = (parent->length & NVITEM_TYPE_MASK)
				| (UINT32)( (int)(parent->length & NVITEM_LEN_MASK)
				    + len ) ;

		NVEDIT_DBGITM_PRINTF("l.up: %s = %d ( %d ) flg %d \r\n"
				, parent->name
				, (parent->length & NVITEM_LEN_MASK), len
				, flag );

		parent = parent->parent ;
	}
}

/******************************************************************************
 *   nvedit_change_base_func( )
 *
 *  Purpose :    save function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_change_base_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_base )
{
	if( nvedit == NULL ){
		return FALSE;
	}

	NVEDIT_DBG_PRINTF("change_base : %d, %x \r\n", dev_type, dev_base);

	switch(dev_type){
#ifdef	SUPPORT_NVRAM_NOR
		case	NVEDIT_NOR:
			nvedit->nor_base = dev_base ;
			break;
#endif	//SUPPORT_NVRAM_NOR

#ifdef	SUPPORT_NVRAM_SFLASH
		case	NVEDIT_SFLASH:
			nvedit->sflash_base = dev_base ;
			break;
#endif	//SUPPORT_NVRAM_SFLASH

#ifdef	SUPPORT_NVRAM_RAM
		case	NVEDIT_RAM:
			nvedit->ram_base = dev_base ;
			break;
#endif	//SUPPORT_NVRAM_RAM

		default:
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *   nvedit_get_base_func( )
 *
 *  Purpose :    get function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_get_base_func(NVEDIT_HANDLER_TYPE *nvedit, UINT32 dev_type, UINT32 *dev_base, UINT32 *dev_count)
{
	if( nvedit == NULL ){
		return FALSE;
	}

	*dev_count = nvedit->update ;

	switch(dev_type){
#ifdef	SUPPORT_NVRAM_NOR
		case	NVEDIT_NOR:
			*dev_base = nvedit->nor_base;
			break;
#endif	//SUPPORT_NVRAM_NOR

#ifdef	SUPPORT_NVRAM_SFLASH
		case	NVEDIT_SFLASH:
			*dev_base = nvedit->sflash_base;
			break;
#endif	//SUPPORT_NVRAM_SFLASH

#ifdef	SUPPORT_NVRAM_RAM
		case	NVEDIT_RAM:
			*dev_base = nvedit->ram_base;
			break;
#endif	//SUPPORT_NVRAM_RAM

		default:
			return FALSE;
	}

	return TRUE;
}

#ifdef SUPPORT_NVRAM_FOOTPRINT
static int nvedit_put_footprint(NVEDIT_HANDLER_TYPE	*nvedit, UINT8 foot )
{
    int i;
    unsigned char *foot_buf;
    UINT32 ioctldata[2];

	extern int sflash_local_low_ioctl(HANDLE handler, UINT32 cmd , VOID *data );

    foot_buf = APP_MALLOC(0x1000);
    if (foot_buf == NULL)
        return -1;
    /*PRINTF("nvram foot 0x%x, address %08x\n", foot, nvedit->sflash_foot_base );*/
    // read sector
    SFLASH_READ(nvedit->sflash_device, nvedit->sflash_foot_base, foot_buf, 0x1000);
    // find empty space
    for (i = 0; i < 0x1000; i++)
    {
        if(foot_buf[i] == (0xff))
            break;
    }
    if (i == 0x1000)
    {
        // erase sector
        ioctldata[0] = nvedit->sflash_foot_base;
		ioctldata[1] = 0x1000;
        sflash_local_low_ioctl(nvedit->sflash_device, SFLASH_CMD_ERASE, ioctldata);
        i = 0;
    }
    SFLASH_WRITE(nvedit->sflash_device, nvedit->sflash_foot_base + i, &foot, 0x1);
    // write foot
    APP_FREE(foot_buf);
    return 0;
}
#endif


/******************************************************************************
 *   nvedit_tree_save_func( )
 *
 *  Purpose :    save function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_save_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_unit, UINT32 mode )
{
	HANDLE flash;
	int	pagesize, pageoffset, totalsize;
	UINT32 ioctldata[7];

	if( nvedit == NULL || nvedit->workbuf == NULL ){
		NVEDIT_DBG_PRINTF("save, null pointer \r\n");
		return FALSE;
	}

#ifdef	SUPPORT_NVEDIT_FULL
	if( mode == TRUE ){
		if( nvedit->root == NULL || nvedit->root->child == NULL ){
			NVEDIT_DBG_PRINTF("save, null node \r\n");
			return FALSE;
		}

		nvedit->table.magic = NVITEM_MAGIC_CODE;
		nvedit->table.total = (nvedit->root->length & NVITEM_LEN_MASK);
		nvedit->table.chksum = 0;

		NVEDIT_DBG_PRINTF("total size (%d) - %d/%d \r\n", nvedit->table.total, dev_type, dev_unit);

		DRIVER_MEMCPY( nvedit->workbuf, &(nvedit->table), sizeof(NVEDIT_TABLE_TYPE) );
		nvedit_tree_pack_func(nvedit, nvedit->root, &(nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)]) );

#ifdef	SUPPORT_NVRAM_SECU
		if( (nvedit->secure == ASSET_ROOT_KEY)
		    || (nvedit->secure == ASSET_KCP_KEY)
		    || (nvedit->secure == ASSET_KPICV_KEY)
		    ){
			UINT32 iastlen, pkglen ;
			UINT8	*assetpkg;

			iastlen = (nvedit->table.total + 15) & (~0x0f); // alignment
			assetpkg = CRYPTO_MALLOC(iastlen + 128);
			if( assetpkg != NULL ){
				pkglen	= DA16X_Secure_Asset_RuntimePack((AssetKeyType_t)(nvedit->secure)
							, /*0x60600C00*/ NVEDIT_ASSET_NONCE
						   	, (AssetUserKeyData_t *)(NULL), NVEDIT_ASSET_ID, "EnNVRAM"
							, &(nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)]), iastlen
							, assetpkg );

				DRIVER_MEMCPY( &(nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)])
						, assetpkg, pkglen);

				nvedit->table.reserved = NVITEM_SET_TYPE(nvedit->secure) | (pkglen & NVITEM_LEN_MASK);

				CRYPTO_FREE(assetpkg);
			}else {
				nvedit->table.reserved = 0xFFFFFFFF;
			}
		}else{
			nvedit->table.reserved = 0xFFFFFFFF;
		}
#endif	//SUPPORT_NVRAM_SECU
#ifdef SUPPORT_CHECK_CRC
		/*	CRC (XOR Checksum) */
		{
			uint16_t length = 0;
			if (nvedit->table.reserved == 0xFFFFFFFF) {
				length = nvedit->table.total - sizeof(NVEDIT_TABLE_TYPE);
			} else {
				length = nvedit->table.reserved & NVITEM_LEN_MASK;
			}
#ifdef SUPPORT_CHECK_CRC_16
			{
				uint16_t checksum = 0;
				checksum = nvedit_swcrc16(&nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)], length);
				if (nvedit->table.reserved == 0xFFFFFFFF) {
					nvedit->table.chksum = (checksum << 16) | 0x8000;
				} else {
					nvedit->table.chksum = (checksum << 16) | 0x8100;
				}
			}
#else
			{
				uint8_t checksum = 0;
			for(uint16_t i = 0; i < length; i++)
			{
				checksum ^= nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE) + i];
			}
			if (nvedit->table.reserved == 0xFFFFFFFF) {
				nvedit->table.chksum = checksum | 0x8000;
			} else {
				nvedit->table.chksum = checksum | 0x8100;
			}
		}
#endif /* SUPPORT_CHECK_CRC_16 */
		}
#endif /* SUPPORT_CHECK_CRC */
		DRIVER_MEMCPY( nvedit->workbuf, &(nvedit->table), sizeof(NVEDIT_TABLE_TYPE));
	} else {
		/* lite-update */
	}
#endif //SUPPORT_NVEDIT_FULL

	pagesize   = 4096;
	pageoffset = 0;

	switch(dev_type){
#ifdef		SUPPORT_NVRAM_NOR
		case	NVEDIT_NOR:
			if( nvedit->nor_device == NULL )
			{
				flash = NOR_CREATE(dev_unit);
				if(flash == NULL) {
					NVEDIT_DBG_PRINTF("nor create error \r\n");
					return FALSE;
				}
				NOR_INIT(flash);
#ifdef	BUILD_OPT_DA16200_ROMALL
				ioctldata[0] = TRUE;
				NOR_IOCTL(flash, NOR_SET_BOOTMODE, ioctldata);
#endif	//BUILD_OPT_DA16200_ROMALL
				nvedit->nor_device = flash ;
			}
			else
			{
				flash = nvedit->nor_device ;
			}

			//ioctldata[0] = NVEDIT_NOR_BASE;
			//ioctldata[1] = NVEDIT_WORBUF_MAX;
			//NOR_IOCTL(flash, NOR_SET_UNLOCK, ioctldata);

			NVEDIT_DBG_PRINTF("NOR base %x (%d) \r\n"
				, NVEDIT_NOR_BASE
				, ((nvedit->table.total & NVITEM_LEN_MASK)+sizeof(NVEDIT_TABLE_TYPE))
				);

			ioctldata[0] = NVEDIT_NOR_BASE;
			ioctldata[1] = NVEDIT_WORBUF_MAX;
			NOR_IOCTL(flash, NOR_SET_ERASE, ioctldata);

#ifdef	SUPPORT_NVRAM_SECU
			if( nvedit->table.reserved != 0xFFFFFFFF ){
				totalsize = (nvedit->table.reserved & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
			}else{
				totalsize = (nvedit->table.total & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
			}
#else	//SUPPORT_NVRAM_SECU
			totalsize = (nvedit->table.total & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
#endif	//SUPPORT_NVRAM_SECU
			for(pageoffset = 0
				; pageoffset < totalsize
				; pageoffset += pagesize){
				pagesize = ((pageoffset+pagesize) > totalsize)
					? (totalsize - pageoffset)
					: pagesize ;
				NOR_WRITE(flash, (NVEDIT_NOR_BASE+pageoffset), &(nvedit->workbuf[pageoffset]), pagesize );
			}

			//ioctldata[0] = NVEDIT_NOR_BASE;
			//ioctldata[1] = NVEDIT_WORBUF_MAX;
			//NOR_IOCTL(flash, NOR_SET_LOCK, ioctldata);
			break;
#endif		//SUPPORT_NVRAM_NOR

#ifdef		SUPPORT_NVRAM_SFLASH
		case	NVEDIT_SFLASH:
			NVEDIT_MGMT_SFLASH_CREATE(nvedit, dev_unit, ioctldata);

			if ( nvedit->sflash_device == NULL ){
				return FALSE;
			}

			flash = nvedit->sflash_device ;

			ioctldata[0] = NVEDIT_SFLASH_SAFE_MODE;
			SFLASH_IOCTL(flash, SFLASH_BUS_CONTROL, ioctldata);

			ioctldata[0] = NVEDIT_SFLASH_BASE;
			ioctldata[1] = NVEDIT_WORBUF_MAX;
			SFLASH_IOCTL(flash, SFLASH_SET_UNLOCK, ioctldata);

			ioctldata[0] = NVEDIT_SFLASH_BASE;
			ioctldata[1] = NVEDIT_WORBUF_MAX;
			SFLASH_IOCTL(flash, SFLASH_CMD_ERASE, ioctldata);

			if ( (SFLASH_IOCTL(flash, SFLASH_GET_MODE, ioctldata) == TRUE)
			     && (SFLASH_SUPPORT_QPP(ioctldata[1]) == TRUE) ){
				ioctldata[0] = NVEDIT_SFLASH_WRITE_MODE;
			}
			else {
				ioctldata[0] = NVEDIT_SFLASH_SAFE_MODE;
			}
			SFLASH_IOCTL(flash, SFLASH_BUS_CONTROL, ioctldata);

#ifdef	SUPPORT_NVRAM_SECU
			if( nvedit->table.reserved != 0xFFFFFFFF ){
				totalsize = (nvedit->table.reserved & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
			}
			else {
				totalsize = (nvedit->table.total & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
			}
#else	//SUPPORT_NVRAM_SECU
			totalsize = (nvedit->table.total & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
#endif	//SUPPORT_NVRAM_SECU

#ifdef SUPPORT_NVRAM_FOOTPRINT
			nvedit_put_footprint(nvedit, 0xAA);
#endif
			for(pageoffset = 0
				; pageoffset < totalsize
				; pageoffset += pagesize){
				pagesize = ((pageoffset+pagesize) > totalsize)
					? (totalsize - pageoffset)
					: pagesize ;
				SFLASH_WRITE(flash, (NVEDIT_SFLASH_BASE+pageoffset), &(nvedit->workbuf[pageoffset]), pagesize );
			}
#ifdef SUPPORT_NVRAM_FOOTPRINT
			nvedit_put_footprint(nvedit, 0x55);
#endif
			ioctldata[0] = NVEDIT_SFLASH_SAFE_MODE;
			SFLASH_IOCTL(flash, SFLASH_BUS_CONTROL, ioctldata);

			ioctldata[0] = NVEDIT_SFLASH_BASE;
			ioctldata[1] = NVEDIT_WORBUF_MAX;
			SFLASH_IOCTL(flash, SFLASH_SET_LOCK, ioctldata);

			NVEDIT_MGMT_SFLASH_CLOSE(nvedit);
			break;
#endif	//SUPPORT_NVRAM_SFLASH

#ifdef	SUPPORT_NVRAM_RAM
		case	NVEDIT_RAM:
			DRIVER_MEMCPY( (void *)(nvedit->ram_base), &(nvedit->workbuf[pageoffset])
					, ((nvedit->table.total & NVITEM_LEN_MASK)+sizeof(NVEDIT_TABLE_TYPE)) );
			break;
#endif	//SUPPORT_NVRAM_RAM

		default:
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *   nvedit_tree_erase_func( )
 *
 *  Purpose :    erase function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_erase_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_unit )
{
	HANDLE flash;
	UINT32 ioctldata[7];

	if( nvedit == NULL ){
		return FALSE;
	}

	switch(dev_type){
#ifdef		SUPPORT_NVRAM_NOR
		case	NVEDIT_NOR:
				if( nvedit->nor_device == NULL )
				{
					flash = NOR_CREATE(dev_unit);
					if(flash == NULL) {
						DRV_DBG_WARN("nor create error");
						return FALSE;
					}
					NOR_INIT(flash);
#ifdef	BUILD_OPT_DA16200_ROMALL
					ioctldata[0] = TRUE;
					NOR_IOCTL(flash, NOR_SET_BOOTMODE, ioctldata);
#endif	//BUILD_OPT_DA16200_ROMALL
					nvedit->nor_device = flash ;
				}
				else
				{
					flash = nvedit->nor_device ;
				}

				//ioctldata[0] = NVEDIT_NOR_BASE;
				//ioctldata[1] = NVEDIT_WORBUF_MAX;
				//NOR_IOCTL(flash, NOR_SET_UNLOCK, ioctldata);

				ioctldata[0] = NVEDIT_NOR_BASE;
				ioctldata[1] = NVEDIT_WORBUF_MAX;
				NOR_IOCTL(flash, NOR_SET_ERASE, ioctldata);

				//ioctldata[0] = NVEDIT_NOR_BASE;
				//ioctldata[1] = NVEDIT_WORBUF_MAX;
				//NOR_IOCTL(flash, NOR_SET_LOCK, ioctldata);

				break;
#endif		//SUPPORT_NVRAM_NOR

#ifdef	SUPPORT_NVRAM_SFLASH
		case	NVEDIT_SFLASH:
				NVEDIT_MGMT_SFLASH_CREATE(nvedit, dev_unit, ioctldata);

				if( nvedit->sflash_device == NULL ){
					return FALSE;
				}

				flash = nvedit->sflash_device ;

				ioctldata[0] = NVEDIT_SFLASH_SAFE_MODE;
				SFLASH_IOCTL(flash, SFLASH_BUS_CONTROL, ioctldata);

				ioctldata[0] = NVEDIT_SFLASH_BASE;
				ioctldata[1] = NVEDIT_WORBUF_MAX;
				SFLASH_IOCTL(flash, SFLASH_SET_UNLOCK, ioctldata);

				ioctldata[0] = NVEDIT_SFLASH_BASE;
				ioctldata[1] = NVEDIT_WORBUF_MAX;
				SFLASH_IOCTL(flash, SFLASH_CMD_ERASE, ioctldata);

				NVEDIT_MGMT_SFLASH_CLOSE(nvedit);

				break;
#endif	//SUPPORT_NVRAM_SFLASH

#ifdef	SUPPORT_NVRAM_RAM
		case	NVEDIT_RAM:
				DRIVER_MEMSET( (void *)(nvedit->ram_base), 0xFF
					, ((nvedit->table.total & NVITEM_LEN_MASK)+sizeof(NVEDIT_TABLE_TYPE)) );
				break;
#endif	//SUPPORT_NVRAM_RAM

		default:
				return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *   nvedit_tree_load_func( )
 *
 *  Purpose :    load function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_load_func(NVEDIT_HANDLER_TYPE	*nvedit, UINT32 dev_type, UINT32 dev_unit )
{
	HANDLE flash;
	int	status, pagesize, pageoffset;
	UINT32 ioctldata[7];
	NVEDIT_TABLE_TYPE *table;

	if( nvedit == NULL ){
		return FALSE;
	}

	NVEDIT_DBG_PRINTF("nv.load: %p %p %p \r\n" , nvedit->workbuf, nvedit->root,
							((nvedit->root!=NULL)? nvedit->root->child : NULL));

	if(nvedit->workbuf == NULL || nvedit->root == NULL || nvedit->root->child != NULL ){
		return FALSE;
	}

	table = (NVEDIT_TABLE_TYPE *)(nvedit->workbuf);

	if( table->magic == NVITEM_MAGIC_CODE ){
		// if nvram is already loaded
		NVEDIT_DBG_PRINTF(" [%s] nvram is already loaded dev_type:%d \r\n" , __func__, dev_type);
		return TRUE;
	}
	else {
		switch(dev_type){
#ifdef	SUPPORT_NVRAM_NOR
			case	NVEDIT_NOR:
				pagesize   = 256;
				pageoffset = 0;

				if( nvedit->nor_device == NULL )
				{
					flash = NOR_CREATE(dev_unit);
					if(flash == NULL) {
						DRV_DBG_WARN("nor create error");
						return FALSE;
					}
					NOR_INIT(flash);
#ifdef	BUILD_OPT_DA16200_ROMALL
					ioctldata[0] = TRUE;
					NOR_IOCTL(flash, NOR_SET_BOOTMODE, ioctldata);
#endif	//BUILD_OPT_DA16200_ROMALL
					nvedit->nor_device = flash ;
				}
				else
				{
					flash = nvedit->nor_device ;
				}

				NOR_READ(flash, NVEDIT_NOR_BASE, nvedit->workbuf, pagesize );
				DRIVER_MEMCPY(&(nvedit->table), nvedit->workbuf, sizeof(NVEDIT_TABLE_TYPE) );

				pageoffset = pagesize;
				pagesize   = NVEDIT_WORBUF_DEFAULT;

				break;
#endif	//SUPPORT_NVRAM_NOR

#ifdef	SUPPORT_NVRAM_SFLASH
			case	NVEDIT_SFLASH:
				pagesize   = 256;
				pageoffset = 0;

				status = NVEDIT_MGMT_SFLASH_CREATE(nvedit, dev_unit, ioctldata);	/* nvedit_sflash_create */
				flash = nvedit->sflash_device ;

				NVEDIT_DBG_PRINTF(" [%s] >>> flash:0x%x status:%d \r\n", __func__, flash, status);

				if( status == 0 ){
					return FALSE;
				}
				else if( status == 2 ) {
					ioctldata[0] = NVEDIT_SFLASH_READ_MODE;
				}
				else {
					ioctldata[0] = NVEDIT_SFLASH_SAFEREAD_MODE;
				}

				NVEDIT_DBG_PRINTF(" [%s] SFLASH_BUS_CONTROL ioctldata:%d \r\n", __func__, ioctldata[0]);
				SFLASH_IOCTL(flash, SFLASH_BUS_CONTROL, ioctldata);

				SFLASH_READ(flash, NVEDIT_SFLASH_BASE, nvedit->workbuf, pagesize );
				NVEDIT_DBG_PRINTF(" [%s] SFLASH_READ: NVEDIT_SFLASH_BASE (pagesize:%d) \r\n", __func__, pagesize);

				DRIVER_MEMCPY(&(nvedit->table), nvedit->workbuf, sizeof(NVEDIT_TABLE_TYPE) );

				pageoffset = pagesize;
				pagesize   = NVEDIT_WORBUF_DEFAULT;

				break;
#endif	//SUPPORT_NVRAM_SFLASH

#ifdef	SUPPORT_NVRAM_RAM
			case	NVEDIT_RAM:
				pagesize   = 256;
				pageoffset = 0;

				DRIVER_MEMCPY(nvedit->workbuf, (void *)(nvedit->ram_base), pagesize );
				DRIVER_MEMCPY(&(nvedit->table), nvedit->workbuf, sizeof(NVEDIT_TABLE_TYPE) );

				pageoffset = pagesize;
				pagesize   = NVEDIT_WORBUF_DEFAULT;

				break;
#endif	//SUPPORT_NVRAM_RAM

			default:
				return FALSE;
		}
	}

	status = FALSE;
	// TRICK : OS Buffer Limit
	if(nvedit->table.magic == NVITEM_MAGIC_CODE){

		if( (nvedit->table.total & NVITEM_LEN_MASK) > (NVEDIT_WORBUF_MAX-sizeof(NVEDIT_TABLE_TYPE)) ){
			nvedit->table.total = (nvedit->table.total & NVITEM_TYPE_MASK)
					| ((NVEDIT_WORBUF_MAX-sizeof(NVEDIT_TABLE_TYPE)) & NVITEM_LEN_MASK) ;
		}

		NVEDIT_DBG_PRINTF("nv.load: magic %x %d \r\n"
				, nvedit->table.magic
				, (nvedit->table.total & NVITEM_LEN_MASK)
			);

#ifdef	SUPPORT_NVEDIT_FULL
		nvedit->root->length = (nvedit->root->length & NVITEM_TYPE_MASK) | (nvedit->table.total & NVITEM_LEN_MASK) ;
#endif	//SUPPORT_NVEDIT_FULL

		{
			int	tablesize;

#ifdef	SUPPORT_NVRAM_SECU
			if( nvedit->table.reserved != 0xFFFFFFFF ){
				switch(NVITEM_GET_TYPE(nvedit->table.reserved)){
					case ASSET_ROOT_KEY:
					case ASSET_KCP_KEY:
					case ASSET_KPICV_KEY:
						tablesize = (nvedit->table.reserved & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
						if( tablesize <= NVEDIT_WORBUF_DEFAULT ){
							status = TRUE;
							break;
						}
						//Otherwise, it is not OK.
						tablesize = 0;
						nvedit->table.reserved = 0xFFFFFFFF;
						status = FALSE;
						break;

					default:
						// If the loaded data is wrong,
						tablesize = 0;
						nvedit->table.reserved = 0xFFFFFFFF;
						status = FALSE;
						break;
				}
			}else{
				tablesize = (nvedit->table.total & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
				status = TRUE;
			}
#else	//SUPPORT_NVRAM_SECU
			tablesize = (nvedit->table.total & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
			status = TRUE;
#endif	//SUPPORT_NVRAM_SECU

			for(/*pageoffset = pagesize*/; pageoffset < tablesize ; pageoffset += pagesize){

				pagesize = (((tablesize - pageoffset) > pagesize)
						? pagesize : (tablesize - pageoffset));
#ifdef	SUPPORT_EON_BURSTREAD_ISSUE
				pagesize = (((pagesize + 15) >> 4) << 4) ; // wraparound
#endif	//SUPPORT_EON_BURSTREAD_ISSUE

				switch(dev_type){
#ifdef	SUPPORT_NVRAM_NOR
					case	NVEDIT_NOR:
						NOR_READ(flash, (NVEDIT_NOR_BASE+pageoffset)
							, &(nvedit->workbuf[pageoffset]), pagesize );
						break;
#endif	//SUPPORT_NVRAM_NOR
#ifdef	SUPPORT_NVRAM_SFLASH
					case	NVEDIT_SFLASH:
						SFLASH_READ(flash, (NVEDIT_SFLASH_BASE+pageoffset)
							, &(nvedit->workbuf[pageoffset]), pagesize );
						break;
#endif	//SUPPORT_NVRAM_SFLASH
#ifdef	SUPPORT_NVRAM_RAM
					case	NVEDIT_RAM:
						DRIVER_MEMCPY(&(nvedit->workbuf[pageoffset])
							, (void *)(nvedit->ram_base+pageoffset), pagesize );
						break;
#endif	//SUPPORT_NVRAM_RAM
					default:
						break;
				}
			}

			//status = TRUE;
		}
	}
#ifdef SUPPORT_CHECK_CRC
	/*  CRC (XOR Checksum) */
	if (status == TRUE
		&& ((nvedit->table.chksum & 0xFF00) != 0x8000)
		&& ((nvedit->table.chksum & 0xFF00) != 0x8100)
		&& ((nvedit->table.chksum & 0xFF00) != 0)) {
		// the crc flag is invalid.
		status = FALSE;
	}
#endif

#ifdef	SUPPORT_NVRAM_SECU
	if( status == TRUE && nvedit->table.reserved != 0xFFFFFFFF ){
		UINT32	tablesize, oastlen;
		UINT8	*outasset;

		tablesize = (nvedit->table.reserved & NVITEM_LEN_MASK) +sizeof(NVEDIT_TABLE_TYPE);
		nvedit->secure = NVITEM_GET_TYPE(nvedit->table.reserved);
#ifdef SUPPORT_CHECK_CRC
		if ((nvedit->table.chksum & 0xFF00) == 0x8100)
		{
			uint16_t length = tablesize - sizeof(NVEDIT_TABLE_TYPE);
#ifdef SUPPORT_CHECK_CRC_16
			uint16_t chksum = 0;
			chksum = nvedit_swcrc16(&nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)], length);
			status = (((nvedit->table.chksum >> 16) & 0xFFFF) == chksum);
#else
			uint8_t chksum = 0;
			for(uint16_t i = 0; i < length; i++)
			{
				chksum ^= nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE) + i];
			}
			status = ((nvedit->table.chksum & 0xFF) == chksum);
#endif /* SUPPORT_CHECK_CRC_16 */
			if (status == FALSE) {
				NVEDIT_ERR_PRINTF("ERROR CHECKSUM - %d:%d\r\n", nvedit->table.chksum, chksum);
			}
		}
#endif /* SUPPORT_CHECK_CRC */

		if (status) {
			outasset = CRYPTO_MALLOC(tablesize);
			if( outasset != NULL ){
				CRYPTO_MEMSET(outasset, 0xFF, tablesize);

				oastlen = DA16X_Secure_Asset_RuntimeUnpack((AssetKeyType_t)(nvedit->secure)
						, (AssetUserKeyData_t *)(NULL), NVEDIT_ASSET_ID
						, &(nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)])
						, tablesize, outasset );
				if( oastlen <= NVEDIT_WORBUF_DEFAULT ){
					DRIVER_MEMCPY( &(nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)])
							, outasset, oastlen);
					status = TRUE;
				}else{
					status = FALSE;
				}

				CRYPTO_FREE(outasset);
			}
		}
	}
#endif	//SUPPORT_NVRAM_SECU

#ifdef SUPPORT_CHECK_CRC
	if (status == TRUE && ((nvedit->table.chksum & 0xFF00) == 0x8000))
	{
		uint16_t length = (nvedit->table.total & NVITEM_LEN_MASK) - sizeof(NVEDIT_TABLE_TYPE);
#ifdef SUPPORT_CHECK_CRC_16
		uint16_t chksum = 0;
		chksum = nvedit_swcrc16(&nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)], length);
		status = (((nvedit->table.chksum >> 16) & 0xFFFF) == chksum);
#else
		uint8_t chksum = 0;
		for(uint16_t i = 0; i < length; i++)
		{
			chksum ^= nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE) + i];
		}
		status = ((nvedit->table.chksum & 0xFF) == chksum);
#endif /* SUPPORT_CHECK_CRC_16 */
		if (status == FALSE) {
			NVEDIT_ERR_PRINTF("ERROR CHECKSUM - %d:%d\r\n", nvedit->table.chksum, chksum);
		}
	}
#endif /* SUPPORT_CHECK_CRC */

	switch(dev_type){
#ifdef	SUPPORT_NVRAM_SFLASH
		case	NVEDIT_SFLASH:
			if( nvedit->sflash_device != NULL )
			{
				NVEDIT_MGMT_SFLASH_CLOSE(nvedit);
			}
			break;
#endif	//SUPPORT_NVRAM_SFLASH

		default:
			break;
	}

#ifdef SUPPORT_CHECK_CRC
	// Force save NVRAM when the CRC flag(0x8000) has not been set.
	if (status == TRUE && (nvedit->table.chksum == 0))
	{
		status = nvedit_tree_parse_func(nvedit);
		if (status == TRUE) {
			DRIVER_MEMSET(nvedit->workbuf, 0xFF, (sizeof(UINT8)* NVEDIT_WORBUF_MAX));
			status = nvedit_tree_save_func(nvedit , dev_type, dev_unit, TRUE);
			if (status == TRUE) {
				nvedit->update = nvedit->update + 1;
				SYS_NVEDIT_IOCTL(NVEDIT_CMD_CLEAR, NULL);
				status = nvedit_tree_load_func(nvedit , dev_type,  dev_unit);
			}
		}
	}
#endif /* SUPPORT_CHECK_CRC */

	return status;
}

/******************************************************************************
 *   nvedit_tree_parse_func( )
 *
 *  Purpose :    parsing function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_parse_func(NVEDIT_HANDLER_TYPE	*nvedit)
{
	int status = TRUE;

	if( (nvedit->table.total & NVITEM_LEN_MASK) >= nvedit->poolsiz ){
		NVEDIT_DBG_PRINTF("nv.parse: overflow (%d vs %d)\r\n"
			, (nvedit->table.total & NVITEM_LEN_MASK)
			, nvedit->poolsiz
			);
		return FALSE;
	}

#ifdef	SUPPORT_NVEDIT_FULL
	NVEDIT_DBG_PRINTF(" >> start nvedit_tree_unpack_func (root:0x%x)\r\n" , nvedit->root);
	status = nvedit_tree_unpack_func(nvedit, nvedit->root );
	NVEDIT_DBG_PRINTF(" >> end nvedit_tree_unpack_func (status:%d)\r\n" , status);
#endif	//SUPPORT_NVEDIT_FULL

	return status;
}


#ifndef	 SUPPORT_NVEDIT_FULL
/******************************************************************************
 *   nvedit_tree_litefind_func( )
 *
 *  Purpose :    litefind function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	int nvedit_tree_litefind_func(char *token, NVEDIT_HANDLER_TYPE *nvedit, char *name, UINT32 *datlen, VOID *data, VOID *pnode)
{
	int	idx;
	UINT32 len;
	char *nameoffset;
	UINT8 *sibling , *child ;
	UINT32	depth, namlen ;
	int     depthmark[(NVEDIT_MAX_DEPTH+1)] ;
	void	*nodename;

	if( nvedit == NULL || nvedit->workbuf == NULL || name == NULL ){
		return FALSE;
	}

	if(nvedit->table.magic != NVITEM_MAGIC_CODE){
		return FALSE;
	}

	nameoffset = name ;
	token[0] = '\0';
	child = &( nvedit->workbuf[sizeof(NVEDIT_TABLE_TYPE)] ) ;
	depth  = 0;
	depthmark[0] = (nvedit->table.total & NVITEM_LEN_MASK) ;

	NVEDIT_DBG_PRINTF("litefind:%s\r\n", nameoffset);

	do{
		// get token
		idx = 0;
		while(*nameoffset != '.' && *nameoffset != '\0'){
			if(idx < NVITEM_NAME_MAX){
				token[idx] = *nameoffset ;
				idx++;
			}
			nameoffset ++;
		}
		if(*nameoffset == '.'){
			nameoffset ++;
		}
		token[idx] = '\0';

		// find child
		sibling = NULL;

		NVEDIT_DBG_PRINTF("litefind: depth [%d] = %d\r\n", depth, depthmark[depth]);

		while( child != NULL && ( depthmark[depth] > 0 ) ){

			NVEDIT_DBG_PRINTF("%s:%s - (%d/%d) \r\n", (char *)child, token, depth, depthmark[depth]);

			if( DRIVER_STRCMP((char *)child, token) == 0){
				// find token node
				sibling = child ;
				break;
			}else{
				// shift name
				namlen = 0;
				while(*((char *)child) != '\0'){
					child  ++;
					namlen ++;
				}
				child  ++;
				namlen ++;

				// shift hash
				NVEDIT_HASH_KEY_MOVE(child);
				// shift len
				nvedit_unpack_uint32( &len, child );
				child = child + sizeof(UINT32) + ( len & NVITEM_LEN_MASK ) ;
				depthmark[depth] = depthmark[depth] - ( namlen + sizeof(UINT32) + ( len & NVITEM_LEN_MASK ) );
			}
		}

		if(sibling == NULL){
			// not matched
			break;
		}else{
			child = sibling ;

			// shift name
			namlen = 0;
			while(*((char *)child) != '\0'){
				child  ++;
				namlen ++;
			}
			child  ++;
			namlen ++;

			// shift hash
			NVEDIT_HASH_KEY_MOVE(child);
			// extract datlen
			child = nvedit_unpack_uint32( &len, child );
			depthmark[depth] = depthmark[depth] - ( namlen + sizeof(UINT32) );
			depth ++;
			depthmark[depth] =  ( len & NVITEM_LEN_MASK ) ;
		}

	}while(*nameoffset != '\0' );

	if(sibling == NULL){
		return FALSE;
	}

	nodename = (char *)sibling;
	NVEDIT_DBG_PRINTF("sibling %s\r\n", (char *)nodename);

	// shift name
	while(*((char *)sibling) != '\0'){
		sibling  ++;
	}
	sibling  ++;

	// move hash
	NVEDIT_HASH_KEY_MOVE(sibling);
	// extract datlen
	sibling = nvedit_unpack_uint32( datlen, sibling );

	if( data != NULL ){
		// fill up data
		switch(((*datlen) & NVITEM_TYPE_MASK)){
			case 	NVITEM_VAR_MARK:
				nvedit_unpack_string( (char *)data, sibling, ((*datlen) & NVITEM_LEN_MASK) );	// String Type
				break;
			case 	NVITEM_NOD_MARK:
				break;
			default:
				switch(((*datlen) & NVITEM_LEN_MASK)){
					case	sizeof(UINT8):
						nvedit_unpack_uint8( (UINT8 *)data, sibling );
						break;
					case	sizeof(UINT16):
						nvedit_unpack_uint16( (UINT16 *)data, sibling );
						break;
					case	sizeof(UINT32):
						nvedit_unpack_uint32( (UINT32 *)data, sibling );
						break;
					default:
						nvedit_unpack_var( data, sibling, ((*datlen) & NVITEM_LEN_MASK) );
						break;
				}
				break;
		}
	}
	else if( pnode != NULL ){
		*((UINT32 *)pnode) = (UINT32)(nodename);
	} else {
		NVEDIT_DBG_PRINTF("data is null \r\n");
	}

	return TRUE;
}

/******************************************************************************
 *   nvedit_tree_liteupdate_func( )
 *
 *  Purpose :    liteupdate function
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int nvedit_tree_liteupdate_func(NVEDIT_HANDLER_TYPE *nvedit, VOID *pnode, VOID *data )
{
	UINT8 *sibling ;
	char  *name;
	UINT32 datlen;
	NVEDIT_HASH_KEY_DECLARE(hash);

	if( nvedit == NULL || nvedit->workbuf == NULL || pnode == NULL ){
		return FALSE;
	}

	sibling = (UINT8 *)pnode;

	// extract name
	name = (char *)sibling ;
	while(*((char *)sibling) != '\0'){
		sibling  ++;
	}
	sibling  ++;

	// extract hash
	NVEDIT_HASH_KEY_UNPACK_UINT32(sibling, 0, hash);
	// extract datlen
	sibling = nvedit_unpack_uint32( &datlen, sibling );

	NVEDIT_DBG_PRINTF("%s:%x \r\n", name, datlen);

	if( data == NULL ){
		NVEDIT_DBG_PRINTF("lupdate: data null \r\n");
		return FALSE;
	}

	// update value
	switch(((datlen) & NVITEM_TYPE_MASK)){
		case 	NVITEM_VAR_MARK:
			if( (DRIVER_STRLEN((char *)data)+1) != ((datlen) & NVITEM_LEN_MASK) ){
				PRINTF("lupdate: access denied (%d, %d) \r\n"
					, DRIVER_STRLEN((char *)data)
					, ((datlen) & NVITEM_LEN_MASK)
					);
				return FALSE;
			}
			nvedit_pack_string( sibling, data );	// String Type
			break;
		case 	NVITEM_NOD_MARK:
			break;
		default:
			switch(((datlen) & NVITEM_LEN_MASK)){
				case	sizeof(UINT8):
					nvedit_pack_uint8( sibling, *((UINT8 *)data) );
					break;
				case	sizeof(UINT16):
					nvedit_pack_uint16( sibling, *((UINT16 *)data) );
					break;
				case	sizeof(UINT32):
					nvedit_pack_uint32( sibling, *((UINT32 *)data) );
					break;
				default:
					nvedit_pack_var( sibling, data, ((datlen) & NVITEM_LEN_MASK) );
					break;
			}
			break;
	}

	return TRUE;
}
#endif //!SUPPORT_NVEDIT_FULL

#ifdef	SUPPORT_NVRAM_SFLASH
/******************************************************************************
 *   nvedit_sflash_create( )
 *
 *  Purpose :    liteupdate function
 *  Input   :
 *  Output  :
 *  Return  :   0 : false, 1 : unknown, 2 : known
 ******************************************************************************/

static int nvedit_sflash_create(NVEDIT_HANDLER_TYPE *nvedit, UINT32 dev_unit, UINT32 *ioctldata)
{
	HANDLE flash;

	if( nvedit->sflash_device == NULL )
	{
		UINT32 wakeupcnt;

		NVEDIT_DBG_PRINTF(CYAN_COLOR " [%s] >>> SFLASH_CREATE Req (dev_unit:%d) \r\n" CLEAR_COLOR, __func__, dev_unit);
		flash = SFLASH_CREATE(dev_unit);
		NVEDIT_DBG_PRINTF(CYAN_COLOR " [%s] >>> SFLASH_CREATE Done (sflash:0x%x) \r\n" CLEAR_COLOR, __func__, flash);
		if(flash == NULL) {
			return 0;
		}

		ioctldata[0] = NVEDIT_SFLASH_DWIDTH;
		NVEDIT_DBG_PRINTF(" [%s] NVEDIT_SFLASH_DWIDTH: %d \r\n", __func__, NVEDIT_SFLASH_DWIDTH);
		SFLASH_IOCTL(flash, SFLASH_BUS_DWIDTH, ioctldata);		/* sflash_local_low_ioctl */

		// setup speed
		ioctldata[0] = da16x_sflash_get_maxspeed();
		NVEDIT_DBG_PRINTF(" [%s] SFLASH_BUS_MAXSPEED: %d \r\n", __func__, ioctldata[0]);
		SFLASH_IOCTL(flash , SFLASH_BUS_MAXSPEED, ioctldata);

		// setup bussel
		ioctldata[0] = da16x_sflash_get_bussel();
		NVEDIT_DBG_PRINTF(" [%s] SFLASH_SET_BUSSEL: %d \r\n", __func__, ioctldata[0]);
		SFLASH_IOCTL(flash , SFLASH_SET_BUSSEL, ioctldata);

		// setup parameters
		if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
			NVEDIT_DBG_PRINTF(" [%s] SFLASH_SET_INFO \r\n", __func__);
			SFLASH_IOCTL(flash, SFLASH_SET_INFO, ioctldata);	/* sflash_local_low_ioctl */
		}

#ifdef	SUPPORT_SLOW_WAKEUP_ISSUE
		// Slow Wakeup Time Issue:
		// Old SFLASHs need a waiting time for reinitialization
		// after image booting.
		//
		// EN25QH256, N24Q256
		// 1. N25Q256 has a feature that tWNVCR is max 3 sec.
		//
		// NOTICE
		// WatchDog Timer for POR Boot is set to 1 sec.
		// So, this procedure should be finished under 1 sec.
		wakeupcnt = 1000;
#else	//SUPPORT_SLOW_WAKEUP_ISSUE
		wakeupcnt = 100;
#endif	//SUPPORT_SLOW_WAKEUP_ISSUE

		while(wakeupcnt > 0 ){
			NVEDIT_DBG_PRINTF(CYAN_COLOR " [%s] SFLASH_INIT Req \r\n" CLEAR_COLOR, __func__);
			if (SFLASH_INIT(flash) == TRUE) {
				NVEDIT_DBG_PRINTF(CYAN_COLOR " [%s] SFLASH_INIT Done(wakeupcnt:%d) \r\n" CLEAR_COLOR, __func__, wakeupcnt);
				break;
			}
			else {
				ioctldata[0] = 0;
				NVEDIT_DBG_PRINTF(" [%s] SFLASH_SET_RESET \r\n", __func__);
				SFLASH_IOCTL(flash, SFLASH_SET_RESET, ioctldata);

				ioctldata[0] = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
				NVEDIT_DBG_PRINTF(" [%s] SFLASH_BUS_CONTROL (bus:0x%x) \r\n", __func__, ioctldata[0]);
				SFLASH_IOCTL(flash, SFLASH_BUS_CONTROL, ioctldata);
			}
			wakeupcnt--;
		}

		if( wakeupcnt == 0 ){
			SFLASH_CLOSE(flash);
			flash = NULL;
		}

		nvedit->sflash_device = flash ;
	}
	else {
		flash = nvedit->sflash_device ;
#ifdef SUPPORT_SLR_PWRWKUP
		ioctldata[0] = 0;

		SFLASH_IOCTL(flash, SFLASH_CMD_WAKEUP, ioctldata);

		if( ioctldata[0] > 0 ){
			ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
			MSLEEP( ioctldata[0] );
		}
#endif //SUPPORT_SLR_PWRWKUP
	}

	NVEDIT_DBG_PRINTF(" [%s] SFLASH_GET_INFO :%d \r\n", __func__, ioctldata[0]);
	ioctldata[0] = 0;
	SFLASH_IOCTL(flash, SFLASH_GET_INFO, ioctldata);

	if (ioctldata[2] == (UINT32)NULL) {
		return 1;
	}
	return 2;
}

#ifdef SUPPORT_CHECK_CRC_16
static const uint16_t RO_fast_crc_nbit_LUT[4][16] = {
	{
	swap16(0x0000),  swap16(0x3331),  swap16(0x6662),  swap16(0x5553),
	swap16(0xccc4),  swap16(0xfff5),  swap16(0xaaa6),  swap16(0x9997),
	swap16(0x89a9),  swap16(0xba98),  swap16(0xefcb),  swap16(0xdcfa),
	swap16(0x456d),  swap16(0x765c),  swap16(0x230f),  swap16(0x103e)
	},
	{
	swap16(0x0000),  swap16(0x0373),  swap16(0x06e6),  swap16(0x0595),
	swap16(0x0dcc),  swap16(0x0ebf),  swap16(0x0b2a),  swap16(0x0859),
	swap16(0x1b98),  swap16(0x18eb),  swap16(0x1d7e),  swap16(0x1e0d),
	swap16(0x1654),  swap16(0x1527),  swap16(0x10b2),  swap16(0x13c1)
	},
	{
	swap16(0x0000),  swap16(0x1021),  swap16(0x2042),  swap16(0x3063),
	swap16(0x4084),  swap16(0x50a5),  swap16(0x60c6),  swap16(0x70e7),
	swap16(0x8108),  swap16(0x9129),  swap16(0xa14a),  swap16(0xb16b),
	swap16(0xc18c),  swap16(0xd1ad),  swap16(0xe1ce),  swap16(0xf1ef)
	},
	{
	swap16(0x0000),  swap16(0x1231),  swap16(0x2462),  swap16(0x3653),
	swap16(0x48c4),  swap16(0x5af5),  swap16(0x6ca6),  swap16(0x7e97),
	swap16(0x9188),  swap16(0x83b9),  swap16(0xb5ea),  swap16(0xa7db),
	swap16(0xd94c),  swap16(0xcb7d),  swap16(0xfd2e),  swap16(0xef1f)
	}
};

uint16_t fast_crc_nbit_lookup(const void *data, int length
		, uint16_t CrcLUT[4][16], uint16_t previousCrc32)
{
	uint16_t crc = swap16(previousCrc32);
	const unsigned short* current = (const unsigned short*) data;

	while( length > 1 ){
		unsigned short one = *current++ ^ (crc);

		crc = CrcLUT[0][(one >> 0) & 0x0f]
			^ CrcLUT[1][(one >> 4) & 0x0f]
			^ CrcLUT[2][(one >> 8) & 0x0f]
			^ CrcLUT[3][(one >> 12) & 0x0f];
		length -= 2;
	}

	if( length > 0 ){
		unsigned short one = *current;

		one = ((one ^ crc) << 8);
		crc = crc >> 8;

		crc = crc
			^ CrcLUT[0][(one >> 0) & 0x0f]
			^ CrcLUT[1][(one >> 4) & 0x0f]
			^ CrcLUT[2][(one >> 8) & 0x0f]
			^ CrcLUT[3][(one >> 12) & 0x0f];
	}

	return swap16(crc);
}

static uint16_t nvedit_swcrc16(uint8_t *data, uint16_t length)
{
	uint16_t crcdata;
	crcdata = fast_crc_nbit_lookup(data, length, (uint16_t (*)[16])RO_fast_crc_nbit_LUT, 0);
	return crcdata;
}
#endif /* SUPPORT_CHECK_CRC_16 */
#endif	/*SUPPORT_NVRAM_SFLASH*/

/* EOF */
