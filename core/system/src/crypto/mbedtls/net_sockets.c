/*
 *  TCP/IP or UDP/IP networking functions
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#pragma GCC diagnostic ignored "-Wcomment"

#if defined(MBEDTLS_NET_C)

//REMOVED: #if !defined(unix) && !defined(__unix__) && !defined(__unix) && \
//REMOVED:     !defined(__APPLE__) && !defined(_WIN32)
//REMOVED: #error "This module only works on Unix and Windows, see MBEDTLS_NET_C in config.h"
//REMOVED: #endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#endif

#include "mbedtls/net_sockets.h"

#include <string.h>

//REMOVED: #if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
//REMOVED:     !defined(EFI32)
//REMOVED:
//REMOVED: #ifdef _WIN32_WINNT
//REMOVED: #undef _WIN32_WINNT
//REMOVED: #endif
//REMOVED: /* Enables getaddrinfo() & Co */
//REMOVED: #define _WIN32_WINNT 0x0501
//REMOVED: #include <ws2tcpip.h>
//REMOVED:
//REMOVED: #include <winsock2.h>
//REMOVED: #include <windows.h>
//REMOVED:
//REMOVED: #if defined(_MSC_VER)
//REMOVED: #if defined(_WIN32_WCE)
//REMOVED: #pragma comment( lib, "ws2.lib" )
//REMOVED: #else
//REMOVED: #pragma comment( lib, "ws2_32.lib" )
//REMOVED: #endif
//REMOVED: #endif /* _MSC_VER */
//REMOVED:
//REMOVED: #define read(fd,buf,len)        recv(fd,(char*)buf,(int) len,0)
//REMOVED: #define write(fd,buf,len)       send(fd,(char*)buf,(int) len,0)
//REMOVED: #define close(fd)               closesocket(fd)
//REMOVED:
//REMOVED: static int wsa_init_done = 0;
//REMOVED:
//REMOVED: #else /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */
//REMOVED:
//REMOVED: #include <sys/types.h>
//REMOVED: #include <sys/socket.h>
//REMOVED: #include <netinet/in.h>
//REMOVED: #include <arpa/inet.h>
//REMOVED: #include <sys/time.h>
//REMOVED: #include <unistd.h>
//REMOVED: #include <signal.h>
//REMOVED: #include <fcntl.h>
//REMOVED: #include <netdb.h>
//REMOVED: #include <errno.h>

//REMOVED: #include "tx_api.h"
//REMOVED: #include "nx_api.h"
//REMOVED: #include "nxd_bsd.h"
//REMOVED:
//REMOVED: #define read(fd,buf,len)        recv(fd,(char*)buf,(int) len,0)
//REMOVED: #define write(fd,buf,len)       send(fd,(char*)buf,(int) len,0)
//REMOVED: #define close(fd)               soc_close(fd)
//REMOVED: #define shutdown(fd, mod)
//REMOVED: #define signal(v,a)
//REMOVED:
//REMOVED: #endif /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */
//REMOVED:
//REMOVED: /* Some MS functions want int and MSVC warns if we pass size_t,
//REMOVED:  * but the standard fucntions use socklen_t, so cast only for MSVC */
//REMOVED: #if defined(_MSC_VER)
//REMOVED: #define MSVC_INT_CAST   (int)
//REMOVED: #else
//REMOVED: #define MSVC_INT_CAST
//REMOVED: #endif
//REMOVED:
//REMOVED: #include <stdio.h>
//REMOVED:
//REMOVED: #include <time.h>
//REMOVED:
//REMOVED: #include <stdint.h>
//REMOVED:

/******************************************************************************
 *  mbedtls_platform_net_sockets( )
 ******************************************************************************/

static mbedtls_net_primitive_type *mbedtls_net_primitives;

/******************************************************************************
 *  mbedtls_platform_net_sockets( )
 ******************************************************************************/

/*
 * Initialize a context
 */
void mbedtls_net_init( mbedtls_net_context *ctx )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_init != NULL) ){
		mbedtls_net_primitives->net_init( ctx );
	}
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect( mbedtls_net_context *ctx, const char *host,
                         const char *port, int proto )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_connect != NULL) ){
		return mbedtls_net_primitives->net_connect( ctx, host, port, proto );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Create a listening socket on bind_ip:port
 */
int mbedtls_net_bind( mbedtls_net_context *ctx, const char *bind_ip, const char *port, int proto )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_bind != NULL) ){
		return mbedtls_net_primitives->net_bind( ctx, bind_ip, port, proto );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Accept a connection from a remote client
 */
int mbedtls_net_accept( mbedtls_net_context *bind_ctx,
                        mbedtls_net_context *client_ctx,
                        void *client_ip, size_t buf_size, size_t *ip_len )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_accept != NULL) ){
		return mbedtls_net_primitives->net_accept(
				bind_ctx, client_ctx,
				client_ip, buf_size, ip_len
			);
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Set the socket blocking or non-blocking
 */
int mbedtls_net_set_block( mbedtls_net_context *ctx )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_set_block != NULL) ){
		return mbedtls_net_primitives->net_set_block( ctx );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

int mbedtls_net_set_nonblock( mbedtls_net_context *ctx )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_set_nonblock != NULL) ){
		return mbedtls_net_primitives->net_set_nonblock( ctx );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Portable usleep helper
 */
void mbedtls_net_usleep( unsigned long usec )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_usleep != NULL) ){
		mbedtls_net_primitives->net_usleep( usec );
	}
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv( void *ctx, unsigned char *buf, size_t len )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_recv != NULL) ){
		return mbedtls_net_primitives->net_recv( ctx, buf, len );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout( void *ctx, unsigned char *buf, size_t len,
                      uint32_t timeout )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_recv_timeout != NULL) ){
		return mbedtls_net_primitives->net_recv_timeout( ctx, buf, len, timeout );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send( void *ctx, const unsigned char *buf, size_t len )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_send != NULL) ){
		return mbedtls_net_primitives->net_send( ctx, buf, len );
	}
	return MBEDTLS_ERR_NET_SOCKET_FAILED;
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free( mbedtls_net_context *ctx )
{
	if( (mbedtls_net_primitives != NULL)
		&& (mbedtls_net_primitives->net_free != NULL) ){
		mbedtls_net_primitives->net_free( ctx );
	}
}

/******************************************************************************
 *  mbedtls_platform_net_sockets( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void mbedtls_platform_net_sockets(const mbedtls_net_primitive_type *primitive)
{
	mbedtls_net_primitives = (mbedtls_net_primitive_type *)primitive;
}

#endif /* MBEDTLS_NET_C */
