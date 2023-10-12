/**
 *****************************************************************************************
 * @file	mosquitto.h
 * @brief	Header file - MQTT module task
 * @todo	File name : mosquitto.h -> mqtt_client_task.h
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

#ifndef	__MOSQUITTO_H__
#define	__MOSQUITTO_H__

#include "stdbool.h"

/* Do not commit if log's define is changed. */
#undef	MQTT_DEBUG_INFO
#undef	MQTT_DEBUG_TEMP
#define	MQTT_DEBUG_ERROR
#define	MQTT_DEBUG_PRINT

#ifdef	MQTT_DEBUG_INFO
#define MQTT_DBG_INFO		PRINTF
#else
#define MQTT_DBG_INFO(...)	do { } while (0);
#endif

#ifdef  MQTT_DEBUG_ERROR
#define MQTT_DBG_ERR		PRINTF
#else
#define MQTT_DBG_ERR(...)	do { } while (0);
#endif

#ifdef  MQTT_DEBUG_TEMP
#define MQTT_DBG_TEMP		PRINTF
#else
#define MQTT_DBG_TEMP(...)	do { } while (0);
#endif

#ifdef  MQTT_DEBUG_PRINT
#define MQTT_DBG_PRINT		PRINTF
#else
#define MQTT_DBG_PRINT(...)	do { } while (0);
#endif


/// mqtt_client error codes
typedef enum _mqtt_client_error_code
{
	/// Connection pending
	MOSQ_ERR_CONN_PENDING				= -1,
	/// No error
	MOSQ_ERR_SUCCESS					= 0,
	/// Out of memeory
	MOSQ_ERR_NOMEM						= 1,
	/// A network protocol error occurred when communicating with the broker
	MOSQ_ERR_PROTOCOL					= 2,
	/// Invalid function arguments provided
	MOSQ_ERR_INVAL						= 3,
	/// The client is not currently connected
	MOSQ_ERR_NO_CONN					= 4,
	/// The connection was refused
	MOSQ_ERR_CONN_REFUSED				= 5,
	/// Message not found (internal error)
	MOSQ_ERR_NOT_FOUND					= 6,
	/// The connection was lost
	MOSQ_ERR_CONN_LOST					= 7,
	/// A TLS error occurred
	MOSQ_ERR_TLS						= 8,
	/// Payload too large
	MOSQ_ERR_PAYLOAD_SIZE				= 9,
	/// This feature is not supported
	MOSQ_ERR_NOT_SUPPORTED				= 10,
	/// Authorization failed
	MOSQ_ERR_AUTH						= 11,
	/// Access denied by ACL
	MOSQ_ERR_ACL_DENIED					= 12,
	/// Unknown error
	MOSQ_ERR_UNKNOWN					= 13,
	/// Unknown error
	MOSQ_ERR_ERRNO						= 14,
	/// Lookup error
	MOSQ_ERR_EAI						= 15,
	/// Proxy error
	MOSQ_ERR_PROXY						= 16,
	/// mqtt_client is forcibly terminated
	MOSQ_ERR_STOP						= 17,
	/// Invalid certificate (internal error)
	MOSQ_ERR_TLS_INVALID_CERT			= 18,
	/// TLS Handshake failed
	MOSQ_ERR_TLS_HANDSHAKE				= 19,
	/// TLS Handshake failed (Certificate)
	MOSQ_ERR_TLS_HANDSHAKE_CERT			= 20,
	/// TLS Handshake failed (Time expired)
	MOSQ_ERR_TLS_HANDSHAKE_TIME_EXPIRED	= 21,
	/// mqtt_client session is forcibly disconnected
	MOSQ_ERR_DISCONNECT					= 23,
} mqtt_client_error_code;

/* Error values */
enum mosq_opt_t
{
	MOSQ_OPT_PROTOCOL_VERSION = 1,
};

/// Define MQTT version to v3.1
#define MQTT_PROTOCOL_V31 3
/// Define MQTT version to v3.1.1
#define MQTT_PROTOCOL_V311 4

/// mqtt_client message struct
struct mosquitto_message
{
	/// Message ID
	int mid;
	/// Message topic
	char *topic;
	/// Message payload
	void *payload;
	/// Message payload length
	int payloadlen;
	/// Message QoS
	int qos;
	/// Message retain use
	bool retain;
};

struct mosquitto;

/**
 ****************************************************************************************
 * @brief Create a new mosquitto client instance.
 * @param[in] id String to use as the client id. If NULL, a random client id will be
 * generated. If id is NULL, clean_session must be true.
 * @param[in] clean_session set to true to instruct the broker to clean all messages
 * and subscriptions on disconnect, false to instruct it to keep them.
 * Note that a client will never discard its own outgoing messages on disconnect.
 * Calling <mosquitto_connect> or <mosquitto_reconnect> will cause the messages to be
 * resent.
 * Use <mosquitto_reinitialise> to reset a client to its original state..
 * @param[in] obj A user pointer that will be passed as an argument to any callbacks that
 * are specified.
 * @return Pointer to a struct mosquitto on success. 
 * NULL on failure. Interrogate errno to determine the cause for the failure:
 * - ENOMEM on out of memory.
 * - EINVAL on invalid input parameters.
 ****************************************************************************************
 */
struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *obj);

/**
 ****************************************************************************************
 * @brief Use to free memory associated with a mosquitto client instance.
 * @param[in] mosq a struct mosquitto pointer to free.
 ****************************************************************************************
 */
void mosquitto_destroy(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief This function allows an existing mosquitto client to be reused. Call on a
 * mosquitto instance to close any open network connections, free memory
 * and reinitialise the client with the new parameters. The end result is the same as the
 * output of <mosquitto_new>.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] id string to use as the client id. If NULL, a random client id will be
 * generated. If id is NULL, clean_session must be true.
 * @param[in] clean_session set to true to instruct the broker to clean all messages
 * and subscriptions on disconnect, false to instruct it to keep them.
 * Must be set to true if the id parameter is NULL.
 * @param[in] obj A user pointer that will be passed as an argument to any callbacks that
 * are specified.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_session, void *obj);

/**
 ****************************************************************************************
 * @brief Configure will information for a mosquitto instance. By default, clients do
 * not have a will.  This must be called before calling <mosquitto_connect>.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] topic the topic on which to publish the will.
 * @param[in] payloadlen the size of the payload (bytes). Valid values are between 0 and
 * 268,435,455.
 * @param[in] payload pointer to the data to send. If payloadlen > 0 this must be a
 * valid memory location.
 * @param[in] qos integer value 0, 1 or 2 indicating the Quality of Service to be used
 * for the will.
 * @param[in] retain set to true to make the will a retained message.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 * - MOSQ_ERR_PAYLOAD_SIZE - if payloadlen is too large.
 ****************************************************************************************
 */
int mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

/**
 ****************************************************************************************
 * @brief Remove a previously configured will. This must be called before calling
 * <mosquitto_connect>.
 * @param[in] mosq a valid mosquitto instance.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 ****************************************************************************************
 */
int mosquitto_will_clear(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Configure username and password for a mosquitton instance. This is only
 * supported by brokers that implement the MQTT spec v3.1. By default, no username or
 * password will be sent.
 * If username is NULL, the password argument is ignored.
 * This must be called before calling mosquitto_connect().
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] username the username to send as a string, or NULL to disable authentication.
 * @param[in] password the password to send as a string. Set to NULL when username is
 * valid in order to send just a username.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_username_pw_set(struct mosquitto *mosq, const char *username, const char *password);

/**
 ****************************************************************************************
 * @brief Connect to an MQTT broker.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] host the hostname or ip address of the broker to connect to.
 * @param[in] port the network port to connect to. Usually 1883.
 * @param[in] keepalive the number of seconds after which the broker should send a PING
 * message to the client if no other messages have been exchanged in that time.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_ERRNO - if a system call returned an error. The variable errno
 * contains the error code, even on Windows.
 ****************************************************************************************
 */
int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive);

/**
 ****************************************************************************************
 * @brief Connect to an MQTT broker. This extends the functionality of
 * <mosquitto_connect> by adding the bind_address parameter. Use this function
 * if you need to restrict network communication over a particular interface.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] host the hostname or ip address of the broker to connect to.
 * @param[in] port the network port to connect to. Usually 1883.
 * @param[in] keepalive the number of seconds after which the broker should send a PING
 * message to the client if no other messages have been exchanged in that time.
 * @param[in] bind_address - the hostname or ip address of the local network interface to
 * bind to.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_ERRNO - if a system call returned an error. The variable errno
 * contains the error code, even on Windows.
 ****************************************************************************************
 */
int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);

/**
 ****************************************************************************************
 * @brief Connect to an MQTT broker. This is a non-blocking call. If you use
 * <mosquitto_connect_bind_async> your client must use the threaded interface
 * <mosquitto_loop_start>. If you need to use <mosquitto_loop>, you must use
 * <mosquitto_connect> to connect the client.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] host the hostname or ip address of the broker to connect to.
 * @param[in] port the network port to connect to. Usually 1883.
 * @param[in] keepalive the number of seconds after which the broker should send a PING
 * message to the client if no other messages have been exchanged in that time.
 * @param[in] bind_address - the hostname or ip address of the local network interface to
 * bind to.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_ERRNO - if a system call returned an error. The variable errno
 * contains the error code, even on Windows.
 ****************************************************************************************
 */
int mosquitto_connect_bind_async(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);

/**
 ****************************************************************************************
 * @brief Reconnect to a broker.
 * @param[in] mosq a valid mosquitto instance.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_reconnect(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Reconnect to a broker. Non blocking version of <mosquitto_reconnect>.
 *
 * This function provides an easy way of reconnecting to a broker after a
 * connection has been lost. It uses the values that were provided in the
 * <mosquitto_connect> or <mosquitto_connect_async> calls. It must not be
 * called before <mosquitto_connect>.
 * @param[in] mosq a valid mosquitto instance.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
 int mosquitto_reconnect_async(struct mosquitto *mosq);
 
 /**
 ****************************************************************************************
 * @brief Disconnect from the broker.
 * @param[in] mosq a valid mosquitto instance.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
 int mosquitto_disconnect(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Publish a message on a given topic.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] mid pointer to an int. If not NULL, the function will set this to the
 * message id of this particular message. This can be then used with the publish callback
 * to determine when the message has been sent.
 * Note that although the MQTT protocol doesn't use message ids for messages with QoS=0,
 * libmosquitto assigns them message ids so they can be tracked with this parameter.
 * @param[in] topic null terminated string of the topic to publish to.
 * @param[in] payloadlen - the size of the payload (bytes).
 * @param[in] payload - pointer to the data to send. If payloadlen > 0 this must be a
 * valid memory location.
 * @param[in] qos integer value 0, 1 or 2 indicating the Quality of Service to be
 * used for the message.
 * @param[in] retain set to true to make the message retained.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 * - MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
 * - MOSQ_ERR_PROTOCOL - if there is a protocol error communicating with the broker.
 * - MOSQ_ERR_PAYLOAD_SIZE - if payloadlen is too large.
 ****************************************************************************************
 */
int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

/**
 ****************************************************************************************
 * @brief Subscribe to a topic.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] mid a pointer to an int. If not NULL, the function will set this to the
 * message id of this particular message. This can be then used with the subscribe
 * callback to determine when the message has been sent.
 * @param[in] sub the subscription pattern.
 * @param[in] qos the requested Quality of Service for this subscription.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 * - MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
 ****************************************************************************************
 */
int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos);

/**
 ****************************************************************************************
 * @brief Subscribe to a topic.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] mid a pointer to an int. If not NULL, the function will set this to the
 * message id of this particular message. This can be then used with the subscribe
 * callback to determine when the message has been sent.
 * @param[in] sub the subscription pattern.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 * - MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
 ****************************************************************************************
 */
int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub);

/**
 ****************************************************************************************
 * @brief Copy the contents of a mosquitto message to another message.
 * Useful for preserving a message received in the on_message() callback.
 * @param[in] dst a pointer to a valid mosquitto_message struct to copy to.
 * @param[in] src a pointer to a valid mosquitto_message struct to copy from.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_message_copy(struct mosquitto_message *dst, const struct mosquitto_message *src);

/**
 ****************************************************************************************
 * @brief Completely free a mosquitto_message struct.
 * @param[in] message pointer to a mosquitto_message pointer to free.
 ****************************************************************************************
 */
void mosquitto_message_free(struct mosquitto_message **message);

/**
 ****************************************************************************************
 * @brief The main network loop for the client. You must call this frequently in order
 * to keep communications between the client and broker working. If incoming data is
 * present it will then be processed. Outgoing commands, from e.g. <mosquitto_publish>,
 * are normally sent immediately that their function is called, but this is not always
 * possible. <mosquitto_loop> will also attempt to send any remaining outgoing messages,
 * which also includes commands that are part of the flow for messages with QoS>0.
 *
 * An alternative approach is to use <mosquitto_loop_start> to run the client loop in its
 * own thread.
 *
 * This calls select() to monitor the client network socket. If you want to integrate
 * mosquitto client operation with your own select() call, use <mosquitto_socket>,
 * <mosquitto_loop_read>, <mosquitto_loop_write> and <mosquitto_loop_misc>.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] timeout Maximum number of milliseconds to wait for network activity
 * in the select() call before timing out. Set to 0 for instant return.
 * Set negative to use the default of 1000ms.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 * - MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
 * - MOSQ_ERR_CONN_LOST - if the connection to the broker was lost.
 * - MOSQ_ERR_PROTOCOL - if there is a protocol error communicating with the broker.
 * - MOSQ_ERR_ERRNO - if a system call returned an error. The variable errno
 * contains the error code, even on Windows.
 ****************************************************************************************
 */
int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets);

/**
 ****************************************************************************************
 * @brief This function call loop() for you in an infinite blocking loop. It is useful
 * for the case where you only want to run the MQTT client loop in your program.
 *
 * It handles reconnecting in case server connection is lost. If you call
 * mosquitto_disconnect() in a callback it will return.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] timeout Maximum number of milliseconds to wait for network activity
 * in the select() call before timing out. Set to 0 for instant return.
 * Set negative to use the default of 1000ms.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 * - MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
 * - MOSQ_ERR_CONN_LOST - if the connection to the broker was lost.
 * - MOSQ_ERR_PROTOCOL - if there is a protocol error communicating with the broker.
 * - MOSQ_ERR_ERRNO - if a system call returned an error. The variable errno
 * contains the error code, even on Windows.
 ****************************************************************************************
 */
int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets);

/**
 ****************************************************************************************
 * @brief Carry out miscellaneous operations required as part of the network loop.
 * This should only be used if you are not using mosquitto_loop() and are monitoring the
 * client network socket for activity yourself.
 *
 * This function deals with handling PINGs and checking whether messages need to be
 * retried, so should be called fairly frequently. 
 * @param[in] mosq a valid mosquitto instance.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
 ****************************************************************************************
 */
int mosquitto_loop_misc(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Used to set options for the client.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] option the option to set.
 * @param[in] value the option specific value.
 ****************************************************************************************
 */
int mosquitto_opts_set(struct mosquitto *mosq, enum mosq_opt_t option, void *value);

/**
 ****************************************************************************************
 * @brief Configure verification of the server hostname in the server certificate. If
 * value is set to true, it is impossible to guarantee that the host you are connecting
 * to is not impersonating your server. This can be useful in initial server testing,
 * but makes it possible for a malicious third party to impersonate your server through
 * DNS spoofing, for example.
 * Do not use this function in a real system. Setting value to true makes the connection
 * encryption pointless.
 * Must be called before <mosquitto_connect>.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] value if set to false, the default, certificate hostname checking is
 * performed. If set to true, no hostname checking is performed and the connection is
 * insecure.
 * @return
 * - MOSQ_ERR_SUCCESS - on success
 * - MOSQ_ERR_INVAL - if the input parameters were invalid
 ****************************************************************************************
 */
int mosquitto_tls_insecure_set(struct mosquitto *mosq, int insecure);

/**
 ****************************************************************************************
 * @brief Register a CA Certificate.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] ca_cert CA Certificate
 * @param[in] ca_cert_len CA Certificate length
 ****************************************************************************************
 */
int mosquitto_tls_ca_cert_set(struct mosquitto *mosq, char *ca_cert, int ca_cert_len);

/**
 ****************************************************************************************
 * @brief Register a Client Certificate
 * @param[in] mosq a valid mosquitto instance
 * @param[in] cert Client Certificate
 * @param[in] cert_len Client Certificate length
 ****************************************************************************************
 */
int mosquitto_tls_cert_set(struct mosquitto *mosq, char *cert, int cert_len);

/**
 ****************************************************************************************
 * @brief Register a Client Private Key
 * @param[in] mosq a valid mosquitto instance
 * @param[in] private_key Client Private Key
 * @param[in] private_key_len Client Private Key
 ****************************************************************************************
 */
int mosquitto_tls_private_key_set(struct mosquitto *mosq, char *private_key, int private_key_len);

/**
 ****************************************************************************************
 * @brief Register a Client Diffie hellman parameter
 * @param[in] mosq a valid mosquitto instance
 * @param[in] dh_param Client Diffie hellman parameter 
 * @param[in] dh_param_len Client Diffie hellman parameter length
 ****************************************************************************************
 */
int mosquitto_tls_dh_param_set(struct mosquitto *mosq, char *dh_param, int dh_param_len);

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
/**
 ****************************************************************************************
 * @brief Register a TLS alpn protocol name
 * @param[in] mosq a valid mosquitto instance
 * @param[in] alpn TLS alpn protocol name
 * @param[in] alpn_count number of ALPN protocol name
 ****************************************************************************************
 */
int mosquitto_tls_alpn_set(struct mosquitto *mosq, char **alpn, int alpn_count);

/**
 ****************************************************************************************
 * @brief Register a TLS SNI
 * @param[in] mosq a valid mosquitto instance
 * @param[in] alpn TLS SNI name
 ****************************************************************************************
 */
int mosquitto_tls_sni_set(struct mosquitto *mosq, char *sni);

/**
 ****************************************************************************************
 * @brief Register TLS cipher suits
 * @param[in] mosq a valid mosquitto instance
 * @param[in] cipher_suits a list of TLS cipher suit
 ****************************************************************************************
 */
int mosquitto_tls_cipher_suits_set(struct mosquitto *mosq, int *cipher_suits);

#endif // __MQTT_TLS_OPTIONAL_CONFIG__
/**
 ****************************************************************************************
 * @brief Set advanced SSL/TLS options. Must be called before <mosquitto_connect>.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] cert_reqs an integer defining the verification requirements the client
 * will impose on the server. This can be one of:
 * - SSL_VERIFY_NONE (0): the server will not be verified in any way.
 * - SSL_VERIFY_PEER (1): the server certificate will be verified
 * and the connection aborted if the verification fails.
 * The default and recommended value is SSL_VERIFY_PEER. Using
 * SSL_VERIFY_NONE provides no security.
 * @param[in] tls_version the version of the SSL/TLS protocol to use as a string. If NULL,
 * the default value is used. The default value and the available values depend on the
 * version of openssl that the library was compiled against. For openssl >= 1.0.1, the
 * available options are tlsv1.2, tlsv1.1 and tlsv1, with tlv1.2 as the default.
 * For openssl < 1.0.1, only tlsv1 is available.
 * @param[in] ciphers a string describing the ciphers available for use. See the
 * "openssl ciphers" tool for more information. If NULL, the default ciphers will be used.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_tls_opts_set(struct mosquitto *mosq, int cert_reqs, const char *tls_version, const char *ciphers);

/**
 ****************************************************************************************
 * @brief Configure the client for pre-shared-key based TLS support. Must be called
 * before <mosquitto_connect>.
 *
 * Cannot be used in conjunction with <mosquitto_tls_set>.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] psk the pre-shared-key in hex format with no leading "0x".
 * @param[in] identity the identity of this client. May be used as the username depending
 * on the server settings.
 * @param[in] ciphers a string describing the ciphers available for use. See the
 * "openssl ciphers" tool for more information. If NULL, the default ciphers will be used.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_tls_psk_set(struct mosquitto *mosq, const char *psk, const char *identity, const char *ciphers);

/**
 ****************************************************************************************
 * @brief Set the connect callback. This is called when the broker sends a CONNACK
 * message in response to a connection.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_connect a callback function in the following form:
 *               void callback(void);
 * - 0 - success
 * - 1 - connection refused (unacceptable protocol version)
 * - 2 - connection refused (identifier rejected)
 * - 3 - connection refused (broker unavailable)
 * - 4-255 - reserved for future use
 ****************************************************************************************
 */
void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(void));

/**
 ****************************************************************************************
 * @brief Set the disconnect callback. This is called when the broker has received the
 * DISCONNECT command and has disconnected the client.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_disconnect a callback function in the following form:
 *                  void callback(void);
 ****************************************************************************************
 */
void mosquitto_disconnect_callback_set(struct mosquitto *mosq, void (*on_disconnect)(void));

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
/**
 ****************************************************************************************
 * @brief Set the disconnect2 callback. This is called when mqtt client is disconnected by 
 * receiving a message with invalid length if the connection is clean_session=0 and qos>0.
 * On receipt of this callback, application needs to clear the message in Broker 
 * by re-connecting with clean_sesion=1.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_disconnect2 a callback function in the following form:
 *                  void callback2(void);
 ****************************************************************************************
 */
void mosquitto_disconnect2_callback_set(struct mosquitto *mosq, void (*on_disconnect2)(void));
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

/**
 ****************************************************************************************
 * @brief Set the publish callback. This is called when a message initiated with
 * <mosquitto_publish> has been sent to the broker successfully.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_publish a callback function in the following form:
 *                  void callback(int mid)
 * @param[in] mid the message id of the sent message.
 ****************************************************************************************
 */
void mosquitto_publish_callback_set(struct mosquitto *mosq, void (*on_publish)(int));

/**
 ****************************************************************************************
 * @brief Set the message callback. This is called when a message is received from the
 * broker.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_message a callback function in the following form:
 *                  void callback(const char *buf, int len, const char *topic)
 * @param[in] buf the message paylod
 * @param[in] len the message paylod length
 * @param[in] topic the message's topic
 ****************************************************************************************
 */
void mosquitto_message_callback_set(struct mosquitto *mosq, void (*on_message)(const char *buf, int len, const char *topic));

/**
 ****************************************************************************************
 * @brief Set the subscribe callback. This is called when the broker responds to a
 * subscription request.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_subscribe a callback function in the following form:
 *                  void callback(void)
 ****************************************************************************************
 */
void mosquitto_subscribe_callback_set(struct mosquitto *mosq, void (*on_subscribe)(void));

/**
 ****************************************************************************************
 * @brief Set the unsubscribe callback. This is called when the broker responds to a
 * unsubscription request.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_unsubscribe a callback function in the following form:
 *                  void callback(void)
 ****************************************************************************************
 */
void mosquitto_unsubscribe_callback_set(struct mosquitto *mosq,	void (*on_unsubscribe)(void));

/**
 ****************************************************************************************
 * @brief Set the logging callback. This should be used if you want event logging
 * information from the client library.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] on_log a callback function in the following form:
 *                  void callback(const char *str)
 * @param[in] str the message string
 ****************************************************************************************
 */
void mosquitto_log_callback_set(struct mosquitto *mosq, void (*on_log)(const char *));

/**
 ****************************************************************************************
 * @brief Set the number of QoS 1 and 2 messages that can be "in flight" at one time.
 * An in flight message is part way through its delivery flow. Attempts to send
 * further messages with <mosquitto_publish> will result in the messages being
 * queued until the number of in flight messages reduces.
 *
 * A higher number here results in greater message throughput, but if set
 * higher than the maximum in flight messages on the broker may lead to
 * delays in the messages being acknowledged.
 *
 * Set to 0 for no maximum.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] max_inflight_messages the maximum number of inflight messages. Defaults
 * to 20.
 * @return
 * - MOSQ_ERR_SUCCESS - on success.
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 ****************************************************************************************
 */
int mosquitto_max_inflight_messages_set(struct mosquitto *mosq,	unsigned int max_inflight_messages);

/**
 ****************************************************************************************
 * @brief Set the number of seconds to wait before retrying messages. This applies to
 * publish messages with QoS>0. May be called at any time.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] message_retry the number of seconds to wait for a response before retrying.
 * Defaults to 20.
 ****************************************************************************************
 */
void mosquitto_message_retry_set(struct mosquitto *mosq, unsigned int message_retry);

/**
 ****************************************************************************************
 * @brief When <mosquitto_new> is called, the pointer given as the "obj" parameter
 * will be passed to the callbacks as user data. The <mosquitto_user_data_set>
 * function allows this obj parameter to be updated at any time. This function
 * will not modify the memory pointed to by the current user data pointer. If
 * it is dynamically allocated memory you must free it yourself.
 * @param[in] mosq a valid mosquitto instance.
 * @param[in] obj A user pointer that will be passed as an argument to any callbacks
 * that are specified.
 ****************************************************************************************
 */
void mosquitto_user_data_set(struct mosquitto *mosq, void *obj);

/**
 ****************************************************************************************
 * @brief Call to obtain a const string description of a mosquitto error number.
 * @param[in] mosq_errno a mosquitto error number.
 * @return A constant string describing the error.
 ****************************************************************************************
 */
const char *mosquitto_strerror(int mosq_errno);

/**
 ****************************************************************************************
 * @brief Call to obtain a const string description of an MQTT connection result.
 * @param[in] connack_code a mosquitto CONNACK code.
 * @return A constant string describing the code.
 ****************************************************************************************
 */
const char *mosquitto_connack_string(int connack_code);

/**
 ****************************************************************************************
 * @brief Tokenise a topic or subscription string into an array of strings representing
 * the topic hierarchy.
 *
 * For example:
 *
 * subtopic: "a/deep/topic/hierarchy"
 *
 * Would result in:
 *
 * topics[0] = "a"
 * topics[1] = "deep"
 * topics[2] = "topic"
 * topics[3] = "hierarchy"
 *
 * and:
 *
 * subtopic: "/a/deep/topic/hierarchy/"
 *
 * Would result in:
 *
 * topics[0] = NULL
 * topics[1] = "a"
 * topics[2] = "deep"
 * topics[3] = "topic"
 * topics[4] = "hierarchy"
 *
 * Example:
 *
 * > char **topics;
 * > int topic_count;
 * > int i;
 * >
 * > mosquitto_sub_topic_tokenise("$SYS/broker/uptime", &topics, &topic_count);
 * >
 * > for(i=0; i<token_count; i++){
 * >     printf("%d: %s\n", i, topics[i]);
 * > }
 *
 * @param[in] subtopic the subscription/topic to tokenise
 * @param[in] topics a pointer to store the array of strings
 * @param[in] count an int pointer to store the number of items in the topics array.
 * @return
 * - MOSQ_ERR_SUCCESS - on success
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_sub_topic_tokenise(const char *subtopic, char ***topics, int *count);

 /**
 ****************************************************************************************
 * @brief Free memory that was allocated in <mosquitto_sub_topic_tokenise>.
 * @param[in] topics pointer to string array.
 * @param[in] count count of items in string array.
 * @return
 * - MOSQ_ERR_SUCCESS - on success
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 ****************************************************************************************
 */
int mosquitto_sub_topic_tokens_free(char ***topics, int count);

/**
 ****************************************************************************************
 * @brief Check whether a topic matches a subscription.
 *
 * For example:
 *
 * foo/bar would match the subscription foo/# or +/bar
 * non/matching would not match the subscription non/+/+
 * @param[in] sub subscription string to check topic against.
 * @param[in] topic topic to check.
 * @param[in] result bool pointer to hold result. Will be set to true if the topic
 * matches the subscription.
 * @return
 * - MOSQ_ERR_SUCCESS - on success
 * - MOSQ_ERR_INVAL - if the input parameters were invalid.
 * - MOSQ_ERR_NOMEM - if an out of memory condition occurred.
 ****************************************************************************************
 */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result);

/**
 ****************************************************************************************
 * @brief Check whether a topic to be used for publishing is valid.
 *
 * This searches for + or # in a topic and checks its length.
 *
 * This check is already carried out in <mosquitto_publish> and <mosquitto_will_set>,
 * there is no need to call it directly before them. It may be useful if you wish to
 * check the validity of a topic in advance of making a connection for example.
 * @param[in] topic the topic to check.
 * @return
 * - MOSQ_ERR_SUCCESS - for a valid topic
 * - MOSQ_ERR_INVAL - if the topic contains a + or a #, or if it is too long.
 ****************************************************************************************
 */
int mosquitto_pub_topic_check(const char *topic);

/**
 ****************************************************************************************
 * @brief Check whether a topic to be used for subscribing is valid.
 *
 * This searches for + or # in a topic and checks that they aren't in invalid positions,
 * such as with foo/#/bar, foo/+bar or foo/bar#, and checks its length.
 *
 * This check is already carried out in <mosquitto_subscribe> and <mosquitto_unsubscribe>,
 * there is no need to call it directly before them.
 * It may be useful if you wish to check the validity of a topic in advance of making a
 * connection for example.
 * @param[in] topic the topic to check.
 * @return
 * - MOSQ_ERR_SUCCESS - for a valid topic
 * - MOSQ_ERR_INVAL - if the topic contains a + or a # that is in an invalid position,
 * or if it is too long.
 ****************************************************************************************
 */
int mosquitto_sub_topic_check(const char *topic);

/**
 ****************************************************************************************
 * @brief Register the sending PINGREQ timer for DPM
 * @param[in] mosq a valid mosquitto instance.
 * @return 0 if succeed, error codes else
 ****************************************************************************************
 */
UINT mosquitto_pingreq_timer_register(struct mosquitto *mosq);

/**
 ****************************************************************************************
 * @brief Unregister the sending PINGREQ timer for DPM
 * @param[in] tid Timer ID
 * @return 0 if succeed, error codes else
 ****************************************************************************************
 */
UINT mosquitto_pingreq_timer_unregister(int tid);

/**
 ****************************************************************************************
 * @brief A function called when the subscriber timer is expired
 ****************************************************************************************
 */
void mosquitto_sub_pingreq_timer_cb(char *timer_name);

/**
 ****************************************************************************************
 * @brief A function called when the publisher timer is expired
 ****************************************************************************************
 */
void mosquitto_pub_pingreq_timer_cb(char *timer_name);

/**
 ****************************************************************************************
 * @brief Call the dpm_app_sleep_ready_set() for setting DPM_SLEEP
 * @param[in] dpm_name DPM module name
 * @param[in] func_name Input __func__ (To track the function called)
 * @param[in] line Input __LINE__ (To track the function called)
 * @return 0 if succeed, error codes else
 ****************************************************************************************
 */
int mqtt_client_dpm_sleep(char *dpm_name, char const *func_name, int line);

/**
 ****************************************************************************************
 * @brief @brief Call the dpm_app_sleep_ready_clear() for clearing DPM_SLEEP
 * @param[in] dpm_name DPM module name
 * @param[in] func_name Input __func__ (To track the function called)
 * @param[in] line Input __LINE__ (To track the function called)
 * @return 0 if succeed, error codes else
 ****************************************************************************************
 */
int mqtt_client_dpm_clear(char *dpm_name, char const *func_name, int line);

#endif	/* __MOSQUITTO_H__ */

/* EOF */
