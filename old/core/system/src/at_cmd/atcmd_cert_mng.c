/**
 ****************************************************************************************
 *
 * @file atcmd_cert_mng.c
 *
 * @brief AT-CMD Certificate Manager
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

#include "atcmd_cert_mng.h"

#if defined    (__SUPPORT_ATCMD_TLS__)

#include "da16x_image.h"
#include "environ.h"
#include "atcmd.h"

#undef  ENABLE_ATCMD_CM_DBG_INFO
#define ENABLE_ATCMD_CM_DBG_ERR

#define ATCMD_CM_DBG    ATCMD_DBG

#if defined (ENABLE_ATCMD_CM_DBG_INFO)
#define ATCMD_CM_INFO(fmt, ...)   \
                                  ATCMD_CM_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ATCMD_CM_INFO(...)        do {} while (0)
#endif  // (ENABLE_ATCMD_CM_DBG_INFO)

#if defined (ENABLE_ATCMD_CM_DBG_ERR)
#define ATCMD_CM_ERR(fmt, ...)    \
                                  ATCMD_CM_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ATCMD_CM_ERR(...)         do {} while (0)
#endif // (ENABLE_ATCMD_CM_DBG_ERR)

void *(*atcmd_cm_malloc)(size_t n) = pvPortMalloc;
void (*atcmd_cm_free)(void *ptr) = vPortFree;

atcmd_cm_cert_info_t *g_atcmd_cm_cert_info = NULL;

void atcmd_cm_set_malloc_free(void *(*malloc_func)(size_t), void (*free_func)(void *))
{
    atcmd_cm_malloc = malloc_func;
    atcmd_cm_free = free_func;

    return ;
}

void atcmd_cm_display_cert_info(atcmd_cm_cert_info_t *cert, unsigned int addr)
{
    DA16X_UNUSED_ARG(addr);

    if (cert->flag == ATCMD_CM_INIT_FLAG) {
        ATCMD_CM_INFO("atcmd_cm_cert_info_t: %p\n", cert);
        ATCMD_CM_INFO("* Size: %ld\n", sizeof(atcmd_cm_cert_info_t));
        ATCMD_CM_INFO("* Name: %s(%ld)\n", cert->name, strlen(cert->name));
        ATCMD_CM_INFO("* Type: %d\n", cert->type);
        ATCMD_CM_INFO("* Sequence: %d\n", cert->seq);
        ATCMD_CM_INFO("* Format: %d\n", cert->format);
        ATCMD_CM_INFO("* Cert Length: %d\n", cert->cert_len);
        ATCMD_CM_INFO("* sFlash addr: 0x%x\n", addr);
    } else {
        ATCMD_CM_INFO("Not init atcmd_cm_cert_info_t\n");
    }

    return ;
}

void atcmd_cm_display_info()
{
    int idx = 0;
    int status = DA_APP_SUCCESS;

    atcmd_cm_cert_t *cert = NULL;

    ATCMD_CM_INFO("sizeof(atcmd_cm_cert_t): %ld\n", sizeof(atcmd_cm_cert_t));

    cert = atcmd_cm_malloc(sizeof(atcmd_cm_cert_t));
    if (!cert) {
        ATCMD_CM_INFO("Failed to allocate memory for cert(%ld)\n", sizeof(atcmd_cm_cert_t));
        return ;
    }

#if defined (ENABLE_ATCMD_CM_DBG_INFO)
    if (g_atcmd_cm_cert_info) {
        ATCMD_CM_INFO("Cert info(%p:%ld) * %d\n", g_atcmd_cm_cert_info,
                      sizeof(atcmd_cm_cert_info_t), ATCMD_CM_MAX_CERT_NUM);

        for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
            atcmd_cm_display_cert_info(&(g_atcmd_cm_cert_info[idx]), g_atcmd_cm_cert_addr_list[idx]);
        }
    }
#endif // ENABLE_ATCMD_CM_DBG_INFO

    ATCMD_CM_INFO("Cert Address\n");
    for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
        atcmd_cm_init_cert_t(cert);

        ATCMD_CM_INFO("#%d. certificate(%p)\n", idx + 1, g_atcmd_cm_cert_addr_list[idx]);

        status = atcmd_cm_read_cert_by_idx(idx, cert);
        if (status) {
            ATCMD_CM_INFO("Failed to read certificate(0x%x)\n", status);
            continue;
        }

        atcmd_cm_display_cert_info(&cert->info, g_atcmd_cm_cert_addr_list[idx]);

#if defined (ENABLE_ATCMD_CM_DBG_INFO)
        //hex_dump((UCHAR *)cert->cert, strlen(cert->cert));
#endif /* ENABLE_ATCMD_CM_DBG_INFO */
    }

    if (cert) {
        atcmd_cm_free(cert);
    }

    return ;
}

void atcmd_cm_init_cert_info_t(atcmd_cm_cert_info_t *cert)
{
    memset(cert, 0x00, sizeof(atcmd_cm_cert_info_t));
    cert->flag = ATCMD_CM_INIT_FLAG;
}

void atcmd_cm_init_cert_t(atcmd_cm_cert_t *cert)
{
    memset(cert, 0x00, sizeof(atcmd_cm_cert_t));

    atcmd_cm_init_cert_info_t(&cert->info);
}

int atcmd_cm_init_cert_info()
{
    int status = DA_APP_SUCCESS;
    HANDLE handler = NULL;

    int idx = 0;
    atcmd_cm_cert_t *cert = NULL;
    UINT32 addr = 0x00;

    size_t cert_info_len = sizeof(atcmd_cm_cert_info_t) * ATCMD_CM_MAX_CERT_NUM;

    if (g_atcmd_cm_cert_info) {
        //Already initialized.
        return status;
    }

    cert = atcmd_cm_malloc(sizeof(atcmd_cm_cert_t));
    if (!cert) {
        ATCMD_CM_ERR("Failed to allocate memory for cert(%ld)\n", sizeof(atcmd_cm_cert_t));
        return DA_APP_NOT_CREATED;

    }

    if (dpm_mode_is_enabled()) {
        status = dpm_user_rtm_get(ATCMD_CM_INFO_NAME,
                                  (unsigned char **)&g_atcmd_cm_cert_info);
        if (status == 0) {
            status = dpm_user_rtm_allocate(ATCMD_CM_INFO_NAME,
                                           (VOID **)&g_atcmd_cm_cert_info,
                                           cert_info_len, 100);
            if (status) {
                ATCMD_CM_ERR("Failed to allocate rtm memory(0x%x,%ld)\n", status, cert_info_len);
                return status;
            }

            memset(g_atcmd_cm_cert_info, 0x00, cert_info_len);
        }
        status = DA_APP_SUCCESS;
    } else {
        g_atcmd_cm_cert_info = atcmd_cm_malloc(cert_info_len);
        if (!g_atcmd_cm_cert_info) {
            ATCMD_CM_ERR("Failed to allocate memory(%ld)\n", cert_info_len);
            status = DA_APP_NOT_CREATED;
            goto end;
        }

        memset(g_atcmd_cm_cert_info, 0x00, cert_info_len);
    }

    handler = flash_image_open((sizeof(UINT32) * 8),
                               g_atcmd_cm_cert_addr_list[0],
                               (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        ATCMD_CM_ERR("Failed to access sflash memory\n");
        status = DA_APP_NOT_SUCCESSFUL;
        goto end;
    }

    for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
        addr = g_atcmd_cm_cert_addr_list[idx];

        ATCMD_CM_INFO("addr(0x%x)\n", addr);

        status = flash_image_read(handler, addr, (VOID *)cert, sizeof(atcmd_cm_cert_t));
        if (status) {
            if (cert->info.flag != ATCMD_CM_INIT_FLAG) {
                ATCMD_CM_INFO("Init sflash memory(%p)\n", addr);

                atcmd_cm_init_cert_t(cert);

                flash_image_write(handler, addr, (VOID *)cert, sizeof(atcmd_cm_cert_t));
            }

            memcpy(&(g_atcmd_cm_cert_info[idx]), &cert->info, sizeof(atcmd_cm_cert_info_t));

            atcmd_cm_display_cert_info(&(g_atcmd_cm_cert_info[idx]), addr);

            status = DA_APP_SUCCESS;
        } else {
            status = DA_APP_NOT_SUCCESSFUL;
        }
    }

    flash_image_close(handler);

    atcmd_cm_display_info();

end:

    if (cert) {
        atcmd_cm_free(cert);
    }

    return status;
}

int atcmd_cm_deinit_cert_info()
{
    if (!g_atcmd_cm_cert_info) {
        return DA_APP_SUCCESS;
    }

    if (dpm_mode_is_enabled()) {
        dpm_user_rtm_release(ATCMD_CM_INFO_NAME);
    } else {
        atcmd_cm_free(g_atcmd_cm_cert_info);
    }

    g_atcmd_cm_cert_info = NULL;

    return DA_APP_SUCCESS;
}

int atcmd_cm_set_cert(char *name, unsigned char type, unsigned char seq,
                      unsigned char format, char *in, size_t inlen)
{
    int status = DA_APP_SUCCESS;
    int idx = 0;
    atcmd_cm_cert_t *cert = NULL;

    // Check validation
    if (name == NULL || strlen(name) >= ATCMD_CM_MAX_NAME) {
        ATCMD_CM_ERR("Invalid Name\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (type != ATCMD_CM_CERT_TYPE_CA_CERT && type != ATCMD_CM_CERT_TYPE_CERT) {
        ATCMD_CM_ERR("Invalid Type\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (   (type == ATCMD_CM_CERT_TYPE_CERT)
        && (seq != ATCMD_CM_CERT_SEQ_CERT && seq != ATCMD_CM_CERT_SEQ_KEY)) {
        ATCMD_CM_ERR("Invalid Sequence number\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (format != ATCMD_CM_CERT_FORMAT_DER && format != ATCMD_CM_CERT_FOMRAT_PEM) {
        ATCMD_CM_ERR("Invalid format\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (inlen > ATCMD_CM_MAX_CERT_BODY) {
        ATCMD_CM_ERR("Invalid Certificate Length(Max:%ld)\n", ATCMD_CM_MAX_CERT_BODY);
        return DA_APP_INVALID_PARAMETERS;
    }

    idx = atcmd_cm_find_idx(name, type, seq);
    if (idx > -1) {
        ATCMD_CM_INFO("Already existed certificate\n");
        return DA_APP_DUPLICATED_ENTRY;
    }

    idx = atcmd_cm_find_empty_idx();
    if (idx < 0) {
        ATCMD_CM_INFO("Not enough space\n");
        return DA_APP_OVERFLOW;
    }

    cert = atcmd_cm_malloc(sizeof(atcmd_cm_cert_t));
    if (!cert) {
        ATCMD_CM_ERR("Failed to allocate memory(%ld)\n", sizeof(atcmd_cm_cert_t));
        return DA_APP_NOT_CREATED;
    }

    atcmd_cm_init_cert_t(cert);

    strcpy(cert->info.name, name);
    cert->info.type = type;
    cert->info.seq = seq;
    cert->info.format = format;
    memcpy(cert->cert, in, inlen);
    cert->info.cert_len = inlen;

    status = atcmd_cm_write_cert_by_idx(idx, cert);
    if (status) {
        ATCMD_CM_ERR("Failed to write certificate(%d)\n", idx);
        goto end;
    }

    memcpy(&(g_atcmd_cm_cert_info[idx]), &(cert->info), sizeof(atcmd_cm_cert_info_t));

end:

    if (cert) {
        atcmd_cm_free(cert);
    }

    return status;
}

int atcmd_cm_get_cert_len(char *name, unsigned char type, unsigned char seq, size_t *outlen)
{
    int ret = 0;
    int idx_list[ATCMD_CM_MAX_CERT_NUM] = {0x00,};
    int idx_num = 0;
    int idx = 0;
    int num = 0;
    size_t total_len = 0;

    ATCMD_CM_INFO("To get certificate(%s(%ld))\n", name, strlen(name));

    *outlen = 0;

    // Find certificate by name
    ret = atcmd_cm_find_idx_list(name, idx_list, &idx_num);
    if (ret || idx_num <= 0) {
        ATCMD_CM_ERR("Failed to find certificate\n");
        return DA_APP_NOT_FOUND;
    }

    // Calculate total length
    for (num = 0 ; num < idx_num ; num++) {
        idx = idx_list[num];

        atcmd_cm_display_cert_info(&g_atcmd_cm_cert_info[idx],
                                   g_atcmd_cm_cert_addr_list[idx]);

        if (type == ATCMD_CM_CERT_TYPE_CA_CERT) {
            total_len += g_atcmd_cm_cert_info[idx].cert_len;
        } else if ((type == ATCMD_CM_CERT_TYPE_CERT) && (g_atcmd_cm_cert_info[idx].seq == seq)) {
            total_len += g_atcmd_cm_cert_info[idx].cert_len;
        }

        ATCMD_CM_INFO("cert len: %ld, total len: %ld\n", g_atcmd_cm_cert_info[idx].cert_len, total_len);
    }

    *outlen = total_len;

    ATCMD_CM_INFO("Total length(%s,%ld)\n", name, total_len);

    return DA_APP_SUCCESS;
}

int atcmd_cm_get_cert(char *name, unsigned char type, unsigned char seq, char *out, size_t *outlen)
{
    int status = DA_APP_SUCCESS;

    // Check validation
    if (name == NULL || strlen(name) >= ATCMD_CM_MAX_NAME) {
        ATCMD_CM_ERR("Invalid Name\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (type != ATCMD_CM_CERT_TYPE_CA_CERT && type != ATCMD_CM_CERT_TYPE_CERT) {
        ATCMD_CM_ERR("Invalid Type\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (   (type == ATCMD_CM_CERT_TYPE_CERT)
        && (seq != ATCMD_CM_CERT_SEQ_CERT && seq != ATCMD_CM_CERT_SEQ_KEY)) {
        ATCMD_CM_ERR("Invalid Sequence number\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (type == ATCMD_CM_CERT_TYPE_CA_CERT) {
        status = atcmd_cm_get_ca_cert_type(name, out, outlen);
        if (status) {
            ATCMD_CM_ERR("Failed to get CA certificate(0x%x)\n", status);
            return status;
        }
    } else if (type == ATCMD_CM_CERT_TYPE_CERT) {
        status = atcmd_cm_get_cert_type(name, seq, out, outlen);
        if (status) {
            ATCMD_CM_ERR("Failed to get certificate(0x%x)\n", status);
            return status;
        }
    }

    return status;
}

int atcmd_cm_get_ca_cert_type(char *name, char *out, size_t *outlen)
{
    int status = DA_APP_SUCCESS;
    atcmd_cm_cert_t *cert = NULL;

    int ret = 0;
    int idx_list[ATCMD_CM_MAX_CERT_NUM] = {0x00,};
    int idx_num = 0;
    int num = 0;
    UINT32 addr = 0x00;

    HANDLE handler = NULL;
    char *pos = NULL;
    size_t total_len = 0;

    ATCMD_CM_INFO("To get certificate(%s(%ld))\n", name, strlen(name));

    // Find certificate by name
    ret = atcmd_cm_find_idx_list(name, idx_list, &idx_num);
    if (ret || idx_num <= 0) {
        ATCMD_CM_ERR("Failed to find certificate\n");
        return DA_APP_NOT_FOUND;
    }

    // Sort by seq
    for (int i = 0 ; i < idx_num - 1 ; i++) {
        for (int j = 0 ; j < idx_num - i - 1 ; j++) {
            // Swap
            if (g_atcmd_cm_cert_info[idx_list[j]].seq > g_atcmd_cm_cert_info[idx_list[j + 1]].seq) {
                ATCMD_CM_ERR("Swap idx:%ld, seq:%ld/%ld\n",
                             idx_list[j],
                             g_atcmd_cm_cert_info[idx_list[j]].seq,
                             g_atcmd_cm_cert_info[idx_list[j + 1]].seq);

                int tmp = idx_list[j];
                idx_list[j] = idx_list[j + 1];
                idx_list[j + 1] = tmp;
            }
        }
    }

    // Calculate total length
    for (num = 0 ; num < idx_num ; num++) {
        atcmd_cm_display_cert_info(&g_atcmd_cm_cert_info[idx_list[num]],
                                   g_atcmd_cm_cert_addr_list[idx_list[num]]);

        if (g_atcmd_cm_cert_info[idx_list[num]].type == ATCMD_CM_CERT_TYPE_CA_CERT) {
            total_len += g_atcmd_cm_cert_info[idx_list[num]].cert_len;
        }
    }

    ATCMD_CM_INFO("outlen(%ld), total_len(%ld)\n", *outlen, total_len);

    if (*outlen < total_len) {
        ATCMD_CM_ERR("Not enough buffer size(%ld,%ld)\n", *outlen, total_len);
        return DA_APP_NOT_SUCCESSFUL;
    }

    cert = atcmd_cm_malloc(sizeof(atcmd_cm_cert_t));
    if (!cert) {
        ATCMD_CM_ERR("Failed to allocate memory(%ld)\n", sizeof(atcmd_cm_cert_t));
        return DA_APP_NOT_CREATED;
    }

    pos = out;

    handler = flash_image_open((sizeof(UINT32) * 8),
                                g_atcmd_cm_cert_addr_list[0],
                                (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        ATCMD_CM_ERR("Failed to access sflash memory\n");
        status = DA_APP_NOT_SUCCESSFUL;
        goto end;
    }

    for (num = 0 ; num < idx_num ; num++) {
        atcmd_cm_init_cert_t(cert);

        addr = g_atcmd_cm_cert_addr_list[idx_list[num]];

        status = flash_image_read(handler, addr, (VOID *)cert, sizeof(atcmd_cm_cert_t));
        if (status) {
            memcpy(pos, cert->cert, cert->info.cert_len);
            pos += cert->info.cert_len;
            *(pos - 1) = '\n';

            ret += DA_APP_SUCCESS;
        } else {
            ret += DA_APP_NOT_SUCCESSFUL;
        }
    }

    flash_image_close(handler);

    *(pos - 1) = '\0';
    *outlen = total_len;

#if defined (ENABLE_ATCMD_CM_DBG_INFO)
    //hex_dump((UCHAR *)out, *outlen);
#endif /* ENABLE_ATCMD_CM_DBG_INFO */

end:

    if (cert) {
        atcmd_cm_free(cert);
    }

    return ret;
}

int atcmd_cm_get_cert_type(char *name, unsigned char seq, char *out, size_t *outlen)
{
    int status = DA_APP_SUCCESS;
    int idx = 0;
    atcmd_cm_cert_t *cert = NULL;

    int used_alloc_mem = pdTRUE;

    ATCMD_CM_INFO("To get certificate(%s(%ld))\n", name, strlen(name));

    idx = atcmd_cm_find_idx(name, ATCMD_CM_CERT_TYPE_CERT, seq);
    if (idx < 0) {
        ATCMD_CM_ERR("Failed to find certificate\n");
        return DA_APP_NOT_FOUND;
    }

    if (*outlen >= sizeof(atcmd_cm_cert_t)) {
        cert = (atcmd_cm_cert_t *)out;
        used_alloc_mem = pdFALSE;
    } else {
        cert = atcmd_cm_malloc(sizeof(atcmd_cm_cert_t));
        if (!cert) {
            ATCMD_CM_ERR("Failed to allocate memory(%ld)\n", sizeof(atcmd_cm_cert_t));
            return DA_APP_NOT_CREATED;
        }
    }

    atcmd_cm_init_cert_t(cert);

    status = atcmd_cm_read_cert_by_idx(idx, cert);
    if (status) {
        ATCMD_CM_ERR("Failed to read certificate(%d. %s(%ld))\n", idx, name, strlen(name));
        goto end;
    }

    if (used_alloc_mem) {
        if (*outlen < cert->info.cert_len) {
            ATCMD_CM_ERR("Buffer size is not enough(%ld, %ld)\n", *outlen, cert->info.cert_len);
            status = DA_APP_UNDERFLOW;
            goto end;
        }

        *outlen = cert->info.cert_len;
        memcpy(out, cert->cert, cert->info.cert_len);
    } else {
        *outlen = cert->info.cert_len;
        memmove(cert, cert->cert, cert->info.cert_len);
    }

end:

    if (used_alloc_mem && cert) {
        atcmd_cm_free(cert);
    }

    return status;
}

int atcmd_cm_clear_cert(char *name, unsigned char type)
{
    int status = 0;

    int idx_list[ATCMD_CM_MAX_CERT_NUM] = {0x00,};
    int idx_num = 0;

    int idx = 0;
    int num = 0;

    // Check validation
    if (name == NULL || strlen(name) >= ATCMD_CM_MAX_NAME) {
        ATCMD_CM_ERR("Invalid Name\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (type != ATCMD_CM_CERT_TYPE_CA_CERT && type != ATCMD_CM_CERT_TYPE_CERT) {
        ATCMD_CM_ERR("Invalid Type\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    status = atcmd_cm_find_idx_list(name, idx_list, &idx_num);
    if (status || idx_num <= 0) {
        ATCMD_CM_ERR("Failed to find certificate(%s(%ld), %d)\n", name, strlen(name), type);
        return DA_APP_NOT_FOUND;
    }

    for (num = 0 ; num < idx_num ; num++) {
        idx = idx_list[num];

        if (g_atcmd_cm_cert_info[idx].type == type) {
            ATCMD_CM_INFO("Deleted certificate(%d)\n", idx);

            atcmd_cm_display_cert_info(&(g_atcmd_cm_cert_info[idx]),
                                       g_atcmd_cm_cert_addr_list[idx]);

            status = atcmd_cm_clear_cert_by_idx(idx);
            if (status) {
                ATCMD_CM_ERR("Failed to clear certificate(%d)\n", idx);
                return status;
            } else {
                atcmd_cm_init_cert_info_t(&(g_atcmd_cm_cert_info[idx]));
            }
        }
    }

    return DA_APP_SUCCESS;
}

const atcmd_cm_cert_info_t *atcmd_cm_get_cert_info(void)
{
    int status = DA_APP_SUCCESS;

    if (g_atcmd_cm_cert_info == NULL) {
        status = atcmd_cm_init_cert_info();
        if (status) {
            return NULL;
        }
    }

    return g_atcmd_cm_cert_info;
}

int atcmd_cm_is_exist_cert(char *name, unsigned char type, unsigned char seq)
{
    return ((atcmd_cm_find_idx(name, type, seq) == -1) ? pdFALSE : pdTRUE);
}

int atcmd_cm_find_idx(char *name, unsigned char type, unsigned char seq)
{
    int idx = 0;

    if (!g_atcmd_cm_cert_info) {
        if (atcmd_cm_init_cert_info()) {
            ATCMD_CM_ERR("Failed to init certificate info\n");
            return -1;
        }
    }

    ATCMD_CM_INFO("To find certificate\n"
                  "* Name: %s(%ld)\n"
                  "* Type: %ld\n"
                  "* Sequence: %ld\n",
                  name, strlen(name), type, seq);

    for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
        atcmd_cm_display_cert_info(&g_atcmd_cm_cert_info[idx],
                                   g_atcmd_cm_cert_addr_list[idx]);

        if (   (strcmp(name, g_atcmd_cm_cert_info[idx].name) == 0)
            && (type == g_atcmd_cm_cert_info[idx].type)
            && (seq == g_atcmd_cm_cert_info[idx].seq)) {
            ATCMD_CM_INFO("Found certificate(%d)\n", idx);
            return idx;
        }

        ATCMD_CM_INFO("Not matched certificate(%d)\n", idx);
    }

    return -1;
}

int atcmd_cm_find_idx_list(char *name, int idx_list[ATCMD_CM_MAX_CERT_NUM], int *idx_num)
{
    int idx = 0;

    memset(idx_list, 0x00, sizeof(int) * ATCMD_CM_MAX_CERT_NUM);
    *idx_num = 0;

    if (!g_atcmd_cm_cert_info) {
        if (atcmd_cm_init_cert_info()) {
            return -1;
        }
    }

    ATCMD_CM_INFO("To find certificate\n"
                  "* Name: %s(%ld)\n", name, strlen(name));

    for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
        atcmd_cm_display_cert_info(&g_atcmd_cm_cert_info[idx], g_atcmd_cm_cert_addr_list[idx]);

        if (strcmp(name, g_atcmd_cm_cert_info[idx].name) == 0) {
            ATCMD_CM_INFO("Found certificate(%d)\n", idx);

            idx_list[*idx_num] = idx;
            (*idx_num)++;
        } else {
            ATCMD_CM_INFO("Not matched certificate(%d)\n", idx);
        }
    }

    return (*idx_num == 0 ? -1 : 0);
}

int atcmd_cm_find_empty_idx(void)
{
    int idx = 0;

    if (!g_atcmd_cm_cert_info) {
        if (atcmd_cm_init_cert_info()) {
            return -1;
        }
    }

    for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
        if (strlen(g_atcmd_cm_cert_info[idx].name) == 0) {
            ATCMD_CM_INFO("Found empty idx(%d)\n", idx);
            return idx;
        }
    }

    return -1;
}

int atcmd_cm_read_cert_by_idx(unsigned int idx, atcmd_cm_cert_t *cert)
{
    if (idx >= ATCMD_CM_MAX_CERT_NUM) {
        ATCMD_CM_ERR("invalid index(%d)\n", idx);
    }

    return atcmd_cm_read_cert_by_addr(g_atcmd_cm_cert_addr_list[idx], cert);
}

int atcmd_cm_read_cert_by_addr(unsigned int addr, atcmd_cm_cert_t *cert)
{
    int status = 0;
    HANDLE handler = NULL;

    ATCMD_CM_INFO("addr(0x%x)\n", addr);

    memset(cert, 0x00, sizeof(atcmd_cm_cert_t));

    handler = flash_image_open((sizeof(UINT32) * 8), addr, (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        ATCMD_CM_ERR("Failed to access SFLASH memory\n");
        return DA_APP_NOT_SUCCESSFUL;
    }

    status = flash_image_read(handler, addr, (VOID *)cert, sizeof(atcmd_cm_cert_t));
    if (status == 0) {
        ATCMD_CM_ERR("Failed to read SFLASH memory(addr=0x%x, size=%d)\n", addr, sizeof(atcmd_cm_cert_t));
        status = DA_APP_NOT_SUCCESSFUL;
    } else {
        status = DA_APP_SUCCESS;
    }

    flash_image_close(handler);

    return status;
}

int atcmd_cm_write_cert_by_idx(unsigned int idx, atcmd_cm_cert_t *cert)
{
    if (idx >= ATCMD_CM_MAX_CERT_NUM) {
        ATCMD_CM_ERR("invalid index(%d)\n", idx);
    }

    return atcmd_cm_write_cert_by_addr(g_atcmd_cm_cert_addr_list[idx], cert);
}

int atcmd_cm_write_cert_by_addr(unsigned int addr, atcmd_cm_cert_t *cert)
{
    int status = 0;
    HANDLE handler = NULL;

    ATCMD_CM_INFO("addr(0x%x)\n", addr);

    handler = flash_image_open((sizeof(UINT32) * 8), addr, (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        ATCMD_CM_ERR("Failed to access SFLASH memory\n");
        return DA_APP_NOT_SUCCESSFUL;
    }

    status = flash_image_write(handler, addr, (VOID *)cert, sizeof(atcmd_cm_cert_t));
    if (status == 0) {
        ATCMD_CM_ERR("Failed to write SFLASH memory(addr=0x%x, size=%d)\n", addr, sizeof(atcmd_cm_cert_t));
        status = DA_APP_NOT_SUCCESSFUL;
    } else {
        status = DA_APP_SUCCESS;
    }

    flash_image_close(handler);

    return status;
}

int atcmd_cm_clear_cert_by_idx(unsigned int idx)
{
    atcmd_cm_cert_t *cert = NULL;

    if (idx >= ATCMD_CM_MAX_CERT_NUM) {
        ATCMD_CM_ERR("invalid index(%d)\n", idx);
    }

    cert = atcmd_cm_malloc(sizeof(atcmd_cm_cert_t));
    if (!cert) {
        ATCMD_CM_ERR("Failed to allocate memory for cert(%ld)\n", sizeof(atcmd_cm_cert_t));
        return DA_APP_NOT_CREATED;
    }

    atcmd_cm_init_cert_t(cert);

    atcmd_cm_write_cert_by_addr(g_atcmd_cm_cert_addr_list[idx], cert);

    atcmd_cm_free(cert);

    return DA_APP_SUCCESS;
}

int atcmd_cm_clear_cert_by_addr(unsigned int addr)
{
    int status = 0;
    HANDLE handler = NULL;

    ATCMD_CM_INFO("addr(0x%x)\n", addr);

    handler = flash_image_open((sizeof(UINT32) * 8), addr,
                               (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        ATCMD_CM_ERR("Failed to access SFLASH memory\n");
        return DA_APP_NOT_SUCCESSFUL;
    }

    status = flash_image_erase(handler, addr, sizeof(atcmd_cm_cert_t));
    if (status == 0) {
        ATCMD_CM_ERR("Failed to erase SFLASH memory(addr=0x%x, size=%d)\n", addr, sizeof(atcmd_cm_cert_t));
        status = DA_APP_NOT_SUCCESSFUL;
    } else {
        status = DA_APP_SUCCESS;
    }

    flash_image_close(handler);

    return status;
}
#endif // (__SUPPORT_ATCMD_TLS__)

/* EOF */
