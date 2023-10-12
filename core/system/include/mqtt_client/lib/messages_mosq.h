/**
 ******************************************************************************************
 * @file		messages_mosq.h
 * @brief	Header file - MQTT messages handler
 * @todo		File name : messages_mosq.h -> mqtt_client_message.h
 ******************************************************************************************
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

#ifndef	__MESSAGES_MOSQ_H__
#define	__MESSAGES_MOSQ_H__

/**
 ****************************************************************************************
 * @brief Cleanup all messages in the main structure mosquitto
 * @param[out] mosq Main structure of mqtt_client
 ****************************************************************************************
 */
void _mosquitto_message_cleanup_all(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Cleanup a message using the message structure
 * @param[out] message Message stucture
 ****************************************************************************************
 */
void _mosquitto_message_cleanup(struct mosquitto_message_all **message);

/**
 ****************************************************************************************
 * @brief Cleanup a message using message ID (if found)
 * @param[out] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] dir mosq_md_in (received) or mosq_md_out (to send)
 * @retrun MOSQ_ERR_SUCCESS - no error / MOSQ_ERR_NOT_FOUND - no found message with the mid
 ****************************************************************************************
 */
int _mosquitto_message_delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir);

/**
 ****************************************************************************************
 * @brief Queue a message to send
 * @param[out] mosq Main structure of mqtt_client
 * @param[out] message Message stucture
 * @param[in] dir mosq_md_in (received) or mosq_md_out (to send)
 * @retrun 0 - to indicate an outgoing message can be started\n
 * 1 - to indicate that the outgoing message queue is full (inflight limit has been reached)
 ****************************************************************************************
 */
int _mosquitto_message_queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir);

/**
 ****************************************************************************************
 * @brief Initialize all parameters related the message in the main structure
 * @param[out] mosq Main structure of mqtt_client
 ****************************************************************************************
 */
void _mosquitto_messages_reconnect_reset(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Remove a message using message ID and find the message pointer
 * @param[out] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] dir mosq_md_in (received) or mosq_md_out (to send)
 * @param[out] message Message stucture
 * @retrun MOSQ_ERR_SUCCESS - no error / MOSQ_ERR_NOT_FOUND - no message with the mid
 ****************************************************************************************
 */
int _mosquitto_message_remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message);

/**
 ****************************************************************************************
 * @brief find a message using message ID
 * @param[in] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @retrun MOSQ_ERR_SUCCESS - no error / MOSQ_ERR_NOT_FOUND - the mid not found
 ****************************************************************************************
 */
int _mosquitto_message_search_mid_in_rx_queue(struct mosquitto *mosq, uint16_t mid);

/**
 ****************************************************************************************
 * @brief Retry to send message not processed completely
 * @param[out] mosq Main structure of mqtt_client
 ****************************************************************************************
 */
void _mosquitto_message_retry_check(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Update status to the QoS message sent completely
 * @param[out] mosq Main structure of mqtt_client
 * @param[in] mid Message ID
 * @param[in] mid Message status
 ****************************************************************************************
 */
int _mosquitto_message_out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state);

#endif	/* __MESSAGES_MOSQ_H__ */
