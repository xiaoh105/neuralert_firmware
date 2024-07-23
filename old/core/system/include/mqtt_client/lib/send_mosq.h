/**
 *****************************************************************************************
 * @file		send_mosq.h
 * @brief	Header file - Generating MQTT messages to send
 * @todo		File name : send_mosq.h -> mqtt_client_send.h
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

#ifndef	__SEND_MOSQ_H__
#define	__SEND_MOSQ_H__

/**
 ****************************************************************************************
 * @brief Send MQTT packet simply (For DISCONNECT, PINGREQ and PINGRESP)
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] command packet type
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_simple_command(struct mosquitto *mosq, uint8_t command);

/**
 ****************************************************************************************
 * @brief Send MQTT packet with message ID (For PUBACK, PUBCOMP, PUBREC, and PUBREL)
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] command packet type
 * @param[in] mid Message ID
 * @param[in] dup true if duplicated message, false else.
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup);

/**
 ****************************************************************************************
 * @brief Send MQTT packet simply (For DISCONNECT, PINGREQ and PINGRESP)
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] keepalive Period of sending PINGREQ
 * @param[in] clean_session true if clean session on, false else.
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session);

/**
 ****************************************************************************************
 * @brief Send MQTT DISCONNECT
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_disconnect(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Send MQTT PINGREQ
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_pingreq(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Send MQTT PINGRESP
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_pingresp(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Send MQTT PUBACK
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_puback(struct mosquitto *mosq, uint16_t mid);

/**
 ****************************************************************************************
 * @brief Send MQTT PUBCOMP
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_pubcomp(struct mosquitto *mosq, uint16_t mid);

/**
 ****************************************************************************************
 * @brief Send MQTT PUBREC
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_pubrec(struct mosquitto *mosq, uint16_t mid);

/**
 ****************************************************************************************
 * @brief Send MQTT PUBREL
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_pubrel(struct mosquitto *mosq, uint16_t mid);

/**
 ****************************************************************************************
 * @brief Send MQTT SUBSCRIBE
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] topic topic to register
 * @param[in] topic_qos MQTT QoS (0~2)
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_subscribe(struct mosquitto *mosq, int *mid, const char *topic, uint8_t topic_qos);

/**
 ****************************************************************************************
 * @brief Send MQTT UNSUBSCRIBE
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] topic topic to unregister
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_unsubscribe(struct mosquitto *mosq, int *mid, const char *topic);

/**
 ****************************************************************************************
 * @brief Handle MQTT PUBLISH to send actually
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] topic topic to publish
 * @param[in] payloadlen payload length
 * @param[in] payload payload to send
 * @param[in] topic_qos MQTT QoS (0~2)
 * @param[in] retain true if retain message, false else.
 * @param[in] dup true if duplicated message, false else.
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, 
								 uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup);

/**
 ****************************************************************************************
 * @brief Send MQTT PUBLISH
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] topic topic to publish
 * @param[in] payloadlen payload length
 * @param[in] payload payload to send
 * @param[in] topic_qos MQTT QoS (0~2)
 * @param[in] retain true if retain message, false else.
 * @param[in] dup true if duplicated message, false else.
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_send_publish(struct mosquitto *mosq, uint16_t mid, const char *topic,
							uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup);

#endif	/* __SEND_MOSQ_H__ */
