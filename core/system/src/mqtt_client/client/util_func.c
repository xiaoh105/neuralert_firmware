/**
 ****************************************************************************************
 *
 * @file util_func.c
 *
 * @brief Util functions for mqtt_client
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

#if defined ( __SUPPORT_MQTT__ )
#include <mqtt_client.h>
#include "mosquitto_internal.h"
#include "memory_mosq.h"
#include "da16200_map.h"
#include "da16x_sntp_client.h"
#include "lwip/sockets.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

UINT8 ut_is_task_running(TaskStatus_t* task_status)
{
	if (task_status->eCurrentState < eDeleted)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void ut_sock_reset (int* sock)
{
	close(*sock);
	*sock = INVALID_SOCKET;
}

void ut_sock_convert2tv(struct timeval* tv, ULONG ultime)
{
	// clear timeval struct
	memset(tv, 0x00, sizeof(struct timeval));

    /* 
    	e.g.) 
    	ultime == 110 (multiple of 10ms) x 10 == 1100 ms
    	sec : 1100 ms / 1000 => 1s
    	usec: 1100 ms % 1000 => 100 ms => 100 x 1000 = 100,000 us 
    */

	// convert into timeval format
	tv->tv_sec = (ultime * 10) / 1000;
	tv->tv_usec = ((ultime * 10) % 1000) * (1000);
}

void ut_sock_convert2ultime(struct timeval* tv, ULONG* ultime)
{
    /* 
    	e.g.)
    	tv->tv_sec = 1
    	tv->tv_usec = 100,000

		tv->tv_sec  : 1 -> 1 x 1000 = 1000 ms
		tv->tv_usec : 100,000 -> 100,000/1000 = 100 ms
		(1000 + 100) ms => (1000 + 100) / 10 = 110 (multiple of 10ms)
    */

	*ultime = (((tv->tv_sec)*(1000) + (tv->tv_usec)/(1000))/10);
}

void ut_sock_opt_get_timeout(mosq_sock_t sock, ULONG* timeval)
{
	struct timeval tv = {0,};
	int status = TRUE;
	socklen_t tv_len = sizeof(tv);

	status = getsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, &tv_len);	
	if(status == -1)
	{
		MQTT_DBG_ERR("getsockopt(SO_SNDTIMEO) fails\r\n");
		status = FALSE;
	}

	ut_sock_convert2ultime(&tv, timeval);
	
	return;
}

int ut_sock_opt_set_timeout (mosq_sock_t sock, ULONG timeval, UINT8 for_recv)
{
	struct timeval tv = {0,};
	int status = TRUE;

	ut_sock_convert2tv(&tv, timeval);

	if (for_recv == TRUE) {

	} else {
		status = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));	
	}
	
	if(status == -1) {
		MQTT_DBG_ERR("setsockopt(SO_SNDTIMEO) fails\r\n");
		status = FALSE;
	}
	
	return status;
}

void ut_mqtt_sem_create(mqtt_mutex_t* mq_mutex) 
{
	if (mq_mutex->in_use == TRUE /* created already */) {return;}

	mq_mutex->mutex_info = xSemaphoreCreateRecursiveMutex();
	mq_mutex->in_use = TRUE;
}

void ut_mqtt_sem_delete(mqtt_mutex_t* mq_mutex) 
{
	if (mq_mutex == NULL             || 
		mq_mutex->mutex_info == NULL || 
		mq_mutex->in_use == FALSE /* deleted already */) {return;}

	vSemaphoreDelete(mq_mutex->mutex_info);
	mq_mutex->in_use = FALSE;
}

void ut_mqtt_sem_take_recur(mqtt_mutex_t* mq_mutex, TickType_t wait_opt) 
{
	if (mq_mutex->in_use == FALSE) {ut_mqtt_sem_create(mq_mutex);}
	
	xSemaphoreTakeRecursive(mq_mutex->mutex_info, wait_opt);
}

int ut_mqtt_is_topic_existing(char* topic)
{
	extern mqttParamForRtm mqttParams;
	for (int i = 0; i < MQTT_MAX_TOPIC; i++) {
		if (mqttParams.topics[i][0] == (char)0) {
			continue;
		}
		
		if (strcmp(mqttParams.topics[i], topic) == 0) {
			return pdTRUE;
		}
	}
	
	return pdFALSE;
}

#endif	// __SUPPORT_MQTT__

/* EOF */
