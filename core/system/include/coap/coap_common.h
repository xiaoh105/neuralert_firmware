/**
 ****************************************************************************************
 *
 * @file coap_common.h
 *
 * @brief CoAP common definition
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
#ifndef	__COAP_COMMON_H__
#define	__COAP_COMMON_H__

#include "sdk_type.h"
#include "da16x_system.h"
#include "app_errno.h"
#include "task.h"
#include "lwip/sockets.h"
#include "da16x_crypto.h"

#include "mbedtls/config.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/ssl_cookie.h"

#include "da16x_dpm_tls.h"
#include "net_bsd_sockets.h"
#include "user_dpm.h"
#include "user_dpm_api.h"
#include "da16x_sys_watchdog.h"

/// CoAP port number
#define COAP_PORT						5683
/// CoAPs port number
#define	COAPS_PORT						5684

/// CoAP maximum option number
#define MAXOPT							16
/// CoAP server Endpoint max segment count
#define MAX_SEGMENTS					3

/// CoAP name length
#define	COAP_MAX_NAME_LEN				32

// CoAP option number registry
/// CoAP option number registry - If-Match
#define	COAP_OPTION_IF_MATCH			1
/// CoAP option number registry - Uri-Host
#define	COAP_OPTION_URI_HOST			3
/// CoAP option number reigstry - ETag
#define	COAP_OPTION_ETAG				4
/// CoAP option number registry - If-None-Match
#define	COAP_OPTION_IF_NONE_MATCH		5
/// CoAP option - Observe
#define	COAP_OPTION_OBSERVE				6
/// CoAP option number reigstry - Uri-Port
#define	COAP_OPTION_URI_PORT			7
/// CoAP option number registry - Location-Path
#define	COAP_OPTION_LOCATION_PATH		8
/// CoAP option number registry - Uri-Path
#define	COAP_OPTION_URI_PATH			11
/// CoAP option number registry - Content-Format
#define	COAP_OPTION_CONTENT_FORMAT		12
/// CoAP option number registry - Max-Age
#define	COAP_OPTION_MAX_AGE				14
/// CoAP option number registry - Uri-Query
#define	COAP_OPTION_URI_QUERY			15
/// CoAP option number reigstry - Accept
#define	COAP_OPTION_ACCEPT				17
/// CoAP option number reigstry - Location-Query
#define	COAP_OPTION_LOCATION_QUERY		20
/// CoAP option number reigstry - Block2
#define	COAP_OPTION_BLOCK_2				23
/// CoAP option number reigstry - Block1
#define	COAP_OPTION_BLOCK_1				27
/// CoAP option number reigstry - Size2
#define	COAP_OPTION_SIZE_2				28
/// CoAP option number reigstry - Proxy-Uri
#define	COAP_OPTION_PROXY_URI			35
/// CoAP option number reigstry - Proxy-Scheme
#define	COAP_OPTION_PROXY_SCHEME		39
/// CoAP option number reigstry - Size1
#define	COAP_OPTION_SIZE_1				60

/// CoAP message header
typedef struct
{
	/// Version number
	uint8_t version;
	/// Message type
	uint8_t type;
	/// Token length
	uint8_t token_len;
	/// Status code
	uint8_t code;
	/// Message-ID
	uint8_t msg_id[2];
} coap_header_t;

/// CoAP buffer structure(Read-only)
typedef struct
{
	/// Pointer to data
	const uint8_t *p;
	/// Length of data
	size_t len;
} coap_buffer_t;

/// CoAP buffer structure(Read/Write)
typedef struct
{
	/// Pointer to data
	uint8_t *p;
	/// Length of data
	size_t len;
} coap_rw_buffer_t;

/// CoAP option structure(Read-only)
typedef struct
{
	/// Option number
	uint8_t num;
	/// Option value
	coap_buffer_t buf;
} coap_option_t;

/// CoAP packet structure(Read-only)
typedef struct
{
	/// Header of the packet
	coap_header_t header;
	/// Token value, size as specified by header.token_len
	coap_buffer_t token;
	/// Number of options
	uint8_t numopts;
	/// Options of the packet
	coap_option_t opts[MAXOPT];
	/// Payload carried by the packet
	coap_buffer_t payload;
	/// Payload carried by the packet
	coap_rw_buffer_t payload_raw;
	/// 0 for payload, 1 for payload_raw
	uint8_t copy_mode;
} coap_packet_t;

/// CoAP option struct(Read/Write)
typedef struct
{
	/// Option number
	uint8_t num;
	/// Option value
	coap_rw_buffer_t buf;
} coap_rw_option_t;

/// CoAP packet structure(Read/write)
typedef struct
{
	/// Header of the packet
	coap_header_t header;
	/// Token value, size as specified by header.token_len
	coap_rw_buffer_t token;
	/// Number of options
	uint8_t numopts;
	/// Options of the packet
	coap_rw_option_t opts[MAXOPT];
	/// Payload carried by the packet
	coap_rw_buffer_t payload;
} coap_rw_packet_t;

/// CoAP method codes
typedef enum
{
	/// Init value
	COAP_METHOD_NONE			= 0,
	/// GET method
	COAP_METHOD_GET				= 1,
	/// POST method
	COAP_METHOD_POST			= 2,
	/// PUT method
	COAP_METHOD_PUT				= 3,
	/// DELETE method
	COAP_METHOD_DELETE			= 4
} coap_method_t;

/// CoAP message type
typedef enum
{
	/// Confirmable
	COAP_MSG_TYPE_CON			= 0,
	/// Non-confirmable
	COAP_MSG_TYPE_NONCON		= 1,
	/// Acknowledgement
	COAP_MSG_TYPE_ACK			= 2,
	/// Reset
	COAP_MSG_TYPE_RESET			= 3
} coap_msgtype_t;

/// CoAPs timer state
typedef enum
{
	CANCELLED	= -1,
	NO_EXPIRY	= 0,
	INT_EXPIRY	= 1,
	FIN_EXPIRY	= 2
} coap_timer_state_t;

/// CoAPs timer information
typedef struct _coap_timer_t
{
	/// Intermediate delay in milliseconds
	unsigned int int_ms;
	/// Final delay in mislliseconds
	unsigned int fin_ms;
	/// Tick
	TickType_t snapshot;
} coap_timer_t;

#define COAP_MAKE_RESP_CODE(clas, detail) ((clas << 5) | (detail))

/// CoAP response codes
typedef enum
{
	COAP_RESP_CODE_CREATED						= COAP_MAKE_RESP_CODE(2, 1),
	COAP_RESP_CODE_DELETED						= COAP_MAKE_RESP_CODE(2, 2),
	COAP_RESP_CODE_VALID						= COAP_MAKE_RESP_CODE(2, 3),
	COAP_RESP_CODE_CONTINUE						= COAP_MAKE_RESP_CODE(2, 31),
	COAP_RESP_CODE_CHANGED						= COAP_MAKE_RESP_CODE(2, 4),
	COAP_RESP_CODE_CONTENT						= COAP_MAKE_RESP_CODE(2, 5),
	COAP_RESP_CODE_BAD_REQUEST					= COAP_MAKE_RESP_CODE(4, 0),
	COAP_RESP_CODE_UNAUTHORIZED					= COAP_MAKE_RESP_CODE(4, 1),
	COAP_RESP_CODE_BAD_OPTION					= COAP_MAKE_RESP_CODE(4, 2),
	COAP_RESP_CODE_FORBIDDEN					= COAP_MAKE_RESP_CODE(4, 3),
	COAP_RESP_CODE_NOT_FOUND					= COAP_MAKE_RESP_CODE(4, 4),
	COAP_RESP_CODE_METHOD_NOT_ALLOWED			= COAP_MAKE_RESP_CODE(4, 5),
	COAP_RESP_CODE_NOT_ACCEPTABLE				= COAP_MAKE_RESP_CODE(4, 6),
	COAP_RESP_CODE_PRECONDITION_FAILED			= COAP_MAKE_RESP_CODE(4, 12),
	COAP_RESP_CODE_REQUEST_ENTITY_TOO_LARGE		= COAP_MAKE_RESP_CODE(4, 13),
	COAP_RESP_CODE_UNSUPPORTED_CONTENT_FORMAT	= COAP_MAKE_RESP_CODE(4, 15),
	COAP_RESP_CODE_INTERNAL_SERVER_ERROR		= COAP_MAKE_RESP_CODE(5, 0),
	COAP_RESP_CODE_NOT_IMPLEMENTED				= COAP_MAKE_RESP_CODE(5, 1),
	COAP_RESP_CODE_BAD_GATEWAY					= COAP_MAKE_RESP_CODE(5, 2),
	COAP_RESP_CODE_SERVICE_UNAVAILABLE			= COAP_MAKE_RESP_CODE(5, 3),
	COAP_RESP_CODE_GATEWAY_TIMEOUT				= COAP_MAKE_RESP_CODE(5, 4),
	COAP_RESP_CODE_PROXYING_NOT_SUPPORTED		= COAP_MAKE_RESP_CODE(5, 5),
} coap_resp_code_t;

/// CoAP content-formats registry
typedef enum
{
	COAP_CONTENT_TYPE_NONE						= -1,
	COAP_CONTENT_TYPE_TEXT_PLAIN				= 0,
	COAP_CONTENT_TYPE_APPLICATION_LINKFORMAT	= 40,
} coap_content_type_t;

/// CoAP internal error codes
typedef enum
{
	COAP_ERR_SUCCESS						= 0,
	COAP_ERR_HEADER_TOO_SHORT				= 1,
	COAP_ERR_VERSION_NOT_1					= 2,
	COAP_ERR_TOKEN_TOO_SHORT				= 3,
	COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER	= 4,
	COAP_ERR_OPTION_TOO_SHORT				= 5,
	COAP_ERR_OPTION_OVERFLOW				= 6,
	COAP_ERR_OPTION_TOO_BIG					= 7,
	COAP_ERR_OPTION_LEN_INVALID				= 8,
	COAP_ERR_BUFFER_TOO_SMALL				= 9,
	COAP_ERR_NOT_SUPPORTED					= 10,
	COAP_ERR_OPTION_DELTA_INVALID			= 11,
} coap_error_t;

/// Endpoint function for CoAP Server
typedef int (*coap_endpoint_func)(coap_rw_buffer_t *scratch,
								  const coap_packet_t *in,
								  coap_packet_t *out,
								  uint8_t msgid_hi,
								  uint8_t msgid_lo);

/// Endpoint path structure
typedef struct
{
	/// Segment count
	int count;
	/// Segment elements
	const char *elems[MAX_SEGMENTS];
} coap_endpoint_path_t;

/// Endpoint structure
typedef struct
{
	/// Method type
	coap_method_t method;
	/// Handler
	coap_endpoint_func handler;
	/// Path towards a resource
	const coap_endpoint_path_t *path;
	/// The 'ct' attribute - content-format
	const char *core_attr;
	/// Title
	const char *title;
	/// The 'rt'
	const char *rt;
	uint8_t is_separate;
} coap_endpoint_t;

#endif	// (__COAP_COMMON_H__)

/* EOF */
