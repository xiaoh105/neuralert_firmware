/**
 *****************************************************************************************
 * @file	pub_client.c
 * @brief	MQTT Publisher main handler
 * @todo	File name : pub_client.c -> mqtt_client_publisher.c
 *****************************************************************************************
 */

/*
Copyright (c) 2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.

Copyright (c) 2019-2022 Modified by Renesas Electronics.
*/

#include "sdk_type.h"

#if defined (__SUPPORT_MQTT__)

#include "mqtt_client.h"
#include "mosquitto_internal.h"
#include "memory_mosq.h"
#if defined ( __SUPPORT_ATCMD__ )
#include "atcmd.h"
#endif // __SUPPORT_ATCMD__
#include "da16x_network_common.h"

mqtt_client_thread mqtt_pub_thd_config = {0, };

int mqtt_pub_create_thread(void)
{
	return 0;
}

int mqtt_pub_term_thread(void)
{
	return 0;
}

int mqtt_client_check_pub_conn(void)
{
	return false;
}
#endif // (__SUPPORT_MQTT__)

/* EOF */
