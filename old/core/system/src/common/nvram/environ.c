 /**
 ****************************************************************************************
 *
 * @file environ.c
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
#include "driver.h"

#include "da16x_system.h"
#include "nvedit.h"
#include "sys_cfg.h"
#include "environ.h"

#ifdef SFLASH_ENV_DEBUGGING
#define ENV_DEBUG
#else
#undef	ENV_DEBUG
#endif

#ifdef ENV_DEBUG
#define	ENV_PRINTF(...)			PRINTF( __VA_ARGS__ )
#define	ENV_ERR_PRINTF(...)		PRINTF( __VA_ARGS__ )
#else
#define	ENV_PRINTF(...)
#define	ENV_ERR_PRINTF(...)		PRINTF( __VA_ARGS__ )
#endif

#define ENV_PUTS(...)		DRV_DBG_TEXT(0, __VA_ARGS__ )
#define ENV_PUTC(...)		PRINTF( __VA_ARGS__ )

#undef	SUPPORT_ENVIRON_TSTAMP


#ifdef	SUPPORT_ENVIRON_TSTAMP
static UINT32	usagecnt, startstamp, endstamp, oldendstamp;
static UINT32	lockstamp, lockstart, loackend;

#define OAL_CURRENT_THREAD_POINTER(...) 	xTaskGetCurrentTaskHandle(__VA_ARGS__)

#define	START_TIMESTAMP()	{					\
	startstamp = da16x_rtc_get_mirror32(FALSE);			\
	}

#define	PRINT_TIMESTAMP(x,env)	{					\
	endstamp = da16x_rtc_get_mirror32(FALSE);			\
	usagecnt++;							\
	ENV_PRINTF(x "(%04d), %-30s, %08x, %8d, %8d, %8d - %p mis %d\n"	\
		, usagecnt, env, endstamp, (endstamp-startstamp)	\
		, lockstamp						\
		, ((oldendstamp == 0 ) ? 0 : (startstamp-oldendstamp))\
		, OAL_CURRENT_THREAD_POINTER()				\
		, da16x_cache_get_misscount()				\
		);							\
		oldendstamp = da16x_btm_get_timestamp();		\
	}

#define	PRINT_TIMESTAMPSAV(x,env,val)	{				\
	endstamp = da16x_rtc_get_mirror32(FALSE);			\
	usagecnt++;							\
	ENV_PRINTF(x "(%04d), %-30s, %08x, %8d, %8d, %8d - %p mis %d\n"	\
		, usagecnt, env, endstamp, (endstamp-startstamp)	\
		, lockstamp						\
		, ((oldendstamp == 0 ) ? 0 : (startstamp-oldendstamp))\
		, OAL_CURRENT_THREAD_POINTER()				\
		, da16x_cache_get_misscount()				\
		);							\
		oldendstamp = da16x_rtc_get_mirror32(FALSE);		\
	}


#else	//SUPPORT_ENVIRON_TSTAMP

#define	START_TIMESTAMP()
#define	PRINT_TIMESTAMP(x,env)
#define	PRINT_TIMESTAMPSAV(x,env,val)

#endif	//SUPPORT_ENVIRON_TSTAMP

extern void do_write_clock_change(UINT32 data32, int disp_flag);

/******************************************************************************
 *  init_environ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	SemaphoreHandle_t	sem_environ;

void	init_environ(void)
{
	sem_environ = xSemaphoreCreateMutex();
	if ( sem_environ == NULL ) {
		PRINTF(" Create error: semenv\n");
	}
}

static	void lock_environ(void)
{
#ifdef	SUPPORT_ENVIRON_TSTAMP
	lockstart = da16x_btm_get_timestamp();
#endif	//SUPPORT_ENVIRON_TSTAMP

	if ( sem_environ != NULL ) {
		xSemaphoreTake(sem_environ, portMAX_DELAY);
	}

#ifdef	SUPPORT_ENVIRON_TSTAMP
	loackend = da16x_btm_get_timestamp();
	lockstamp = loackend - lockstart;
#endif	//SUPPORT_ENVIRON_TSTAMP
}

static	void unlock_environ(void)
{
#ifdef	SUPPORT_ENVIRON_TSTAMP
	lockstart = da16x_btm_get_timestamp();
#endif	//SUPPORT_ENVIRON_TSTAMP

	if ( sem_environ != NULL ) {
		xSemaphoreGive(sem_environ);
	}

#ifdef	SUPPORT_ENVIRON_TSTAMP
	loackend = da16x_btm_get_timestamp();
	lockstamp = lockstamp + (loackend - lockstart);
#endif	//SUPPORT_ENVIRON_TSTAMP
}

void check_environ(void)
{
	if ( sem_environ != NULL ) {
		do{
			if (xSemaphoreTake(sem_environ, 100) == pdTRUE) {
				break;
			}
			PRINTF("WARN: envrion is locked \n");

		} while(1);
	}
}

/******************************************************************************
 *  gen_fullkeyname( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static char *gen_fullkeyname(char *rootname, char *envname)
{
	char *fullkey = NULL;

	if ( rootname != NULL && envname != NULL ) {

		fullkey = pvPortMalloc( DRIVER_STRLEN(rootname) + DRIVER_STRLEN(envname) + 2 );
		if ( fullkey != NULL ) {
			DRIVER_SPRINTF(fullkey, "%s.%s", rootname, envname);
		}
	}
	return fullkey ;
}

/******************************************************************************
 *  free_fullkeyname( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void free_fullkeyname(char *keyname)
{
	vPortFree(keyname);
}

/******************************************************************************
 *  da16x_environ_lock( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void da16x_environ_lock(UINT32 flag)
{
	if ( flag == TRUE ) {
		lock_environ();
	}
	else {
		unlock_environ();
	}
}

/******************************************************************************
 *  da16x_getenv( )
 *
 *  Purpose :   get an environment variable
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

char *da16x_getenv(UINT32 device, char *rootname, char *envname)
{
#ifdef	SUPPORT_NVEDIT_FULL
	UINT32 ioctldata[5];
	char	*fullkey, *value ;

	START_TIMESTAMP();

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return NULL;
	}

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)fullkey;
	ioctldata[1] = (UINT32)NULL;
	value = NULL;

	/* find a key */
	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if ((VOID *)(ioctldata[1]) != NULL) {
			/* if key node is exist */
			NVEDIT_TREE_ITEM *node;
			node = (NVEDIT_TREE_ITEM *)ioctldata[1];
			value = (char *)(node->value) ;
		}
	}

	free_fullkeyname(fullkey);

	PRINT_TIMESTAMP("getenv",envname);

	unlock_environ();

	return value;
#else	//SUPPORT_NVEDIT_FULL
	UINT32 datlen;
	UINT32 ioctldata[5];
	char	*fullkey ;

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return NULL;
	}

	lock_environ();

	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	ioctldata[2] = (UINT32)fullkey;
	ioctldata[3] = (UINT32)&datlen;
	ioctldata[4] = (UINT32)NULL;

	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LITEFIND, ioctldata) == TRUE) {
		ENV_PRINTF("%s=", fullkey);
		ENV_PUTS( (char *)(ioctldata[4]) , ((datlen&NVITEM_LEN_MASK)-1) );
		ENV_PUTC( "\n");
	}

	free_fullkeyname(fullkey);

	unlock_environ();

	return (char *)(ioctldata[4]);
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_setenv_flag( )
 *
 *  Purpose :   change or add an environment variable
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_setenv_flag(UINT32 flag, UINT32 device, char *rootname, char *envname, char *value)
{
#ifdef	SUPPORT_NVEDIT_FULL
	UINT32 ioctldata[4];
	char	*fullkey ;

	START_TIMESTAMP();

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return FALSE;
	}

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)fullkey;
	ioctldata[1] = (UINT32)NULL;

	ENV_PRINTF("[%s] [%s:%s] \r\n", __func__, envname, value);

	/* find a key */
	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if ((VOID *)(ioctldata[1]) != NULL) {
			/* if key node is exist */
			SYS_NVEDIT_WRITE( ioctldata[1], value, (DRIVER_STRLEN(value)+1));
			ENV_PRINTF("[%s] SYS_NVEDIT_WRITE [%s:%s] \r\n", __func__, envname, value);
		}
	}

	free_fullkeyname(fullkey);

	/* if key node is not exist, add new key */
	if ((VOID *)(ioctldata[1]) == NULL ) {
		ioctldata[0] = (UINT32) rootname ;
		ioctldata[1] = (UINT32) envname ;

		ioctldata[2] = (DRIVER_STRLEN(value)+1) ;
		ioctldata[2] = NVITEM_LEN_MASK & ioctldata[2] ;
		ioctldata[2] = NVITEM_VAR_MARK | ioctldata[2] ;
		ioctldata[3] = (UINT32) value ;

		ENV_PRINTF("[%s] NVEDIT_CMD_ADD [root:%s env:%s val:%s] \r\n", __func__, rootname, envname, value);
		if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_ADD, ioctldata) == TRUE) {
			ioctldata[0] = device;
			ioctldata[1] = 0; /* UNIT_0 */

			if ( flag == FALSE ) {
				PRINT_TIMESTAMPSAV("setenv.t",envname,value);

				unlock_environ();

				return TRUE;
			}

			/* sync b/w stored data and working data */
			ENV_PRINTF("[%s] NVEDIT_CMD_SAVE [root:%s env:%s val:%s] \r\n", __func__, rootname, envname, value);
			if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata) == TRUE) {
				PRINT_TIMESTAMPSAV("setenv",envname,value);

				unlock_environ();

				return TRUE;
			}
			else {
				ENV_ERR_PRINTF(" [%s:%d] ERR: - save, %s\n", __func__, __LINE__, envname);
			}
		}
		else {
			ENV_ERR_PRINTF(" [%s:%d] ERR: - add, %s\n", __func__, __LINE__, envname);
		}
	}
	else {
		if ( flag == FALSE ) {
			PRINT_TIMESTAMPSAV("setenv.t",envname,value);

			unlock_environ();

			return TRUE;
		}

		ioctldata[0] = device;
		ioctldata[1] = 0; /* UNIT_0 */

		/* sync b/w stored data and working data */
		if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata) == TRUE) {

			PRINT_TIMESTAMPSAV("setenv",envname,value);

			unlock_environ();

			return TRUE;
		}
		else {
			ENV_ERR_PRINTF(" [%s:%d] ERR: - save, %s\n", __func__, __LINE__, envname);
		}
	}

	PRINT_TIMESTAMPSAV("setenv",envname,value);

	unlock_environ();

	return FALSE;
#else	//SUPPORT_NVEDIT_FULL
	UINT32 datlen;
	UINT32 ioctldata[5];
	char	*fullkey ;

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return FALSE;
	}

	lock_environ();

	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	ioctldata[2] = (UINT32)fullkey;
	ioctldata[3] = (UINT32)&datlen;
	ioctldata[4] = (UINT32)( (value==NULL)? envname : value ) ;

	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LITEUPDATE, ioctldata) != TRUE) {
		ENV_ERR_PRINTF(" [%s:%d] ERR: - %s\n", __func__, __LINE__, envname);
	}

	free_fullkeyname(fullkey);

	unlock_environ();

	return TRUE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_putenv_flag( )
 *
 *  Purpose :   change or add an environment variable
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_putenv_flag(UINT32 flag, UINT32 device, char *rootname, char *envname)
{
#ifdef	SUPPORT_NVEDIT_FULL
	return da16x_setenv_flag(flag, device, rootname, envname, envname);
#else	//SUPPORT_NVEDIT_FULL
	/* not supported */
	return FALSE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_unsetenv_flag( )
 *
 *  Purpose :   deletes the variable name from the environment
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_unsetenv_flag(UINT32 flag, UINT32 device, char *rootname, char *envname)
{
#ifdef	SUPPORT_NVEDIT_FULL
	int    status, count;
	UINT32 ioctldata[5];
	char	*fullkey ;

	if ( *envname == '*' ) {
		// if first character is wildcard,
		return da16x_clearenv_flag( flag, device, rootname );
	}

	START_TIMESTAMP();

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return FALSE;
	}

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	/* delete a key */
	for(status = TRUE, count = 0; status == TRUE; count++ )
	{
		ioctldata[0] = (UINT32) fullkey ;
		ioctldata[1] = (flag == FALSE) ? TRUE : FALSE ;
		status = SYS_NVEDIT_IOCTL( NVEDIT_CMD_DEL, ioctldata);
	}

	if ( count == 0) {
		ENV_ERR_PRINTF(" [%s:%d] ERR: - %s\n", __func__, __LINE__, envname);
	}
	else {
		status = TRUE;
	}

	if ( status == TRUE && flag == TRUE ) {
		/* sync b/w stored data and working data */
		ioctldata[0] = device;
		ioctldata[1] = 0; /* UNIT_0 */
		status = SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata);
	}

	free_fullkeyname(fullkey);

	if ( flag == TRUE ) {
		PRINT_TIMESTAMP("unsetenv",envname);
	}
	else {
		PRINT_TIMESTAMP("unsetenv.t",envname);
	}

	unlock_environ();

	return status;
#else	//SUPPORT_NVEDIT_FULL
	/* not supported */
	return FALSE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_clearenv_flag( )
 *
 *  Purpose :   clear the environment
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_clearenv_flag(UINT32 flag, UINT32 device, char *rootname)
{
#ifdef	SUPPORT_NVEDIT_FULL
	int    status;
	UINT32 ioctldata[5];

	if ( rootname == NULL ) {
		return FALSE;
	}

	START_TIMESTAMP();

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)rootname;
	ioctldata[1] = (UINT32)NULL;
	status = FALSE;

	/* find a key */
	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if ((VOID *)(ioctldata[1]) != NULL) {
			/* if key node is exist */
			ioctldata[0] = (UINT32) (VOID *)(ioctldata[1]) ;
			status = SYS_NVEDIT_IOCTL( NVEDIT_CMD_CLEAR, ioctldata);

			if ( status == TRUE && flag == TRUE ) {
				/* sync b/w stored data and working data */
				ioctldata[0] = device;
				ioctldata[1] = 0; /* UNIT_0 */
				status = SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata);
			}
		}
	}

	if ( status == FALSE ) {
		ENV_ERR_PRINTF(" [%s:%d] ERR: - %s\n", __func__, __LINE__, rootname);
	}

	if ( flag == TRUE ) {
		PRINT_TIMESTAMP("clearenv",rootname);
	}
	else {
		PRINT_TIMESTAMP("clearenv.t",rootname);
	}

	unlock_environ();

	if (status != FALSE){
		do_write_clock_change(DA16X_SYSTEM_CLOCK, pdFALSE);
	}

	return status;
#else	//SUPPORT_NVEDIT_FULL
	/* not supported */
	return FALSE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_saveenv( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_saveenv(UINT32 device)
{
#ifdef	SUPPORT_NVEDIT_FULL
	int    status;
	UINT32 ioctldata[2];

	START_TIMESTAMP();

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	status = SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata);

	if ( status == FALSE ) {
		ENV_ERR_PRINTF(" [%s:%d] ERR \n", __func__, __LINE__);
	}

	PRINT_TIMESTAMP("saveenv","null");

	unlock_environ();

	extern char set_enable_restart_dhcp_client(char flag);
	set_enable_restart_dhcp_client(pdTRUE); // for wifi recofnig

	return status;
#else	//SUPPORT_NVEDIT_FULL
	/* not supported */
	return FALSE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_printenv( )
 *
 *  Purpose :   print name and value pairs for environment all
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_printenv(UINT32 device, char *rootname)
{
#ifdef	SUPPORT_NVEDIT_FULL
	int    status;
	UINT32 ioctldata[5];

	if ( rootname == NULL ) {
		return FALSE;
	}

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)rootname;
	ioctldata[1] = (UINT32)NULL;
	status = FALSE;

	/* find a key */
	ENV_PRINTF(" [%s:%d] NVEDIT_CMD_PRINT %s\n", __func__, __LINE__, rootname);
	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if ((VOID *)(ioctldata[1]) != NULL) {
			/* if key node is exist */
			ioctldata[0] = (UINT32) (VOID *)(ioctldata[1]) ;
			ENV_PRINTF(" [%s:%d] NVEDIT_CMD_PRINT ioctl[0]:%s \n", __func__, __LINE__, ioctldata[0]);
			status = SYS_NVEDIT_IOCTL( NVEDIT_CMD_PRINT, ioctldata);
		}
	}

	if ( status == FALSE ) {
		ENV_ERR_PRINTF(" [%s:%d] ERR %s\n", __func__, __LINE__, rootname);
	}

	unlock_environ();

	return status;
#else	//SUPPORT_NVEDIT_FULL
	lock_environ();
	SYS_NVCONFIG_VIEW(rootname);
	unlock_environ();

	return TRUE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_setenv_bin( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_setenv_bin(UINT32 device, char *rootname, char *envname, void *value, UINT32 len)
{
#ifdef	SUPPORT_NVEDIT_FULL
	UINT32 ioctldata[5];
	char	*fullkey ;

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return FALSE;
	}

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)fullkey;
	ioctldata[1] = (UINT32)NULL;

	/* find a key */
	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if ((VOID *)(ioctldata[1]) != NULL) {
			/* if key node is exist */
			SYS_NVEDIT_WRITE( ioctldata[1], value, len);
		}
	}

	free_fullkeyname(fullkey);

	/* if key node is not exist, add new key */
	if ((VOID *)(ioctldata[1]) == NULL ) {
		ioctldata[0] = (UINT32) rootname ;
		ioctldata[1] = (UINT32) envname ;

		ioctldata[2] = len ;
		ioctldata[2] = NVITEM_LEN_MASK & ioctldata[2] ;

		ioctldata[3] = (UINT32) value ;

		if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_ADD, ioctldata) == TRUE) {
			ioctldata[0] = device;
			ioctldata[1] = 0; /* UNIT_0 */

			/* sync b/w stored data and working data */
			if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata) == TRUE) {
				unlock_environ();
				return TRUE;
			}
			else {
				ENV_ERR_PRINTF(" [%s:%d] ERR: - save, %s\n", __func__, __LINE__, envname);
			}
		}
		else {
			ENV_ERR_PRINTF(" [%s:%d] ERR: - add, %s\n", __func__, __LINE__, envname);
		}
	}
	else {
		ioctldata[0] = device;
		ioctldata[1] = 0; /* UNIT_0 */

		/* sync b/w stored data and working data */
		if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata) == TRUE) {
			unlock_environ();
			return TRUE;
		}
		else {
			ENV_ERR_PRINTF(" [%s:%d] ERR: - save, %s\n", __func__, __LINE__, envname);
		}
	}

	unlock_environ();

	return FALSE;
#else	//SUPPORT_NVEDIT_FULL
	UINT32 datlen;
	UINT32 ioctldata[5];
	char	*fullkey ;

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return FALSE;
	}

	lock_environ();

	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	ioctldata[2] = (UINT32)fullkey;
	ioctldata[3] = (UINT32)&datlen;
	ioctldata[4] = (UINT32)( (value==NULL)? envname : value ) ;

	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LITEUPDATE, ioctldata) != TRUE) {
		ENV_ERR_PRINTF(" [%s:%d] ERR: - %s\n", __func__, __LINE__, envname);
	}

	free_fullkeyname(fullkey);

	unlock_environ();

	return TRUE;
#endif	//SUPPORT_NVEDIT_FULL
}

/******************************************************************************
 *  da16x_getenv_bin( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void *da16x_getenv_bin(UINT32 device, char *rootname, char *envname, UINT32 *len)
{
#ifdef	SUPPORT_NVEDIT_FULL
	UINT32 ioctldata[5];
	char	*fullkey , *value;

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return NULL;
	}

	lock_environ();

	/* sync b/w stored data and working data */
	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)fullkey;
	ioctldata[1] = (UINT32)NULL;
	value = NULL;
	*len  = 0;

	/* find a key */
	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if ((VOID *)(ioctldata[1]) != NULL) {
			/* if key node is exist */
			NVEDIT_TREE_ITEM *node;
			node = (NVEDIT_TREE_ITEM *)ioctldata[1];
			value = (char *)(node->value) ;
			(*len) = (UINT32)( NVITEM_LEN_MASK & (node->length) );
		}
	}

	free_fullkeyname(fullkey);

	unlock_environ();

	return value;
#else	//SUPPORT_NVEDIT_FULL
	UINT32 datlen;
	UINT32 ioctldata[6];
	char	*fullkey ;

	fullkey = gen_fullkeyname( rootname, envname );
	if ( fullkey == NULL ) {
		return NULL;
	}

	lock_environ();

	ioctldata[0] = device;
	ioctldata[1] = 0; /* UNIT_0 */
	ioctldata[2] = (UINT32)fullkey;
	ioctldata[3] = (UINT32)&datlen;
	ioctldata[4] = (UINT32)NULL;

	*len  = 0;

	if ( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LITEFIND, ioctldata) == TRUE) {
		ENV_PRINTF("%s=%s\n", fullkey);
		*len = (UINT32)( NVITEM_LEN_MASK & datlen );
	}

	free_fullkeyname(fullkey);

	unlock_environ();

	return (char *)(ioctldata[4]);
#endif	//SUPPORT_NVEDIT_FULL
}

/* EOF */
