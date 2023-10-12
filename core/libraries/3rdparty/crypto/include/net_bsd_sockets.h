/**
 * \file net_bsd_sockets.h
 *
 * \brief Network communication functions
 *
 * Wrapper for mebedtls/net_sockets
 *
 */
#ifndef MBEDTLS_NET_BSD_SOCKETS_H
#define MBEDTLS_NET_BSD_SOCKETS_H

#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Initialize a context
 *                 Just makes the context ready to be used or freed safely.
 *
 * \param ctx      Context to initialize
 */
void mbedtls_net_bsd_init( mbedtls_net_context *ctx );


/**
 * \brief           Initiate a connection with host:port, local:port in the given protocol
 *
 * \param ctx       Socket to use
 * \param host      Host to connect to
 * \param port      Port to connect to
 * \param proto     Protocol: MBEDTLS_NET_PROTO_TCP or MBEDTLS_NET_PROTO_UDP
 * \param bind_ip   IP to bind to, can be NULL
 * \param bind_port Port number to use
 *
 * \return         0 if successful, or one of:
 *                      MBEDTLS_ERR_NET_SOCKET_FAILED,
 *                      MBEDTLS_ERR_NET_UNKNOWN_HOST,
 *                      MBEDTLS_ERR_NET_CONNECT_FAILED
 *
 * \note           Sets the socket in connected mode even with UDP.
 */
int mbedtls_net_bsd_connect_with_bind( mbedtls_net_context *ctx, const char *host,
									   const char *port, int proto,
									   const char *bind_ip, const char *bind_port );

/**
 * \brief          Initiate a connection with host:port in the given protocol
 *
 * \param ctx      Socket to use
 * \param host     Host to connect to
 * \param port     Port to connect to
 * \param proto    Protocol: MBEDTLS_NET_PROTO_TCP or MBEDTLS_NET_PROTO_UDP
 *
 * \return         0 if successful, or one of:
 *                      MBEDTLS_ERR_NET_SOCKET_FAILED,
 *                      MBEDTLS_ERR_NET_UNKNOWN_HOST,
 *                      MBEDTLS_ERR_NET_CONNECT_FAILED
 *
 * \note           Sets the socket in connected mode even with UDP.
 */
int mbedtls_net_bsd_connect( mbedtls_net_context *ctx, const char *host, const char *port, int proto );

/**
 * \brief          Create a receiving socket on bind_ip:port in the chosen
 *                 protocol. If bind_ip == NULL, all interfaces are bound.
 *
 * \param ctx      Socket to use
 * \param bind_ip  IP to bind to, can be NULL
 * \param port     Port number to use
 * \param proto    Protocol: MBEDTLS_NET_PROTO_TCP or MBEDTLS_NET_PROTO_UDP
 *
 * \return         0 if successful, or one of:
 *                      MBEDTLS_ERR_NET_SOCKET_FAILED,
 *                      MBEDTLS_ERR_NET_BIND_FAILED,
 *                      MBEDTLS_ERR_NET_LISTEN_FAILED
 *
 * \note           Regardless of the protocol, opens the sockets and binds it.
 *                 In addition, make the socket listening if protocol is TCP.
 */
int mbedtls_net_bsd_bind( mbedtls_net_context *ctx, const char *bind_ip, const char *port, int proto );

/**
 * \brief           Accept a connection from a remote client
 *
 * \param bind_ctx  Relevant socket
 * \param client_ctx Will contain the connected client socket
 * \param client_ip Will contain the client IP address
 * \param buf_size  Size of the client_ip buffer
 * \param ip_len    Will receive the size of the client IP written
 *
 * \return          0 if successful, or
 *                  MBEDTLS_ERR_NET_ACCEPT_FAILED, or
 *                  MBEDTLS_ERR_NET_BUFFER_TOO_SMALL if buf_size is too small,
 *                  MBEDTLS_ERR_SSL_WANT_READ if bind_fd was set to
 *                  non-blocking and accept() would block.
 */
int mbedtls_net_bsd_accept( mbedtls_net_context *bind_ctx,
                        mbedtls_net_context *client_ctx,
                        void *client_ip, size_t buf_size, size_t *ip_len );

/**
 * \brief          Set the socket blocking
 *
 * \param ctx      Socket to set
 *
 * \return         0 if successful, or a non-zero error code
 */
int mbedtls_net_bsd_set_block( mbedtls_net_context *ctx );

/**
 * \brief          Set the socket non-blocking
 *
 * \param ctx      Socket to set
 *
 * \return         0 if successful, or a non-zero error code
 */
int mbedtls_net_bsd_set_nonblock( mbedtls_net_context *ctx );

/**
 * \brief          Portable usleep helper
 *
 * \param usec     Amount of microseconds to sleep
 *
 * \note           Real amount of time slept will not be less than
 *                 select()'s timeout granularity (typically, 10ms).
 */
void mbedtls_net_bsd_usleep( unsigned long usec );

/**
 * \brief          Read at most 'len' characters. If no error occurs,
 *                 the actual amount read is returned.
 *
 * \param ctx      Socket
 * \param buf      The buffer to write to
 * \param len      Maximum length of the buffer
 *
 * \return         the number of bytes received,
 *                 or a non-zero error code; with a non-blocking socket,
 *                 MBEDTLS_ERR_SSL_WANT_READ indicates read() would block.
 */
int mbedtls_net_bsd_recv( void *ctx, unsigned char *buf, size_t len );

/**
 * \brief          Write at most 'len' characters. If no error occurs,
 *                 the actual amount read is returned.
 *
 * \param ctx      Socket
 * \param buf      The buffer to read from
 * \param len      The length of the buffer
 *
 * \return         the number of bytes sent,
 *                 or a non-zero error code; with a non-blocking socket,
 *                 MBEDTLS_ERR_SSL_WANT_WRITE indicates write() would block.
 */
int mbedtls_net_bsd_send( void *ctx, const unsigned char *buf, size_t len );

/**
 * \brief          Read at most 'len' characters, blocking for at most
 *                 'timeout' seconds. If no error occurs, the actual amount
 *                 read is returned.
 *
 * \param ctx      Socket
 * \param buf      The buffer to write to
 * \param len      Maximum length of the buffer
 * \param timeout  Maximum number of milliseconds to wait for data
 *                 0 means no timeout (wait forever)
 *
 * \return         the number of bytes received,
 *                 or a non-zero error code:
 *                 MBEDTLS_ERR_SSL_TIMEOUT if the operation timed out,
 *                 MBEDTLS_ERR_SSL_WANT_READ if interrupted by a signal.
 *
 * \note           This function will block (until data becomes available or
 *                 timeout is reached) even if the socket is set to
 *                 non-blocking. Handling timeouts with non-blocking reads
 *                 requires a different strategy.
 */
int mbedtls_net_bsd_recv_timeout( void *ctx, unsigned char *buf, size_t len,
                      uint32_t timeout );

/**
 * \brief          Gracefully shutdown the connection and free associated data
 *
 * \param ctx      The context to free
 */
void mbedtls_net_bsd_free( mbedtls_net_context *ctx );

int mbedtls_net_bsd_init_dpm(mbedtls_net_context_dpm *ctx, char *name);
int mbedtls_net_bsd_connect_timeout_dpm(mbedtls_net_context_dpm *ctx, const char *host,
										const char *port, int proto, unsigned int timeout);
int mbedtls_net_bsd_connect_with_bind_timeout_dpm(mbedtls_net_context_dpm *ctx,
												  const char *host, const char *port, int proto,
												  const char *bind_ip, const char *bind_port,
												  unsigned int timeout);
int mbedtls_net_bsd_recv_dpm(void *ctx, unsigned char *buf, size_t len);
int mbedtls_net_bsd_send_dpm(void *ctx, const unsigned char *buf, size_t len);
int mbedtls_net_bsd_recv_timeout_dpm(void *ctx, unsigned char *buf, size_t len,
									 uint32_t timeout);
void mbedtls_net_bsd_free_dpm(mbedtls_net_context_dpm *ctx);
#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_NET_BSD_SOCKETS_H */
