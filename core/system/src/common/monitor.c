/**
 ****************************************************************************************
 *
 * @file monitor.c
 *
 * @brief Console monitor module
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

#include "da16x_system.h"
#include "da16x_types.h"
#include "console.h"
#include "monitor.h"
#include "command.h"

#include "sys_feature.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

//----------------------------------------------------------
// Features
//----------------------------------------------------------

#undef    DA16X_CON_SAFETY_SUPPORT


//----------------------------------------------------------

static char *mon_argv[MAX_ARGC_NUM];
static int   mon_argc;
static char  hist_buf[MAX_HIS_NUM][MAX_CMD_LEN + 1];
static int   hist_cnt = -1;
static char  mon_buf[MAX_CMD_LEN + 1];

#ifdef    DA16X_CON_SAFETY_SUPPORT
DATAALIGN static OAL_SEMAPHORE    _mutex_cmd_instance;
#endif

/* DOS_KEY */
static char pre_key1 = 0;
static char pre_key2 = 0;
static char pre_key3 = 0;
static char cur_hist_idx = -1;

static void monitor_gets(char *buf);

//----------------------------------------------------------
//
//
//----------------------------------------------------------

void   monitor_init(void)
{
    int i;

    for (i = 0; i < MAX_HIS_NUM; i++)
    {
        hist_buf[i][0] = '\0';
    }

    mon_buf[0] = '\0';
    hist_cnt = -1;

#ifdef    DA16X_CON_SAFETY_SUPPORT
    OAL_CREATE_SEMAPHORE(&_mutex_cmd_instance, "cmdmut", 1, OAL_FIFO) ;
#endif
}

static void monitor_gets(char *buf)
{
    char ch = 0 ;
    int  i = 0;

    while (1) {
        ch = GETC();

        pre_key3 = pre_key2;
        pre_key2 = pre_key1;
        pre_key1 = ch;

        if (ch == '\b') {    /* backspace */
            if (i > 0) {

                if (console_hidden_check() != pdFALSE) {
                    PUTC(' ');
                }

                if (console_hidden_check() != pdFALSE) {
                    PUTC('\b');
                }

                i--;
                PRINTF("\x1b[K"); /* esc[0K Clear line from cursor right */

            } else {
                if (console_hidden_check() != pdFALSE) {
                    PUTC(' ');
                }
            }

        } else if (ch == '\n' || ch == '\r') {
            /* DOS_KEY */
            pre_key1 = 0;
            pre_key2 = 0;
            pre_key3 = 0;
            cur_hist_idx = -1;

            buf[i++] = '\0';
            break;
        } else if (   (i > MAX_CMD_LEN)
                   && !(pre_key3 == 0x1B && pre_key2 == 0x5B && (pre_key1 == 0x41 || pre_key1 == 0x42))) {
            PUTC('\b');
            PRINTF("\x1b[0K"); /* Clear line from cursor right */
            continue;
        } else {
            /* DOS_KEY VT100 ESC sequences */
            /*  esc[  */
            if (pre_key3 == 0x1B && pre_key2 == 0x5B) {  /* Check esc[ */
                if (pre_key1 == 0x41 || pre_key1 == 0x42) {  /* Check A or B */
                    if (pre_key1 == 0x41) {   /* UP KEY : esc[A */
                        PRINTF("\x1b[B"); /* DOWN key : esc[B */
                    }

                    if (hist_cnt >= 0) {
                        int hist_idx = 0;
                        int buf_len = 0;

                        /* UP Key */
                        if (pre_key1 == 0x41) {
                            if (hist_cnt > cur_hist_idx) {
                                cur_hist_idx++;
                            } else {
                                cur_hist_idx = 0;
                            }
                        }
                        /* Down Key */
                        else if (pre_key1 == 0x42) {
                            if (0 < cur_hist_idx) {
                                cur_hist_idx--;
                            } else {
                                cur_hist_idx = (char)hist_cnt;
                            }
                        }

                        hist_idx = (MAX_HIS_NUM + hist_cnt - cur_hist_idx) % MAX_HIS_NUM;

                        if (i < MAX_CMD_LEN) {
                            buf_len = (i >= 2 ? i - 2 : 0);
                        } else {
                            buf_len = i;
                        }

                        /* Delete right before character */
                        while (buf_len--) {
                            PUTC('\b'); /* backspace */
                        }

                        strcpy(buf, hist_buf[hist_idx]);    // replace
                        PRINTF("%s", buf);
                        i = strlen(buf);

                        /* Delete next line of current cursor position */
                        PRINTF("\x1b[0K"); /* esc[0K */
                    } else {
                        //PRINTF("i=%d\x1b[D\x1b[D\x1b[D", i); // Debug

                        memset(buf, 0, i);
                        /* Delete right before character */
                        while (i > 2) {
                            i--;
                            PUTC('\b'); /* backspace */
                        }
                        i = 0;
                        /* Delete next line of current cursor position */
                        PRINTF("\x1b[0K"); /* esc[0K */
                    }

                    continue;
                } else if (pre_key1 == 0x44) {     /* LEFT key*/
                    /* Return cursor position to before one */
                    PRINTF("\x1b[C"); /* esc[C */
                    if (i < MAX_CMD_LEN) {
                        i = i - 2; /* Delete 0x1b or 0x5b in buf */
                    }

                    continue;
                } else if (pre_key1 == 0x43) {     /* RIGHT key*/
                    /* Return cursor position to before one */
                    PRINTF("\x1b[D"); /* esc[C */
                    if (i < MAX_CMD_LEN) {
                        i = i - 2; /* Delete 0x1b or 0x5b in buf */
                    }

                    continue;
                }
            }

            if (i < MAX_CMD_LEN) {
                buf[i++] = ch;
            } else {
                PUTC('\b');
                PRINTF("\x1b[0K"); /* esc[0K    Clear line from cursor right */
            }
        }
    }
}


/* white-space is ' ' ',' '\f' '\n' '\r' '\t' '\v' */
int isspace2(char c)
{
    if (c == ' ' || c == ',' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v') {
        return 1;
    } else {
        return 0;
    }
}


int make_argv(char *s, int argvsz, char *argv[])
{
    int argc = 0;

    /* split into argv */
    while (argc < argvsz - 1) {
        /* skip any white space */
        while ((*s == ' ') || (*s == '\t') || (*s == '\r') || (*s == '\n')) {
            ++s;
        }

        if (*s == '\0') {     /* end of s, no more args */
            break;
        }

        /* find end of string */
        if (*s == '"') {
            /* string parameter */
            ++s;
            argv[argc++] = s;    /* begin of argument string */

            while (*s && (*s != '"') && (*s != '\r') && (*s != '\n')) {
                ++s;
            }

            if (*s == '\0') {       /* end of s, no more args */
                break;
            }

            if (*s == '"') {
                *s = '\0';
            }

            *s++ = '\0';    /* terminate current arg */

        } else if (*s == '\'') {
            /* string parameter */
            argv[argc++] = s;    /* begin of argument string    */
            ++s;

            while (*s && (*s != '\'') && (*s != '\r') && (*s != '\n')) {
                ++s;
            }

            if (*s == '\0' || *s == '\'') {   /* end of s, no more args    */
                break;
            }
        } else {
            /* non string parameter */
            argv[argc++] = s;    /* begin of argument string */

            while (*s && (*s != ' ') && (*s != '\t') && (*s != '\r') && (*s != '\n')) {
                ++s;
            }

            if (*s == '\0') {      /* end of s, no more args */
                break;
            }

            *s++ = '\0';    /* terminate current arg */
        }
    }

    argv[argc] = NULL;

    return argc;
}


void Monitor(char *prompt)
{
    int sys_wdog_id = da16x_sys_wdog_id_get_Console_IN();
    int firstCheck = 0;
#ifdef __SUPPORT_CONSOLE_PWD__
    char    *tmpString = NULL;
#endif /* __SUPPORT_CONSOLE_PWD__ */

    /* Initialize */
    monitor_init();
    cmd_stack_initialize();

#ifdef __SUPPORT_CONSOLE_PWD__

    /* NOTICE !!! */
    /* Remove this code to disable "Console Password" feature by force. */
    if (read_nvram_int((const char *)NVR_KEY_PASSWORD_SVC, (int *)&password_svc)) {
        password_svc = DFLT_PASSWORD_SVC;
    }

    tmpString = (char *)read_nvram_string((const char *)NVR_KEY_PASSWORD);

    if (tmpString != NULL) {
        strncpy((char *)password_val, tmpString, MAX_CMD_LEN);
    } else {
        strncpy((char *)password_val, (char *)DEFAULT_PASSWORD, MAX_CMD_LEN);
    }

    if (password_svc == pdFALSE) {
        password_ok = pdTRUE;
    }

#endif /* __SUPPORT_CONSOLE_PWD__ */

    /* Execute */
    while (1) {
        /* During the DPM Wakeup, this prompt display let to hold booting */
        /* So, First prompt display should be blocking */
        if (firstCheck) {
#ifdef __SUPPORT_CONSOLE_PWD__

            if (password_ok)
#endif /* __SUPPORT_CONSOLE_PWD__ */
                print_full_path_name(prompt, mon_buf, MAX_CMD_LEN);
        }

        firstCheck = 1;

#ifdef __SUPPORT_CONSOLE_PWD__
        if (password_svc) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            vTaskDelay(5);
            if (checkPassword()) {
                continue;    /* NOK */
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }
#endif /* __SUPPORT_CONSOLE_PWD__ */

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        monitor_gets(mon_buf);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        exec_mon_cmd(mon_buf);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }
}


void exec_mon_cmd(char *ptr)
{
    UINT hist_idx = 0;
    UINT idx = 0;
    char tmp_str[CON_BUF_SIZ];

#ifdef DA16X_CON_SAFETY_SUPPORT
    OAL_OBTAIN_SEMAPHORE(&_mutex_cmd_instance, OAL_SUSPEND);
#endif /*  DA16X_CON_SAFETY_SUPPORT */

    memset(tmp_str, 0, CON_BUF_SIZ);

    for (idx = 0; ptr[idx] != '\0'; idx++) {
        /* Remove All control character (0x00~0x1F , 0x7F) */
        if ((0xFF & ptr[idx]) > 0x1F && (0xFF & ptr[idx]) != 0x7F) {
            tmp_str[hist_idx++] = ptr[idx];
        } else if (ptr[idx] == '\r' || ptr[idx] == '\n') {   /* chopping */
            ptr[idx] = '\0';
            break;
        }
    }

    memset(ptr, 0, idx);
    strcpy(ptr, tmp_str);

    /* command backup */
    if (ptr[0] != '.' && ptr[0] != '!' && ptr[0] != '\0') {
        if (hist_cnt >= MAX_HIS_NUM - 1) {
            /* Append to History if different from previous command. */
            if (strcmp(hist_buf[hist_cnt], ptr) != 0) {
                for (idx = 0; idx < MAX_HIS_NUM; idx++) {
                    strcpy(hist_buf[idx], hist_buf[idx+1]);
#ifdef DEBUG_DOS_KEY
                    if (hist_buf[idx][0] != '\0') {
                        PRINTF("[MAX] %d:%s <= hist_buf[%d]\n", idx + 1, hist_buf[idx], idx);
                    }
#endif /* DEBUG_DOS_KEY */
                }

                if (hist_cnt >= MAX_HIS_NUM-1) {
                    if (strlen(ptr) > MAX_CMD_LEN) {
                        strncpy(hist_buf[MAX_HIS_NUM - 1], ptr, MAX_CMD_LEN);
                    } else {
                        strcpy(hist_buf[MAX_HIS_NUM - 1], ptr);
                    }
#ifdef DEBUG_DOS_KEY
                    PRINTF("[MAX] %d:%s <= hist_buf[%d]\n", MAX_HIS_NUM, hist_buf[hist_cnt], hist_cnt);
#endif /* DEBUG_DOS_KEY */
                }
            }
        } else {
            /* Append to History if different from previous command. */
            if (hist_cnt == -1 || (hist_cnt >= 0 && strcmp(hist_buf[hist_cnt], ptr) != 0)) {
                if (strlen(ptr) > MAX_CMD_LEN) {
                    strncpy(hist_buf[hist_cnt + 1], ptr, MAX_CMD_LEN);
                } else {
                    strcpy(hist_buf[hist_cnt + 1], ptr);
                }
            }

#ifdef DEBUG_DOS_KEY
            for (idx = 0; idx < MAX_HIS_NUM; idx++) {
                if (hist_buf[idx][0] != '\0') {
                    PRINTF("[SPC] %d:%s <= hist_buf[%d]\n", idx + 1, hist_buf[idx], idx);
                }
            }
#endif /* DEBUG_DOS_KEY */
        }
    }

    /* history check */
    if (ptr[0] == '.') {
        /* previous command */
        hist_idx = hist_cnt;
        strcpy( ptr, hist_buf[hist_idx] );    /* replace */
        hist_idx = 0;
    }
#ifdef    __SUPPORT_CMD_HISTORY__    // Unsupported command to reduce code size
    else if (ptr[0] == '!') {
        /* history command */
        if (ptr[1] == '\0' || ptr[1] == '\r' || ptr[1] == '\n') {
            /* history view */
            PRINTF("Command history:\n");

            for (idx = 0; idx < MAX_HIS_NUM; idx++) {
                if (hist_buf[idx][0] != '\0' && hist_buf[idx][0] != '!') {
#ifdef DEBUG_DOS_KEY /* debug  */
                    PRINTF("%d:%s <= hist_buf[%d]\n", idx + 1, hist_buf[idx], idx);
#else
                    PRINTF("%d:%s\n", idx + 1, hist_buf[idx]);
#endif /* DEBUG_DOS_KEY */
                }
            }

#ifdef DA16X_CON_SAFETY_SUPPORT
            OAL_RELEASE_SEMAPHORE(&_mutex_cmd_instance);
#endif /* DA16X_CON_SAFETY_SUPPORT */
            return;
        } else {
            /* history run */
            idx = atoi(&(ptr[1]));

            if (idx <= MAX_HIS_NUM) {
                strcpy( ptr, hist_buf[idx - 1] );    /* replace */
            }
        }

        hist_idx = 0;
    }
#endif    /* __SUPPORT_CMD_HISTORY__ */
    else {
        hist_idx = 1;
    }

    /* command parsing */
    mon_argc = make_argv(ptr, MAX_ARGC_NUM, mon_argv);

    /* command running */
    if (mon_argc != 0) {
        cmd_interface_action(mon_argc, mon_argv);

        if (   (ptr[0] != '.' && ptr[0] != '!' && ptr[0] != '\0')
            && (hist_cnt == -1 || (hist_cnt >= 0 && strcmp(hist_buf[hist_cnt], ptr) != 0))) {

            if (hist_cnt+1 >= MAX_HIS_NUM-1) {
                hist_cnt = MAX_HIS_NUM-1;
            } else {
                hist_cnt = hist_cnt + 1;
            }
        }
    }

#ifdef DA16X_CON_SAFETY_SUPPORT
    OAL_RELEASE_SEMAPHORE(&_mutex_cmd_instance);
#endif /* DA16X_CON_SAFETY_SUPPORT */
}

#undef    PROMPT
#if defined ( RAM_2ND_BOOT )
  #define PROMPT    "/BOOT"
#elif defined ( XIP_CACHE_BOOT )
  #define PROMPT    "/"CHIPSET_NAME
#else
  #define PROMPT    Error...
#endif // BOOT_LOADER
void thread_console_in(void* thread_input)
{
    int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;

    DA16X_UNUSED_ARG(thread_input);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
    if (sys_wdog_id >= 0) {
        da16x_sys_wdog_id_set_Console_IN(sys_wdog_id);
    }

    Monitor(PROMPT);

    da16x_sys_watchdog_unregister(sys_wdog_id);
}

/* EOF */
