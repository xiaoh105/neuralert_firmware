/**
 *****************************************************************************************
 * @file	util_mosq.h
 * @brief	Header file - MQTT Utilities
 * @todo	File name : util_mosq.h -> mqtt_client_util.h
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

#ifndef	__UTIL_MOSQ_H__
#define	__UTIL_MOSQ_H__

#include "da16x_time.h"

/**
 ****************************************************************************************
 * @brief Check whather keep-alive period is expired and send PINGREQ if expired.
 * @param[in] mosq Main structure of mqtt_client
 ****************************************************************************************
 */
void _mosquitto_check_keepalive(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief return message ID (Incremented by 1)
 * @param[in] mosq Main structure of mqtt_client
 * @return Message ID to send
 ****************************************************************************************
 */
uint16_t _mosquitto_mid_generate(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief return current time
 * @return Current time (unsigned long long)
 ****************************************************************************************
 */
da16x_time_t mosquitto_time(void);

/**
 ****************************************************************************************
 * @brief Register MQTT Will
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] topic Will topic to register
 * @param[in] payloadlen Will message length
 * @param[in] payload Will message to send
 * @param[in] qos Will QoS
 * @param[in] retain true if retain message, false else.
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

/**
 ****************************************************************************************
 * @brief Cleanup MQTT Will information
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_will_clear(struct mosquitto *mosq);

#endif	/* __UTIL_MOSQ_H__ */
