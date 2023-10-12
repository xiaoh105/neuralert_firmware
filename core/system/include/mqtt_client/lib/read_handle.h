/**
 *****************************************************************************************
 * @file	read_handle.h
 * @brief	Header file - Procssing MQTT messages received
 * @todo	File name : read_handle.h -> mqtt_client_read.h
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

/**
 ****************************************************************************************
 * @brief Match the API and received message.
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_packet_handle(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT CONNACK packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_connack(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT PINGREQ packet received (unused)
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_pingreq(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT PINGRESP packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_pingresp(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT PUBACK and PUBCOMP packet received
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] type The string "PUBACK" or "PUBCOMP"
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_pubackcomp(struct mosquitto *mosq, const char *type);

/**
 ****************************************************************************************
 * @brief Process MQTT PUBLISH packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_publish(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT PUBREC packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_pubrec(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT PUBREL packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_pubrel(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT SUBACK packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_suback(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Process MQTT UNSUBACK packet received
 * @param[in] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_handle_unsuback(struct mosquitto *mosq);
