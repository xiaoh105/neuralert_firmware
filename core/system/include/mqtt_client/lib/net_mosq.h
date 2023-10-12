/**
 *****************************************************************************************
 * @file	net_mosq.h
 * @brief	Header file - MQTT implementation with network stack
 * @todo	File name : net_mosq.h -> mqtt_client_net.h
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

#ifndef	__NET_MOSQ_H__
#define	__NET_MOSQ_H__

/// Macros for accessing the MSB of a uint16_t
#define MOSQ_MSB(A) (uint8_t)((A & 0xFF00) >> 8)
/// Macros for accessing the LSB of a uint16_t
#define MOSQ_LSB(A) (uint8_t)(A & 0x00FF)

#define SUB_SOCK_NAME	"msub_sock"
#define PUB_SOCK_NAME	"mpub_sock"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/**
 ****************************************************************************************
 * @brief Memory allocation to send a MQTT packet
 * @param[in] packet Main structure of the MQTT packet to send
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_packet_alloc(struct _mosquitto_packet *packet);

/**
 ****************************************************************************************
 * @brief Cleanup a MQTT packet
 * @param[in] packet Main structure of the MQTT packet to cleanup
 ****************************************************************************************
 */
void _mosquitto_packet_cleanup(struct _mosquitto_packet *packet);

/**
 ****************************************************************************************
 * @brief Queue a packet to send
 * @param[out] mosq Main structure of mqtt_client
 * @param[in] packet Main structure of the MQTT packet to send
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_packet_queue(struct mosquitto *mosq, struct _mosquitto_packet *packet);

/**
 ****************************************************************************************
 * @brief Create a socket and connect it to 'ip' on port 'port'.
 * @param[out] mosq Main structure of mqtt_client
 * @param[in] host Broker IP address
 * @param[in] host port port number
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_socket_connect(struct mosquitto *mosq, const char *host, uint16_t port);

/**
 ****************************************************************************************
 * @brief Close a socket associated with a context and set it to -1.
 * @param[out] mosq Main structure of mqtt_client
 * @return 1 on failure and returns 0 on success.
 ****************************************************************************************
 */
int _mosquitto_socket_close(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief NX_TCP_SOCKET alloc/create/bind/connect a socket
 * @param[out] mosq Main structure of mqtt_client
 * @param[in] host Broker IP address
 * @param[in] host port port number
 * @return MOSQ_ERR_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int _mosquitto_try_connect(struct mosquitto *mosq, const char *host, uint16_t port);

/**
 ****************************************************************************************
 * @brief TLS init/set/connect if MQTT TLS is set 1
 * @param[out] mosq Main structure of mqtt_client
 * @return MOSQ_ERR_SUCCESS if succeed, TLS error code else.
 ****************************************************************************************
 */
int _mosquitto_socket_connect_step3(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Read 1-byte on input packet pointer
 * @param[in] packet Packet pointer of the MQTT packet received
 * @param[out] byte Output value (1-byte)
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
int _mosquitto_read_byte(struct _mosquitto_packet *packet, uint8_t *byte);

/**
 ****************************************************************************************
 * @brief Read a data of specific size
 * @param[in] packet Packet pointer of the MQTT packet received
 * @param[out] bytes Output data
 * @param[in] count Data size (byte)
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
int _mosquitto_read_bytes(struct _mosquitto_packet *packet, void *bytes, uint32_t count);

/**
 ****************************************************************************************
 * @brief Read a string from MQTT Packet
 * @param[in] packet Packet pointer of the MQTT packet received
 * @param[out] str Output string
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
int _mosquitto_read_string(struct _mosquitto_packet *packet, char **str);

/**
 ****************************************************************************************
 * @brief Read uint16_t data (2-byte)
 * @param[in] packet Packet pointer of the MQTT packet received
 * @param[out] str Output word
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
int _mosquitto_read_uint16(struct _mosquitto_packet *packet, uint16_t *word);

/**
 ****************************************************************************************
 * @brief Write 1-byte on output packet pointer
 * @param[in] packet Packet pointer of the MQTT packet to send
 * @param[out] byte Input value (1-byte)
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
void _mosquitto_write_byte(struct _mosquitto_packet *packet, uint8_t byte);

/**
 ****************************************************************************************
 * @brief Write a data of specific size
 * @param[in] packet Packet pointer of the MQTT packet to send
 * @param[out] bytes Input data
 * @param[in] count Data size (byte)
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
void _mosquitto_write_bytes(struct _mosquitto_packet *packet, const void *bytes, uint32_t count);

/**
 ****************************************************************************************
 * @brief Write a string from MQTT Packet
 * @param[in] packet Packet pointer of the MQTT packet to send
 * @param[out] str Input string
 * @param[in] length String length
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
void _mosquitto_write_string(struct _mosquitto_packet *packet, const char *str, uint16_t length);

/**
 ****************************************************************************************
 * @brief Write uint16_t data (2-byte)
 * @param[in] packet Packet pointer of the MQTT packet to send
 * @param[out] str Input word
 * @return MOSQ_ERR_SUCCESS if succeed, MOSQ_ERR_PROTOCOL else.
 ****************************************************************************************
 */
void _mosquitto_write_uint16(struct _mosquitto_packet *packet, uint16_t word);

int _mosquitto_net_read(struct mosquitto *mosq, void *buf, size_t count);
int _mosquitto_net_write(struct mosquitto *mosq, void *buf, size_t count);

/**
 ****************************************************************************************
 * @brief Read a data of specific size with timeout (including TLS decyption)
 * @param[out] mosq Main structure of mqtt_client
 * @param[out] buf Output data
 * @param[in] count Data size (byte)
 * @param[in] timeout If it is expired, this API returns zero. (No packet received)
 * @return Size of payload (0 - No data / -1 - Disconnected)
 ****************************************************************************************
 */
int _mosquitto_net_read_with_timeout(struct mosquitto *mosq, void *buf, size_t count, int timeout);

/**
 ****************************************************************************************
 * @brief Write a data of specific size with timeout (including TLS Encyption)
 * @param[out] mosq Main structure of mqtt_client
 * @param[out] buf Input data
 * @param[in] count Data size (byte)
 * @param[in] timeout If it is expired, this API returns zero. (Data transfer failure)
 * @return 0 if succeed, -1 else.
 ****************************************************************************************
 */
int _mosquitto_net_write_with_timeout(struct mosquitto *mosq, void *buf, size_t count, ULONG timeout /*multiple of 10 ms*/);

int _mosquitto_packet_write(struct mosquitto *mosq);
int _mosquitto_packet_read(struct mosquitto *mosq);

int _mosquitto_tls_init(struct mosquitto *mosq);
int _mosquitto_tls_deinit(struct mosquitto *mosq);
int _mosquitto_tls_set(struct mosquitto *mosq);
int _mosquitto_send_func(void *ctx, const unsigned char *buf, size_t len);
UINT _mosquitto_send_packet(struct mosquitto *mosq, char *packet_ptr, size_t count, ULONG wait_option /*multiple of 10 ms*/);

int mosquitto__socket_connect_tls(struct mosquitto *mosq);
UINT _mosquitto__socket_restore_tls(struct mosquitto *mosq);
UINT _mosquitto__socket_save_tls(struct mosquitto *mosq);
UINT _mosquitto__socket_clear_tls(struct mosquitto *mosq);

#endif	/* __NET_MOSQ_H__ */
