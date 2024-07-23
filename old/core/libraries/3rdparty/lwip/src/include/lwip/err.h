/**
 * @file
 * lwIP Error codes
 */
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_ERR_H
#define LWIP_HDR_ERR_H

#include "lwip/opt.h"
#include "lwip/arch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup infrastructure_errors Error codes
 * @ingroup infrastructure
 * @{
 */

/** Definitions for error constants. */
typedef enum {
/** No error, everything OK. */
  ERR_OK         = 0,
/** Out of memory error.     */
  ERR_MEM        = -1,
/** Buffer error.            */
  ERR_BUF        = -2,
/** Timeout.                 */
  ERR_TIMEOUT    = -3,
/** Routing problem.         */
  ERR_RTE        = -4,
/** Operation in progress    */
  ERR_INPROGRESS = -5,
/** Illegal value.           */
  ERR_VAL        = -6,
/** Operation would block.   */
  ERR_WOULDBLOCK = -7,
/** Address in use.          */
  ERR_USE        = -8,
/** Already connecting.      */
  ERR_ALREADY    = -9,
/** Conn already established.*/
  ERR_ISCONN     = -10,
/** Not connected.           */
  ERR_CONN       = -11,
/** Low-level netif error    */
  ERR_IF         = -12,

/** Connection aborted.      */
  ERR_ABRT       = -13,
/** Connection reset.        */
  ERR_RST        = -14,
/** Connection closed.       */
  ERR_CLSD       = -15,
/** Illegal argument.        */
  ERR_ARG        = -16,
/** Unknown .        */
  ERR_UNKNOWN    = -17,

/** Not 200 OK          */
  ERR_NOT_FOUND    = -18
} err_enum_t;

#define ER_SUCCESS                      0x00
#define ER_NO_PACKET                    0x01
#define ER_DELETED                      0x02
#define ER_WAIT_ERROR                   0x03
#define ER_SIZE_ERROR                   0x04
#define ER_DELETE_ERROR                 0x05
#define ER_INVALID_PACKET               0x06
#define ER_NOT_ENABLED                  0x07
#define ER_ALREADY_ENABLED              0x08
#define ER_NO_MORE_ENTRIES              0x09
#define ER_IP_ADDRESS_ERROR             0x0A
#define ER_ALREADY_BOUND                0x0B
#define ER_NOT_CREATED                  0x0C
#define ER_DUPLICATE_LISTEN             0x0D
#define ER_IN_PROGRESS                  0x0E
#define ER_NOT_CONNECTED                0x0F
#define ER_NOT_SUCCESSFUL               0x11
#define ER_CONNECTION_PENDING           0x12
#define ER_NOT_IMPLEMENTED              0x13
#define ER_NOT_SUPPORTED                0x14
#define ER_INVALID_PARAMETERS           0x15
#define ER_NOT_FOUND                    0x16
#define ER_DUPLICATED_ENTRY             0x17
#define ER_PARAMETER_ERROR              0x18
#define ER_NO_MEMORY                    0x19
#define ER_NO_EVENTS                    0x1A
#define ER_QUEUE_ERROR                  0x1B
#define ER_QUEUE_EMPTY                  0x1C
#define ER_DISCONNECTED                 0x1D
#define ER_INIT_SECURE                  0x1E
#define ER_SSL                          0x1F
#define ER_BIND                         0x20
#define ER_SOCKET                       0x21
#define ER_LISTEN                       0x22
#define ER_MUTEX_CREATE                 0x23
#define ER_ARGUMENT                     0x24

/** Define LWIP_ERR_T in cc.h if you want to use
 *  a different type for your platform (must be signed). */
#ifdef LWIP_ERR_T
typedef LWIP_ERR_T err_t;
#else /* LWIP_ERR_T */
typedef s8_t err_t;
#endif /* LWIP_ERR_T*/

/**
 * @}
 */

#ifdef LWIP_DEBUG
extern const char *lwip_strerr(err_t err);
#else
#define lwip_strerr(x) ""
#endif /* LWIP_DEBUG */

#if !NO_SYS
int err_to_errno(err_t err);
#endif /* !NO_SYS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_ERR_H */
