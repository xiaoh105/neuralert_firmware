/**
 ****************************************************************************************
 *
 * @file da16x_cert.h
 *
 * @brief Define for Certificate to access sFlash memory. 
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

#ifndef __DA16X_CERT_H__
#define __DA16X_CERT_H__

#include "sdk_type.h"
#include "da16x_system.h"
#include "da16x_types.h"
#include "da16200_map.h"
#include "environ.h"
#include "assert.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	CERT_MAX_LENGTH     (1024 * 4)
#define FLASH_WRITE_LENGTH  (1024 * 4)

typedef enum {
    DA16X_CERT_ERR_OK = 0,
    DA16X_CERT_ERR_NOK,
    DA16X_CERT_ERR_INVALID_MODULE,
    DA16X_CERT_ERR_INVALID_TYPE,
    DA16X_CERT_ERR_INVALID_FORMAT,
    DA16X_CERT_ERR_INVALID_LENGTH,
    DA16X_CERT_ERR_INVALID_FLASH_ADDR,
    DA16X_CERT_ERR_INVALID_PARAMS,
    DA16X_CERT_ERR_FOPEN_FAILED,
    DA16X_CERT_ERR_MEM_FAILED,
    DA16X_CERT_ERR_EMPTY_CERTIFICATE
} da16x_cert_err_t;

typedef enum {
    DA16X_CERT_MODULE_NONE = -1,
    DA16X_CERT_MODULE_MQTT = 0,
    DA16X_CERT_MODULE_HTTPS_CLIENT = 1,
    DA16X_CERT_MODULE_WPA_ENTERPRISE = 2
} da16x_cert_module_t;

typedef enum {
    DA16X_CERT_TYPE_NONE = -1,
    DA16X_CERT_TYPE_CA_CERT = 0,
    DA16X_CERT_TYPE_CERT = 1,
    DA16X_CERT_TYPE_PRIVATE_KEY = 2,
    DA16X_CERT_TYPE_DH_PARAMS = 3
} da16x_cert_type_t;

typedef enum {
    DA16X_CERT_FORMAT_NONE = -1,
    DA16X_CERT_FORMAT_DER = 0,
    DA16X_CERT_FORMAT_PEM = 1
} da16x_cert_format_t;

typedef enum {
    DA16X_CERT_MODE_NONE = -1,
    DA16X_CERT_MODE_STORE = 0,
    DA16X_CERT_MODE_DELETION = 1
} da16x_cert_mode_t;

typedef struct {
    unsigned int module;
    unsigned int type;
    unsigned int format;
    unsigned int cert_len;
} da16x_cert_info_t;

#define DA16X_CERT_MARGIN       (0)
#define DA16X_CERT_MAX_LENGTH   (FLASH_WRITE_LENGTH - sizeof(da16x_cert_info_t) - DA16X_CERT_MARGIN)

typedef struct {
    da16x_cert_info_t info;
    unsigned char cert[DA16X_CERT_MAX_LENGTH];
} da16x_cert_t;

// Compile assert
static_assert(FLASH_WRITE_LENGTH >= sizeof(da16x_cert_t),
        "da16x_cert_t is too large for the size of underlying FLASH_WRITE_LENGTH");

/**
 ****************************************************************************************
 * @brief Write certificate to specific sFlash memory address.
 * @param[in] flash_addr Specific sFlash memory address to write certificate.
 * @param[in] buf Pointer to write certificate to sFlash memory address.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int cert_flash_write(int flash_addr, char *buf, size_t len);

/**
 ****************************************************************************************
 * @brief Read ceritificate from specific sFlash memory address.
 * @param[in] flash_addr Specific sFlash memory address to read certificate.
 * @param[out] result Point to read certificate from sFlash memory address.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int cert_flash_read(int flash_addr, char *result, size_t len);

/**
 ****************************************************************************************
 * @brief Read ceritificate from specific sFlash memory address without opening handler.
 * @param[in] flash_handler Handler to read specific sFlash memory address.
 * @param[in] flash_addr Specific sFlash memory address to read certificate.
 * @param[out] result Point to read certificate from sFlash memory address.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success. The handler must be open.
 ****************************************************************************************
 */
int cert_flash_read_no_fopen(HANDLE flash_handler, int flash_addr, char *result, size_t len);

/**
 ****************************************************************************************
 * @brief Delete certificate from specific sFlash memory address.
 * @param[in] flash_addr Specific sFlash memory address to delete certificate.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int cert_flash_delete(int flash_addr);

/**
 ****************************************************************************************
 * @brief Delete stored certificates.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int cert_flash_delete_all(void);

/**
 ****************************************************************************************
 * @brief Write certificate to specific sflash memory address by module and type.
 * @param[in] module Module ID. 0 (MQTT), 1 (HTTPs client for OTA), 2 (WPA Enterprise).
 * @param[in] type Certificate type. 0 (CA certificate), 1 (Certificate), 2 (Private key), 3 (DH params).
 * @param[in] format Certificate format. 0 (DER), 1 (PEM).
 * @param[in] in Pointer to write certificiate.
 * @param[in] inlen Length of certificate.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_cert_write(int module, int type, int format, unsigned char *in, size_t inlen);

/**
 ****************************************************************************************
 * @brief Read certificate from specific sflash memory by module and type.
 * @param[in] module Module ID. 0 (MQTT), 1 (HTTPs client for OTA), 2 (WPA Enterprise).
 * @param[in] type Certificate type. 0 (CA certificate), 1 (Certificate), 2 (Private key), 3 (DH params).
 * @param[out] format Certificate format. 0 (DER), 1 (PEM).
 * @param[out] out Pointer to read certificiate.
 * @param[out] outlen Length of certificate.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_cert_read(int module, int type, int *format, unsigned char *out, size_t *outlen);

/**
 ****************************************************************************************
 * @brief Read certificate from specific sflash memory by module and type.
 * @param[in] flash_handler Hanlder to read certificate. It must be open.
 * @param[in] module Module ID. 0 (MQTT), 1 (HTTPs client for OTA), 2 (WPA Enterprise).
 * @param[in] type Certificate type. 0 (CA certificate), 1 (Certificate), 2 (Private key), 3 (DH params).
 * @param[out] format Certificate format. 0 (DER), 1 (PEM).
 * @param[out] out Pointer to read certificiate.
 * @param[out] outlen Length of certificate.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_cert_read_no_fopen(HANDLE flash_handler, int module, int type, int *format, unsigned char *out, size_t *outlen);

/**
 ****************************************************************************************
 * @brief Delete certificate from specific sflash memory by module and type.
 * @param[in] flash_handler Hanlder to read certificate. It must be open.
 * @param[in] module Module ID. 0 (MQTT), 1 (HTTPs client for OTA), 2 (WPA Enterprise).
 * @param[in] type Certificate type. 0 (CA certificate), 1 (Certificate), 2 (Private key), 3 (DH params).
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_cert_delete_no_fopen(HANDLE flash_handler, int module, int type);

/**
 ****************************************************************************************
 * @brief Delete certificate from specific sflash memory by module and type.
 * @param[in] module Module ID. 0 (MQTT), 1 (HTTPs client for OTA), 2 (WPA Enterprise).
 * @param[in] type Certificate type. 0 (CA certificate), 1 (Certificate), 2 (Private key), 3 (DH params).
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_cert_delete(int module, int type);

/**
 ****************************************************************************************
 * @brief Check whether certificate is exist or not.
 * @param[in] module Module ID. 0 (MQTT), 1 (HTTPs client for OTA), 2 (WPA Enterprise).
 * @param[in] type Certificate type. 0 (CA certificate), 1 (Certificate), 2 (Private key), 3 (DH params).
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_cert_is_exist_cert(int module, int type);

int da16x_cert_display_cert(int module, int type);

void da16x_cert_display_certs();

#ifdef __cplusplus
}
#endif

#endif // __DA16X_CERT_H__
