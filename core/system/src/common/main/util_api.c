/**
 ****************************************************************************************
 *
 * @file util_api.c
 *
 * @brief Utility APIs for user function
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

#include "sdk_type.h"
#include "da16x_types.h"

#include "environ.h"
#include "da16x_time.h"
#include "da16x_image.h"
#include "common_def.h"
#include "util_api.h"
#include "da16x_system.h"
#include "lwip/err.h"
#include "da16x_dpm.h"
#include "da16x_dpm_rtm_mem.h"
#include "da16x_compile_opt.h"
#include "da16x_sntp_client.h"
#if defined (__SUPPORT_WIFI_CONN_CB__)
#include "lwip/priv/tcp_priv.h"
#include "da16x_dhcp_server.h"
#include "dhcpserver.h"
#endif // __SUPPORT_WIFI_CONN_CB__
#ifdef __SUPPORT_REMOVE_MAC_NAME__
#include "app_provision.h"
#endif // __SUPPORT_REMOVE_MAC_NAME__

/// Hexa Dump API //////////////////////////////////////////////////////////
/*
direction: 0: UART0, 1: ATCMD_INTERFACE
output_fmt: 0: ascii only, 1 hexa only, 2 hexa with ascii
*/

void hexa_dump_print(u8 *title, const void *buf, size_t len, char direction, char output_fmt)
{
    size_t i, llen;
    const u8 *pos = buf;
    const size_t line_len = 16;
    int hex_index = 0;
    char *buf_prt = NULL;

    buf_prt = pvPortMalloc(64);
    if (buf_prt == NULL) {
        PRINTF("[%s] Failed to allocate the temporary buffer ...\n", __func__);
        return;
    }

    if (output_fmt) {
        PRINTF(">>> %s \n", title);
    }

    if (buf == NULL) {
        PRINTF(" - hexdump%s(len=%lu): [NULL]\n",
            output_fmt == OUTPUT_HEXA_ONLY ? "":"_ascii", (unsigned long) len);

        vPortFree(buf_prt);

        return;
    }

    if (output_fmt) {
        PRINTF("- (len=%lu):\n", (unsigned long) len);
    }

    while (len) {
        char tmp_str[4];

        llen = len > line_len ? line_len : len;

        memset(buf_prt, 0, 64);

        if (output_fmt) {
            sprintf(buf_prt, "[%08x] ", hex_index);

            for (i = 0; i < llen; i++) {
                sprintf(tmp_str, " %02x", pos[i]);
                strcat(buf_prt, tmp_str);
            }

            hex_index = hex_index + i;

            for (i = llen; i < line_len; i++) {
                strcat(buf_prt, "   ");  /* _xx */
            }

            if (direction) {
#ifdef  __TEST_USER_AT_CMD__
                PRINTF_ATCMD("%s  ", buf_prt);
#endif
            } else {
                PRINTF("%s  ", buf_prt);
            }

            memset(buf_prt, 0, 64);
        }

        if (output_fmt == OUTPUT_HEXA_ASCII || output_fmt == OUTPUT_ASCII_ONLY) {
            for (i = 0; i < llen; i++) {
                if (   (pos[i] > 0x20 && pos[i] < 0x7f)
                    || (output_fmt == OUTPUT_ASCII_ONLY && (pos[i]  == 0x0d
                    || pos[i]  == 0x0a
                    || pos[i]  == 0x0c))) {

                    sprintf(tmp_str, "%c", pos[i]);
                    strcat(buf_prt, tmp_str);
                } else if (output_fmt) {
                    strcat(buf_prt, ".");
                }
            }
        }

        if (output_fmt) {
            for (i = llen; i < line_len; i++) {
                strcat(buf_prt, " ");
            }

            if (direction) {
                strcat(buf_prt, "\n\r");    /* ATCMD */
            } else {
                strcat(buf_prt, "\n");        /* Normal */
            }
        }

        if (direction) {
#ifdef  __TEST_USER_AT_CMD__
            PRINTF_ATCMD(buf_prt);
#endif
        } else {
            PRINTF("%s", buf_prt);
        }

        pos += llen;
        len -= llen;
    }

    vPortFree(buf_prt);
}

void hex_dump(UCHAR *data, UINT length)
{
    hexa_dump_print((u8 *)"", data, length, 0, OUTPUT_HEXA_ASCII);
}


void hex_dump_atcmd(UCHAR *data, UINT length)
{
    hexa_dump_print((u8 *)"", data, length, 1, OUTPUT_HEXA_ASCII);
}

int is_duplicate_string_found(char **str_array, int str_count)
{
    int duplicate_found = 0;
    unsigned int i;

    for (i = str_count; i > 0; i--) {
        unsigned int j;

        if (str_array[i - 1] == NULL) {
            continue;
        }

        for (j = i - 1; j > 0; j--) {
            if (strcmp(str_array[i - 1], str_array[j - 1]) == 0) {
                duplicate_found = 1;
                break;
            }
        }

        if (duplicate_found)
            break;
    }

    return duplicate_found;
}

#define CHECK_RANGE(min, max, val, ret_val) if( val < min || val > max ){ return( ret_val ); }
int is_date_time_valid(struct tm *t)
{
    /*
        int tm_sec;
        int tm_min;
        int tm_hour;

        int tm_mday;
        int tm_mon;
        int tm_year;
    */

    int month_len;
    int month;

    month = t->tm_mon + 1;
    CHECK_RANGE( 70, 8099, t->tm_year, -2); // 1970 ~ 9999
    CHECK_RANGE( 0, 23,    t->tm_hour, -3);
    CHECK_RANGE( 0, 59,    t->tm_min,  -3);
    CHECK_RANGE( 0, 59,    t->tm_sec,  -3);

    switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        month_len = 31;
        break;
    case 4: case 6: case 9: case 11:
        month_len = 30;
        break;
    case 2:
        if ( ( !( t->tm_year % 4 ) && t->tm_year % 100 ) || !( t->tm_year % 400 ) ) {
            month_len = 29;
        } else {
            month_len = 28;
        }
        break;
    default:
        return -2;
    }
    CHECK_RANGE( 1, month_len, t->tm_mday, -2);

    return pdTRUE;
}

/* this custom atoi function returns 0 (error) when "21aaaa" (where non digit character is
   ever included) is passed over, unlike atoi() which returns 21 (takes and converts
   digit characters only until a non-digit character is met) */
int atoi_custom (char* str)
{
    int res = 0, minus_sign = 0;

    if (str == NULL) {
        return 0;
    }

    for (int i = 0; str[i] != '\0'; ++i) {
        if (i == 0) {
             if (str[i] >= '0' && str[i] <= '9') {
                 res = res * 10 + str[i] - '0';
             } else if (str[i] == '-') {
                 minus_sign = 1;
             } else if (str[i] == '+') {
                 minus_sign = 0;
             } else {
                return 0;
             }
        } else {
            if (str[i] >= '0' && str[i] <= '9') {
                res = res * 10 + str[i] - '0';
            } else {
                return 0;
            }
        }
    }

    return (minus_sign?(res*(-1)):(res));
}

int get_int_val_from_str(char* param, int* int_val, atoi_err_policy policy)
{
    int result = -1, param_len, int_val_old;

    if (param == NULL || int_val == NULL) {
        return -1;
    }

    param_len = strlen(param);
    int_val_old = *int_val;

    if (param_len == 1) {
        if (param[0] == '0') {
            // "0" <- non error 0 return
            *int_val = 0;
            result = 0; /* SUCCESS */
        } else {
            // check if valid single digit 1 ~ 9
            *int_val = atoi_custom(param);

            if (*int_val > 0 && *int_val < 10) {
                // valid value: 1~9
                result = 0; /* SUCCESS */
            } else {
                // error: e.g. == 0
                *int_val = int_val_old;
                result = -1;
            }
        }
    } else if (param_len == 0) {
        *int_val = int_val_old;
        result = -1;
    } else {
        //    param_len > 1

        if (policy == POL_1) {
            // leading "0" / "+" / "-0" are not allowed
            if (param[0] == '0' || param[0] == '+')
                return -1;

            if (param[0] == '-' && param[1] == '0')
                return -1;
        } else if (policy == POL_2) {
            // leading "+" / "-0" are not allowed
            if (param[0] == '+')
                return -1;

            if (param[0] == '-' && param[1] == '0')
                return -1;
        }

        *int_val = atoi_custom(param);

        if (*int_val > -1 && *int_val < 10) {
            // considered error
            *int_val = int_val_old;
            result = -1;
        } else {
            result = 0; /* SUCCESS */
        }
    }

    return result;
}

int get_int_val_from_date_time_str(char* param, int* int_val)
{
    int param_len;
    char *vaild_digit = "0123456789";

    if (param == NULL || int_val == NULL) {
        return -1;
    }

    param_len = strlen(param);

    if (param_len == 2) {
        if (strchr(vaild_digit, param[0]) == NULL || strchr(vaild_digit, param[1]) == NULL) {
            return -1;
        }
    } else {
        return -1;
    }

    *int_val = atoi_custom(param);

    return 0;
}

int    chk_valid_macaddr(char *macaddr_str)
{
#define MAC_ADDR_LEN_W_DEL      17
#define MAC_ADDR_DEL            ':'

    int     input_mac_len;
    char    input_mac_str[MAC_ADDR_LEN_W_DEL + 1];

    input_mac_len = strlen(macaddr_str);

    if (input_mac_len != MAC_ADDR_LEN_W_DEL) {
        return pdFALSE;
    }

    memset(input_mac_str, MAC_ADDR_LEN_W_DEL, 0);
    strcpy(input_mac_str, macaddr_str);

    for (int i = 0; i < MAC_ADDR_LEN_W_DEL; i++) {
        if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
            if (input_mac_str[i] != MAC_ADDR_DEL) {
                return pdFALSE;
            } else {
                continue;
            }
        }

        if (!isxdigit((int)(input_mac_str[i]))) {
            return pdFALSE;
        }
    }

    return pdTRUE;
}

int is_correct_query_arg(int argc, char *str)
{
    return (argc == 2 && strcmp(str, "?") == 0);
}

int is_in_valid_range(int val, int min, int max)
{
    if (val >= min && val <= max) {
        return pdTRUE;
    } else {
        return pdFALSE;
    }
}


#if defined(__RUNTIME_CALCULATION__)

//// For Code Run-time Calculation ////////////////////////////////////////////
#define MAX_TICK_TRACE     30
#define MAX_TICK_STRING    30
#define TICK_SAVE_BIT      0x80000000

UINT  tick_trace[MAX_TICK_TRACE];
UCHAR tick_trace_string[MAX_TICK_TRACE][MAX_TICK_STRING];
int   save_tick_count = 0;
ULONG _start_tick = 0;

#define TICK_SCALE
void set_start_time_for_printf(char inital)
{
    /** Runtime calculation should be moved
     ** During POR/RESET , happen fault, so for fast reconntion, let's do set as 0
     */
    if (inital) {
        _start_tick = 0;
    } else {
#ifdef TICK_SCALE
        _start_tick = xTaskGetTickCount();
#else
        _start_tick = get_fci_dpm_curtime();
#endif
    }

    memset(tick_trace, 0, MAX_TICK_TRACE*sizeof(UINT));
    memset(tick_trace_string, 0, MAX_TICK_TRACE*MAX_TICK_STRING);
}

void printf_with_run_time(char * string)
{
    ULONG cur_tick, diff_tick;
    ULONG diff_tick_MS;

#ifdef TICK_SCALE
    cur_tick = xTaskGetTickCount();
#else
    cur_tick = get_fci_dpm_curtime();
#endif

    diff_tick = cur_tick - _start_tick;
#ifdef TICK_SCALE
    diff_tick_MS = diff_tick * 10;
#else
    diff_tick_MS = diff_tick / 1000;
#endif

#if    defined(RAM_2ND_BOOT)
    Printf(CYAN_COLOR "[ ~ %d.%03d sec ] %s \n" CLEAR_COLOR,
#elif defined(XIP_CACHE_BOOT)
    PRINTF(CYAN_COLOR "[ ~ %d.%03d sec ] %s \n" CLEAR_COLOR,
#endif
                ((int)diff_tick_MS / 1000),
                (diff_tick_MS % 1000),
                string);
}

void save_with_run_time(char * string)
{
    ULONG cur_tick;

    cur_tick = xTaskGetTickCount();

    if (save_tick_count < MAX_TICK_TRACE) {
        strncpy((char *)(&tick_trace_string[save_tick_count][0]), string, MAX_TICK_STRING-1);
        tick_trace[save_tick_count] = cur_tick | TICK_SAVE_BIT;
        save_tick_count++;
    }
}

void printf_save_with_run_time(void)
{
    ULONG save_tick, diff_tick;
    ULONG diff_tick_MS;
    int    i;

    for (i = 0; i < MAX_TICK_TRACE; i++) {
        save_tick = tick_trace[i];

        if (save_tick & TICK_SAVE_BIT) {
            save_tick &= ~TICK_SAVE_BIT;
            diff_tick = save_tick - _start_tick;
            diff_tick_MS = diff_tick * 10;

#if    defined(RAM_2ND_BOOT)
            Printf(GREEN_COLOR "[ ~ %d.%03d sec ] %s \n" CLEAR_COLOR,
#elif defined(XIP_CACHE_BOOT)
            PRINTF(YELLOW_COLOR "[ ~ %d.%03d sec ] %s \n" CLEAR_COLOR,
#endif
                ((int)diff_tick_MS / 1000),
                (diff_tick_MS % 1000),
                &tick_trace_string[i]);
        }
    }
}
#else
void set_start_time_for_printf(char inital)
{
    DA16X_UNUSED_ARG(inital);
}

void printf_with_run_time(char *string)
{
    DA16X_UNUSED_ARG(string);
}

void save_with_run_time(char *string)
{
    DA16X_UNUSED_ARG(string);
}

void printf_save_with_run_time(void)
{
}
#endif    /* __RUNTIME_CALCULATION__ */

#ifdef __BOOT_CONN_TIME_PRINT__
ULONG    _boot_start_tick = 0;
void set_boot_conn_start_time(void)
{
    _boot_start_tick = xTaskGetTickCount();;
}

void log_boot_conn_with_run_time(char * string)
{
    ULONG cur_tick, diff_tick;
    ULONG diff_tick_MS;

    cur_tick = xTaskGetTickCount();

    diff_tick = cur_tick - _boot_start_tick;
    diff_tick_MS = diff_tick * 10;

    PRINTF(CYAN_COLOR "[ ~ %d.%03d sec ] %s \n" CLEAR_COLOR,
                ((int)diff_tick_MS / 1000),
                (diff_tick_MS % 1000),
                string);
}

#endif

//// For Serial-Flash Read/Write APIs /////////////////////////////////////////
static UINT util_sflash_read(UINT sflash_addr, VOID *rd_buf, UINT rd_size)
{
    HANDLE  sflash_handler;
    UINT    status = TRUE;

    /* Open SFLASH device */
    sflash_handler = flash_image_open(
                                    (sizeof(UINT32) * 8),
                                    sflash_addr,
                                    (VOID *)&da16x_environ_lock);
    if (sflash_handler == NULL) {
        PRINTF("- SFLASH access error !!! \n");
        return    FALSE;
    }

    memset(rd_buf, 0, rd_size);

    /* Read SFLASH user area */
    status = flash_image_read(sflash_handler,
                            sflash_addr,
                            (VOID *)rd_buf,
                            rd_size);

    if (status == 0) {
        PRINTF("- SFLASH read error : addr=0x%x, size=%d\n", sflash_addr, rd_size);
        return FALSE;
    }

    /* Close SFLASH device */
    flash_image_close(sflash_handler);

    return TRUE;
}

static UINT util_sflash_write(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size)
{
    HANDLE    sflash_handler;
    UINT    wr_buf_addr;
    UINT    offset;
    INT    remained_wr_size;
    UCHAR    *sf_sector_buf = NULL;
    UINT    status = TRUE;

    sf_sector_buf = (UCHAR *)APP_MALLOC(SF_SECTOR_SZ);

    if (sf_sector_buf == NULL) {
        PRINTF("- SFLASH write error : APP_MALLOC fail...\n");

        return FALSE;
    }

    sflash_handler = flash_image_open((sizeof(UINT32)*8),
                                    sflash_addr,
                                    (VOID *)&da16x_environ_lock);

aligned_addr :
    wr_buf_addr = (UINT)wr_buf;
    remained_wr_size = wr_size;

    /* Check Serial-Flash sector alignment */
    if ((sflash_addr % SF_SECTOR_SZ) == 0) {
        for (offset = 0; offset < wr_size; offset += SF_SECTOR_SZ) {
            /* Make write data for 4KB sector */
            memset(sf_sector_buf, 0, SF_SECTOR_SZ);

            if (remained_wr_size < SF_SECTOR_SZ) {
                /* Pre-read to write */
                status = flash_image_read(sflash_handler,
                                            (sflash_addr + offset),
                                            (VOID *)sf_sector_buf,
                                            SF_SECTOR_SZ);
                if (status == 0) {
                    // Read Error
                    PRINTF("- SFLASH write : pre_read error : addr=0x%x\n", (sflash_addr + offset));
                    status = FALSE;
                    goto sflash_wr_end;
                }

                /* Copy remained write data */
                memcpy(sf_sector_buf, (void *)(wr_buf_addr + offset), remained_wr_size);
            } else {
                memcpy(sf_sector_buf, (void *)(wr_buf_addr + offset), SF_SECTOR_SZ);
            }

            remained_wr_size -= SF_SECTOR_SZ;

            flash_image_write(sflash_handler,
                        (sflash_addr + offset),
                        (VOID *)sf_sector_buf,
                        SF_SECTOR_SZ);

            status = TRUE;
        }
    } else {
        UINT    sflash_aligned_addr;

        offset = (sflash_addr % SF_SECTOR_SZ);
        sflash_aligned_addr = sflash_addr - offset;

        /* Make write data for 4KB sector */
        memset(sf_sector_buf, 0, SF_SECTOR_SZ);

        /* Pre-read to write */
        status = flash_image_read(sflash_handler,
                                sflash_aligned_addr,
                                (VOID *)sf_sector_buf,
                                SF_SECTOR_SZ);
        if (status == 0) {
            // Read Error
            PRINTF("- SFLASH write : pre_read error : addr=0x%x\n", sflash_aligned_addr);

            status = FALSE;
            goto sflash_wr_end;
        }

        /* Write first sector */
        if (wr_size > SF_SECTOR_SZ - offset) {
            memcpy(&sf_sector_buf[offset], wr_buf, SF_SECTOR_SZ - offset);
        } else {
            memcpy(&sf_sector_buf[offset], wr_buf, wr_size);
        }

        flash_image_write(sflash_handler,
                    sflash_aligned_addr,
                    (VOID *)sf_sector_buf,
                    SF_SECTOR_SZ);

        remained_wr_size -= (SF_SECTOR_SZ - offset);

        status = TRUE;

        /* Check remained write data */
        if (remained_wr_size > 0) {
            wr_buf = (UCHAR *)(&wr_buf[SF_SECTOR_SZ - offset]);
            wr_size -= (SF_SECTOR_SZ - offset);

            /* Next aligned sector address */
            sflash_addr = sflash_aligned_addr + SF_SECTOR_SZ;

            goto aligned_addr;
        } else {
            goto sflash_wr_end;
        }
    }

sflash_wr_end :
    flash_image_close(sflash_handler);
    APP_FREE(sf_sector_buf);

    return status;
}

UINT util_sflash_erase(UINT sflash_addr)
{
    HANDLE    sflash_handler;
#ifdef ENABLE_ERASE_SIZE
    UINT    erase_size = 0;
#endif
    UINT    status = TRUE;

    sflash_handler = flash_image_open((sizeof(UINT32)*8), sflash_addr, (VOID *)&da16x_environ_lock);
    if (sflash_handler == NULL) {
        return 0;
    }

#ifdef ENABLE_ERASE_SIZE
    erase_size = flash_image_erase(sflash_handler, (UINT32)sflash_addr, SF_SECTOR_SZ);
#else
    flash_image_erase(sflash_handler, (UINT32)sflash_addr, SF_SECTOR_SZ);
#endif

    flash_image_close(sflash_handler);

    return status;
}

UINT user_sflash_read(UINT sflash_addr, VOID *rd_buf, UINT rd_size)
{
    /* Check address range */
    if ((sflash_addr < SFLASH_USER_AREA_START) || (sflash_addr >= SFLASH_USER_AREA_END)) {
        PRINTF("- SFLASH read error : Wrong address offset (0x%x ~ 0x%x) !!!\n",
                        SFLASH_USER_AREA_START, SFLASH_USER_AREA_END);

        return    FALSE;
    }

    if ((sflash_addr + rd_size) >= SFLASH_USER_AREA_END) {
        PRINTF("- SFLASH read error : Wrong size (0x%x ~ 0x%x) !!!\n", SFLASH_USER_AREA_START, SFLASH_USER_AREA_END);
        return    FALSE;
    }

    return util_sflash_read(sflash_addr, rd_buf, rd_size);
}

UINT user_sflash_write(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size)
{
    /* Check address range */
    if ((sflash_addr < SFLASH_USER_AREA_START) || (sflash_addr >= SFLASH_USER_AREA_END)) {
        PRINTF("- SFLASH write error : Wrong address offset (0x%x ~ 0x%x) !!!\n",
                  SFLASH_USER_AREA_START, SFLASH_USER_AREA_END);

        return FALSE;
    }

    if ((sflash_addr + wr_size) >= SFLASH_USER_AREA_END) {
        PRINTF("- SFLASH write error : Wrong size (0x%x ~ 0x%x) !!!\n",
                  SFLASH_USER_AREA_START, SFLASH_USER_AREA_END);

        return FALSE;
    }

    return util_sflash_write(sflash_addr, wr_buf, wr_size);
}

UINT user_sflash_erase(UINT sflash_addr)
{
    /* Check address range */
    if ((sflash_addr < SFLASH_USER_AREA_START) || (sflash_addr >= SFLASH_USER_AREA_END)) {
        PRINTF("- SFLASH erase error : Wrong address offset (0x%x ~ 0x%x) !!!\n",
                  SFLASH_USER_AREA_START, SFLASH_USER_AREA_END);

        return FALSE;
    }

    return util_sflash_erase(sflash_addr);
}

#if defined (__BLE_COMBO_REF__)
UINT ble_sflash_read(UINT sflash_addr, VOID *rd_buf, UINT rd_size)
{
    /* Sanity check */
    if (sflash_addr < SFLASH_14531_BLE_AREA_START || sflash_addr >= SFLASH_14531_BLE_AREA_END) {
        PRINTF("- SFLASH read error: addr (0x%x) should be (0x%x ~ 0x%x)!\n",
               sflash_addr,
               SFLASH_14531_BLE_AREA_START, SFLASH_14531_BLE_AREA_END);

        return FALSE;
    }

    if ((sflash_addr + rd_size) > SFLASH_14531_BLE_AREA_END) {
        PRINTF("- SFLASH read error: addr(0x%x)+size(0x%x)= 0x%x > 0x%x \n",
               sflash_addr, rd_size,
               sflash_addr + rd_size, SFLASH_14531_BLE_AREA_END);

        return FALSE;
    }

    return util_sflash_read(sflash_addr, rd_buf, rd_size);
}

UINT ble_sflash_write(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size)
{
    /* Sanity check */
    if (sflash_addr < SFLASH_14531_BLE_AREA_START || sflash_addr >= SFLASH_14531_BLE_AREA_END) {
        PRINTF("- SFLASH write error: addr (0x%x) should be (0x%x ~ 0x%x)!\n",
                sflash_addr,
                SFLASH_14531_BLE_AREA_START, SFLASH_14531_BLE_AREA_END);

        return FALSE;
    }

    if ((sflash_addr + wr_size) > SFLASH_14531_BLE_AREA_END) {
        PRINTF("- SFLASH write error: addr(0x%x)+size(0x%x)= 0x%x > 0x%x \n",
               sflash_addr, wr_size,
               sflash_addr + wr_size, SFLASH_14531_BLE_AREA_END);
        return FALSE;
    }

    return util_sflash_write(sflash_addr, wr_buf, wr_size);
}

#ifdef ENABLE_BLE_SFLASH_ERASE
UINT ble_sflash_erase(UINT sflash_addr)
{
    /* Sanity check */
    if (sflash_addr < SFLASH_14531_BLE_AREA_START || sflash_addr >= SFLASH_14531_BLE_AREA_END) {
        PRINTF("- SFLASH write error: addr (0x%x) should be (0x%x ~ 0x%x)!\n",
                sflash_addr,
                SFLASH_14531_BLE_AREA_START, SFLASH_14531_BLE_AREA_END);

        return FALSE;
    }

    return util_sflash_erase(sflash_addr);
}
#endif

#endif // ( __BLE_COMBO_REF__ )


//// For Firmware version APIs /////////////////////////////////////////

#define BOOT_IMG_VER    0
#define SLIB_IMG_VER    1
#define RTOS_IMG_VER    2
UINT get_firmware_version(UINT fw_type, UCHAR *get_ver)
{
    UCHAR    *rd_buf = NULL;
    HANDLE  sflash_handler;
    UINT     sflash_addr = 0;
    UINT    rd_size = 0;
    UINT    ver_len = 0;

    if (fw_type == BOOT_IMG_VER) {
        sflash_addr = 0;
    } else if (get_cur_boot_index() == 1) {
        if (fw_type == SLIB_IMG_VER) {
            sflash_addr = SFLASH_CM_1_BASE;
        } else if (fw_type == RTOS_IMG_VER) {
            sflash_addr = SFLASH_RTOS_1_BASE;
        } else {
            PRINTF("[%s] Wrong FW type (%d)\n", __func__, fw_type);
            return ver_len;
        }
    } else {
        if (fw_type == SLIB_IMG_VER) {
            sflash_addr = SFLASH_CM_0_BASE;
        } else if (fw_type == RTOS_IMG_VER) {
            sflash_addr = SFLASH_RTOS_0_BASE;
        } else {
            PRINTF("[%s] Wrong FW type (%d)\n", __func__, fw_type);
            return ver_len;
        }
    }

    rd_buf = (UCHAR *)pvPortMalloc(SF_SECTOR_SZ);

    if (rd_buf == NULL) {
        PRINTF("[%s] malloc fail ...\n", __func__);
        return ver_len;
    }

    memset(rd_buf, 0, SF_SECTOR_SZ);

    /* Open SFLASH device */
    sflash_handler = flash_image_open((sizeof(UINT32) * 8),
                                    sflash_addr,
                                    (VOID *)&da16x_environ_lock);
    if (sflash_handler == NULL) {
        PRINTF("- SFLASH access error !!! \n");

        vPortFree(rd_buf);

        return    ver_len;
    }

    /* Read SFLASH user area */
    rd_size = flash_image_read(sflash_handler,
                                sflash_addr,
                                (VOID *)rd_buf,
                                SF_SECTOR_SZ);

    /* Close SFLASH device */
    flash_image_close(sflash_handler);

    if (rd_size != SF_SECTOR_SZ) {
        // Read Error
        PRINTF("- SFLASH read error : addr=0x%x, size=%d\n", sflash_addr, rd_size);

        vPortFree(rd_buf);

        return ver_len;
    }

    memcpy(get_ver, rd_buf + SFLASH_VER_OFFSET, SFLASH_VER_SIZE);

    ver_len = strlen((char *)(rd_buf + SFLASH_VER_OFFSET));
    if (ver_len > SFLASH_VER_SIZE)
        ver_len = SFLASH_VER_SIZE;

    vPortFree(rd_buf);

    return ver_len;
}

void fw_ver_display(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    unsigned char get_ver[SFLASH_VER_SIZE] = { 0x00, };

    get_firmware_version(SLIB_IMG_VER, get_ver);
    PRINTF("%s\n", get_ver);

    memset(get_ver, 0, SFLASH_VER_SIZE);
    get_firmware_version(RTOS_IMG_VER, get_ver);
    PRINTF("%s\n", get_ver);

    return;
}


//// For create random port APIs /////////////////////////////////////////
static inline ULONG get_random_value(void)
{
#ifdef    __TIME64__
    __time64_t now;
    ULONG    rand_val;

    da16x_time64(NULL, &now);
#else
    __time32_t now;
    ULONG    rand_val;

    now = da16x_time32(NULL);
#endif    /* __TIME64__ */

    srand(now);

    rand_val = (ULONG)rand();

    return rand_val;
}

/*
 * Get random value ( 8bits )
 */
UCHAR get_random_value_uchar(void)
{
    ULONG result;

    result = (UCHAR)(get_random_value() & 0x000000FF);

    return result;
}

/*
 * Get random value ( 16bits )
 */
USHORT get_random_value_ushort(void)
{
    USHORT    result;

    result = (USHORT)(get_random_value() & 0x0000FFFF);

    return result;
}

/*
 * Get random value ( 32bits )
 */
ULONG get_random_value_ulong(void)
{
    return get_random_value();
}

unsigned short get_random_value_ushort_range(int lower, int upper)
{
    if (lower < 1 || upper < 2 || lower >= upper) {return 0;}

    return (unsigned short)((get_random_value_ushort() % (upper - lower + 1)) + lower);
}

//// For get SCAN result API //////////////////////////////////////////////////

#define SCAN_BSSID_IDX       0
#define SCAN_FREQ_IDX        1
#define SCAN_SIGNAL_IDX      2
#define SCAN_FLGA_IDX        3
#define SCAN_SSID_IDX        4

#define HIDDEN_SSID_DETECTION_CHAR    '\t'

#define _SWAP_LONG(a, b)                    \
    do {                                    \
        long tmp = 0;                       \
        memcpy(&tmp, a, sizeof(long));      \
        memcpy(a, b, sizeof(long));         \
        memcpy(b, &tmp, sizeof(long));      \
    } while (0)


static char *result_buf[MAX_SCAN_AP_CNT][5];
static char scan_result[SCAN_RSP_BUF_SIZE] = { 0, };

static void start_scan(void)
{
    memset(scan_result, 0, SCAN_RSP_BUF_SIZE);

    da16x_cli_reply("scan", NULL, scan_result);

    if (strlen(scan_result) < 30) {
        PRINTF("Scan: %s\n", scan_result);
    }
}

static void make_scan_list(scan_result_t *pktPtr)
{
    int    sort_key = SCAN_SIGNAL_IDX;
    int    i, j, chk_idx;
    int    scanned_cnt = 0;
    UINT    ap_idx = 0;
    char    *saved_scan_result = NULL;
    char    *scan_result_ptr = NULL;
    char    rssi_str[4];

    saved_scan_result = (char *)APP_MALLOC(SCAN_RSP_BUF_SIZE);

    memset(result_buf, 0, sizeof(result_buf));
    memset(saved_scan_result, 0, SCAN_RSP_BUF_SIZE);

    /* Copy background scan result */
    memcpy(saved_scan_result, scan_result, SCAN_RSP_BUF_SIZE);

    scan_result_ptr = saved_scan_result;

    if (strlen(scan_result_ptr) < 30) {
        printf("Scan: %s\n", scan_result_ptr);

        vPortFree(saved_scan_result);

        return;
    }

    scanned_cnt = 0;

    /* [START] ======= Separate param ======= */
    /* parse arguments */
    result_buf[scanned_cnt++][0] = strtok(scan_result_ptr, "\n"); /* Title */

    if (result_buf[0][0] == NULL) {
        printf(ANSI_COLOR_RED "No Scan Resutls\n" ANSI_NORMAL);
    } else {
        while ((result_buf[scanned_cnt][SCAN_BSSID_IDX] = strtok(NULL, "\t")) != NULL) {    /* BSSID */
            if ((result_buf[scanned_cnt][SCAN_FREQ_IDX] = strtok(NULL, "\t")) != NULL) {    /* Freq. */
                if ((result_buf[scanned_cnt][SCAN_SIGNAL_IDX] = strtok(NULL, "\t")) != NULL) {    /* Signal*/
                    if ((result_buf[scanned_cnt][SCAN_FLGA_IDX] = strtok(NULL, "\t")) != NULL) {    /* Flag */
                        result_buf[scanned_cnt][SCAN_SSID_IDX] = strtok(NULL, "\n");    /* SSID */
                    }
                }
            }

            scanned_cnt++;
            if (scanned_cnt > (MAX_SCAN_AP_CNT - 1)) {
                break;
            }
        }

        /* sort */
        for (i = 1; i < scanned_cnt-1; ++i ) {
            for (j = 1; j < scanned_cnt-i; ++j) {
                if (atoi(result_buf[j][sort_key]) < atoi(result_buf[j+1][sort_key])) {
                    _SWAP_LONG(&result_buf[j][SCAN_BSSID_IDX], &result_buf[j+1][SCAN_BSSID_IDX]);
                    _SWAP_LONG(&result_buf[j][SCAN_FREQ_IDX], &result_buf[j+1][SCAN_FREQ_IDX]);
                    _SWAP_LONG(&result_buf[j][SCAN_SIGNAL_IDX], &result_buf[j+1][SCAN_SIGNAL_IDX]);
                    _SWAP_LONG(&result_buf[j][SCAN_FLGA_IDX], &result_buf[j+1][SCAN_FLGA_IDX]);
                    _SWAP_LONG(&result_buf[j][SCAN_SSID_IDX], &result_buf[j+1][SCAN_SSID_IDX]);
                }
            }
        }

        for (chk_idx = 1; chk_idx < scanned_cnt; chk_idx++) {
            /* SSID */
            if (result_buf[chk_idx][SCAN_SSID_IDX][0] == HIDDEN_SSID_DETECTION_CHAR) {    /* Hidden SSID */
                strncpy(pktPtr->scanned_ap_info[ap_idx].ssid, "[Hidden]", 128);
            } else {
                strncpy(pktPtr->scanned_ap_info[ap_idx].ssid, result_buf[chk_idx][SCAN_SSID_IDX], 128);
            }

            /* Security Mode */
            if (   (strstr(result_buf[chk_idx][SCAN_FLGA_IDX], "WPA") == NULL)
                && (strstr(result_buf[chk_idx][SCAN_FLGA_IDX], "WEP") == NULL)) {
                pktPtr->scanned_ap_info[ap_idx].auth_mode = AP_OPEN_MODE;
            } else {
                pktPtr->scanned_ap_info[ap_idx].auth_mode = AP_SECURE_MODE;
            }

            /* RSSI Mode */
            memset(rssi_str, 0, 4);
            strncpy(rssi_str, result_buf[chk_idx][SCAN_SIGNAL_IDX], 3);
            pktPtr->scanned_ap_info[ap_idx].rssi = atoi(rssi_str);

            /* Check & delete duplicatied ssid */
            if (ap_idx != 0) {
                for (i = 0; i < (int)ap_idx ; i++) {
                    if (   !strcmp(pktPtr->scanned_ap_info[i].ssid, pktPtr->scanned_ap_info[ap_idx].ssid)
                        && (pktPtr->scanned_ap_info[i].auth_mode == pktPtr->scanned_ap_info[ap_idx].auth_mode)) {
                        memset(pktPtr->scanned_ap_info[ap_idx].ssid, 0, 128);
                        pktPtr->scanned_ap_info[ap_idx].auth_mode = 0;
                        ap_idx--;
                        break;
                    }
                }
            }

            ap_idx++;
        }

        pktPtr->ssid_cnt = ap_idx;
    }

    APP_FREE(saved_scan_result);
}


/*
 * SCAN List Result in use_buffer_ptr
 */
void get_scan_result(void *user_buf_ptr)
{
    scan_result_t *scan_result_buf = (scan_result_t *)user_buf_ptr;

    while (1) {
        if (is_supplicant_done()) {
            break;
        } else {
            vTaskDelay(10);
        }
    }

    if (scan_result_buf == NULL) {
        PRINTF("[%s] Request buffer is NULL ...\n", __func__);
        return;
    }

    /* Start SCAN */
    start_scan();

    /* Make scan result into user_alloc buffer */
    make_scan_list(scan_result_buf);
}


///////////////////////////////////////////////////////////////////////////////

//
//// Register Notify callback function for Wi-Fi connection /////////////////////
//

/*
 * Register Customer call-back functions
 */
#if defined ( __SUPPORT_WIFI_CONN_CB__ )
#include "event_groups.h"

EventGroupHandle_t	evt_grp_wifi_conn_notify = NULL;

/* Station mode */
short wifi_conn_fail_reason    = 0;
short wifi_disconn_reason      = 0;

/* AP mode */
short ap_wifi_conn_fail_reason = 0;
short ap_wifi_disconn_reason   = 0;

//
// void (*wifi_conn_notify_cb)(void)
//
static void wifi_conn_cb(void)
{
    if (evt_grp_wifi_conn_notify != NULL) {
        if (get_run_mode() == SYSMODE_AP_ONLY) {
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_CONN_SUCC_SOFTAP);
        } else {
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_CONN_SUCC_STA);
#if defined ( __BLE_PROVISIONING_SAMPLE__ )
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_CONN_SUCC_STA_4_BLE);
#endif // __BLE_PROVISIONING_SAMPLE__
        }
    }
}

//
// void (*wifi_conn_fail_notify_cb)(int reason_code)
//
// reason_code :
//     WLAN_REASON_PEERKEY_MISMATCH    : Wrong password
//
static void wifi_conn_fail_cb(short reason_code)
{
    if (evt_grp_wifi_conn_notify != NULL) {
        if (get_run_mode() == SYSMODE_AP_ONLY) {
            ap_wifi_conn_fail_reason = reason_code;
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_CONN_FAIL_SOFTAP);
        } else {
            wifi_conn_fail_reason = reason_code;
#if defined ( __SUPPORT_BLE_PROVISIONING__ )
            xEventGroupSetBits(evt_grp_wifi_conn_notify, (WIFI_CONN_FAIL_STA | WIFI_CONN_FAIL_STA_4_BLE) );
#else
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_CONN_FAIL_STA);
#endif // __SUPPORT_BLE_PROVISIONING__
        }
    }
}

//
// void (*wifi_disconn_notify_cb)(int reason_code)
//
static void wifi_disconn_cb(short reason_code)
{
    if (evt_grp_wifi_conn_notify != NULL) {
        if (get_run_mode() == SYSMODE_AP_ONLY) {
            ap_wifi_disconn_reason = reason_code;
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_DISCONN_SOFTAP);
        } else {
            wifi_disconn_reason = reason_code;
            xEventGroupSetBits(evt_grp_wifi_conn_notify, WIFI_DISCONN_STA);
        }
    }
}

static void ap_sta_disconnected_cb(const unsigned char mac[6])
{
    unsigned char mac_addr[6] = {0x00,};
    ip4_addr_t ip_addr = {0x00,};

    memcpy(mac_addr, mac, sizeof(mac_addr));

    if (is_dhcp_server_running()) {
        // Search IP address on MAC
        if (dhcps_search_ip_on_mac(mac_addr, &ip_addr)) {
            // Abandon tcp connection
            tcp_abandon_remote_ip(&ip_addr);
        }
    }

    return ;
}
#endif    // __SUPPORT_WIFI_CONN_CB__

void regist_wifi_notify_cb(void)
{
#if defined ( __SUPPORT_WIFI_CONN_CB__ )
    // Create sync-up event
    evt_grp_wifi_conn_notify = xEventGroupCreate();
    if (evt_grp_wifi_conn_notify == NULL) {
        PRINTF("\n\n>>> Failed to create Wi-Fi connection notify-cb event !!!\n\n");
        return;
    }

    /* Wi-Fi connection call-back */
    wifi_conn_notify_cb_regist(wifi_conn_cb);

    /* Wi-Fi connection call-back */
    wifi_conn_fail_notify_cb_regist(wifi_conn_fail_cb);

    /* Wi-Fi disconnection call-back */
    wifi_disconn_notify_cb_regist(wifi_disconn_cb);

    /* AP-STA-DISCONNECTED call-back */
    ap_sta_disconnected_notify_cb_regist(ap_sta_disconnected_cb);
#endif    // __SUPPORT_WIFI_CONN_CB__
}

void wifi_conn_fail_noti_to_atcmd_host(void)
{
#if defined ( __SUPPORT_ATCMD__  )
    extern void atcmd_wf_jap_dap_print_with_cause(int is_jap);

    atcmd_wf_jap_dap_print_with_cause(1); // connect trial failure
#if defined (__SUPPORT_MQTT__)
    extern int atcmd_wfdap_condition_resolved;

    atcmd_wfdap_condition_resolved = FALSE;
#endif // __SUPPORT_MQTT__
#endif // __SUPPORT_ATCMD__
}

#if !defined ( __BLE_COMBO_REF__ )
void combo_ble_sw_reset(void) {    return;}
#endif // !__BLE_COMBO_REF__

///////////////////////////////////////////////////////////////////////////////
//
//// For supplicant APIs //////////////////////////////////////////////////
//
#include "user_dpm.h"
#include "user_dpm_api.h"

#define BSSID_LEN        6

extern dpm_supp_conn_info_t *dpm_supp_conn_info;
extern UCHAR    cur_connected_bssid[BSSID_LEN];

UINT get_connected_bssid(unsigned char *bssid)
{
    /* Check buffer allocation status. */
    if (bssid == NULL) {
        PRINTF("\nBuffer pointer is NULL...\n");
        return DA_APP_PTR_ERROR;
    }

    /* Check Wi-Fi connection status. */
    if (is_supplicant_done() == 0) {
        PRINTF("\nNo Wi-Fi connection...\n");
        return DA_APP_NOT_SUPPORTED;
    }

    /* Get connected BSSID */
    if (dpm_mode_is_wakeup() == 1) {
        memcpy(bssid, dpm_supp_conn_info->bssid, BSSID_LEN);
    } else {
        memcpy(bssid, cur_connected_bssid, BSSID_LEN);
    }

    return DA_APP_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////

//
//// rtm user pool api ////////////////////////////////////////////////////////
//

extern unsigned char    dpm_dbg_cmd_flag;
extern dpm_user_rtm_pool *user_rtm_pool;
extern unsigned int get_dpm_dbg_level(void);

unsigned int user_rtm_pool_allocate(char *name,
                                   void **memory_ptr,
                                   unsigned long memory_size,
                                   unsigned long wait_option)
{
    DA16X_UNUSED_ARG(wait_option);

    dpm_user_rtm *user_memory = NULL;

    // to check parameters
    if (   (name == NULL)
        || (strlen(name) >= REG_NAME_DPM_MAX_LEN)
        || (memory_size == 0)) {
        return ER_INVALID_PARAMETERS;
    }

    // to check duplicated name
    if (dpm_user_rtm_search(name) != NULL) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= 1 /*MSG_ERROR*/) {
            PRINTF("[%s] Already registered user rtm(%s)\n", __func__, name);
        }

        return ER_DUPLICATED_ENTRY;
    }

    // to allocate memory
    user_memory = (dpm_user_rtm *)da16x_dpm_rtm_mem_calloc((memory_size + sizeof(dpm_user_rtm)), sizeof(unsigned char));
    if (user_memory == NULL) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= 1 /*MSG_ERROR*/) {
            PRINTF("[%s] Failed to allocate memory(size:%d)\n", __func__, memory_size);
        }

        return ER_NO_MEMORY;
    }

    // update lfree. It has to be called after allocate & free
    user_rtm_pool->free_ptr = da16x_dpm_rtm_mem_get_free_ptr();

    // to set user rtm
    strcpy(user_memory->name, name);
    user_memory->size = memory_size;
    user_memory->start_user_addr = (UCHAR *)(user_memory) + sizeof(dpm_user_rtm);
    user_memory->next_user_addr = NULL;

    // to add user rtm into chain
    dpm_user_rtm_add(user_memory);

    // to set user memory point
    *memory_ptr = user_memory->start_user_addr;

    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= 5 /* MSG_EXCESSIVE*/) {
        PRINTF("[%s] Allocated memory information\n", __func__);
        PRINTF("\tname:%s\n"
            "\tsize:%ld\n"
            "\tstart address:%p\n"
            "\tuser address:%p\n",
            user_memory->name,
            user_memory->size,
            user_memory,
            user_memory->start_user_addr);
    }

    return ER_SUCCESS;
}


unsigned int user_rtm_release(char *name)
{
    dpm_user_rtm *cur_mem = NULL;

    if ((name == NULL) || (strlen(name) >= REG_NAME_DPM_MAX_LEN)) {
        return  ER_INVALID_PARAMETERS;
    }

    cur_mem = dpm_user_rtm_remove(name);
    if (cur_mem != NULL) {
        da16x_dpm_rtm_mem_free(cur_mem);

        //update lfree. It has to be called after allocate & free
        user_rtm_pool->free_ptr = da16x_dpm_rtm_mem_get_free_ptr();

        return 0;
    }

    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= 1 /*MSG_ERROR*/) {
        PRINTF("[%s] Not found user data(%s)\n", __func__, name);
    }

    return ER_NOT_FOUND;
}


unsigned int user_rtm_get(char *name, unsigned char **data)
{
    dpm_user_rtm *cur = NULL;

    if ((name == NULL) || (strlen(name) >= REG_NAME_DPM_MAX_LEN)) {
        return  0;
    }

    cur = dpm_user_rtm_search(name);
    if (cur != NULL) {
        *data = cur->start_user_addr;
        return cur->size;
    }

    return 0;
}

int user_rtm_pool_init_chk(void)
{
    return dpm_user_rtm_pool_init_chk();
}

/////////////////////////////////////////////////////////////////////////////////////////

//
//// for ble combo common func //////////////////////////////////////////////////////////
//

void (*combo_usr_todo_before_sleep)(void) = NULL;
void combo_set_usr_todo_before_sleep(void (*user_func)(void))
{
    combo_usr_todo_before_sleep = user_func;
}

void combo_make_sleep_rdy(void)
{
#if !defined ( __BLE_COMBO_REF__ ) || defined (__ENABLE_SAMPLE_APP__)
    return;
#elif defined ( __BLE_COMBO_REF__ ) && !defined (__ENABLE_SAMPLE_APP__)

  #if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__) && defined (__APP_SLEEP2_MONITOR__)
        extern int8_t sleep2_monitor_is_running(void);
        extern void sleep2_monitor_set_sleep2_rdy(void);

        if (sleep2_monitor_is_running() == TRUE) {
            sleep2_monitor_set_sleep2_rdy();
        }

  #elif defined (__ENABLE_DPM_FOR_GTL_BLE_APP__) && !defined (__APP_SLEEP2_MONITOR__)
        extern void app_todo_before_sleep(void);

        app_todo_before_sleep();
  #endif    // __ENABLE_DPM_FOR_GTL_BLE_APP__ && !__APP_SLEEP2_MONITOR__

    if (combo_usr_todo_before_sleep) {
        combo_usr_todo_before_sleep();
    }
#endif // !defined ( __BLE_COMBO_REF__ ) || defined (__ENABLE_SAMPLE_APP__)
}

///////////////////////////////////////////////////////////////////////////////

int wifi_chk_prov_state(int* is_prof_exist, int* is_prof_disabled)
{
    char *result_str = (char *)pvPortMalloc(32);

    if (result_str == NULL) {
        PRINTF("[%s] memory alloc error! \n", __func__);
        return -1;
    }

    memset(result_str, 0x00, 32);
    da16x_cli_reply("get_network 0 ssid", NULL, result_str);

    if (strcmp(result_str, "FAIL") == 0) {
        *is_prof_exist = pdFALSE;
        *is_prof_disabled = pdTRUE;
    } else {
        *is_prof_exist = pdTRUE;

        memset(result_str, 0x00, 32);
        da16x_cli_reply("get_network 0 disabled", NULL, result_str);

        *is_prof_disabled = (strcmp(result_str, "0") == 0)? pdFALSE : pdTRUE;
    }

    vPortFree(result_str);

    return 0;

}

extern unsigned char cmd_combo_is_sleep2_triggered;
extern unsigned long long cmd_combo_sleep2_dur;
extern void force_dpm_abnormal_sleep_by_wifi_conn_fail(void);

void let_dpm_daemon_trigger_sleep2(char* context, int seconds)
{
    cmd_combo_sleep2_dur = (unsigned long long)seconds;
    cmd_combo_is_sleep2_triggered = pdTRUE;

    if (context == NULL) {
        ;
    }
#if defined (__SUPPORT_ATCMD__)
    else if (context != NULL && strcmp(context, "at") == 0) {
        PRINTF_ATCMD("\r\nOK\r\n");
    }
#endif // __SUPPORT_ATCMD__

    force_dpm_abnormal_sleep_by_wifi_conn_fail();
}

/*
    Wait sntp sync
     0: sync ok
    -1: sync not ok until the wait_sec
    -2: sntp not enabled
*/
int sntp_wait_sync(int wait_sec)
{
    extern unsigned char sntp_support_flag;
    int ret = 0;

    if (sntp_support_flag == pdTRUE) {
        int i = 0;
        while (1) {    /* waiting for SNTP Sync */
            if (get_sntp_use() == FALSE) {
                PRINTF(">>> Not using SNTP \n");
                ret = -2;
                break;
            }

            if (is_sntp_sync() == TRUE) {
                ret = 0;
                break;
            }

            if ((++i % wait_sec*10) == 0) {
                PRINTF(RED_COLOR ">>> SNTP not synced within %d sec \n" CLEAR_COLOR,
                    wait_sec);
                ret = -1;
                break;
            }

            vTaskDelay(10);
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//// For Factory-Reset APIs //////////////////////////////////////////////////
//
#include "nvedit.h"
#include "environ.h"


void factory_reset_sta_mode(void)
{
    int status;

    PRINTF("Set STA Mode ...\n");

    status = da16x_clearenv(ENVIRON_DEVICE, "app"); /* Factory reset */
    if (status == FALSE) {
        recovery_NVRAM();
    }

    vTaskDelay(10);
}

/* For Customer's configuration */
softap_config_t _ap_config_param = { 0, };
softap_config_t *ap_config_param = (softap_config_t *) &_ap_config_param;

extern void set_customer_softap_config(void);

void factory_reset_ap_mode(void)
{
    char default_ssid[MAX_SSID_LEN + 3];
    char default_psk[MAX_PASSKEY_LEN + 3];
    int status;

    PRINTF("Set Soft-AP Mode ...\n");

    status = da16x_clearenv(ENVIRON_DEVICE, "app"); /* Factory reset */
    if (status == FALSE) {
        recovery_NVRAM();
    }

    vTaskDelay(10);

    /* Default value set-up */
    write_tmp_nvram_int((const char *)NVR_KEY_PROFILE_1, (int)MODE_ENABLE);
    PUTC('.'); vTaskDelay(1);
    write_tmp_nvram_int((const char *)"N1_mode", (int)2);
    PUTC('.'); vTaskDelay(1);
    write_tmp_nvram_int((const char *)NVR_KEY_SYSMODE, (int)DFLT_SYSMODE);
    PUTC('.'); vTaskDelay(1);

    /* Set customer's Soft-AP configuration */
    set_customer_softap_config();

    // Customer configuration : ~/system/src/customer/system_start.c
    if (ap_config_param->customer_cfg_flag == MODE_ENABLE) {
        char    tmp_buf[128] = { 0, };

        /* Prefix + Mac Address */
        if (strlen(ap_config_param->ssid_name) > 0) {
            strncpy(tmp_buf, ap_config_param->ssid_name, MAX_SSID_LEN);
        } else {
            sprintf(tmp_buf, "%s", CHIPSET_NAME);
        }

        memset(default_ssid, 0, MAX_SSID_LEN + 3);

        if (gen_ssid(tmp_buf, WLAN1_IFACE, 1, default_ssid, sizeof(default_ssid)) == -1) {
            PRINTF("SSID Error\n");
        }

        /* PSK */
        memset(default_psk, 0, MAX_PASSKEY_LEN + 3);

        if (strlen(ap_config_param->psk) > 0) {
            sprintf(default_psk, "\"%s\"", ap_config_param->psk);

            /* Auth Type */
            if (ap_config_param->auth_type == AP_SECURITY_MODE) {
                write_tmp_nvram_string(NVR_KEY_PROTO_1, proto_RSN);
                write_nvram_string(NVR_KEY_AUTH_TYPE_1, MODE_AUTH_WPA_PSK_STR); /* WPA-PSK */
            }
        } else {
            gen_default_psk(default_psk);
        }

        write_tmp_nvram_string((const char *)NVR_KEY_ENCKEY_1, default_psk);

        write_tmp_nvram_string((const char *)NVR_KEY_SSID_1, default_ssid);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_int((const char *)NVR_KEY_NETMODE_1, (int)DFLT_NETMODE_1);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_CHANNEL, (int)DFLT_AP_CHANNEL);
        PUTC('.'); vTaskDelay(1);

        if (strlen(ap_config_param->country_code) > 0) {
            write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, ap_config_param->country_code);
        } else {
            write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, (const char *)DFLT_AP_COUNTRY_CODE);
        }

        PUTC('.'); vTaskDelay(1);

        if (ap_config_param->customer_ip_address == IPADDR_CUSTOMER) {
            write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, ap_config_param->ip_addr);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, ap_config_param->subnet_mask);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, ap_config_param->default_gw);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, ap_config_param->dns_ip_addr);
            PUTC('.'); vTaskDelay(1);
        } else {
            write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, (const char *)DFLT_AP_IP);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, (const char *)DFLT_AP_SUBNET);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, (const char *)DFLT_AP_GW);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, (const char *)DFLT_AP_DNS);
            PUTC('.'); vTaskDelay(1);
        }

        if (ap_config_param->customer_dhcpd_flag == DHCPD_CUSTOMER) {
            write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, MODE_ENABLE);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, ap_config_param->dhcpd_lease_time);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, ap_config_param->dhcpd_start_ip);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, ap_config_param->dhcpd_end_ip);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP,
                                   ap_config_param->dhcpd_dns_ip_addr);
            PUTC('.'); vTaskDelay(1);
        } else {
            write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, (int)DFLT_AP_DHCP_S);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, (int)DFLT_DHCP_S_LEASE_TIME);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, (const char *)DFLT_DHCP_S_S_IP);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, (const char *)DFLT_DHCP_S_E_IP);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP, (const char *)DFLT_DHCP_S_DNS_IP);
            PUTC('.'); vTaskDelay(1);
        }
    } else {    // Default configuration
        /* SSID+MACADDRESS */
        memset(default_ssid, 0, MAX_SSID_LEN + 3);

        if (gen_ssid(CHIPSET_NAME, WLAN1_IFACE, 1, default_ssid, sizeof(default_ssid)) == -1) {
            PRINTF("SSID Error\n");
        }

        write_tmp_nvram_string((const char *)NVR_KEY_SSID_1, default_ssid);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_NETMODE_1, (int)DFLT_NETMODE_1);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_CHANNEL, (int)DFLT_AP_CHANNEL);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, (const char *)DFLT_AP_COUNTRY_CODE);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, (const char *)DFLT_AP_IP);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, (const char *)DFLT_AP_SUBNET);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, (const char *)DFLT_AP_GW);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, (const char *)DFLT_AP_DNS);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, (int)DFLT_AP_DHCP_S);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, (int)DFLT_DHCP_S_LEASE_TIME);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, (const char *)DFLT_DHCP_S_S_IP);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, (const char *)DFLT_DHCP_S_E_IP);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP, (const char *)DFLT_DHCP_S_DNS_IP);
        PUTC('.'); vTaskDelay(1);
    }

    if (save_tmp_nvram()) {
        PRINTF("\nOK\n");
    } else {
        PRINTF("\nError\n");
    }
}

int is_in_softap_acs_mode(void)
{
    int tmp_res, tmp_freq;
    int res = pdFALSE;

    if (get_run_mode() != SYSMODE_AP_ONLY) {
        return pdFALSE;
    }

    tmp_res = read_nvram_int(NVR_KEY_CHANNEL, &tmp_freq);

    if (tmp_res == -1) {
        // the key not existing .. softap is not fully set up yet
        return pdFALSE;
    }

    if (tmp_freq == 0) {
        res = pdTRUE;
    }

    return res;
}

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
#if defined ( __SUPPORT_FACTORY_RST_CONCURR_MODE__ )
void factory_reset_concurrent_mode(void)
{
#ifdef __SUPPORT_REMOVE_MAC_NAME__
    char default_ssid[MAX_SSID_LEN + 3];
    char default_psk[MAX_PASSKEY_LEN + 3];
    int status;

    PRINTF("Set Concurrentr-Mode (STA + Soft-AP) ...\n");

    status = da16x_clearenv(ENVIRON_DEVICE, "app"); /* Factory reset */
    if (status == FALSE) {
        recovery_NVRAM();
    }

    vTaskDelay(10);

    /* Default value set-up */
    write_tmp_nvram_int((const char *)NVR_KEY_PROFILE_1, (int)MODE_ENABLE);
    PUTC('.'); vTaskDelay(1);

    write_tmp_nvram_int((const char *)"N1_mode", (int)2);
    PUTC('.'); vTaskDelay(1);

    write_tmp_nvram_int((const char *)NVR_KEY_SYSMODE, (int)SYSMODE_STA_N_AP);    // STA + Soft-AP
    PUTC('.'); vTaskDelay(1);

    write_nvram_string(NVR_KEY_AUTH_TYPE_1, MODE_AUTH_WPA_PSK_STR); /* WPA-PSK */
    PUTC('.'); vTaskDelay(1);


    memset(default_ssid, 0, MAX_SSID_LEN + 3);
    memset(default_psk, 0, MAX_PASSKEY_LEN + 3);


    PRINTF("Set APP SSID PW for concurrent\n");
    sprintf(default_ssid, "\"%s\"", PREDEFINE_SSID);
    sprintf(default_psk, "\"%s\"", PREDEFINE_PW);


    write_tmp_nvram_string((const char *)NVR_KEY_ENCKEY_1, default_psk);

    write_tmp_nvram_string((const char *)NVR_KEY_SSID_1, default_ssid);
    PUTC('.'); vTaskDelay(1);

    write_tmp_nvram_int((const char *)NVR_KEY_NETMODE_1, (int)DFLT_NETMODE_1);
    PUTC('.'); vTaskDelay(1);
    write_tmp_nvram_int((const char *)NVR_KEY_CHANNEL, (int)DFLT_AP_CHANNEL);
    PUTC('.'); vTaskDelay(1);

    write_tmp_nvram_int((const char *)NVR_KEY_PROACTKEY_C, 0);
        PUTC('.'); vTaskDelay(1);


    if (strlen(ap_config_param->country_code) > 0) {
        write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, ap_config_param->country_code);
    } else {
        write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, (const char *)DFLT_AP_COUNTRY_CODE);
    }

    PUTC('.'); vTaskDelay(1);

    if (ap_config_param->customer_ip_address == IPADDR_CUSTOMER) {
        write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, ap_config_param->ip_addr);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, ap_config_param->subnet_mask);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, ap_config_param->default_gw);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, ap_config_param->dns_ip_addr);
        PUTC('.'); vTaskDelay(1);
    } else {
        write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, (const char *)DFLT_AP_IP);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, (const char *)DFLT_AP_SUBNET);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, (const char *)DFLT_AP_GW);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, (const char *)DFLT_AP_DNS);
        PUTC('.'); vTaskDelay(1);
    }

    if (ap_config_param->customer_dhcpd_flag == DHCPD_CUSTOMER) {
        write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, MODE_ENABLE);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, ap_config_param->dhcpd_lease_time);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, ap_config_param->dhcpd_start_ip);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, ap_config_param->dhcpd_end_ip);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP, ap_config_param->dhcpd_dns_ip_addr);
        PUTC('.'); vTaskDelay(1);
    } else {
        write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, (int)DFLT_AP_DHCP_S);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, (int)DFLT_DHCP_S_LEASE_TIME);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, (const char *)DFLT_DHCP_S_S_IP);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, (const char *)DFLT_DHCP_S_E_IP);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP, (const char *)DFLT_DHCP_S_DNS_IP);
        PUTC('.'); vTaskDelay(1);
    }


    if (save_tmp_nvram()) {
        PRINTF("\nOK\n");
    } else {
        PRINTF("\nError\n");
    }
#else
    char default_ssid[MAX_SSID_LEN + 3];
    char default_psk[MAX_PASSKEY_LEN + 3];
    int status;

    PRINTF("Set Concurrentr-Mode (STA + Soft-AP) ...\n");

    status = da16x_clearenv(ENVIRON_DEVICE, "app"); /* Factory reset */
    if (status == FALSE) {
        recovery_NVRAM();
    }

    vTaskDelay(10);

    /* Default value set-up */
    write_tmp_nvram_int((const char *)NVR_KEY_PROFILE_1, (int)MODE_ENABLE);
    PUTC('.'); vTaskDelay(1);

    write_tmp_nvram_int((const char *)"N1_mode", (int)2);
    PUTC('.'); vTaskDelay(1);

    write_tmp_nvram_int((const char *)NVR_KEY_SYSMODE, (int)SYSMODE_STA_N_AP);    // STA + Soft-AP
    PUTC('.'); vTaskDelay(1);

    /* Set customer's Soft-AP configuration */
    set_customer_softap_config();

    // Customer configuration : ~/system/src/customer/system_start.c
    if (ap_config_param->customer_cfg_flag == MODE_ENABLE) {
        char    tmp_buf[128] = { 0, };

        /* Prefix + Mac Address */
        if (strlen(ap_config_param->ssid_name) > 0) {
            strncpy(tmp_buf, ap_config_param->ssid_name, MAX_SSID_LEN);
        } else {
            sprintf(tmp_buf, "%s", CHIPSET_NAME);
        }

        memset(default_ssid, 0, MAX_SSID_LEN + 3);

        if (gen_ssid(tmp_buf, WLAN1_IFACE, 1, default_ssid, sizeof(default_ssid)) == -1) {
            PRINTF("SSID Error\n");
        }

        /* PSK */
        memset(default_psk, 0, MAX_PASSKEY_LEN + 3);

        if (strlen(ap_config_param->psk) > 0) {
            sprintf(default_psk, "\"%s\"", ap_config_param->psk);

            /* Auth Type */
            if (ap_config_param->auth_type == AP_SECURITY_MODE) {
                write_tmp_nvram_string(NVR_KEY_PROTO_1, proto_RSN);
                write_nvram_string(NVR_KEY_AUTH_TYPE_1, MODE_AUTH_WPA_PSK_STR); /* WPA-PSK */
            }
        } else {
            gen_default_psk(default_psk);
        }

        write_tmp_nvram_string((const char *)NVR_KEY_ENCKEY_1, default_psk);

        write_tmp_nvram_string((const char *)NVR_KEY_SSID_1, default_ssid);
        PUTC('.'); vTaskDelay(1);

        write_tmp_nvram_int((const char *)NVR_KEY_NETMODE_1, (int)DFLT_NETMODE_1);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_CHANNEL, (int)DFLT_AP_CHANNEL);
        PUTC('.'); vTaskDelay(1);

        if (strlen(ap_config_param->country_code) > 0) {
            write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, ap_config_param->country_code);
        } else {
            write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, (const char *)DFLT_AP_COUNTRY_CODE);
        }

        PUTC('.'); vTaskDelay(1);

        if (ap_config_param->customer_ip_address == IPADDR_CUSTOMER) {
            write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, ap_config_param->ip_addr);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, ap_config_param->subnet_mask);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, ap_config_param->default_gw);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, ap_config_param->dns_ip_addr);
            PUTC('.'); vTaskDelay(1);
        } else {
            write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, (const char *)DFLT_AP_IP);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, (const char *)DFLT_AP_SUBNET);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, (const char *)DFLT_AP_GW);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, (const char *)DFLT_AP_DNS);
            PUTC('.'); vTaskDelay(1);
        }

        if (ap_config_param->customer_dhcpd_flag == DHCPD_CUSTOMER) {
            write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, MODE_ENABLE);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, ap_config_param->dhcpd_lease_time);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, ap_config_param->dhcpd_start_ip);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, ap_config_param->dhcpd_end_ip);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP,
                                   ap_config_param->dhcpd_dns_ip_addr);
            PUTC('.'); vTaskDelay(1);
        } else {
            write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, (int)DFLT_AP_DHCP_S);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, (int)DFLT_DHCP_S_LEASE_TIME);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, (const char *)DFLT_DHCP_S_S_IP);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, (const char *)DFLT_DHCP_S_E_IP);
            PUTC('.'); vTaskDelay(1);

            write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP, (const char *)DFLT_DHCP_S_DNS_IP);
            PUTC('.'); vTaskDelay(1);
        }
    } else {    // Default configuration
        /* SSID+MACADDRESS */
        memset(default_ssid, 0, MAX_SSID_LEN + 3);

        if (gen_ssid(CHIPSET_NAME, WLAN1_IFACE, 1, default_ssid, sizeof(default_ssid)) == -1) {
            PRINTF("SSID Error\n");
        }

        write_tmp_nvram_string((const char *)NVR_KEY_SSID_1, default_ssid);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_NETMODE_1, (int)DFLT_NETMODE_1);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_CHANNEL, (int)DFLT_AP_CHANNEL);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_COUNTRY_CODE, (const char *)DFLT_AP_COUNTRY_CODE);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_IPADDR_1, (const char *)DFLT_AP_IP);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_NETMASK_1, (const char *)DFLT_AP_SUBNET);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_GATEWAY_1, (const char *)DFLT_AP_GW);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DNSSVR_1, (const char *)DFLT_AP_DNS);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_DHCPD, (int)DFLT_AP_DHCP_S);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_int((const char *)NVR_KEY_DHCP_TIME, (int)DFLT_DHCP_S_LEASE_TIME);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_S_IP, (const char *)DFLT_DHCP_S_S_IP);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_E_IP, (const char *)DFLT_DHCP_S_E_IP);
        PUTC('.'); vTaskDelay(1);
        write_tmp_nvram_string((const char *)NVR_KEY_DHCP_DNS_IP, (const char *)DFLT_DHCP_S_DNS_IP);
        PUTC('.'); vTaskDelay(1);
    }

    if (save_tmp_nvram()) {
        PRINTF("\nOK\n");
    } else {
        PRINTF("\nError\n");
    }
#endif // __SUPPORT_REMOVE_MAC_NAME__
}
#endif // __SUPPORT_FACTORY_RST_CONCURR_MODE__

int put_switch_sysmode_4_concurrent_mode(int switch_sysmode)
{
    // Check validity
    if ((switch_sysmode < SYSMODE_STA_ONLY) || (switch_sysmode > SYSMODE_STA_N_AP)) {
        PRINTF("- No proper switch_sysmode (%d)\n", switch_sysmode);
        return pdFALSE;
    }

    // Save "SWITCH_SYSMODE" to NVRAM which will be used as return sysmode
    write_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int)switch_sysmode);

    return pdTRUE;
}

int factory_reset_btn_onetouch(void)
{
    int cur_run_mode;
    int prev_run_mode;

    cur_run_mode = get_run_mode();

    // Change system running mode between current mode <-> concurrent-mode
    switch (cur_run_mode) {
    case SYSMODE_STA_ONLY :
        // Check if Soft-AP profile exists or not.
        if (read_nvram_int((const char *)NVR_KEY_PROFILE_1, (int *)&prev_run_mode) != 0) {
            PRINTF("\n!!! STA_ONLY mode : Does not change to concurrent-mode (STA+Soft-AP) !!!\n\n");
            return pdFALSE;
        }

        // Change to concurrent-mode : STA + Soft-AP
        write_tmp_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int)SYSMODE_STA_ONLY);
        write_tmp_nvram_int((const char *)NVR_KEY_SYSMODE, (int)SYSMODE_STA_N_AP);
        break;

    case SYSMODE_AP_ONLY  :
        // Check if STA profile exists or not.
        if (read_nvram_int((const char *)NVR_KEY_PROFILE_0, (int *)&prev_run_mode) != 0) {
            PRINTF("\n!!! SYSMODE_AP_ONLY mode : Does not change to concurrent-mode (STA+Soft-AP) !!!\n\n");
            return pdFALSE;
        }

        // Change to concurrent-mode : STA + Soft-AP
        write_tmp_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int)SYSMODE_AP_ONLY);
        write_tmp_nvram_int((const char *)NVR_KEY_SYSMODE, (int)SYSMODE_STA_N_AP);
        break;

    case SYSMODE_STA_N_AP :
        // Read saved previous sys_run_mode in NVRAM
        if (read_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int *)&prev_run_mode) != 0) {
            PRINTF("\n!!! Not defined yet to switch sys_run_mode in NVRAM !!!\n\n");
            return pdFALSE;
        }

        // Change to concurrent-mode : Previous running mode
        write_tmp_nvram_int((const char *)NVR_KEY_SYSMODE, prev_run_mode);
        write_tmp_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int)SYSMODE_STA_N_AP);
        break;

    default :
        PRINTF("!!! Unknown sys_run_mode (%d)\n\n", cur_run_mode);
        return pdFALSE;
    }

    save_tmp_nvram();

    return pdTRUE;
}
#endif    /* __SUPPORT_WIFI_CONCURRENT__ */

int is_netif_down_ever_happpened(void)
{
    extern int netif_down_ever_happened;
    return netif_down_ever_happened;
}

/* EOF */
