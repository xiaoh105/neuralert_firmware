/**
 ****************************************************************************************
 *
 * @file da16x_cert.c
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

#include "da16x_cert.h"
#include "da16x_image.h"
#include "util_api.h"

#undef  ENABLE_DA16X_CERT_DBG_INFO
#define	ENABLE_DA16X_CERT_DBG_ERR

#define DA16X_CERT_PRT  PRINTF

#if defined (ENABLE_DA16X_CERT_DBG_INFO)
#define DA16X_CERT_INFO(fmt, ...)   \
	DA16X_CERT_PRT("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DA16X_CERT_INFO(...)    do {} while (0)
#endif  // (ENABLE_DA16X_CERT_DBG_INFO)

#if defined (ENABLE_DA16X_CERT_DBG_ERR)
#define DA16X_CERT_ERR(fmt, ...)    \
	DA16X_CERT_PRT("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DA16X_CERT_ERR(...) do {} while (0)
#endif // (ENABLE_DA16X_CERT_DBG_ERR)


static void *da16x_cert_internal_calloc(size_t n, size_t size)
{
	void *buf = NULL;
	size_t buflen = (n * size);

	buf = pvPortMalloc(buflen);
	if (buf) {
		memset(buf, 0x00, buflen);
	}

	return buf;
}

static void da16x_cert_internal_free(void *f)
{
	if (f == NULL) {
		return ;
	}

	vPortFree(f);
    f = NULL;

    return ;
}

void *(*da16x_cert_calloc)(size_t n, size_t size) = da16x_cert_internal_calloc;
void (*da16x_cert_free)(void *ptr) = da16x_cert_internal_free;

static int da16x_cert_is_valid_module(int module)
{
    DA16X_CERT_INFO("Module:%d\n", module);

    switch (module) {
    case DA16X_CERT_MODULE_MQTT:
    case DA16X_CERT_MODULE_HTTPS_CLIENT:
    case DA16X_CERT_MODULE_WPA_ENTERPRISE:
        return pdTRUE;
    default:
        return pdFALSE;
    }
}

static int da16x_cert_is_valid_type(int type)
{
    DA16X_CERT_INFO("Type:%d\n", type);

    switch (type) {
    case DA16X_CERT_TYPE_CA_CERT:
    case DA16X_CERT_TYPE_CERT:
    case DA16X_CERT_TYPE_PRIVATE_KEY:
    case DA16X_CERT_TYPE_DH_PARAMS:
        return pdTRUE;
    default:
        return pdFALSE;
    }
}

static int da16x_cert_is_valid_format(int format)
{
    DA16X_CERT_INFO("Format:%d\n", format);

    switch (format) {
    case DA16X_CERT_FORMAT_DER:
    case DA16X_CERT_FORMAT_PEM:
        return pdTRUE;
    default:
        return pdFALSE;
    }
}

static int da16x_cert_is_valid_length(size_t len)
{
    DA16X_CERT_INFO("Length:%lu\n", len);

    if (len > DA16X_CERT_MAX_LENGTH) {
        return pdFALSE;
    }

    return pdTRUE;
}

static unsigned int da16x_cert_get_sflash_address(int module, int type)
{
    DA16X_CERT_INFO("Module:%d, type:%d\n", module, type);

    if (module == DA16X_CERT_MODULE_MQTT) {
        if (type == DA16X_CERT_TYPE_CA_CERT) {
            return SFLASH_ROOT_CA_ADDR1;
        } else if (type == DA16X_CERT_TYPE_CERT) {
            return SFLASH_CERTIFICATE_ADDR1;
        } else if (type == DA16X_CERT_TYPE_PRIVATE_KEY) {
            return SFLASH_PRIVATE_KEY_ADDR1;
        } else if (type == DA16X_CERT_TYPE_DH_PARAMS) {
            return SFLASH_DH_PARAMETER1;
        }
    } else if (module == DA16X_CERT_MODULE_HTTPS_CLIENT) {
        if (type == DA16X_CERT_TYPE_CA_CERT) {
            return SFLASH_ROOT_CA_ADDR2;
        } else if (type == DA16X_CERT_TYPE_CERT) {
            return SFLASH_CERTIFICATE_ADDR2;
        } else if (type == DA16X_CERT_TYPE_PRIVATE_KEY) {
            return SFLASH_PRIVATE_KEY_ADDR2;
        } else if (type == DA16X_CERT_TYPE_DH_PARAMS) {
            return SFLASH_DH_PARAMETER2;
        }
    } else if (module == DA16X_CERT_MODULE_WPA_ENTERPRISE) {
        if (type == DA16X_CERT_TYPE_CA_CERT) {
            return SFLASH_ENTERPRISE_ROOT_CA;
        } else if (type == DA16X_CERT_TYPE_CERT) {
            return SFLASH_ENTERPRISE_CERTIFICATE;
        } else if (type == DA16X_CERT_TYPE_PRIVATE_KEY) {
            return SFLASH_ENTERPRISE_PRIVATE_KEY;
        } else if (type == DA16X_CERT_TYPE_DH_PARAMS) {
            return SFLASH_ENTERPRISE_DH_PARAMETER;
        }
    }

    return 0;
}

static int da16x_cert_get_module(unsigned int flash_addr)
{
    DA16X_CERT_INFO("Addr:0x%x\n", flash_addr);

    switch (flash_addr) {
    case SFLASH_ROOT_CA_ADDR1:
        return DA16X_CERT_MODULE_MQTT;
    case SFLASH_CERTIFICATE_ADDR1:
        return DA16X_CERT_MODULE_MQTT;
    case SFLASH_PRIVATE_KEY_ADDR1:
        return DA16X_CERT_MODULE_MQTT;
    case SFLASH_DH_PARAMETER1:
        return DA16X_CERT_MODULE_MQTT;
    case SFLASH_ROOT_CA_ADDR2:
        return DA16X_CERT_MODULE_HTTPS_CLIENT;
    case SFLASH_CERTIFICATE_ADDR2:
        return DA16X_CERT_MODULE_HTTPS_CLIENT;
    case SFLASH_PRIVATE_KEY_ADDR2:
        return DA16X_CERT_MODULE_HTTPS_CLIENT;
    case SFLASH_DH_PARAMETER2:
        return DA16X_CERT_MODULE_HTTPS_CLIENT;
    case SFLASH_ENTERPRISE_ROOT_CA:
        return DA16X_CERT_MODULE_WPA_ENTERPRISE;
    case SFLASH_ENTERPRISE_CERTIFICATE:
        return DA16X_CERT_MODULE_WPA_ENTERPRISE;
    case SFLASH_ENTERPRISE_PRIVATE_KEY:
        return DA16X_CERT_MODULE_WPA_ENTERPRISE;
    case SFLASH_ENTERPRISE_DH_PARAMETER:
        return DA16X_CERT_MODULE_WPA_ENTERPRISE;
    }

    return DA16X_CERT_MODULE_NONE;
}

static int da16x_cert_get_type(unsigned int flash_addr)
{
    DA16X_CERT_INFO("Addr:0x%x\n", flash_addr);

    switch (flash_addr) {
    case SFLASH_ROOT_CA_ADDR1:
        return DA16X_CERT_TYPE_CA_CERT;
    case SFLASH_CERTIFICATE_ADDR1:
        return DA16X_CERT_TYPE_CERT;
    case SFLASH_PRIVATE_KEY_ADDR1:
        return DA16X_CERT_TYPE_PRIVATE_KEY;
    case SFLASH_DH_PARAMETER1:
        return DA16X_CERT_TYPE_DH_PARAMS;
    case SFLASH_ROOT_CA_ADDR2:
        return DA16X_CERT_TYPE_CA_CERT;
    case SFLASH_CERTIFICATE_ADDR2:
        return DA16X_CERT_TYPE_CERT;
    case SFLASH_PRIVATE_KEY_ADDR2:
        return DA16X_CERT_TYPE_PRIVATE_KEY;
    case SFLASH_DH_PARAMETER2:
        return DA16X_CERT_TYPE_DH_PARAMS;
    case SFLASH_ENTERPRISE_ROOT_CA:
        return DA16X_CERT_TYPE_CA_CERT;
    case SFLASH_ENTERPRISE_CERTIFICATE:
        return DA16X_CERT_TYPE_CERT;
    case SFLASH_ENTERPRISE_PRIVATE_KEY:
        return DA16X_CERT_TYPE_PRIVATE_KEY;
    case SFLASH_ENTERPRISE_DH_PARAMETER:
        return DA16X_CERT_TYPE_DH_PARAMS;
    }

    return DA16X_CERT_TYPE_NONE;
}

static int da16x_cert_is_pem_format(const char *buf)
{
    /* DA16200 only checks ----BEGIN string.
     * -----BEGIN CERTIFICATE-----
     * -----BEGIN RSA PRIVATE KEY-----
     * -----BEGIN EC PRIVATE KEY-----
     * -----BEGIN PRIVATE KEY-----
     * -----BEGIN ENCRYPTED PRIVATE KEY-----
     */
    const char *pem_prefix = "-----BEGIN";
    const size_t pem_prefix_len = strlen(pem_prefix);

    if ((strlen(buf) > pem_prefix_len)
        && (strnstr(buf, pem_prefix, pem_prefix_len) != NULL)) {
        return pdTRUE;
    }

    return pdFALSE;
}

int cert_flash_write(int flash_addr, char *buf, size_t len)
{
    int err = 0;
    int module = DA16X_CERT_MODULE_NONE;
    int type = DA16X_CERT_TYPE_NONE;

    DA16X_UNUSED_ARG(len);

    DA16X_CERT_INFO("flash_addr(0x%x), len(%lu)\n", flash_addr, len);

    if (!da16x_cert_is_pem_format((const char *)buf)) {
        DA16X_CERT_ERR("Not pem format\n");
        return -1;
    }

    module = da16x_cert_get_module(flash_addr);
    if (module == DA16X_CERT_MODULE_NONE) {
        DA16X_CERT_ERR("Failed to get module(0x%x)\n", flash_addr);
        return -1;
    }

    type = da16x_cert_get_type(flash_addr);
    if (type == DA16X_CERT_TYPE_NONE) {
        DA16X_CERT_ERR("Failed to get type(0x%x)\n", flash_addr);
        return -1;
    }

    err = da16x_cert_write(module, type, DA16X_CERT_FORMAT_PEM, (unsigned char *)buf, strlen(buf));
    if (err) {
        DA16X_CERT_ERR("Failed to write certificate(addr:0x%x, module:%d, type:%d, format:%d, buflen:%lu, err:%d)\n",
                flash_addr, module, type, DA16X_CERT_FORMAT_PEM, strlen(buf), err);
        return -1;
    }

    return 0;
}

int cert_flash_read(int flash_addr, char *result, size_t len)
{
    int err = DA16X_CERT_ERR_OK;
    int module = DA16X_CERT_MODULE_NONE;
    int type = DA16X_CERT_TYPE_NONE;
    int format = DA16X_CERT_FORMAT_NONE;
    size_t outlen = len;

    DA16X_CERT_INFO("flash_addr(0x%x), len(%lu)\n", flash_addr, len);

    module = da16x_cert_get_module(flash_addr);
    if (module == DA16X_CERT_MODULE_NONE) {
        DA16X_CERT_ERR("Failed to get module(0x%x)\n", flash_addr);
        return -1;
    }

    type = da16x_cert_get_type(flash_addr);
    if (type == DA16X_CERT_TYPE_NONE) {
        DA16X_CERT_ERR("Failed to get type(0x%x)\n", flash_addr);
        return -1;
    }

    err = da16x_cert_read(module, type, &format, (unsigned char *)result, &outlen);
    if (err == DA16X_CERT_ERR_OK) {
        if (format != DA16X_CERT_FORMAT_PEM) {
            DA16X_CERT_ERR("Not PEM format(%d)\n", format);
            memset(result, 0x00, len);
            return -1;
        }
    } else if (err == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        memset(result, 0xFF, len);
    } else {
        DA16X_CERT_ERR("Failed to read certificate(%d)\n", err);
        return -1;
    }

    return 0;
}

int cert_flash_read_no_fopen(HANDLE flash_handler, int flash_addr, char *result, size_t len)
{
    int err = 0;
    int module = DA16X_CERT_MODULE_NONE;
    int type = DA16X_CERT_TYPE_NONE;
    int format = DA16X_CERT_FORMAT_NONE;
    size_t outlen = len;

    DA16X_CERT_INFO("flash_addr(0x%x), len(%lu)\n", flash_addr, len);

    module = da16x_cert_get_module(flash_addr);
    if (module == DA16X_CERT_MODULE_NONE) {
        DA16X_CERT_ERR("Failed to get module(0x%x)\n", flash_addr);
        return -1;
    }

    type = da16x_cert_get_type(flash_addr);
    if (type == DA16X_CERT_TYPE_NONE) {
        DA16X_CERT_ERR("Failed to get type(0x%x)\n", flash_addr);
        return -1;
    }

    err = da16x_cert_read_no_fopen(flash_handler, module, type, &format, (unsigned char *)result, &outlen);
    if (err == DA16X_CERT_ERR_OK) {
        if (format != DA16X_CERT_FORMAT_PEM) {
            DA16X_CERT_ERR("Not PEM format(%d)\n", format);
            memset(result, 0x00, len);
            return -1;
        }
    } else if (err == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        memset(result, 0xFF, len);
    } else {
        DA16X_CERT_ERR("Failed to read certificate(%d)\n", err);
        return -1;
    }

    return 0;
}

int cert_flash_delete(int flash_addr)
{
    int err = 0;
    int module = DA16X_CERT_MODULE_NONE;
    int type = DA16X_CERT_TYPE_NONE;

    DA16X_CERT_INFO("flash_addr(0x%x)\n", flash_addr);

    module = da16x_cert_get_module(flash_addr);
    if (module == DA16X_CERT_MODULE_NONE) {
        DA16X_CERT_ERR("Failed to get module(0x%x)\n", flash_addr);
        return -1;
    }

    type = da16x_cert_get_type(flash_addr);
    if (type == DA16X_CERT_TYPE_NONE) {
        DA16X_CERT_ERR("Failed to get type(0x%x)\n", flash_addr);
        return -1;
    }

    err = da16x_cert_delete(module, type);
    if (err) {
        DA16X_CERT_ERR("Failed to delete certificate(%d)\n", err);
        return -1;
    }

    return 0;
}

int cert_flash_delete_all(void)
{
    int err = 0;

    err += cert_flash_delete(SFLASH_ROOT_CA_ADDR1);
    err += cert_flash_delete(SFLASH_CERTIFICATE_ADDR1);
    err += cert_flash_delete(SFLASH_PRIVATE_KEY_ADDR1);
    err += cert_flash_delete(SFLASH_DH_PARAMETER1);

    err += cert_flash_delete(SFLASH_ROOT_CA_ADDR2);
    err += cert_flash_delete(SFLASH_CERTIFICATE_ADDR2);
    err += cert_flash_delete(SFLASH_PRIVATE_KEY_ADDR2);
    err += cert_flash_delete(SFLASH_DH_PARAMETER2);

    err += cert_flash_delete(SFLASH_ENTERPRISE_ROOT_CA);
    err += cert_flash_delete(SFLASH_ENTERPRISE_CERTIFICATE);
    err += cert_flash_delete(SFLASH_ENTERPRISE_PRIVATE_KEY);
    err += cert_flash_delete(SFLASH_ENTERPRISE_DH_PARAMETER);

    return err;
}

int da16x_cert_write(int module, int type, int format, unsigned char *in, size_t inlen)
{
    int err = DA16X_CERT_ERR_OK;
    HANDLE flash_handler = NULL;
    da16x_cert_t *da16x_cert_ptr = NULL;
    unsigned char *buf = NULL;
    unsigned int flash_addr = 0x00;

    DA16X_CERT_INFO("module(%d), type(%d), format(%d), inlen(%lu)\n",
            module, type, format, inlen);

    //Check module
    if (!da16x_cert_is_valid_module(module)) {
        DA16X_CERT_ERR("Invaild module(%d)\n", module);
        return DA16X_CERT_ERR_INVALID_MODULE;
    }

    //Check type
    if (!da16x_cert_is_valid_type(type)) {
        DA16X_CERT_ERR("Invaild type(%d)\n", type);
        return DA16X_CERT_ERR_INVALID_TYPE;
    }

    //Check format
    if (!da16x_cert_is_valid_format(format)) {
        DA16X_CERT_ERR("Invaild format(%d)\n", format);
        return DA16X_CERT_ERR_INVALID_FORMAT;
    }

    //Check length
    if (!da16x_cert_is_valid_length(inlen)) {
        DA16X_CERT_ERR("Invaild length(%lu)\n", inlen);
        return DA16X_CERT_ERR_INVALID_LENGTH;
    }

    //TODO: is required to check vaildation of certificate using parsing?
    //Check cert
    if (!in) {
        DA16X_CERT_ERR("Invalid certificate\n");
        return DA16X_CERT_ERR_INVALID_PARAMS;
    }

    buf = da16x_cert_calloc(FLASH_WRITE_LENGTH, sizeof(unsigned char));
    if (!buf) {
        DA16X_CERT_ERR("Failed to allocate memory(%lu)\n", FLASH_WRITE_LENGTH);
        return DA16X_CERT_ERR_MEM_FAILED;
    }

    //Construct cert data
    da16x_cert_ptr = (da16x_cert_t *)buf;
    da16x_cert_ptr->info.module = module;
    da16x_cert_ptr->info.type = type;
    da16x_cert_ptr->info.format = format;
    da16x_cert_ptr->info.cert_len = inlen;
    memcpy(da16x_cert_ptr->cert, in, inlen);
    if ((format == DA16X_CERT_FORMAT_PEM)
        && (strlen((const char *)in) == inlen)) {
        da16x_cert_ptr->info.cert_len += 1;
    }

    da16x_cert_ptr = NULL;

    //Get sflash memory address
    flash_addr = da16x_cert_get_sflash_address(module, type);
    if (!flash_addr) {
        DA16X_CERT_ERR("Failed to get sFlash memory(%d,%d)\n", module, type);
        return DA16X_CERT_ERR_INVALID_FLASH_ADDR;
    }

    //Get handler
    flash_handler = flash_image_open((sizeof(UINT32) * 8),
                                    (UINT32)flash_addr,
                                    (VOID *)&da16x_environ_lock);
    if (flash_handler == NULL) {
        DA16X_CERT_ERR("Failed to access sFlash memory(0x%x)\n", flash_addr);
        err = DA16X_CERT_ERR_FOPEN_FAILED;
        goto end;
    }

    //Write cert data
    flash_image_write(flash_handler, (UINT32)flash_addr, (VOID *)buf, FLASH_WRITE_LENGTH);

    err = DA16X_CERT_ERR_OK;

end:

    //Close handler
    if (flash_handler) {
        flash_image_close(flash_handler);
        flash_handler = NULL;
    }

    //Release cert data
    if (buf) {
        da16x_cert_free(buf);
        buf = NULL;
    }

    return err;
}

int da16x_cert_read(int module, int type, int *format, unsigned char *out, size_t *outlen)
{
    int err = DA16X_CERT_ERR_OK;
    HANDLE flash_handler = NULL;
    unsigned int flash_addr = 0x00;

    DA16X_CERT_INFO("module(%d), type(%d), outlen(%lu)\n", module, type, outlen);

    //Check module
    if (!da16x_cert_is_valid_module(module)) {
        DA16X_CERT_ERR("Invaild module(%d)\n", module);
        return DA16X_CERT_ERR_INVALID_MODULE;
    }

    //Check type
    if (!da16x_cert_is_valid_type(type)) {
        DA16X_CERT_ERR("Invaild type(%d)\n", type);
        return DA16X_CERT_ERR_INVALID_TYPE;
    }

    //Get sflash memory address
    flash_addr = da16x_cert_get_sflash_address(module, type);
    if (!flash_addr) {
        DA16X_CERT_ERR("Failed to get sFlash memory(%d,%d)\n", module, type);
        err = DA16X_CERT_ERR_INVALID_FLASH_ADDR;
        goto end;
    }

    //Get handler
    flash_handler = flash_image_open((sizeof(UINT32) * 8),
                                    (UINT32)flash_addr,
                                    (VOID *)&da16x_environ_lock);
    if (flash_handler == NULL) {
        DA16X_CERT_ERR("Failed to access sFlash memory(0x%x)\n", flash_addr);
        err = DA16X_CERT_ERR_FOPEN_FAILED;
        goto end;
    }

    err = da16x_cert_read_no_fopen(flash_handler, module, type, format, out, outlen);
    if (err) {
#if !defined(ENABLE_DA16X_CERT_DBG_INFO)
        if (err != DA16X_CERT_ERR_EMPTY_CERTIFICATE)
#endif // ! ENABLE_DA16X_CERT_DBG_INFO
        {
            DA16X_CERT_ERR("Failed to get certificate"
                    "(addr:0x%x, module:%d, type:%d, outlen:%lu, err:%d)\n",
                    flash_addr, module, type, *format, *outlen, err);
        }
        goto end;
    }

end:

    //Close handler
    if (flash_handler) {
        flash_image_close(flash_handler);
        flash_handler = NULL;
    }

    return err;
}

int da16x_cert_read_no_fopen(HANDLE flash_handler, int module, int type, int *format, unsigned char *out, size_t *outlen)
{
    int err = DA16X_CERT_ERR_OK;
    da16x_cert_t *da16x_cert_ptr = NULL;
    unsigned char *buf = NULL;
    unsigned int flash_addr = 0x00;
    char *cert_ptr = NULL;
    size_t cert_len = 0;

    DA16X_CERT_INFO("module(%d), type(%d), outlen(%lu)\n", module, type, *outlen);

    if (format) {
        *format = DA16X_CERT_FORMAT_NONE;
    }

    //Check module
    if (!da16x_cert_is_valid_module(module)) {
        DA16X_CERT_ERR("Invaild module(%d)\n", module);
        return DA16X_CERT_ERR_INVALID_MODULE;
    }

    //Check type
    if (!da16x_cert_is_valid_type(type)) {
        DA16X_CERT_ERR("Invaild type(%d)\n", type);
        return DA16X_CERT_ERR_INVALID_TYPE;
    }

    //Check handler
    if (!flash_handler) {
        DA16X_CERT_ERR("Invalid handler\n");
        return DA16X_CERT_ERR_INVALID_PARAMS;
    }

    buf = da16x_cert_calloc(FLASH_WRITE_LENGTH, sizeof(unsigned char));
    if (!buf) {
        DA16X_CERT_ERR("Failed to allocate memory(%lu)\n", FLASH_WRITE_LENGTH);
        return DA16X_CERT_ERR_MEM_FAILED;
    }

    //Get sflash memory address
    flash_addr = da16x_cert_get_sflash_address(module, type);
    if (!flash_addr) {
        DA16X_CERT_ERR("Failed to get sFlash memory(%d,%d)\n", module, type);
        err = DA16X_CERT_ERR_INVALID_FLASH_ADDR;
        goto end;
    }

    //Read certificate
    flash_image_read(flash_handler, (UINT32)flash_addr, (void *)buf, FLASH_WRITE_LENGTH);

    if (buf[0] == 0xFF) {
        //No certificate
        DA16X_CERT_INFO("Empty certificate\n");
        err = DA16X_CERT_ERR_EMPTY_CERTIFICATE;
    } else if (da16x_cert_is_pem_format((const char *)buf)) {
        //~ SDK v3.2.7.X
        if (out) {
            if (*outlen < strlen((const char *)buf) + 1) {
                DA16X_CERT_ERR("Not enough space(%lu:%lu)\n", *outlen, strlen((const char *)buf));
                err = DA16X_CERT_ERR_INVALID_LENGTH;
            } else {
                cert_ptr = (char *)buf;
                cert_len = strlen((const char *)buf) + 1;
            }
        }

        if ((err == DA16X_CERT_ERR_OK) && format) {
            *format = DA16X_CERT_FORMAT_PEM;
        }
    } else {
        //SDK v3.2.8.0 ~
        da16x_cert_ptr = (da16x_cert_t *)buf;
        if (out) {
            if (*outlen < da16x_cert_ptr->info.cert_len) {
                DA16X_CERT_ERR("Not enough space(%lu:%lu)\n", *outlen, da16x_cert_ptr->info.cert_len);
                err = DA16X_CERT_ERR_INVALID_LENGTH;
            } else {
                cert_ptr = (char *)da16x_cert_ptr->cert;
                cert_len = da16x_cert_ptr->info.cert_len;
            }
        }

        if ((err == DA16X_CERT_ERR_OK) && format) {
            *format = da16x_cert_ptr->info.format;
        }
    }

    if ((err == DA16X_CERT_ERR_OK) && out && cert_ptr && cert_len) {
        memcpy(out, cert_ptr, cert_len);
        *outlen = cert_len;
    }

end:

    cert_ptr = NULL;
    cert_len = 0;
    da16x_cert_ptr = NULL;

    if (buf) {
        da16x_cert_free(buf);
        buf = NULL;
    }

    return err;
}

int da16x_cert_delete_no_fopen(HANDLE flash_handler, int module, int type)
{
    int err = DA16X_CERT_ERR_OK;
    unsigned int flash_addr = 0x00;

    DA16X_CERT_INFO("module(%d), type(%d)\n", module, type);

    //Check handler
    if (!flash_handler) {
        DA16X_CERT_ERR("Invaild handler\n");
        return DA16X_CERT_ERR_INVALID_PARAMS;
    }

    //Check module
    if (!da16x_cert_is_valid_module(module)) {
        DA16X_CERT_ERR("Invaild module(%d)\n", module);
        return DA16X_CERT_ERR_INVALID_MODULE;
    }

    //Check type
    if (!da16x_cert_is_valid_type(type)) {
        DA16X_CERT_ERR("Invaild type(%d)\n", type);
        return DA16X_CERT_ERR_INVALID_TYPE;
    }

    //Get sflash memory address
    flash_addr = da16x_cert_get_sflash_address(module, type);
    if (!flash_addr) {
        DA16X_CERT_ERR("Failed to get sFlash memory(%d,%d)\n", module, type);
        return DA16X_CERT_ERR_INVALID_FLASH_ADDR;
    }

    err = (int)flash_image_erase(flash_handler, (UINT32)flash_addr, FLASH_WRITE_LENGTH);
    if (err == 0) {
        DA16X_CERT_ERR("Failed to erase data(0x%x)\n", flash_addr);
        err = DA16X_CERT_ERR_NOK;
    } else {
        err = DA16X_CERT_ERR_OK;
    }

    return err;
}

int da16x_cert_delete(int module, int type)
{
    int err = 0;
    HANDLE flash_handler = NULL;
    unsigned int flash_addr = 0x00;

    DA16X_CERT_INFO("module(%d), type(%d)\n", module, type);

    //Get sflash memory address
    flash_addr = da16x_cert_get_sflash_address(module, type);
    if (!flash_addr) {
        DA16X_CERT_ERR("Failed to get sFlash memory(%d,%d)\n", module, type);
        return DA16X_CERT_ERR_INVALID_FLASH_ADDR;
    }

    //Get handler
    flash_handler = flash_image_open((sizeof(UINT32) * 8),
                                    (UINT32)flash_addr,
                                    (VOID *)&da16x_environ_lock);
    if (flash_handler == NULL) {
        DA16X_CERT_ERR("Failed to access sFlash memory(0x%x)\n", flash_addr);
        return DA16X_CERT_ERR_FOPEN_FAILED;
    }

    err = da16x_cert_delete_no_fopen(flash_handler, module, type);
    if (err) {
        DA16X_CERT_ERR("Failed to delete certificate(module:%d, type:%d, err:%d)\n",
                module, type, err);
    }

    //Close hanlder
    if (flash_handler) {
        flash_image_close(flash_handler);
        flash_handler = NULL;
    }

    return err;
}

int da16x_cert_is_exist_cert(int module, int type)
{
    int err = DA16X_CERT_ERR_OK;

    DA16X_CERT_INFO("module(%d), type(%d)\n", module, type);

    err = da16x_cert_read(module, type, NULL, NULL, 0);
    if (err == DA16X_CERT_ERR_OK) {
        return pdTRUE;
    }

    return pdFALSE;
}

int da16x_cert_display_cert(int module, int type)
{
#if defined(ENABLE_DA16X_CERT_DBG_INFO)
    int err = DA16X_CERT_ERR_OK;
    da16x_cert_t *da16x_cert_ptr = NULL;
    unsigned char *buf = NULL;
    unsigned int flash_addr = 0x00;
    HANDLE flash_handler = NULL;

    DA16X_CERT_PRT("module(%d), type(%d)\n", module, type);

    //Check module
    if (!da16x_cert_is_valid_module(module)) {
        DA16X_CERT_ERR("Invaild module(%d)\n", module);
        return DA16X_CERT_ERR_INVALID_MODULE;
    }

    //Check type
    if (!da16x_cert_is_valid_type(type)) {
        DA16X_CERT_ERR("Invaild type(%d)\n", type);
        return DA16X_CERT_ERR_INVALID_TYPE;
    }

    //Get sflash memory address
    flash_addr = da16x_cert_get_sflash_address(module, type);
    if (!flash_addr) {
        DA16X_CERT_ERR("Failed to get sFlash memory(%d,%d)\n", module, type);
        return DA16X_CERT_ERR_INVALID_FLASH_ADDR;
    }

    //Allocate buffer
    buf = da16x_cert_calloc(FLASH_WRITE_LENGTH, sizeof(unsigned char));
    if (!buf) {
        DA16X_CERT_ERR("Failed to allocate memory(%lu)\n", FLASH_WRITE_LENGTH);
        return DA16X_CERT_ERR_MEM_FAILED;
    }

    //Get handler
    flash_handler = flash_image_open((sizeof(UINT32) * 8),
                                    (UINT32)flash_addr,
                                    (VOID *)&da16x_environ_lock);
    if (flash_handler == NULL) {
        DA16X_CERT_ERR("Failed to access sFlash memory(0x%x)\n", flash_addr);
        err = DA16X_CERT_ERR_FOPEN_FAILED;
        goto end;
    }

    //Read certificate
    flash_image_read(flash_handler, (UINT32)flash_addr, (void *)buf, FLASH_WRITE_LENGTH);

    DA16X_CERT_PRT("*%-20s: 0x%x\n", "Flash Address", flash_addr);

    if (buf[0] == 0xFF) {
        //No certificate
        DA16X_CERT_PRT("*%-20s: %s\n", "Existence", "X");
    } else if (da16x_cert_is_pem_format((const char *)buf)) {
        //~ SDK v3.2.7.X
        DA16X_CERT_PRT("*%-20s: %s\n", "Existence", "O");
        DA16X_CERT_PRT("*%-20s: %s\n", "Version", "~ SDK v3.2.7.X");
        DA16X_CERT_PRT("*%-20s(%lu)\n", "Certificate", strlen((const char *)buf));
        hex_dump((unsigned char *)buf, strlen((const char *)buf));
    } else {
        //SDK v3.2.8.0 ~
        da16x_cert_ptr = (da16x_cert_t *)buf;
        DA16X_CERT_PRT("*%-20s: %s\n", "Existence", "O");
        DA16X_CERT_PRT("*%-20s: %s\n", "Version", "SDK v3.2.8.0 ~");
        DA16X_CERT_PRT("*%-20s: %d\n", "Module", da16x_cert_ptr->info.module);
        DA16X_CERT_PRT("*%-20s: %d\n", "Type", da16x_cert_ptr->info.type);
        DA16X_CERT_PRT("*%-20s: %d\n", "Format", da16x_cert_ptr->info.format);
        DA16X_CERT_PRT("*%-20s: %lu\n", "Certificate", da16x_cert_ptr->info.cert_len);
        hex_dump((unsigned char *)da16x_cert_ptr->cert, da16x_cert_ptr->info.cert_len);
    }

end:

    da16x_cert_ptr = NULL;

    if (buf) {
        da16x_cert_free(buf);
        buf = NULL;
    }

    //Close hanlder
    if (flash_handler) {
        flash_image_close(flash_handler);
        flash_handler = NULL;
    }

    return err;
#else
    DA16X_UNUSED_ARG(module);
    DA16X_UNUSED_ARG(type);

    //Enable ENABLE_DA16X_CERT_DBG_INFO.

    return DA16X_CERT_ERR_OK;
#endif // ENABLE_DA16X_CERT_DBG_INFO
}

void da16x_cert_display_certs()
{
#if defined(ENABLE_DA16X_CERT_DBG_INFO)
    int modules[] = {
        DA16X_CERT_MODULE_MQTT,
        DA16X_CERT_MODULE_HTTPS_CLIENT,
        DA16X_CERT_MODULE_WPA_ENTERPRISE,
        DA16X_CERT_MODULE_NONE
    };

    int types[] = {
        DA16X_CERT_TYPE_CA_CERT,
        DA16X_CERT_TYPE_CERT,
        DA16X_CERT_TYPE_PRIVATE_KEY,
        DA16X_CERT_TYPE_DH_PARAMS,
        DA16X_CERT_TYPE_NONE
    };

    int m_idx = 0;
    int t_idx = 0;

    for (m_idx = 0 ; modules[m_idx] != DA16X_CERT_MODULE_NONE ; m_idx++) {
        for (t_idx = 0 ; types[t_idx] != DA16X_CERT_TYPE_NONE ; t_idx++) {
            da16x_cert_display_cert(modules[m_idx], types[t_idx]);
        }
    }
#endif // ENABLE_DA16X_CERT_DBG_INFO

    return ;
}
