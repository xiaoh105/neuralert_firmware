/**
 ****************************************************************************************
 *
 * @file atcmd_cert_mng.h
 *
 * @brief AT-CMD Certificate Manager
 *
 * Copyright (c) 2016-2020 Renesas Electronics. All rights reserved.
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

#ifndef __ATCMD_CERT_MNG_H__
#define __ATCMD_CERT_MNG_H__

#include "sdk_type.h"

#if defined (__SUPPORT_ATCMD_TLS__)

#include "user_dpm.h"
#include "user_dpm_api.h"
#include "da16x_system.h"
#include "command.h"
#include "util_api.h"
#include "app_errno.h"

#define ATCMD_CM_CERT_TYPE_CA_CERT     0
#define ATCMD_CM_CERT_TYPE_CERT        1

#define ATCMD_CM_CERT_SEQ_CERT         1
#define ATCMD_CM_CERT_SEQ_KEY          2

#define ATCMD_CM_CERT_FORMAT_DER       0
#define ATCMD_CM_CERT_FOMRAT_PEM       1

#define ATCMD_CM_MAX_CERT_NUM          (5 * 2)
#define ATCMD_CM_MAX_NAME              32
#define ATCMD_CM_MAX_CERT_LEN          (4096)
#define ATCMD_CM_MAX_CERT_HEADER       (40)    //sizeof(atcmd_cm_cert_info_t)
#define ATCMD_CM_MAX_CERT_BODY         (ATCMD_CM_MAX_CERT_LEN - ATCMD_CM_MAX_CERT_HEADER)
#define ATCMD_CM_INFO_NAME             "ATCMD_CM_INFO"

#define ATCMD_CM_INIT_FLAG             0x01

typedef struct _atcmd_cm_cert_info_t
{
    unsigned char flag;
    unsigned char type;
    unsigned char seq;
    unsigned char format;
    char name[ATCMD_CM_MAX_NAME];
    size_t cert_len;
} atcmd_cm_cert_info_t;

//NOTE: Size of atcmd_cm_cert_t has to be less than 4Kbytes.
typedef struct _atcmd_cm_cert_t
{
    atcmd_cm_cert_info_t info;
    char cert[ATCMD_CM_MAX_CERT_BODY];
} atcmd_cm_cert_t;

static const unsigned int g_atcmd_cm_cert_addr_list[ATCMD_CM_MAX_CERT_NUM] =
{
    SFLASH_ATCMD_TLS_CERT_01,
    SFLASH_ATCMD_TLS_CERT_02,
    SFLASH_ATCMD_TLS_CERT_03,
    SFLASH_ATCMD_TLS_CERT_04,
    SFLASH_ATCMD_TLS_CERT_05,
    SFLASH_ATCMD_TLS_CERT_06,
    SFLASH_ATCMD_TLS_CERT_07,
    SFLASH_ATCMD_TLS_CERT_08,
    SFLASH_ATCMD_TLS_CERT_09,
    SFLASH_ATCMD_TLS_CERT_10
};

void atcmd_tlsc_set_malloc_free(void *(*malloc_func)(size_t), void (*free_func)(void *));

void atcmd_cm_display_cert_info(atcmd_cm_cert_info_t *cert, unsigned int addr);
void atcmd_cm_display_info();
void atcmd_cm_init_cert_info_t(atcmd_cm_cert_info_t *cert);
void atcmd_cm_init_cert_t(atcmd_cm_cert_t *cert);
int atcmd_cm_init_cert_info();
int atcmd_cm_deinit_cert_info();

/**
 ****************************************************************************************
 * @brief Store a certificate/CA list data.
 * @param[in] name Name of the certificate.
 * @param[in] type Type of the certificate(0:CA Certificate, 1:Client/Server Certificates)
 * @param[in] seq Sequence numer
 * @param[in] format Format of the CA/Certificate/Key(0:DER, 1:PEM)
 * @param[in] in Certificate data to be store
 * @param[in] inlen Length of certificate data to be store.
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int atcmd_cm_set_cert(char *name, unsigned char type, unsigned char seq,
                      unsigned char format, char *in, size_t inlen);

/**
 ****************************************************************************************
 * @brief Return length of a certificate/CA list data.
 * @param[in] name Name of the certificate.
 * @param[in] type Type of the certificate(0:CA Certificate, 1:Client/Server Certificates)
 * @param[in] seq Sequence numer
 * @param[out] outlen Length of restored certificate data
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int atcmd_cm_get_cert_len(char *name, unsigned char type, unsigned char seq, size_t *outlen);

/**
 ****************************************************************************************
 * @brief Restore a certificate/CA list data.
 * @param[in] name Name of the certificate.
 * @param[in] type Type of the certificate(0:CA Certificate, 1:Client/Server Certificates)
 * @param[in] seq Sequence numer
 * @param[out] out Restored Certificate data 
 * @param[out] outlen Length of restored certificate data
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int atcmd_cm_get_cert(char *name, unsigned char type, unsigned char seq,
                      char *out, size_t *outlen);

int atcmd_cm_get_ca_cert_type(char *name, char *out, size_t *outlen);
int atcmd_cm_get_cert_type(char *name, unsigned char seq, char *out, size_t *outlen);

/**
 ****************************************************************************************
 * @brief Delete a certificate/CA list data.
 * @param[in] name Name of the certificate.
 * @param[in] type Type of the certificate(0:CA Certificate, 1:Client/Server Certificates)
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int atcmd_cm_clear_cert(char *name, unsigned char type);

const atcmd_cm_cert_info_t *atcmd_cm_get_cert_info(void);
int atcmd_cm_is_exist_cert(char *name, unsigned char type, unsigned char seq);
int atcmd_cm_find_idx(char *name, unsigned char type, unsigned char seq);
int atcmd_cm_find_idx_list(char *name, int idx_list[ATCMD_CM_MAX_CERT_NUM], int *idx_num);
int atcmd_cm_find_empty_idx(void);
int atcmd_cm_read_cert_by_idx(unsigned int idx, atcmd_cm_cert_t *cert);
int atcmd_cm_read_cert_by_addr(unsigned int addr, atcmd_cm_cert_t *cert);
int atcmd_cm_write_cert_by_idx(unsigned int idx, atcmd_cm_cert_t *cert);
int atcmd_cm_write_cert_by_addr(unsigned int addr, atcmd_cm_cert_t *cert);
int atcmd_cm_clear_cert_by_idx(unsigned int idx);
int atcmd_cm_clear_cert_by_addr(unsigned int addr);

#endif // (__SUPPORT_ATCMD_TLS__)

#endif // __ATCMD_CERT_MNG_H__

/* EOF */
