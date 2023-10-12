/**
 ****************************************************************************************
 *
 * @file user_dpm_manager.c
 *
 * @brief DPM manager for customer's applications
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
#include "user_dpm_manager.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "app_errno.h"
#include "da16x_dpm.h"
#include "da16x_sys_watchdog.h"

// Key function for DPM Manager
void    (*initDpmConfigFunction)() = NULL;

#if defined ( __SUPPORT_DPM_MANAGER__ )

#undef  FOR_DEBUGGING_PKT
#undef  FOR_DEBUGGING_PKT_DUMP
#undef  FOR_DEBUGGING_HOLD_DPM
#undef  FOR_DEBUGGING_HOLD_ABNORMAL_CHECK

#undef  SUPPORT_RTM_TCP_KEEPALIVE
#undef  DTLS_TIMER_LOG

#define CLEAR           CLEAR_COLOR

#define BLACK           BLACK_COLOR
#define RED             RED_COLOR
#define GREEN           GREEN_COLOR
#define YELLOW          YELLOW_COLOR
#define BLUE            BLUE_COLOR
#define MAGENTA         MAGENTA_COLOR
#define CYAN            CYAN_COLOR
#define WHITE           WHITE_COLOR

#define B_BLACK         ANSI_BCOLOR_BLACK
#define B_RED           ANSI_BCOLOR_RED
#define B_GREEN         ANSI_BCOLOR_GREEN
#define B_YELLOW        ANSI_BCOLOR_YELLOW
#define B_BLUE          ANSI_BCOLOR_BLUE
#define B_MEGENTA       ANSI_BCOLOR_MAGENTA
#define B_CYAN          ANSI_BCOLOR_CYAN
#define B_WHITE         ANSI_BCOLOR_WHITE

#define BL_RED          ANSI_BCOLOR_LIGHT_RED
#define BL_GREEN        ANSI_BCOLOR_LIGHT_GREEN
#define BL_YELLOW       ANSI_BCOLOR_LIGHT_YELLOW
#define BL_BLUE         ANSI_BCOLOR_LIGHT_BLUE
#define BL_MAGENTA      ANSI_BCOLOR_LIGHT_MAGENTA
#define BL_CYAN         ANSI_BCOLOR_LIGHT_CYAN
#define BL_WHITE        ANSI_BCOLOR_LIGHT_WHITE

#if defined ( FOR_DEBUGGING_PKT_DUMP )
extern void hexa_dump_print(u8 *title, const void *buf, size_t len, char direction, char output_fmt);
#endif // FOR_DEBUGGING_PKT_DUMP

// External functions
#if defined ( FOR_DEBUGGING_HOLD_DPM )
extern void hold_dpm_sleepd(void);
#endif // FOR_DEBUGGING_HOLD_DPM

#if defined (FOR_DEBUGGING_HOLD_ABNORMAL_CHECK)
extern void holdDpmAbnormalCheck(void);
#endif // FOR_DEBUGGING_HOLD_ABNORMAL_CHECK

// External variables
extern int  interface_select;        // Current interface

extern char *dns_A_Query(char *domain_name, ULONG wait_option);

static void *rtmUserParamsPtr = NULL;
static int  dpmControlManagerDoneFlag = 0;
#if !defined (__ALLOWED_MULTI_CONNECTION__)
static UINT    connSessionId;
#endif    /*__ALLOWED_MULTI_CONNECTION__*/

dpm_user_config_t *rtmDpmConfigPtr = NULL;
dpm_user_config_t dpmUserConfigs;

#if !defined (__LIGHT_DPM_MANAGER__)
static void dpmTcpSvrKaCbSubSess(UINT sessionId);
#endif    // !__LIGHT_DPM_MANAGER__

EventGroupHandle_t    dpmManagerEventGroup = NULL;


static void dpmTcpCliKaCbSess(UINT sessionNo);
static void errorCallback(UINT error_code, char *comment);
static void dpmControlManager(void *arg);
static void dpmEventManager(void *arg);
static session_config_t *getSessionInfoPtr(UINT sessionNo);

// SUPPORT_SECURE
static UINT dmSecureInit(SECURE_INFO_T *config);
static UINT dmSecureShutdown(SECURE_INFO_T *config);
static UINT dmSecureHandshake(SECURE_INFO_T *config);
static UINT dmSecureSend(SECURE_INFO_T *config, VOID *buf, size_t len, ULONG wait_option);
static int  dmSecureRecv(SECURE_INFO_T *config, void *packet_ptr, ULONG wait_option);
static int  dmSecureRecvCallback(void *ctx, unsigned char *buf, size_t len);
#if !defined (__LIGHT_DPM_MANAGER__)
static int  dmSecureSendCallback(void *ctx, const unsigned char *buf, size_t len);
#endif    // !__LIGHT_DPM_MANAGER__

void dpm_tcpc_ka_cb_sess0(char *timer_name);
void dpm_tcpc_ka_cb_sess1(char *timer_name);
void dpm_tcpc_ka_cb_sess2(char *timer_name);
void dpm_tcpc_ka_cb_sess3(char *timer_name);

#if !defined (__LIGHT_DPM_MANAGER__)
void dpm_tcps_ka_cb_sub0(char *timer_name);
void dpm_tcps_ka_cb_sub1(char *timer_name);
#endif /*(__LIGHT_DPM_MANAGER__)*/

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 2)
void dpm_tcps_ka_cb_sub2(char *timer_name);
#endif //
#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 3)
void dpm_tcps_ka_cb_sub3(char *timer_name);
#endif //

int dpm_mng_init_rng(void);
int dpm_mng_deinit_rng(void);

#if !defined (__LIGHT_DPM_MANAGER__)
void tlsRestoreSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr);
void tlsDeinitSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr);
void tlsClearSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr);
#endif /*__LIGHT_DPM_MANAGER__*/

TaskHandle_t dpmControlManagerHandle = NULL;
TaskHandle_t dpmEventManagerHandle = NULL;

#ifdef DM_MUTEX
SemaphoreHandle_t   dmMutex;
#endif

const char *dpm_mng_rng_pers = "dpm_manager_tls";

static int dpm_mng_rng_cnt = 0;
mbedtls_ctr_drbg_context *dpm_mng_ctr_drbg_ctx = NULL;
mbedtls_entropy_context  *dpm_mng_entropy_ctx = NULL;

int dpm_mng_init_rng(void)
{
    int ret = 0;

    PRINTF(" [%s:%d]dpm_mng_rng_ctx(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    dpm_mng_rng_cnt++;

    if (dpm_mng_rng_cnt == 1) {
        dpm_mng_ctr_drbg_ctx = pvPortMalloc(sizeof(mbedtls_ctr_drbg_context));
        if (!dpm_mng_ctr_drbg_ctx) {
            DM_DBG_ERR(" [%s:%d] Failed to allocate ctr drbg(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_ctr_drbg_context));
            goto cleanup;
        }

        mbedtls_ctr_drbg_init(dpm_mng_ctr_drbg_ctx);

        dpm_mng_entropy_ctx = pvPortMalloc(sizeof(mbedtls_entropy_context));
        if (!dpm_mng_entropy_ctx)
        {
            DM_DBG_ERR(" [%s:%d] Failed to allocate entropy(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_entropy_context));
            goto cleanup;
        }

        mbedtls_entropy_init(dpm_mng_entropy_ctx);

        ret = mbedtls_ctr_drbg_seed(dpm_mng_ctr_drbg_ctx,
                                    mbedtls_entropy_func,
                                    dpm_mng_entropy_ctx,
                                    (const unsigned char *)dpm_mng_rng_pers,
                                    strlen(dpm_mng_rng_pers));
        if (ret)
        {
            DM_DBG_ERR(" [%s:%d] Failed to set ctr-drbg seed(0x%x)\n",
                    __func__, __LINE__, -ret);
            goto cleanup;
        }
    }

    return 0;

cleanup:

    dpm_mng_deinit_rng();

    return -1;
}

int dpm_mng_setup_rng(mbedtls_ssl_config *ssl_conf)
{
    DM_DBG_INFO(" [%s:%d]dpm_mng_rng_ctx(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    if (dpm_mng_rng_cnt == 0) {
        DM_DBG_ERR(" [%s:%d] Not init tls rng\n");
        return -1;
    }

    if (ssl_conf == NULL) {
        DM_DBG_ERR(" [%s:%d] Invalid parameter\n");
        return -1;
    }

    mbedtls_ssl_conf_rng(ssl_conf, mbedtls_ctr_drbg_random, dpm_mng_ctr_drbg_ctx);

    DM_DBG_INFO(" [%s:%d] Done(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    return 0;
}

int dpm_mng_cookie_setup_rng(mbedtls_ssl_cookie_ctx *cookie_ctx)
{
    int ret = 0;

    DM_DBG_INFO(" [%s:%d]dpm_mng_rng_ctx(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    if (dpm_mng_rng_cnt == 0) {
        DM_DBG_ERR(" [%s:%d] Not init tls rng\n");
        return -1;
    }

    if (cookie_ctx == NULL) {
        DM_DBG_ERR(" [%s:%d] Invalid parameter\n");
        return -1;
    }

    ret = mbedtls_ssl_cookie_setup(cookie_ctx, mbedtls_ctr_drbg_random, dpm_mng_ctr_drbg_ctx);
    if (ret) {
        DM_DBG_ERR(" [%s:%d] Filed to setup cookie(0x%x)\n", __func__, __LINE__, -ret);
    }

    DM_DBG_INFO(" [%s:%d] Done(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    return ret;
}

int dpm_mng_deinit_rng(void)
{
    DM_DBG_INFO(" [%s:%d]dpm_mng_rng_ctx(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    if (dpm_mng_rng_cnt == 1) {
        if (dpm_mng_ctr_drbg_ctx) {
            mbedtls_ctr_drbg_free(dpm_mng_ctr_drbg_ctx);
            vPortFree(dpm_mng_ctr_drbg_ctx);
            dpm_mng_ctr_drbg_ctx = NULL;
        }

        if (dpm_mng_entropy_ctx) {
            mbedtls_entropy_free(dpm_mng_entropy_ctx);
            vPortFree(dpm_mng_entropy_ctx);
            dpm_mng_entropy_ctx = NULL;
        }
    } else if (dpm_mng_rng_cnt == 0) {
        return 0;
    }

    dpm_mng_rng_cnt--;

    DM_DBG_INFO(" [%s:%d] Done(%d)\n", __func__, __LINE__, dpm_mng_rng_cnt);

    return 0;
}

unsigned int getPacketSendingStatus(void *confPtr, UINT type)
{
#if defined (__LIGHT_DPM_MANAGER__)
    DA16X_UNUSED_ARG(type);
#endif    //__LIGHT_DPM_MANAGER__

#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
#endif    // !__LIGHT_DPM_MANAGER__
    session_config_t    *sessConfPtr;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER)
    {
        tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)confPtr;
        return (tcpRxSubTaskConfPtr->sendingFlag);
    }
    else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        sessConfPtr = (session_config_t *)confPtr;
        return (sessConfPtr->sendingFlag);
    }
}

void setPacketSendingStatus(void *confPtr, UINT type)
{
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
#else
    DA16X_UNUSED_ARG(type);
#endif    // !__LIGHT_DPM_MANAGER__
    session_config_t    *sessConfPtr;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER) {
        tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)confPtr;
        tcpRxSubTaskConfPtr->sendingFlag = pdTRUE;
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        sessConfPtr = (session_config_t *)confPtr;
        sessConfPtr->sendingFlag = pdTRUE;
    }
}

void clrPacketSendingStatus(void *confPtr, UINT type)
{
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
#else
    DA16X_UNUSED_ARG(type);
#endif    // !__LIGHT_DPM_MANAGER__
    session_config_t    *sessConfPtr;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER) {
        tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)confPtr;
        tcpRxSubTaskConfPtr->sendingFlag = pdFALSE;
    }
    else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        sessConfPtr = (session_config_t *)confPtr;
        sessConfPtr->sendingFlag = pdFALSE;
    }
}

unsigned int getPacketReceivingStatus(void *confPtr, UINT type)
{
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
#else
    DA16X_UNUSED_ARG(type);
#endif    // !__LIGHT_DPM_MANAGER__
    session_config_t    *sessConfPtr;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER) {
        tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)confPtr;
        return (tcpRxSubTaskConfPtr->receivingFlag);
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        sessConfPtr = (session_config_t *)confPtr;
        return (sessConfPtr->receivingFlag);
    }
}

void setPacketReceivingStatus(void *confPtr, UINT type)
{
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
#else
    DA16X_UNUSED_ARG(type);
#endif    // !__LIGHT_DPM_MANAGER__
    session_config_t    *sessConfPtr;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER) {
        tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)confPtr;
        tcpRxSubTaskConfPtr->receivingFlag = pdTRUE;
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        sessConfPtr = (session_config_t *)confPtr;
        sessConfPtr->receivingFlag = pdTRUE;
    }
}

void clrPacketReceivingStatus(void *confPtr, UINT type)
{
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
#else
    DA16X_UNUSED_ARG(type);
#endif    // !__LIGHT_DPM_MANAGER__
    session_config_t    *sessConfPtr;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER) {
        tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)confPtr;
        tcpRxSubTaskConfPtr->receivingFlag = pdFALSE;
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        sessConfPtr = (session_config_t *)confPtr;
        sessConfPtr->receivingFlag = pdFALSE;
    }
}

dpm_user_config_t *dpm_get_user_config(void)
{
    return &dpmUserConfigs;
}

dpm_user_config_t *dpm_get_rtm_user_config(void)
{
    return rtmDpmConfigPtr;
}

#if defined ( __RUNTIME_CALCULATION__ )
void printCurrTickDm(char *comment)
{
#if defined ( DM_DEBUG_INFO )
    ULONG curr_tick;

    curr_tick = xTaskGetTickCount();
    DM_DBG_INFO(YELLOW "+%s tick: %d \n" CLEAR, comment, (int)curr_tick);
#else
    DA16X_UNUSED_ARG(comment);
#endif    // DM_DEBUG_INFO
}
#endif    // __RUNTIME_CALCULATION__

void printDpmConfigRtm(void)
{
    int    i;
    timer_config_t        *timerConfPtr;
    session_config_t    *sessConfPtr;

    PRINTF(" < DPM Config RTM > \n");
    PRINTF("  bootInitCallback:0x%x \n", rtmDpmConfigPtr->bootInitCallback);
    PRINTF("  wakeupInitCallback:0x%x \n", rtmDpmConfigPtr->wakeupInitCallback);

    for (i = 0; i < MAX_TIMER_CNT; i++) {
        if (rtmDpmConfigPtr->timerConfig[i].timerType) {
            timerConfPtr = &rtmDpmConfigPtr->timerConfig[i];
            PRINTF("  timer%d Type:%d \n", i, timerConfPtr->timerType);
            PRINTF("  timer%d Interval:%d \n", i, timerConfPtr->timerInterval);
            PRINTF("  timer%d Callback:0x%x \n", i, timerConfPtr->timerCallback);
        }
    }

    for (i = 0; i < MAX_SESSION_CNT; i++) {
        if (rtmDpmConfigPtr->sessionConfig[i].sessionType) {
            sessConfPtr = &rtmDpmConfigPtr->sessionConfig[i];
            PRINTF("  session%d type:%d \n", i, sessConfPtr->sessionType);
            PRINTF("  session%d myPort:%d \n", i, sessConfPtr->sessionMyPort);
            PRINTF("  session%d serverIp:%s \n", i, sessConfPtr->sessionServerIp);
            PRINTF("  session%d serverPort:%d \n", i, sessConfPtr->sessionServerPort);
            PRINTF("  session%d recvCallback:0x%x \n", i, sessConfPtr->sessionRecvCallback);
        }
    }

    PRINTF("  externWakeupCallback:0x%x \n", rtmDpmConfigPtr->externWakeupCallback);
    PRINTF("  sizeOfRetentionMemory:%d \n", rtmDpmConfigPtr->sizeOfRetentionMemory);
    PRINTF("  ptrDataFromRetentionMemory:0x%x \n", rtmDpmConfigPtr->ptrDataFromRetentionMemory);
}

void printDpmConfig(void)
{
    int    i;
    timer_config_t    *timerConfPtr;
    session_config_t  *sessConfPtr;

    PRINTF(" < DPM Config > \n");
    PRINTF("  bootInitCallback:0x%x \n", dpmUserConfigs.bootInitCallback);
    PRINTF("  wakeupInitCallback:0x%x \n", dpmUserConfigs.wakeupInitCallback);

    for (i = 0; i < MAX_TIMER_CNT; i++) {
        if (dpmUserConfigs.timerConfig[i].timerType) {
            timerConfPtr = &dpmUserConfigs.timerConfig[i];
            PRINTF("  timer%d Type:%d \n", i, timerConfPtr->timerType);
            PRINTF("  timer%d Interval:%d \n", i, timerConfPtr->timerInterval);
            PRINTF("  timer%d Callback:0x%x \n", i, timerConfPtr->timerCallback);
        }
    }

    for (i = 0; i < MAX_SESSION_CNT; i++) {
        if (dpmUserConfigs.sessionConfig[i].sessionType) {
            sessConfPtr = &dpmUserConfigs.sessionConfig[i];
            PRINTF("  session%d type:%d \n", i, sessConfPtr->sessionType);
            PRINTF("  session%d myPort:%d \n", i, sessConfPtr->sessionMyPort);
            PRINTF("  session%d serverIp:%s \n", i, sessConfPtr->sessionServerIp);
            PRINTF("  session%d serverPort:%d \n", i, sessConfPtr->sessionServerPort);
            PRINTF("  session%d recvCallback:0x%x \n", i, sessConfPtr->sessionRecvCallback);
        }
    }

    PRINTF("  externWakeupCallback:0x%x \n", dpmUserConfigs.externWakeupCallback);
    PRINTF("  errorCallback:0x%x \n", dpmUserConfigs.errorCallback);
    PRINTF("  sizeOfRetentionMemory:%d \n", dpmUserConfigs.sizeOfRetentionMemory);
    PRINTF("  ptrDataFromRetentionMemory:0x%x \n", dpmUserConfigs.ptrDataFromRetentionMemory);
}

static UINT assignDpmRtmArea(char *rtmName, void **rtmPtrPtr, UINT size)
{
    UINT    len;
    UINT    status;
    int    sysmode;

    if (!dpm_mode_is_wakeup()) {
        sysmode = get_run_mode();
    } else {
        /* !!! Caution !!!
         * Do not read operation from NVRAM when dpm_wakeup.
         */
        sysmode = SYSMODE_STA_ONLY;
    }

    if (sysmode != SYSMODE_STA_ONLY) {
        return pdFALSE;
    }

    DM_DBG_INFO(" [%s] START \n", __func__);

    len = dpm_user_rtm_get(rtmName, (UCHAR **)rtmPtrPtr);

    if (len == 0) {    // New allocation
        status = dpm_user_rtm_allocate(rtmName, rtmPtrPtr, size, 1000);
        DM_DBG_INFO(" [%s] after RTM alloc, status=0x%x, rtmPtrPtr=%p\n", __func__, status, rtmPtrPtr);

        if (status != ERR_OK) {
            DM_DBG_ERR(RED "[%s] Failed to allocate rtm(0x%02x)\n" CLEAR, __func__, status);
            errorCallback(DM_ERROR_ALLOCATE_RTM, "Fail to allocate user rtm");
            return pdFALSE;
        }
    } else {
        if (len != size) {
            DM_DBG_INFO(" [%s] Invalid alloc size ...\n", __func__);

            errorCallback(DM_ERROR_INVALID_SIZE_RTM, "Fail to allocate size user rtm");
            return pdFALSE;
        }
    }

    DM_DBG_INFO(" [%s] END rtmPtrPtr:0x%p \n", __func__, rtmPtrPtr);

    return pdTRUE;
}

#ifndef __DPM_MNG_SAVE_RTM__
static void saveDpmConfigToRtm(void)
{
    int    sysmode;

    if (!dpm_mode_is_wakeup()) {
        sysmode = get_run_mode();
    } else {
        /* !!! Caution !!!
         * Do not read operation from NVRAM when dpm_wakeup.
         */
        sysmode = SYSMODE_STA_ONLY;
    }

    if (sysmode != SYSMODE_STA_ONLY) {
        return;
    }

    if (rtmDpmConfigPtr == NULL) {
        return;
    }

    // Restore dpmUserConfigs values
    if (rtmDpmConfigPtr) {
        //Save values in dpm_user_config_t
        memcpy(rtmDpmConfigPtr, &dpmUserConfigs, sizeof(dpm_user_config_t));
        DM_DBG_INFO(" [%s] saved user Params \n", __func__);
    } else {
        DM_DBG_ERR(RED "[%s] rtmDpmConfigPtr is NULL \n" CLEAR, __func__);
    }
}
#endif // !(__DPM_MNG_SAVE_RTM__)

static void saveUserParamsToRtm(void)
{
    int    sysmode;

    if (!dpm_mode_is_wakeup()) {
        sysmode = get_run_mode();
    } else {
        /* !!! Caution !!!
         * Do not read operation from NVRAM when dpm_wakeup.
         */
        sysmode = SYSMODE_STA_ONLY;
    }

    if (sysmode != SYSMODE_STA_ONLY) {
        return;
    }

    if (rtmUserParamsPtr == NULL) {
        return;
    }

    // Restore dpmUserConfigs values
    if (rtmDpmConfigPtr) {
        memcpy(rtmUserParamsPtr,
            dpmUserConfigs.ptrDataFromRetentionMemory,
            dpmUserConfigs.sizeOfRetentionMemory);

        DM_DBG_INFO(" [%s] saved En673Params \n", __func__);
    } else {
        DM_DBG_ERR(RED "[%s] rtmDpmConfigPtr is NULL \n" CLEAR, __func__);
    }
}

static int readFromRtm(char *rtmName, void *configs, void **rtmPtr, UINT size)
{
    if (assignDpmRtmArea(rtmName, rtmPtr, size) == pdFALSE) {
        DM_DBG_ERR(RED "[%s] ERROR \n" CLEAR, __func__);
        errorCallback(DM_ERROR_ASSIGN_RTM, "Cannot assign RTM area");
        return 1;
    }

    // Restore dpmUserConfigs values
    memcpy(configs, *rtmPtr, size);

    DM_DBG_INFO(" [%s] COPY_DONE, rtmDpmConfigPtr:0x%p \n", __func__, rtmDpmConfigPtr);

    return 0;
}

static int __set_dpm_sleep(char *mod_name, char *calling, int line)
{
    //DM_DBG_INFO(YELLOW " [%s] SET DPM SLEEP %s <-- %s:%d \n" CLEAR,
    //                                __func__, mod_name, calling, line);

    DA16X_UNUSED_ARG(line);

    if (dpm_mode_is_enabled() != DPM_ENABLED) {
        return 0;
    }

    if (dpm_app_sleep_ready_set(mod_name)) {
        DM_DBG_ERR(RED "[%s] SET DPM SLEEP ERROR %s <-- %s \n" CLEAR, __func__, mod_name, calling);
        return 1;
    }

    return 0;
}

static int __clr_dpm_sleep(char *mod_name, char *calling, int line)
{
    //DM_DBG_INFO(YELLOW " [%s] CLR DPM SLEEP %s <-- %s:%d \n" CLEAR,
    //                                __func__, mod_name, calling, line);

    DA16X_UNUSED_ARG(line);

    if (dpm_mode_is_enabled() != DPM_ENABLED) {
        return 0;
    }

    if (dpm_app_sleep_ready_clear(mod_name)) {
        DM_DBG_ERR(RED "[%s] CLR DPM SLEEP ERROR %s <-- %s \n" CLEAR, __func__, mod_name, calling);
        return 1;
    }

    return 0;
}

static void executeUserCallback(void (*callback)(), int argc, void *argv[], int jobStartFlag)
{
    DM_DBG_INFO(" [%s] Call th register function (argc:%d) \n", __func__, argc);

    if (callback == NULL) {
        DM_DBG_ERR(" [%s] Not a registered callback function (argc:%d) \n", __func__, argc);
        return;
    }

    if (jobStartFlag) {
        dpm_mng_job_start();
    }

    if (argc == 0) {
        DM_DBG_ERR(RED "[%s] Error argc : %d \n" CLEAR, __func__, argc);
    } else if (argc == 1) {
        callback();
    } else if (argc == 2) {
        callback(*(int *)argv[1]);
    } else if (argc == 3) {
        callback(*(int *)argv[1], *(int *)argv[2]);
    } else if (argc == 4) {
        callback(*(int *)argv[1], *(int *)argv[2], *(int *)argv[3]);
    } else if (argc == 5) {
        callback(*(int *)argv[1], *(int *)argv[2], *(int *)argv[3], *(int *)argv[4]);
    } else if (argc == 6) {
        callback(*(int *)argv[1], *(int *)argv[2], *(int *)argv[3], *(int *)argv[4],
                 *(int *)argv[5]);
    } else {
        if (jobStartFlag) {
            dpm_mng_job_done();
        }
        DM_DBG_ERR(RED "[%s] Error argc : %d \n" CLEAR, __func__, argc);
    }
}

static int initDpmConfig(void)
{
    if (initDpmConfigFunction) {
        memset(&dpmUserConfigs, 0, sizeof(dpm_user_config_t));
        initDpmConfigFunction(&dpmUserConfigs);        // Set config by user
        dpmUserConfigs.magicCode = DPM_MAGIC_CODE;
        return 0;
    } else {
        DM_DBG_ERR(RED "!!! No register infomation fo initDpmConfigFunction !!!\n" CLEAR, __func__);
        return 1;
    }
}

static int initSessionConfigByWakeup(void)
{
    int    i;

    for (i = 0; i < MAX_SESSION_CNT; i++) {
        dpmUserConfigs.sessionConfig[i].socket = 0;
        dpmUserConfigs.sessionConfig[i].rxDataBuffer = NULL;
        dpmUserConfigs.sessionConfig[i].runStatus = SESSION_STATUS_STOP;

        memset(&dpmUserConfigs.sessionConfig[i].secureConfig, 0x00, sizeof(SECURE_INFO_T));
    }

    return 0;
}

static UINT generateClientLocalPort(UINT session_type)
{
    UINT port = get_random_value_ushort();    //generate random port number.
    int idx = 0;

    if (session_type != REG_TYPE_TCP_CLIENT && session_type != REG_TYPE_UDP_CLIENT) {
        DM_DBG_ERR(" [%s] Not allowed session type(%d)\n", __func__, session_type);
        return 0;
    }

    do {
        session_config_t *sessConfPtr = &dpmUserConfigs.sessionConfig[idx];

        if ((sessConfPtr->sessionType == session_type) && (sessConfPtr->localPort == port)) {
            port++;
        } else {
            idx++;
        }
    } while (idx < MAX_SESSION_CNT);

    return port;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// start dpm Event Manager /////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
static void dpmEventManager(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;
    unsigned int maskedEvent;
    unsigned int waitCount = 0;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    dpm_app_register(DPM_EVENT_REG_NAME, 0);

    dpmManagerEventGroup = xEventGroupCreate();
    if (dpmManagerEventGroup == NULL) {
        da16x_sys_watchdog_unregister(sys_wdog_id);
        return;
    }

    DM_DBG_INFO(BLUE " [%s] Start dpmEventManager (%s) ...\n" CLEAR, __func__, DPM_EVENT_REG_NAME);
    dpm_app_wakeup_done(DPM_EVENT_REG_NAME);        // Noti to timer (Can receive timer ISR)

    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

event_loop:
        __set_dpm_sleep(DPM_EVENT_REG_NAME, (char *)__func__, (int) __LINE__);

        vTaskDelay(1);

        DM_DBG_INFO(BLUE " [%s] Wait event %s \n" CLEAR, __func__, DPM_EVENT_REG_NAME);
        maskedEvent = xEventGroupWaitBits(dpmManagerEventGroup,
                            DPM_EVENT__ALL,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);

        __clr_dpm_sleep(DPM_EVENT_REG_NAME, (char *)__func__, (int) __LINE__);
        DM_DBG_INFO(BLUE " [%s] Receive event %s : 0x%x \n" CLEAR, __func__, DPM_EVENT_REG_NAME, maskedEvent);

        while (!dpmControlManagerDoneFlag) {
            if (++waitCount > 20) {
                DM_DBG_ERR(RED "[%s] Wait timeout (event:0x%x) \n" CLEAR, __func__, maskedEvent);
                xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT__ALL);
                goto event_loop;
            }

            vTaskDelay(2);
        }

        if (maskedEvent & DPM_EVENT_TIMER1) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TIMER1);
            executeUserCallback(dpmUserConfigs.timerConfig[0].timerCallback, 1, NULL, pdTRUE);
        }

        if (maskedEvent & DPM_EVENT_TIMER2) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TIMER2);
            executeUserCallback(dpmUserConfigs.timerConfig[1].timerCallback, 1, NULL, pdTRUE);
        }

        if (maskedEvent & DPM_EVENT_TIMER3) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TIMER3);
            executeUserCallback(dpmUserConfigs.timerConfig[2].timerCallback, 1, NULL, pdTRUE);
        }

        if (maskedEvent & DPM_EVENT_TIMER4) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TIMER4);
            executeUserCallback(dpmUserConfigs.timerConfig[3].timerCallback, 1, NULL, pdTRUE);
        }

#if !defined (__LIGHT_DPM_MANAGER__)
        if (maskedEvent & DPM_EVENT_TCP_S_CB_TIMER1) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_S_CB_TIMER1);
            dpmTcpSvrKaCbSubSess(1);
        }

        if (maskedEvent & DPM_EVENT_TCP_S_CB_TIMER2) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_S_CB_TIMER2);
            dpmTcpSvrKaCbSubSess(2);
        }

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 2)
        if (maskedEvent & DPM_EVENT_TCP_S_CB_TIMER3) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_S_CB_TIMER3);
            dpmTcpSvrKaCbSubSess(3);
        }

#endif    // DM_TCP_SVR_MAX_SESS && DM_TCP_SVR_MAX_SESS > 2

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 3)
        if (maskedEvent & DPM_EVENT_TCP_S_CB_TIMER4) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_S_CB_TIMER4);
            dpmTcpSvrKaCbSubSess(4);
        }
#endif    // DM_TCP_SVR_MAX_SESS && DM_TCP_SVR_MAX_SESS > 3

#endif    // !__LIGHT_DPM_MANAGER__

        if (maskedEvent & DPM_EVENT_TCP_C_CB_TIMER1) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_C_CB_TIMER1);
            dpmTcpCliKaCbSess(1);
        }

        if (maskedEvent & DPM_EVENT_TCP_C_CB_TIMER2) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_C_CB_TIMER2);
            dpmTcpCliKaCbSess(2);
        }

        if (maskedEvent & DPM_EVENT_TCP_C_CB_TIMER3) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_C_CB_TIMER3);
            dpmTcpCliKaCbSess(3);
        }

        if (maskedEvent & DPM_EVENT_TCP_C_CB_TIMER4) {
            xEventGroupClearBits(dpmManagerEventGroup, DPM_EVENT_TCP_C_CB_TIMER4);
            dpmTcpCliKaCbSess(4);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////
// end dpm Event Manager ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////
// start DPM Manager common ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
static int    initSessionCommon(session_config_t *sessConfPtr)
{
    sessConfPtr->requestCmd = CMD_NONE;

#ifdef SESSION_MUTEX
    sessConfPtr->socketMutex = xSemaphoreCreateMutex();
    if ( sessConfPtr->socketMutex == NULL ) {
        DM_DBG_ERR(" [%s] semaphore create error \n", __func__);
        errorCallback(DM_ERROR_SOCKET_MUTEX_CREATE_FAIL, "socket Mutex create fail");
        return ER_MUTEX_CREATE;
    }
#endif
    sessConfPtr->runStatus = SESSION_STATUS_INIT;

    return ERR_OK;
}

static void deinitSessionCommon(session_config_t *sessConfPtr)
{
#ifdef SESSION_MUTEX
    vSemaphoreDelete(sessConfPtr->socketMutex);
#endif
    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        dpm_app_unregister(sessConfPtr->dpmRegName);
    }
    sessConfPtr->sessionConnStatus = 0xFF;
    sessConfPtr->runStatus = SESSION_STATUS_STOP;
}

void connectCallback(session_config_t *sessConfPtr, UINT status)
{
    void    *paramData[3] = {NULL, };

    paramData[1] = (void *)&sessConfPtr;
    paramData[2] = (void *)&status;

    sessConfPtr->sessionConnStatus = status;

    if ((sessConfPtr->sessionNo == SESSION1) && (dpmUserConfigs.sessionConfig[0].sessionConnectCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[0].sessionConnectCallback, 3, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION2) && (dpmUserConfigs.sessionConfig[1].sessionConnectCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[1].sessionConnectCallback, 3, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION3) && (dpmUserConfigs.sessionConfig[2].sessionConnectCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[2].sessionConnectCallback, 3, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION4) && (dpmUserConfigs.sessionConfig[3].sessionConnectCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[3].sessionConnectCallback, 3, paramData, pdTRUE);
    }
}

void receiveCallback(session_config_t *sessConfPtr, char *data, UINT len, ULONG ip, UINT port)
{
    void    *paramData[6] = {NULL, };

    paramData[1] = (void *)&sessConfPtr;
    paramData[2] = (void *)&data;
    paramData[3] = (void *)&len;
    paramData[4] = (void *)&ip;
    paramData[5] = (void *)&port;

    if ((sessConfPtr->sessionNo == SESSION1) && (dpmUserConfigs.sessionConfig[0].sessionRecvCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[0].sessionRecvCallback, 6, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION2) && (dpmUserConfigs.sessionConfig[1].sessionRecvCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[1].sessionRecvCallback, 6, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION3) && (dpmUserConfigs.sessionConfig[2].sessionRecvCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[2].sessionRecvCallback, 6, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION4) && (dpmUserConfigs.sessionConfig[3].sessionRecvCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[3].sessionRecvCallback, 6, paramData, pdTRUE);
    }
}

void setupSecureCallback(session_config_t *sessConfPtr, void *config)
{
    void    *paramData[2] = {NULL, };

#ifdef DM_MUTEX
    xSemaphoreTake(dmMutex, portMAX_DELAY);
#endif
    paramData[1] = (void *)&config;

    if ((sessConfPtr->sessionNo == SESSION1) && (dpmUserConfigs.sessionConfig[0].sessionSetupSecureCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[0].sessionSetupSecureCallback, 2, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION2) && (dpmUserConfigs.sessionConfig[1].sessionSetupSecureCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[1].sessionSetupSecureCallback, 2, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION3) && (dpmUserConfigs.sessionConfig[2].sessionSetupSecureCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[2].sessionSetupSecureCallback, 2, paramData, pdTRUE);
    } else if ((sessConfPtr->sessionNo == SESSION4) && (dpmUserConfigs.sessionConfig[3].sessionSetupSecureCallback)) {
        executeUserCallback(dpmUserConfigs.sessionConfig[3].sessionSetupSecureCallback, 2, paramData, pdTRUE);
    }

#ifdef DM_MUTEX
    xSemaphoreGive(dmMutex);
#endif
}

static void errorCallback(UINT error_code, char *comment)
{
    void    *paramData[3] = {NULL, };

    paramData[1] = (void *)&error_code;
    paramData[2] = (void *)&comment;

    if (dpmUserConfigs.errorCallback) {
        executeUserCallback(dpmUserConfigs.errorCallback, 3, paramData, pdFALSE);
    }
}

static int sessionRunConfirm(session_config_t *sessConfPtr, int tick_wait_time)
{
    int    waitCount = 0;

run_confirm:
    if (sessConfPtr->runStatus != SESSION_STATUS_RUN) {
        if (++waitCount <= tick_wait_time) {
            vTaskDelay(1);
            goto run_confirm;
        }

        DM_DBG_ERR(RED "[%s:%d] Failed to wait for running status (type:%d, wait_time:%d) \n" CLEAR,
                        __func__, __LINE__, sessConfPtr->sessionType, tick_wait_time);
        return pdFALSE;
    } else {
        return pdTRUE;
    }
}

int handshakingFlag = 0;
int startHandshaking()
{
#ifdef SEPERATE_HANDSHAKE
    int    waitCount = 0;

wait:
    if (handshakingFlag == pdTRUE) {
        if (++waitCount > 100) {
            DM_DBG_ERR(RED "[%s] Failure to wait for a handshake \n" CLEAR, __func__);
            return ERR_TIMEOUT;
        }
        vTaskDelay(10);
        goto wait;
    }

    handshakingFlag = pdTRUE;

    return ERR_OK;
#endif
}

void endHandshaking()
{
#ifdef SEPERATE_HANDSHAKE
    handshakingFlag = pdFALSE;
#endif
}

#if !defined (__LIGHT_DPM_MANAGER__)
int rtcRegistTcpServerKeepaliveTimer(tcpRxSubTaskConf_t *tcpRxSubTaskConfPtr)
{
    session_config_t    *sessConfPtr = tcpRxSubTaskConfPtr->sessConfPtr;
    VOID                (*functionPtr)(char * arg) = NULL;

    switch (tcpRxSubTaskConfPtr->subIndex) {
        case 0:
            functionPtr = dpm_tcps_ka_cb_sub0;
            break;

        case 1:
            functionPtr = dpm_tcps_ka_cb_sub1;
            break;

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 2)
        case 2:
            functionPtr = dpm_tcps_ka_cb_sub2;
            break;
#endif

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 3)
        case 3:
            functionPtr = dpm_tcps_ka_cb_sub3;
            break;
#endif

        default:
            DM_DBG_ERR(RED "[%s] subIndex error (%d) \n" CLEAR, __func__, tcpRxSubTaskConfPtr->subIndex);
            return -1;
            break;
    }

    tcpRxSubTaskConfPtr->subSessionKaTid = rtc_register_timer(sessConfPtr->sessionKaInterval * 1000,
                                                            tcpRxSubTaskConfPtr->tcpRxSubTaskName,
                                                            tcpRxSubTaskConfPtr->subSessionKaTid,
                                                            0,
                                                            functionPtr); //msec

    DM_DBG_INFO(CYAN " [%s] Register Keep-Alive sub_idx:%d name:%s ka_interval:%d ka_tid:%d \n" CLEAR,
                                                            __func__,
                                                            tcpRxSubTaskConfPtr->subIndex,
                                                            tcpRxSubTaskConfPtr->tcpRxSubTaskName,
                                                            sessConfPtr->sessionKaInterval,
                                                            tcpRxSubTaskConfPtr->subSessionKaTid);

    return tcpRxSubTaskConfPtr->subSessionKaTid;
}
#endif // __LIGHT_DPM_MANAGER__

int rtcRegistTcpClientKeepaliveTimer(session_config_t *sessConfPtr)
{
    void (*functionPtr)(char * arg) = NULL;

    switch (sessConfPtr->sessionNo) {
    case SESSION1:
        functionPtr = dpm_tcpc_ka_cb_sess0;
        break;

    case SESSION2:
        functionPtr = dpm_tcpc_ka_cb_sess1;
        break;

    case SESSION3:
        functionPtr = dpm_tcpc_ka_cb_sess2;
        break;

    case SESSION4:
        functionPtr = dpm_tcpc_ka_cb_sess3;
        break;

    default:
        DM_DBG_ERR(RED "[%s] Session_no error (%d) \n" CLEAR, __func__, sessConfPtr->sessionNo);
        return -1;
        break;
    }

    sessConfPtr->sessionKaTid = rtc_register_timer(sessConfPtr->sessionKaInterval * 1000,
                                                   sessConfPtr->dpmRegName,
                                                   sessConfPtr->sessionKaTid,
                                                   1,
                                                   functionPtr); //msec

    DM_DBG_INFO(CYAN "[%s] Reg KA session:%d itv:%d tid:%d \n" CLEAR,
                      __func__,
                      sessConfPtr->sessionNo,
                      sessConfPtr->sessionKaInterval,
                      sessConfPtr->sessionKaTid);

    return sessConfPtr->sessionKaTid;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// end DPM Manager common //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////
// start Secure ////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void ssl_ctx_clear(SECURE_INFO_T *config)
{
    config->server_ctx = NULL;
    config->listen_ctx = NULL;
    config->client_ctx = NULL;

    config->ssl_ctx = NULL;
    config->ssl_conf = NULL;
    config->cookie_ctx = NULL;

    config->cacert_crt = NULL;
    config->cert_crt = NULL;
    config->pkey_ctx = NULL;
    config->pkey_alt_ctx = NULL;
}

UINT tls_save_ssl(char *name, SECURE_INFO_T *config)
{
    int ret = 0;

    if (dpm_mode_is_enabled() != DPM_ENABLED) {
        return 0;
    }

    //DM_DBG_INFO(" [%s:%d] Save tls session(%s) \n", __func__, __LINE__, name);

    ret = set_tls_session(name, config->ssl_ctx);
    if (ret) {
        DM_DBG_ERR(RED "[%s:%d] Failed to save tls session(name:%s, status:0x%x) \n" CLEAR,
                        __func__, __LINE__, name, -ret);

        return ER_NOT_SUCCESSFUL;
    }

    return ERR_OK;
}

UINT tls_restore_ssl(char *name, SECURE_INFO_T *config)
{
    int ret = 0;

    if (dpm_mode_is_enabled() != DPM_ENABLED) {
        return 0;
    }

    ret = get_tls_session(name, config->ssl_ctx);
    if (ret == ER_NOT_FOUND) {
        //DM_DBG_INFO(RED "[%s:%d] Not found(%s) \n" CLEAR, __func__, __LINE__, name);
        return ERR_OK;
    } else if (ret != 0) {
        DM_DBG_INFO(RED "[%s:%d] Failed to restore tls session(0x%x) \n" CLEAR, __func__, __LINE__, -ret);
        return ER_NOT_SUCCESSFUL;
    }

    return ERR_OK;
}

UINT tls_clear_ssl(char *name, SECURE_INFO_T *config)
{
    DA16X_UNUSED_ARG(config);

    int ret = 0;

    if (dpm_mode_is_enabled() != DPM_ENABLED) {
        return 0;
    }

    DM_DBG_INFO(" [%s:%d] Clear tls session(%s) \n", __func__, __LINE__, name);

    ret = clr_tls_session(name);
    if (ret) {
        DM_DBG_ERR(RED "[%s:%d] Failed to clear tls session(0x%x) \n" CLEAR, __func__, __LINE__, ret);
        return ER_NOT_SUCCESSFUL;
    }

    return ERR_OK;
}

static void dmDtlsTimerStart(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
    SECURE_TIMER *timer_ptr = (SECURE_TIMER *)ctx;

#ifdef DTLS_TIMER_LOG
    DM_DBG_INFO(" [%s:%d] timer(%s), int_ms(%d), fin_ms(%d), state(%d)\n",
            __func__, __LINE__, timer_ptr->name, int_ms, fin_ms, timer_ptr->state);
#endif

    if (!timer_ptr) {
        DM_DBG_ERR(RED "[%s] Invaild paramter \n" CLEAR, __func__);
        return;
    }

    timer_ptr->int_ms = int_ms;
    timer_ptr->fin_ms = fin_ms;

    if (fin_ms != 0) {
        timer_ptr->snapshot = xTaskGetTickCount();
    }

    return;
}

static int dmDtlsTimerGetStatus(void *ctx)
{
    SECURE_TIMER *timer_ptr = (SECURE_TIMER *)ctx;
    TickType_t elapsed = 0;

    if (timer_ptr == NULL) {
        return CANCELLED;
    }

    if (timer_ptr->fin_ms == 0) {
        return CANCELLED;
    }

    elapsed = xTaskGetTickCount() - timer_ptr->snapshot;

    if (elapsed >= pdMS_TO_TICKS(timer_ptr->fin_ms)) {
        return FIN_EXPIRY;
    }

    if (elapsed >= pdMS_TO_TICKS(timer_ptr->int_ms)) {
        return INT_EXPIRY;
    }

    return NO_EXPIRY;
}

void dmSecureDeinit(SECURE_INFO_T *config)
{
    DM_DBG_INFO(YELLOW " [%s] Started \n" CLEAR, __func__);

    if (config->sessionType == REG_TYPE_UDP_CLIENT) {
        if (config->server_ctx) {
            mbedtls_net_free(config->server_ctx);
            vPortFree(config->server_ctx);
        }
    }

    if (config->sessionType == REG_TYPE_UDP_SERVER) {
        if (config->listen_ctx) {
            mbedtls_net_free(config->listen_ctx);
            vPortFree(config->listen_ctx);
        }

        if (config->client_ctx) {
            mbedtls_net_free(config->client_ctx);
            vPortFree(config->client_ctx);
        }
    }

    if (config->ssl_ctx) {
        mbedtls_ssl_free(config->ssl_ctx);
        vPortFree(config->ssl_ctx);
    }

    if (config->ssl_conf) {
        mbedtls_ssl_config_free(config->ssl_conf);
        vPortFree(config->ssl_conf);
    }

    dpm_mng_deinit_rng();

    if (config->cacert_crt) {
        mbedtls_x509_crt_free(config->cacert_crt);
        vPortFree(config->cacert_crt);
    }

    if (config->cert_crt) {
        mbedtls_x509_crt_free(config->cert_crt);
        vPortFree(config->cert_crt);
    }

    if (config->pkey_ctx) {
        mbedtls_pk_free(config->pkey_ctx);
        vPortFree(config->pkey_ctx);
    }

    if (config->pkey_alt_ctx) {
        mbedtls_pk_free(config->pkey_alt_ctx);
        vPortFree(config->pkey_alt_ctx);
    }

    if (config->cookie_ctx) {
        mbedtls_ssl_cookie_free(config->cookie_ctx);
        vPortFree(config->cookie_ctx);
    }

    ssl_ctx_clear(config);

    DM_DBG_INFO(" [%s] Done \n", __func__);

    return ;
}

static UINT dmSecureInit(SECURE_INFO_T *config)
{
    DM_DBG_INFO(YELLOW " [%s] Started \n" CLEAR, __func__);

    if (config->sessionType == REG_TYPE_UDP_CLIENT) {
        if (!config->server_ctx) {
            config->server_ctx = pvPortMalloc(sizeof(mbedtls_net_context));
            if (!config->server_ctx) {
                DM_DBG_ERR(" [%s:%d] Failed to allocate server context(%d)\n",
                        __func__, __LINE__, sizeof(mbedtls_net_context));
                goto error;
            }

            mbedtls_net_init(config->server_ctx);
        }
    }

    if (config->sessionType == REG_TYPE_UDP_SERVER) {
        if (!config->listen_ctx) {
            config->listen_ctx = pvPortMalloc(sizeof(mbedtls_net_context));
            if (!config->listen_ctx) {
                DM_DBG_ERR(" [%s:%d] Failed to allocate listen context(%d)\n",
                        __func__, __LINE__, sizeof(mbedtls_net_context));
                goto error;
            }

            mbedtls_net_init(config->listen_ctx);
        }

        if (!config->client_ctx) {
            config->client_ctx = pvPortMalloc(sizeof(mbedtls_net_context));
            if (!config->client_ctx) {
                DM_DBG_ERR(" [%s:%d] Failed to allocate client context(%d)\n",
                        __func__, __LINE__, sizeof(mbedtls_net_context));
                goto error;
            }

            mbedtls_net_init(config->client_ctx);
        }
    }

    if (!config->ssl_ctx) {
        config->ssl_ctx = pvPortMalloc(sizeof(mbedtls_ssl_context));
        if (!config->ssl_ctx) {
            DM_DBG_ERR(" [%s:%d] Failed to allocate ssl context(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_ssl_context));
            goto error;
        }

        mbedtls_ssl_init(config->ssl_ctx);
    }

    if (!config->ssl_conf) {
        config->ssl_conf = pvPortMalloc(sizeof(mbedtls_ssl_config));
        if (!config->ssl_conf)
        {
            DM_DBG_ERR(" [%s:%d] Failed to allocate ssl config(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_ssl_config));
            goto error;
        }

        mbedtls_ssl_config_init(config->ssl_conf);
    }

    if (dpm_mng_init_rng()) {
        DM_DBG_ERR(" [%s:%d] Failed to init random generator\n",
                   __func__, __LINE__);
        goto error;
    }

    if (!config->cacert_crt) {
        config->cacert_crt = pvPortMalloc(sizeof(mbedtls_x509_crt));
        if (!config->cacert_crt) {
            DM_DBG_ERR(" [%s:%d] Failed to allocate cacert crt(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_x509_crt));
            goto error;
        }

        mbedtls_x509_crt_init(config->cacert_crt);
    }

    if (!config->cert_crt) {
        config->cert_crt = pvPortMalloc(sizeof(mbedtls_x509_crt));
        if (!config->cert_crt) {
            DM_DBG_ERR(" [%s:%d] Failed to allocate cert crt(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_x509_crt));
            goto error;
        }

        mbedtls_x509_crt_init(config->cert_crt);
    }

    if (!config->pkey_ctx) {
        config->pkey_ctx = pvPortMalloc(sizeof(mbedtls_pk_context));
        if (!config->pkey_ctx) {
            DM_DBG_ERR(" [%s:%d] Failed to allocate private key context(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_pk_context));
            goto error;
        }

        mbedtls_pk_init(config->pkey_ctx);
    }

    if (!config->pkey_alt_ctx) {
        config->pkey_alt_ctx = pvPortMalloc(sizeof(mbedtls_pk_context));
        if (!config->pkey_alt_ctx)
        {
            DM_DBG_ERR(" [%s:%d] Failed to allocate private key context for alt(%d)\n",
                    __func__, __LINE__, sizeof(mbedtls_pk_context));
            goto error;
        }

        mbedtls_pk_init(config->pkey_alt_ctx);
    }

    if (!config->cookie_ctx) {
        config->cookie_ctx = pvPortMalloc(sizeof(mbedtls_ssl_cookie_ctx));
        if (!config->cookie_ctx) {
            DM_DBG_ERR(" [%s:%d] Failed to allocate cookie context\n", __func__, __LINE__);
            goto error;
        }

        mbedtls_ssl_cookie_init(config->cookie_ctx);
    }

    return ERR_OK;

error:

    dmSecureDeinit(config);

    return ER_NOT_CREATED;
}

static UINT dmSecureShutdown(SECURE_INFO_T *config)
{
    int ret = 0;

    if (!config->ssl_ctx) {
        DM_DBG_ERR(RED "[%s] ssl_ctx is none \n" CLEAR, __func__);
        return ER_NOT_SUCCESSFUL;
    }

    ret = mbedtls_ssl_session_reset(config->ssl_ctx);
    if (ret) {
        DM_DBG_ERR(RED "[%s:%d] Failed to reset session(0x%x)\n" CLEAR, __func__, __LINE__, -ret);
        return ER_NOT_SUCCESSFUL;
    }

    return ERR_OK;
}

static UINT dmSecureHandshake(SECURE_INFO_T *config)    // TCP Server/Client handshake
{
    UINT status = ERR_OK;
    int ret = 0;

    if (startHandshaking()) {
        DM_DBG_ERR(RED "[%s] Failed to wait_timeout \n" CLEAR, __func__);
        return ER_NOT_SUCCESSFUL;
    }

    DM_DBG_INFO(" [%s] Start TLS handshake \n", __func__);

    while ((ret = mbedtls_ssl_handshake(config->ssl_ctx)) != 0) {
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            DM_DBG_ERR(RED "[%s:%d] Failed to process TLS handshake(0x%x)\n" CLEAR,
                                                                    __func__, __LINE__, -ret);
            status = ER_NOT_SUCCESSFUL;
            goto end_of_tcp_handshake;
        }

        //TODO: need to verify auth mode
    }

    DM_DBG_INFO(BLUE " [%s] Success to progress TLS handshake \n" CLEAR, __func__);

end_of_tcp_handshake:

    endHandshaking();
    return status;
}

static int dmSecureSendCallback(void *ctx, const unsigned char *buf, size_t len)
{
    SECURE_INFO_T        *config = (SECURE_INFO_T *)ctx;
    session_config_t    *sessConfPtr = config->sessionConfPtr;
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
    udpSubSessConf_t    *udpSubSessInfoPtr;
#endif    // !__LIGHT_DPM_MANAGER__
    int        status = ERR_OK;
    int        socket = 0;
    ULONG    udp_peer_ip;
    USHORT    udp_peer_port;
    struct    sockaddr_in    peer_addr;

    if (len == 0) {
        DM_DBG_ERR(RED "[%s] Error len (%d) \n" CLEAR, __func__, len);
        return ERR_ARG;
    }
#if !defined (__LIGHT_DPM_MANAGER__)
    if (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER) {
        tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[config->subIndex];
        socket = tcpRxSubTaskConfPtr->clientSock;
        DM_DBG_INFO(" [%s] Start TCP_SERVER sub_idx:%d len:%d socket_%d \n", __func__, config->subIndex, len, socket);
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER) {
        udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[config->subIndex];
        socket = udpSubSessInfoPtr->clientSock;
        DM_DBG_INFO(" [%s] Start UDP_SERVER sub_idx:%d len:%d socket:%d \n", __func__, config->subIndex, len, socket);
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    {
        socket = sessConfPtr->socket;
        DM_DBG_INFO(" [%s] Start CLIENT type:%d len:%d socket:%d \n", __func__, sessConfPtr->sessionType, len, socket);
    }

    DM_DBG_INFO(" [%s] ready to send (ptr:0x%x socket:%d buf:0x%x len:%d type:%d) \n",
                                    __func__, sessConfPtr, socket, buf, len, sessConfPtr->sessionType);

    if (
#if !defined (__LIGHT_DPM_MANAGER__)
        (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER) ||
#endif    // !__LIGHT_DPM_MANAGER__
        (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) ) {
        // Send data to TCP server
        status = send(socket, buf, len, 0);
        if (status <= 0) {
            DM_DBG_ERR(RED "[%s] tcp socket send error (%d)\n" CLEAR, __func__, status);
        } else {
            DM_DBG_INFO(" [%s] <=== send OK (len:%d, status:%d) \n", __func__, len, status);
        }

        return status;
    }
#if !defined (__LIGHT_DPM_MANAGER__)
    else if (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER) {
        udp_peer_ip = iptolong(udpSubSessInfoPtr->peerIpAddr);
        udp_peer_port = udpSubSessInfoPtr->peerPort;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr.s_addr = htonl(udp_peer_ip);
        peer_addr.sin_port = htons(udp_peer_port);

        DM_DBG_INFO(CYAN " [%s] data send socket:%d ip:0x%x, port:%d len:%d \n" CLEAR,
                        __func__, socket, udp_peer_ip, udp_peer_port, len);
        // Send data to UDP server
        status = sendto(socket, buf, len, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        if (status != (int)len) {
            DM_DBG_ERR(RED "[%s] Failed to data send (%d)\n" CLEAR, __func__, status);
            errorCallback(DM_ERROR_SEND_FAIL, "Fail to udp server socket send");

            return status;
        } else {
            DM_DBG_INFO(CYAN " [%s] data send OK len:%d \n" CLEAR, __func__, len);
            return (int)len;
        }
    }
#endif    // !__LIGHT_DPM_MANAGER__
    else if (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT) {
        udp_peer_ip = (ULONG)iptolong(sessConfPtr->sessionServerIp);
        udp_peer_port = (USHORT)(sessConfPtr->sessionServerPort);

        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr.s_addr = htonl(udp_peer_ip);
        peer_addr.sin_port = htons(udp_peer_port);

        DM_DBG_INFO(CYAN " [%s] data send ip:0x%x(%s), port:%d len:%d \n" CLEAR,
                        __func__, udp_peer_ip, sessConfPtr->sessionServerIp, udp_peer_port, len);
        // Send data to UDP server
        status = sendto(socket, buf, len, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        if (status != (int)len) {
            DM_DBG_ERR(RED "[%s] Failed to data send (%d)\n" CLEAR, __func__, status);
            errorCallback(DM_ERROR_SEND_FAIL, "Fail to udp client socket send");

            return status;
        } else {
            return (int)len;
        }
    } else {
        DM_DBG_ERR(RED "[%s] Type error (%d)\n" CLEAR, __func__, sessConfPtr->sessionType);
        errorCallback(DM_ERROR_INVALID_TYPE, "Fail session type");
        return ERR_ARG;
    }
}

static UINT dmSecureSend(SECURE_INFO_T *config, VOID *buf, size_t len, ULONG wait_option)
{
    UINT status = ERR_OK;
    int ret = 0;

    config->timeout = wait_option;

    DM_DBG_INFO(" [%s:%d] Start send to packet(len:%d)\n" CLEAR, __func__, __LINE__, len);
    while ((ret = mbedtls_ssl_write(config->ssl_ctx, buf, len)) <= 0) {      // dmSecureSendCallback
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            DM_DBG_ERR(RED "[%s:%d] Failed to send packet(0x%x)\n" CLEAR, __func__, __LINE__, -ret);
            status = ER_NOT_SUCCESSFUL;
            break;
        }
    }
    DM_DBG_INFO(" [%s:%d] End send to packet(ret:%d)\n" CLEAR, __func__, __LINE__, ret);

    return status;
}

static int dmSecureRecvCallback(void *ctx, unsigned char *buf, size_t len)
{
    SECURE_INFO_T        *config = (SECURE_INFO_T *)ctx;
    session_config_t    *sessConfPtr = config->sessionConfPtr;
#if !defined (__LIGHT_DPM_MANAGER__)
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr = NULL;
#endif    // !__LIGHT_DPM_MANAGER__
    struct    sockaddr_in    peer_addr;
    int        addr_len;
    int        socket = 0;
    int        ret = 0;

    if (!config) {
        DM_DBG_ERR(RED "[%s] Invalid ctx\n" CLEAR, __func__);
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    }

#if !defined (__LIGHT_DPM_MANAGER__)
    if (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER) {
        //xSemaphoreTake(sessConfPtr->socketMutex, portMAX_DELAY);
        // Get socket pointer
        tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[config->subIndex];
        socket = tcpRxSubTaskConfPtr->clientSock;

        setPacketReceivingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);

        if (buf == NULL || len == 0) {
            DM_DBG_ERR(RED "[%s] Invalid buf(0x%x) or len(%d) \n" CLEAR, __func__, buf, len);
            clrPacketReceivingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
            return 0;
        }

        ret = recv(socket, buf, len, 0);
        if (ret == 0) {  // DISCONNECT
            DM_DBG_ERR(RED "[%s] NOT_CONNECTED (%d)\n" CLEAR, __func__, ret);
            clrPacketReceivingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
            return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
        } else if (ret < 0) {
            clrPacketReceivingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
            return MBEDTLS_ERR_SSL_TIMEOUT;
        }

        clrPacketReceivingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER) {
        // Get socket pointer
        socket = sessConfPtr->socket;
        setPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);

        ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)&peer_addr, (socklen_t *)&addr_len);
        if (ret < 0) {
            clrPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
            return MBEDTLS_ERR_SSL_TIMEOUT;
        } else {
            DM_DBG_INFO(" [%s:%d] recvfrom (peer_ip:0x%x peer_port:%d len:%d \n",
                    __func__, __LINE__, htonl(peer_addr.sin_addr.s_addr), htons(peer_addr.sin_port), ret);
            clrPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
            return 0;
        }

        clrPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
    } else
#endif    // !__LIGHT_DPM_MANAGER__

    if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
#ifdef SESSION_MUTEX
        xSemaphoreTake(sessConfPtr->socketMutex, portMAX_DELAY);
#endif
        // Get socket pointer
        socket = sessConfPtr->socket;
        setPacketReceivingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);

        if (buf == NULL || len == 0) {
            DM_DBG_ERR(RED "[%s] Invalid buf(0x%x) or len(%d) \n" CLEAR, __func__, buf, len);
            clrPacketReceivingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
            return 0;
        }

        ret = recv(socket, buf, len, 0);
        if (ret == 0) {  // DISCONNECT
            DM_DBG_ERR(RED "[%s] NOT_CONNECTED (%d)\n" CLEAR, __func__, ret);
            clrPacketReceivingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
            return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
        } else if (ret < 0) {
            clrPacketReceivingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
            return MBEDTLS_ERR_SSL_TIMEOUT;
        }

        clrPacketReceivingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
#ifdef SESSION_MUTEX
        xSemaphoreGive(sessConfPtr->socketMutex);
#endif
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT) {
        // Get socket pointer
        socket = sessConfPtr->socket;
        setPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);

        ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)&peer_addr, (socklen_t *)&addr_len);
        if (ret < 0) {
            clrPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);
            return MBEDTLS_ERR_SSL_TIMEOUT;
        } else {
            DM_DBG_INFO(" [%s:%d] recvfrom (peer_ip:0x%x peer_port:%d len:%d \n",
                    __func__, __LINE__, htonl(peer_addr.sin_addr.s_addr), htons(peer_addr.sin_port), ret);
        }

        clrPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);
    }

    return ret;
}

static int dmSecureRecv(SECURE_INFO_T *config, void *packet_ptr, ULONG wait_option)
{
    int status = ERR_OK;
    int ret = 0;
    unsigned char *recv_buf = NULL;
    size_t recv_len = 0;

    config->timeout = wait_option;

    if (
#if !defined (__LIGHT_DPM_MANAGER__)
        (((session_config_t *)config->sessionConfPtr)->sessionType == REG_TYPE_UDP_SERVER) ||
#endif    // !__LIGHT_DPM_MANAGER__
        (((session_config_t *)config->sessionConfPtr)->sessionType == REG_TYPE_UDP_CLIENT))
    {
        mbedtls_ssl_conf_read_timeout(config->ssl_conf, wait_option * 10);
    }

    ret = mbedtls_ssl_read(config->ssl_ctx, NULL, (size_t)NULL);
    if (ret < 0) {
#if !defined (__LIGHT_DPM_MANAGER__)
        if (((session_config_t *)config->sessionConfPtr)->sessionType == REG_TYPE_TCP_SERVER) {
            if ((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_TIMEOUT)) {
                status = ERR_TIMEOUT;
            } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                status = ERR_CONN;
            } else {
                status = ERR_UNKNOWN;
                DM_DBG_ERR(RED "[%s:%d] Failed to read ssl packet(0x%x:%d)\n" CLEAR,
                                __func__, __LINE__, -ret, status);
            }
        } else if (((session_config_t *)config->sessionConfPtr)->sessionType == REG_TYPE_UDP_SERVER) {
            if ((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
                status = ERR_TIMEOUT;
            } else if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
                status = ERR_TIMEOUT;
            } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                status = ERR_CONN;
            } else {
                status = ERR_UNKNOWN;
                DM_DBG_ERR(RED "[%s:%d] Failed to read ssl packet(0x%x:%d)\n" CLEAR,
                                __func__, __LINE__, -ret, status);
            }
        } else
#endif    // !__LIGHT_DPM_MANAGER__
        if (((session_config_t *)config->sessionConfPtr)->sessionType == REG_TYPE_TCP_CLIENT) {
            if ((ret == MBEDTLS_ERR_SSL_WANT_WRITE) || (ret == MBEDTLS_ERR_SSL_TIMEOUT)) {
                status = ERR_TIMEOUT;
            } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                status = ERR_CONN;
            } else {
                status = ERR_UNKNOWN;
                DM_DBG_ERR(RED "[%s:%d] Failed to read ssl packet(0x%x:%d)\n" CLEAR,
                                                                __func__, __LINE__, -ret, status);
            }
        } else if (((session_config_t *)config->sessionConfPtr)->sessionType == REG_TYPE_UDP_CLIENT) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_TIMEOUT:
                status = ERR_TIMEOUT;
                break;

            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                DM_DBG_INFO(" [%s:%d] Need more data \n", __func__, __LINE__);
                status = ERR_TIMEOUT;
                break;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                DM_DBG_INFO(" [%s:%d] Connection was closed gracefully \n", __func__, __LINE__);
                status = ERR_CONN;
                break;

            case MBEDTLS_ERR_NET_CONN_RESET:
                DM_DBG_INFO(" [%s:%d] Connection was reset by peer \n", __func__, __LINE__);
                status = ERR_CONN;
                break;

            default:
                DM_DBG_INFO(" [%s:%d] Failed to recv data (ret:0x%x) \n", __func__, __LINE__, -ret);
                status = ERR_UNKNOWN;
                break;
            }
        } else {
            DM_DBG_ERR(RED "[%s] Unknown type (type:%d, ret:0x%02x)\n",
                        __func__,
                        ((session_config_t *)config->sessionConfPtr)->sessionType,
                        -ret);
        }

        goto end_of_func;
    }

    recv_len = mbedtls_ssl_get_bytes_avail(config->ssl_ctx);

    if (recv_len == 0) {
        status = ERR_TIMEOUT;
        goto end_of_func;
    }

    recv_buf = pvPortMalloc(recv_len);
    if (!recv_buf) {
        DM_DBG_ERR(RED "[%s:%d] Failed to allocate recv packet\n" CLEAR, __func__, __LINE__);
        status = ERR_MEM;
        goto end_of_func;
    }

    ret = mbedtls_ssl_read(config->ssl_ctx, recv_buf, recv_len);

    if (ret < 0) {
        switch (ret) {
        case MBEDTLS_ERR_SSL_TIMEOUT:
            DM_DBG_INFO(" [%s] Timeout \n", __func__);
            status = ERR_TIMEOUT;
            break;

        case MBEDTLS_ERR_SSL_WANT_READ:
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            DM_DBG_INFO(" [%s] Need more data(0x%x) \n", __func__, -ret);
            status = ERR_TIMEOUT;
            break;

        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
            DM_DBG_INFO(" [%s] Connection was closed gracefully \n", __func__ );
            status = ERR_CONN;
            break;

        case MBEDTLS_ERR_NET_CONN_RESET:
            DM_DBG_INFO(" [%s] Connection was reset by peer \n", __func__);
            status = ERR_CONN;
            break;

        default:
            DM_DBG_INFO(" [%s] Failed to recv data(0x%x) \n", __func__, -ret);
            status = ERR_UNKNOWN;
            break;
        }

        DM_DBG_ERR(RED "[%s:%d] Failed to read ssl packet(0x%x)\n" CLEAR, __func__, __LINE__, -ret);
        goto end_of_func;
    }

    DM_DBG_INFO(" [%s:%d] Read ssl packet(0x%x) \n", __func__, __LINE__, recv_len);
    memcpy(packet_ptr, recv_buf, recv_len);
    status = (int)recv_len;

end_of_func:

    if (recv_buf) {
        vPortFree(recv_buf);
    }

    return status;
}

static int dmSecureRecvTimeoutCallback(void *ctx, unsigned char *buf, size_t len, uint32_t timeout)
{
    SECURE_INFO_T        *config = (SECURE_INFO_T *)ctx;
    session_config_t    *sessConfPtr = config->sessionConfPtr;

#ifdef DM_DEBUG_INFO
    int        socket = 0;
#endif    //DM_DEBUG_INFO
    int        status = MBEDTLS_ERR_NET_RECV_FAILED;

#ifdef DM_DEBUG_INFO
    socket = sessConfPtr->socket;
#endif    //DM_DEBUG_INFO

    if (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER) {
        DM_DBG_INFO(RED "[%s] Not support TCP_SERVER \n" CLEAR, __func__);
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER) {
        DM_DBG_INFO(CYAN "[%s] socket:%d config->client_ctx->fd:%d buf:0x%x len:%d \n" CLEAR,
                          __func__, socket, config->client_ctx->fd, buf, len);
        status = mbedtls_net_recv_timeout(config->client_ctx, buf, len, timeout);    //mbedtls_net_bsd_recv_timeout
    } else if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
        DM_DBG_INFO(RED "[%s] Not support TCP_CLIENT \n" CLEAR, __func__);
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT) {
        DM_DBG_INFO(CYAN "[%s] socket:%d config->server_ctx->fd:%d buf:0x%x len:%d \n" CLEAR,
                          __func__, socket, config->server_ctx->fd, buf, len);
        status = mbedtls_net_recv_timeout(config->server_ctx, buf, len, timeout);    // mbedtls_net_bsd_recv_timeout
    } else {
        DM_DBG_ERR(RED "[%s] Type error (%d)\n" CLEAR, __func__, sessConfPtr->sessionType);
        errorCallback(DM_ERROR_INVALID_TYPE, "Fail session type");
        return ERR_ARG;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// end Secure //////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined (__LIGHT_DPM_MANAGER__)
//
// start dpm TCP Server Manager
//

int sendTcpServer(session_config_t *sessConfPtr, int sub_idx, char *data_buf, UINT len_buf)
{
    UINT        ret;
    UINT        status;
    UINT        waitSendTime = 0;
    SECURE_INFO_T    *config = NULL;
    int            tcp_tx_sock;

    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;

    DM_DBG_INFO(CYAN " [%s] send TCP packet(sub_idx:%d buf:0x%x len:%d) \n" CLEAR,
                                                    __func__, sub_idx, data_buf, len_buf);

    /* At this case,,, dpm_sleep operation was started already */
    if (chk_dpm_pdown_start()) {
        DM_DBG_ERR(RED "[%s] Error - In progressing DPM sleep !!!" CLR_COL, __func__);
        return ER_NOT_SUCCESSFUL;
    }

    tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[sub_idx];

    setPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);

retry_check:
    if (tcpRxSubTaskConfPtr->runStatus != SESSION_STATUS_RUN) {
        if (++waitSendTime <= 100) {
            vTaskDelay(2);
            goto retry_check;
        }

        DM_DBG_ERR(RED "[%s] Not ready to send \n" CLEAR, __func__);
        clrPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
        return ER_NOT_ENABLED;
    }

    tcp_tx_sock = tcpRxSubTaskConfPtr->clientSock;

#if defined ( __RUNTIME_CALCULATION__ )
    printCurrTickDm("Send - start");
#endif    // __RUNTIME_CALCULATION__

    DM_DBG_INFO(CYAN " [%s] send Packet(len_buf:%d) \n" CLEAR, __func__, len_buf);
#ifdef FOR_DEBUGGING_PKT_DUMP
    hexa_dump_print("send Packet ", data_buf, len_buf, 0, 2);
#endif

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        config = &tcpRxSubTaskConfPtr->secureConfig;
        if (config->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
#ifdef __SUPPORT_DPM_TCP_KEEPALIVE__
            if (sessConfPtr->sessionKaInterval) {
                rtcRegistTcpServerKeepaliveTimer(tcpRxSubTaskConfPtr);
            }

            if ((data_buf == NULL) && (len_buf == 0xFFFFFFFF)) {      // Keepalive
                status = (UINT)send_tcp_keepalive_dpm(tcp_tx_sock);
                DM_DBG_INFO(CYAN " [%s] Send keepalive Packet(status:%d) \n" CLEAR, __func__, status);
                clrPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
                return ER_SUCCESS;
            }
#endif
            status = dmSecureSend(config, data_buf, len_buf, TCP_SERVER_DEF_WAIT_TIME);
            if (status) {
                DM_DBG_ERR(RED "[%s] Failed TCP send packet(buf:0x%x len:%d sock_fd:%d) \n" CLEAR,
                                __func__, data_buf, len_buf, tcp_tx_sock);

                errorCallback(DM_ERROR_SEND_FAIL, "Fail to tcp server socket send");
                sessConfPtr->requestCmd = CMD_REQUEST_RESTART;
            } else {
                ret = tls_save_ssl(tcpRxSubTaskConfPtr->tcpRxSubTaskName, config);
                tcpRxSubTaskConfPtr->lastAccessRTC = RTC_GET_COUNTER() >> 15;
                if (ret) {
                    DM_DBG_ERR("[%s] Failed to save tls session(0x%02x)\n", __func__, ret);
                }
            }
        } else {
            DM_DBG_ERR(RED "[%s] Not yet handshake \n" CLEAR, __func__, data_buf, len_buf);
            status = ER_NOT_SUCCESSFUL;
        }

        clrPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
        return status;
    }

#ifdef __SUPPORT_DPM_TCP_KEEPALIVE__
    if (sessConfPtr->sessionKaInterval) {
        rtcRegistTcpServerKeepaliveTimer(tcpRxSubTaskConfPtr);
    }

    if ((data_buf == NULL) && (len_buf == 0xFFFFFFFF)) {      // Keepalive
        status = (UINT)send_tcp_keepalive_dpm(tcp_tx_sock);
        DM_DBG_INFO(CYAN " [%s] Send keepalive Packet(status:%d) \n" CLEAR, __func__, status);
        clrPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
        return ER_SUCCESS;
    }
#endif

    // Send data to TCP client
    status = send(tcp_tx_sock, data_buf, len_buf, 0);
    if (status <= 0) {  // NOT CONNECT
        DM_DBG_ERR(RED "[%s] tcp socket send error (0x%x)\n" CLEAR, __func__, status);

        errorCallback(DM_ERROR_SEND_FAIL, "Fail to tcp server socket send");
        sessConfPtr->requestCmd = CMD_REQUEST_RESTART;

        clrPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
        return ER_NOT_CONNECTED;
    } else {
        tcpRxSubTaskConfPtr->lastAccessRTC = RTC_GET_COUNTER() >> 15;
    }

#if defined ( FOR_DEBUGGING_PKT )
    printCurrTickDm("Send - end");
#endif    // FOR_DEBUGGING_PKT

    clrPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER);
    return ER_SUCCESS;
}

void dpmTcpServerKeepaliveCallback(int sub_idx, char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    BaseType_t    xHigherPriorityTaskWoken, xResult;
    UINT        event;

    if (sub_idx == 0) {
        event = DPM_EVENT_TCP_S_CB_TIMER1;
    } else if (sub_idx == 1) {
        event = DPM_EVENT_TCP_S_CB_TIMER2;
    } else if (sub_idx == 2) {
        event = DPM_EVENT_TCP_S_CB_TIMER3;
    } else if (sub_idx == 3) {
        event = DPM_EVENT_TCP_S_CB_TIMER4;
    } else {
        DM_DBG_ERR(RED "[%s] sub_idx no error (%d) \n" CLEAR, __func__, sub_idx);
        return;
    }

    if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
        DM_DBG_INFO(" [%s] Call TCP Server CB timer event 0x%x (FromISR) \n", __func__, event);
        xHigherPriorityTaskWoken = pdFALSE;

        xResult = xEventGroupSetBitsFromISR(dpmManagerEventGroup, event, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
              * switch should be requested. The macro used is port specific and will
              * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
              * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        DM_DBG_INFO(" [%s] Call TCP Server CB timer event 0x%x \n", __func__, event);
        xEventGroupSetBits(dpmManagerEventGroup, event);
    }
}

void dpm_tcps_ka_cb_sub0(char *timer_name)
{
    dpmTcpServerKeepaliveCallback(0, timer_name);
}

void dpm_tcps_ka_cb_sub1(char *timer_name)
{
    dpmTcpServerKeepaliveCallback(1, timer_name);
}

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 2)
void dpm_tcps_ka_cb_sub2(char *timer_name)
{
    dpmTcpServerKeepaliveCallback(2, timer_name);
}
#endif

#if defined(DM_TCP_SVR_MAX_SESS) && (DM_TCP_SVR_MAX_SESS > 3)
void dpm_tcps_ka_cb_sub3(char *timer_name)
{
    dpmTcpServerKeepaliveCallback(3, timer_name);
}
#endif

static void dpmTcpSvrKaCbSubSess(UINT subSessionId)
{
    int subSessionIndex = subSessionId-1;
    session_config_t   *sessConfPtr;
    tcpRxSubTaskConf_t *tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[subSessionIndex];

    DM_DBG_INFO(YELLOW_COLOR " [%s] #%d sub Keep-Alive \n" CLEAR_COLOR, __func__, subSessionId);

    sessConfPtr = tcpRxSubTaskConfPtr->sessConfPtr;
    if (sessConfPtr == NULL) {
        DM_DBG_ERR(RED_COLOR " [%s] Error not found tcp server session \n" CLEAR_COLOR, __func__);
        return;
    }

    if ((tcpRxSubTaskConfPtr->allocFlag == 0) || (tcpRxSubTaskConfPtr->clientSock < 0)) {
        DM_DBG_INFO(RED_COLOR " [%s] Keep-Alive not alloc_flag(%d,%d) \n" CLEAR_COLOR,
            __func__, tcpRxSubTaskConfPtr->allocFlag, tcpRxSubTaskConfPtr->clientSock);
        return;
    }

#ifdef __SUPPORT_DPM_TCP_KEEPALIVE__
    // Send TCP keep-alive packet
    sendTcpServer(sessConfPtr, subSessionIndex, NULL, 0xFFFFFFFF);
#endif
}

void printSessionTcpSubIndex()    // TCP Server Only
{
    int    index;
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;

    PRINTF(" << tcpRxSubTask config>> \n");
    tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[0];

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {

        PRINTF(" %d) Flag:%d runSt:%d No:%d name:%s peerP:%d peerIp:%s socket:%d Buffer:0x%x la_tick:%d \n",
                  index,
                  tcpRxSubTaskConfPtr->allocFlag,
                  tcpRxSubTaskConfPtr->runStatus,
                  tcpRxSubTaskConfPtr->subIndex,
                  tcpRxSubTaskConfPtr->tcpRxSubTaskName,
                  tcpRxSubTaskConfPtr->peerPort,
                  tcpRxSubTaskConfPtr->peerIpAddr,
                  tcpRxSubTaskConfPtr->clientSock,
                  tcpRxSubTaskConfPtr->tcpServerSessDataBuffer,
                  tcpRxSubTaskConfPtr->lastAccessRTC);


        tcpRxSubTaskConfPtr++;
    }
}

void initDoneSubTask(void)    // TCP Server Only
{
    int    index;
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;

    tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[0];

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        if (tcpRxSubTaskConfPtr->allocFlag) {
            dpm_app_wakeup_done(tcpRxSubTaskConfPtr->tcpRxSubTaskName);    // Noti to timer (Can receive timer ISR)
            PRINTF(" %d) init_done subTask allocFlag:%d runSt:%d name:%s \n",
                    tcpRxSubTaskConfPtr->subIndex,
                    tcpRxSubTaskConfPtr->allocFlag,
                    tcpRxSubTaskConfPtr->runStatus,
                    tcpRxSubTaskConfPtr->tcpRxSubTaskName);
        }

        tcpRxSubTaskConfPtr++;
    }
}

void clearTcpSubTaskRunStatus(void)
{
    int    index;

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        dpmUserConfigs.tcpRxTaskConfig[index].runStatus = SESSION_STATUS_STOP;
    }
}

void clearNotActTcpSubTask(void)
{
    int    index;
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[index];
        if (tcpRxSubTaskConfPtr->runStatus == SESSION_STATUS_STOP) {
            tcpRxSubTaskConfPtr->allocFlag = pdFALSE;
            memset(tcpRxSubTaskConfPtr, 0, sizeof(tcpRxSubTaskConf_t));
        }
    }
}

int getSessionTcpSubIndexByPort(ULONG ip, ULONG port)    // TCP Server Only
{
    DA16X_UNUSED_ARG(ip);

    int    index;
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;

    DM_DBG_INFO(" [%s] Start sub index search (ip:0x%x port:%d)\n" CLEAR, __func__, ip, port);

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[index];

        if (tcpRxSubTaskConfPtr->allocFlag == pdTRUE) {
            if (tcpRxSubTaskConfPtr->peerPort == port) {
                DM_DBG_INFO(" [%s] Find sub index %d (port:%d)\n" CLEAR, __func__, index, port);
                return index;
            }
        }
    }

    DM_DBG_INFO(" [%s] Not found sub index (ip:0x%x port:%d)\n", __func__, ip, port);

    return -1;
}

int getSessionTcpSubIndexBySocket(int socket)    // TCP Server Only
{
    int    index;
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;

    DM_DBG_INFO(" [%s] Start sub index search (socket: %d)\n" CLEAR, __func__, socket);

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[index];
        if (tcpRxSubTaskConfPtr->allocFlag == pdTRUE) {
            if (tcpRxSubTaskConfPtr->clientSock == socket) {
                DM_DBG_INFO(" [%s] Find sub index %d (socket:%d)\n" CLEAR, __func__, index, socket);
                return index;
            }
        }
    }

    DM_DBG_INFO(" [%s] Not found sub index (socket:%d)\n", __func__, socket);

    return -1;
}

static int getEmptyTcpSubIndex(session_config_t *sessConfPtr)
{
    int    index;

    // Find empty index
    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        if (sessConfPtr->tcpSubInfo[index]->allocFlag == pdFALSE) {
            break;
        }
    }

    return index;
}

static int getOldestAccessTcpSubIndex(session_config_t *sessConfPtr)
{
    int   index;
    ULONG lastAccessRTC = 0xFFFFFFFF;

    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        if (lastAccessRTC > sessConfPtr->tcpSubInfo[index]->lastAccessRTC) {
            lastAccessRTC = sessConfPtr->tcpSubInfo[index]->lastAccessRTC;
        }
    }

    DM_DBG_INFO(" [%s] lastAccessRTC :%d \n", __func__, lastAccessRTC);
    for (index = 0; index < DM_TCP_SVR_MAX_SESS; index++) {
        if (lastAccessRTC == sessConfPtr->tcpSubInfo[index]->lastAccessRTC) {
            break;
        }
    }

    if (index != DM_TCP_SVR_MAX_SESS) {
        DM_DBG_INFO(" [%s] return sub_idx:%d lastAccessRTC :%d \n", __func__,
                    index, sessConfPtr->tcpSubInfo[index]->lastAccessRTC);
    } else {
        index = -1;
    }

    return index;
}

void tcpServerSubTask(void *pvParameters)    // TASK
{
    int sys_wdog_id = -1;
    int   status;
    UINT  dm_tcp_no_packet_count = 0;
    int   rx_data_len;
    struct timeval sockopt_timeout;
#ifdef    DM_DEBUG_INFO
    UINT  stopReason;
#endif    //DM_DEBUG_INFO
    char  *tcp_rx_pkt = NULL;
    tcpRxSubTaskConf_t *tcpRxSubTaskConfPtr;
    session_config_t   *sessConfPtr;
    SECURE_INFO_T      *config;
    UINT  delayCountBySendingFlag = 0;
    struct netif       *netif;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    // Set the pointer.
    tcpRxSubTaskConfPtr = (tcpRxSubTaskConf_t *)pvParameters;
    sessConfPtr = tcpRxSubTaskConfPtr->sessConfPtr;
    config = &tcpRxSubTaskConfPtr->secureConfig;

    DM_DBG_INFO(CYAN " [%s] Started subIndex:%d by client(%s:%d)...\n" CLEAR,
        __func__, tcpRxSubTaskConfPtr->subIndex, tcpRxSubTaskConfPtr->peerIpAddr, tcpRxSubTaskConfPtr->peerPort);

tcp_server_sub_restart:

    da16x_sys_watchdog_notify(sys_wdog_id);

    tcpRxSubTaskConfPtr->allocFlag = pdTRUE;
    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_INIT;

    // DPM register sub-task
    dpm_app_register(tcpRxSubTaskConfPtr->tcpRxSubTaskName, tcpRxSubTaskConfPtr->peerPort);

    // Set receiving possible flag for master port
    dpm_app_data_rcv_ready_set(tcpRxSubTaskConfPtr->tcpRxSubTaskName);        // Noti to umac(Can receive packet)

    config = &tcpRxSubTaskConfPtr->secureConfig;
    config->subIndex = tcpRxSubTaskConfPtr->subIndex;
    config->sessionConfPtr = sessConfPtr;

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        config->sessionType = sessConfPtr->sessionType;
        config->localPort = sessConfPtr->sessionMyPort;

        netif = netif_get_by_index(2);    // WLAN0:2, WLAN1:3
        sprintf(config->localIp, "%d.%d.%d.%d",
                                ip4_addr1_16(&netif->ip_addr),
                                ip4_addr2_16(&netif->ip_addr),
                                ip4_addr3_16(&netif->ip_addr),
                                ip4_addr4_16(&netif->ip_addr));

        DM_DBG_INFO(" [%s] type:%d local address: %s:%d \n",
                    __func__, config->sessionType, config->localIp, config->localPort);

        config->timeout = TCP_SERVER_DEF_WAIT_TIME;

        ssl_ctx_clear(config);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        status = dmSecureInit(config);    // TCP SERVER
        if (status) {
            DM_DBG_ERR(" [%s] Failed to init ssl(0x%02x) \n", __func__, status);

            errorCallback(DM_ERROR_SECURE_INIT_FAIL, "Fail to secure init");
#ifdef    DM_DEBUG_INFO
            stopReason = DM_ERROR_SECURE_INIT_FAIL;
#endif

            tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

            goto tcpRxSubTaskEnd;
        }

        setupSecureCallback(sessConfPtr, config);    // call back : secure config setup

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        status = tls_restore_ssl(tcpRxSubTaskConfPtr->tcpRxSubTaskName, config);
        if (status) {
            DM_DBG_ERR(" [%s:%d] Failed to restore ssl(0x%02x)\n", __func__, __LINE__);

            errorCallback(DM_ERROR_RESTORE_SSL_FAIL, "Fail to restore ssl");
#ifdef    DM_DEBUG_INFO
            stopReason = DM_ERROR_RESTORE_SSL_FAIL;
#endif

            tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

            goto tcpRxSubTaskEnd;
        }

        mbedtls_ssl_set_bio(config->ssl_ctx, (void *)config, dmSecureSendCallback, dmSecureRecvCallback, NULL);

        DM_DBG_INFO(" [%s] set_bio init done \n", __func__);
    }

    if (sessConfPtr->supportSecure) {
        sockopt_timeout.tv_sec = 0;
        sockopt_timeout.tv_usec = TCP_SVR_RX_SECURE_TIMEOUT * 1000;
    } else {
        sockopt_timeout.tv_sec = 0;
        sockopt_timeout.tv_usec = TCP_SVR_RX_TIMEOUT * 1000;
    }

    status = setsockopt(tcpRxSubTaskConfPtr->clientSock,
                        SOL_SOCKET,
                        SO_RCVTIMEO,
                        &sockopt_timeout,
                        sizeof(sockopt_timeout));
    if (status != 0) {
        DM_DBG_ERR(RED "[%s] Failed to set socket option - SO_RCVTIMEOUT(%d:%d) \n" CLEAR,
                            __func__, status, errno);
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        da16x_sys_watchdog_suspend(sys_wdog_id);

        status = dmSecureHandshake(config);        // TCP Server handshake
        if (status) {
            DM_DBG_ERR(" [%s] Failed to process TLS handshake(0x%x)\n", __func__, status);

            errorCallback(DM_ERROR_HANDSHAKE_FAIL, "Fail to handshake");
#ifdef    DM_DEBUG_INFO
            stopReason = DM_ERROR_HANDSHAKE_FAIL;
#endif

            tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

            goto tcpRxSubTaskEnd;
        } else {
            DM_DBG_INFO(BLUE " [%s:%d] Success TLS handshake \n" CLEAR, __func__, __LINE__);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        status = tls_save_ssl(tcpRxSubTaskConfPtr->tcpRxSubTaskName, config);
        if (status) {
            DM_DBG_ERR(" [%s:%d] Failed to save tls session(0x%02x)\n", __func__, __LINE__, status);
        }
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    connectCallback(sessConfPtr, 0);    // Connected

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_CONNECTED;

    sessConfPtr->connSessionId = tcpRxSubTaskConfPtr->subIndex;
    DM_DBG_INFO(" [%s] connSessionId: %d \n", __func__, sessConfPtr->connSessionId);

    if (sessConfPtr->sessionKaInterval) {
        int i;

        // For TCP Keep-Alive ...
        i = 0;
        i = setsockopt(tcpRxSubTaskConfPtr->clientSock, SOL_SOCKET, SO_KEEPALIVE, (char *)&i, sizeof(i));
        if (i < 0) {

            errorCallback(DM_ERROR_SETSOCKOPT_FAIL, "Fail to tcp setsockopt");
#ifdef    DM_DEBUG_INFO
            stopReason = DM_ERROR_SETSOCKOPT_FAIL;
#endif
            tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

            goto tcpRxSubTaskEnd;
        }

#if defined ( SUPPORT_RTM_TCP_KEEPALIVE )
        // Save TCP keep-alive timer id to RTM area
        dpm_tcp_socket_keep_alive_set(tcpRxSubTaskConfPtr->tcpRxSubTaskName,
                                    tcpRxSubTaskConfPtr->subSessionKaTid);
#endif    // SUPPORT_RTM_TCP_KEEPALIVE
    }

    // Receive data
    if (tcp_rx_pkt == NULL) {
        tcpRxSubTaskConfPtr->tcpServerSessDataBuffer = pvPortMalloc(TCP_SERVER_RX_BUF_SIZE);
        tcp_rx_pkt = (char *)tcpRxSubTaskConfPtr->tcpServerSessDataBuffer;
    }

    if (!tcp_rx_pkt) {
        DM_DBG_ERR(RED "[%s] Failed to allocate memory ...\n" CLEAR, __func__);

        errorCallback(DM_ERROR_RX_BUFFER_ALLOC_FAIL, "Fail to rx buffer");
#ifdef    DM_DEBUG_INFO
        stopReason = DM_ERROR_RX_BUFFER_ALLOC_FAIL;
#endif
        tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

        goto tcpRxSubTaskEnd;
    }

    // Notice initialize done to DPM module
    dpm_app_wakeup_done(tcpRxSubTaskConfPtr->tcpRxSubTaskName);            // Noti to timer (Can receive timer ISR)
    dpm_app_data_rcv_ready_set(tcpRxSubTaskConfPtr->tcpRxSubTaskName);    // Noti to umac(Can receive packet)

    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_RUN;
    DM_DBG_INFO(" [%s] STATUS_RUN sub_idx:%d \n", __func__, tcpRxSubTaskConfPtr->subIndex);

    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        /* If DPM power down scheme was already started,
          * skip next tcp rx operataion.
          */
        if (dpm_sleep_is_started()) {
            vTaskDelay(10);
            continue;
        }

        if (sessConfPtr->supportSecure) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            status = dmSecureRecv(config, tcp_rx_pkt, SSL_READ_TIMEOUT);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            if (status == ERR_TIMEOUT) {
                rx_data_len = -1;
            } else if (status == ERR_CONN) {
                rx_data_len = 0;
            } else if (status == ERR_UNKNOWN) {
                rx_data_len = 0;
            } else {
                rx_data_len = status;
            }
        } else {
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

#ifdef SESSION_MUTEX
            xSemaphoreTake(sessConfPtr->socketMutex, portMAX_DELAY);
#endif
            rx_data_len = recv(tcpRxSubTaskConfPtr->clientSock, tcp_rx_pkt, TCP_SERVER_RX_BUF_SIZE, 0);
#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }

        if (rx_data_len > 0) {
            __clr_dpm_sleep(tcpRxSubTaskConfPtr->tcpRxSubTaskName, (char *)__func__, (int) __LINE__);

            dm_tcp_no_packet_count = 0;

            if (rx_data_len > TCP_SERVER_RX_BUF_SIZE) {
                DM_DBG_ERR(" [%s] Failed to packet size (size:%d)\n", __func__, rx_data_len);
                tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_INIT;
                break;
            }

            *(tcp_rx_pkt+rx_data_len) = 0;

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            receiveCallback(sessConfPtr,
                            (char *)tcp_rx_pkt,
                            rx_data_len,
                            iptolong(tcpRxSubTaskConfPtr->peerIpAddr),
                            tcpRxSubTaskConfPtr->peerPort);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            if (sessConfPtr->sessionKaInterval) {
                // Create RTC timer for TCP keep-alive on DPM mode
                rtcRegistTcpServerKeepaliveTimer(tcpRxSubTaskConfPtr);
                if (tcpRxSubTaskConfPtr->subSessionKaTid == 0) {
                    da16x_sys_watchdog_notify(sys_wdog_id);

                    da16x_sys_watchdog_suspend(sys_wdog_id);

                    errorCallback(DM_ERROR_TIMER_CREATE, "Fail to create ka timer");

                    da16x_sys_watchdog_suspend(sys_wdog_id);

#ifdef    DM_DEBUG_INFO
                    stopReason = DM_ERROR_TIMER_CREATE;
#endif
                    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

                    goto tcpRxSubTaskEnd;
                }

#if defined ( SUPPORT_RTM_TCP_KEEPALIVE )
                // Save TCP keep-alive timer id to RTM area
                dpm_tcp_socket_keep_alive_delete(tcpRxSubTaskConfPtr->tcpRxSubTaskName, tcpRxSubTaskConfPtr->subSessionKaTid);
                dpm_tcp_socket_keep_alive_set(tcpRxSubTaskConfPtr->tcpRxSubTaskName, tcpRxSubTaskConfPtr->subSessionKaTid);
#endif    // SUPPORT_RTM_TCP_KEEPALIVE
            }

            tcpRxSubTaskConfPtr->lastAccessRTC = RTC_GET_COUNTER() >> 15;
        } else if (rx_data_len == 0) {       // DISCONN
            __clr_dpm_sleep(tcpRxSubTaskConfPtr->tcpRxSubTaskName, (char *)__func__, (int) __LINE__);
            DM_DBG_ERR(RED "[%s] NOT_CONNECTED (%s)\n" CLEAR, __func__, tcpRxSubTaskConfPtr->tcpRxSubTaskName);

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            connectCallback(sessConfPtr, DM_ERROR_SOCKET_NOT_CONNECT);    // Disconnected

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            // Enable to enter DPM SLEEP
            if (sessConfPtr->sessionKaInterval) {
                // Delete saved RTC timer information
                rtc_cancel_timer(tcpRxSubTaskConfPtr->subSessionKaTid);

#if defined ( SUPPORT_RTM_TCP_KEEPALIVE )
                dpm_tcp_socket_keep_alive_delete(tcpRxSubTaskConfPtr->tcpRxSubTaskName, tcpRxSubTaskConfPtr->subSessionKaTid);
#endif    // SUPPORT_RTM_TCP_KEEPALIVE
            }

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            errorCallback(DM_ERROR_SOCKET_NOT_CONNECT, "socket disconnect");

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

#if defined ( DM_DEBUG_INFO )
            stopReason = DM_ERROR_SOCKET_NOT_CONNECT;
#endif
            tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

            goto tcpRxSubTaskEnd;
        } else {  // no packet, timeout
            vTaskDelay(1);
            if (tcpRxSubTaskConfPtr->requestCmd == CMD_REQUEST_STOP) {
                DM_DBG_ERR(" [%s] Stop TCP Server by request \n", __func__, status);

                tcpRxSubTaskConfPtr->requestCmd = CMD_NONE;
#ifdef    DM_DEBUG_INFO
                stopReason = DM_ERROR_STOP_REQUEST;
#endif
                tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

                goto tcpRxSubTaskEnd;
            }

            if (tcpRxSubTaskConfPtr->requestCmd == CMD_REQUEST_RESTART) {
                DM_DBG_ERR(" [%s] Restart TCP Server by request \n", __func__, status);

                tcpRxSubTaskConfPtr->requestCmd = CMD_NONE;

                close(tcpRxSubTaskConfPtr->clientSock);
                shutdown(tcpRxSubTaskConfPtr->clientSock, SHUT_RDWR);
                tcpRxSubTaskConfPtr->clientSock = -1;

                if (sessConfPtr->supportSecure) {
                    dmSecureShutdown(config);
                    dmSecureDeinit(config);
                    tls_clear_ssl(tcpRxSubTaskConfPtr->tcpRxSubTaskName, config);
                }
#ifdef    DM_DEBUG_INFO
                stopReason = DM_ERROR_RESTART_REQUEST;
#endif
                tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_INIT;
                vTaskDelay(1);
                goto tcp_server_sub_restart;
            }

            if (dm_tcp_no_packet_count ++ > 30) {
                //DM_DBG_INFO(" [%s] ER_NO_PACKET (%d) \n" , __func__, rx_data_len);
                dm_tcp_no_packet_count = 0;
            }

            if (dpm_mode_is_enabled() == DPM_ENABLED) {
                if (sessConfPtr->connSessionId != tcpRxSubTaskConfPtr->subIndex) {
#if defined (__ALLOWED_MULTI_CONNECTION__)

#else    // __ALLOWED_MULTI_CONNECTION__
                    DM_DBG_ERR(RED "[%s] Disconnecting (conn_subIndex:%d, my_subIndex:%d)\n" CLEAR,
                                __func__, sessConfPtr->connSessionId, tcpRxSubTaskConfPtr->subIndex);

                    da16x_sys_watchdog_notify(sys_wdog_id);

                    da16x_sys_watchdog_suspend(sys_wdog_id);

                    connectCallback(sessConfPtr, status);

                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

#ifdef    DM_DEBUG_INFO
                    stopReason = DM_ERROR_CLOSED_CONNECTION;
#endif
                    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

                    goto tcpRxSubTaskEnd;
#endif    // __ALLOWED_MULTI_CONNECTION__
                }
            } else {
#if defined (__ALLOWED_MULTI_CONNECTION__)

#else    // __ALLOWED_MULTI_CONNECTION__
                if (connSessionId != tcpRxSubTaskConfPtr->subIndex) {
                    DM_DBG_ERR(RED "[%s] Disconnecting (connId:%d, myId:%d)\n" CLEAR,
                            __func__, connSessionId, tcpRxSubTaskConfPtr->subIndex);

                    da16x_sys_watchdog_notify(sys_wdog_id);

                    da16x_sys_watchdog_suspend(sys_wdog_id);

                    connectCallback(sessConfPtr, status);

                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

#ifdef    DM_DEBUG_INFO
                    stopReason = DM_ERROR_CLOSED_CONNECTION;
#endif
                    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

                    goto tcpRxSubTaskEnd;
                }
#endif    // __ALLOWED_MULTI_CONNECTION__
            }
        }

        if (   (getPacketSendingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER))
            || (getPacketReceivingStatus(tcpRxSubTaskConfPtr, REG_TYPE_TCP_SERVER)) ) {
            if (delayCountBySendingFlag++ < 100) {
                //DM_DBG_INFO(" [%s] send/recv pending (%d) \n", __func__, delayCountBySendingFlag);
                continue;
            } else {
                DM_DBG_ERR(RED "[%s] send/recv pending \n" CLEAR, __func__);
            }
            delayCountBySendingFlag = 0;
        }

        if (sessConfPtr->supportSecure) {
            status = tls_save_ssl(tcpRxSubTaskConfPtr->tcpRxSubTaskName, config);
            if (status) {
                DM_DBG_ERR(" [%s] Failed to save tls session(0x%02x)\n", __func__, status);
            }
        }

        // Enable to enter DPM SLEEP
        __set_dpm_sleep(tcpRxSubTaskConfPtr->tcpRxSubTaskName, (char *)__func__, (int)__LINE__);
    }

tcpRxSubTaskEnd:

    if (sessConfPtr->sessionKaInterval) {
        // Delete TCP keep-alive timer
        rtc_cancel_timer(tcpRxSubTaskConfPtr->subSessionKaTid);
    }

    if (tcpRxSubTaskConfPtr->tcpServerSessDataBuffer) {
        vPortFree(tcpRxSubTaskConfPtr->tcpServerSessDataBuffer);
        tcpRxSubTaskConfPtr->tcpServerSessDataBuffer = NULL;
    }

    close(tcpRxSubTaskConfPtr->clientSock);
    shutdown(tcpRxSubTaskConfPtr->clientSock, SHUT_RDWR);
    tcpRxSubTaskConfPtr->clientSock = -1;

    if (sessConfPtr->supportSecure) {
        dmSecureShutdown(config);
        dmSecureDeinit(config);
        tls_clear_ssl(tcpRxSubTaskConfPtr->tcpRxSubTaskName, config);
    }

    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_STOP;
    tcpRxSubTaskConfPtr->allocFlag = pdFALSE;

    DM_DBG_INFO(CYAN " [%s] Stop TCP server sub task by reason:0x%x (task no:%d) \n" CLEAR,
                                    __func__, stopReason, tcpRxSubTaskConfPtr->subIndex);

    dpm_app_unregister(tcpRxSubTaskConfPtr->tcpRxSubTaskName);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
    return;
}

static ULONG createtcpServerSubTask(session_config_t *sessConfPtr, int sub_idx)
{
    BaseType_t    status;
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr = sessConfPtr->tcpSubInfo[sub_idx];

    // Check existing completed-task
    if (tcpRxSubTaskConfPtr->tcpRxSubTaskHandler != pdFALSE) {
        // task terminate/delete
    } else {

    }

    tcpRxSubTaskConfPtr->subIndex = sub_idx;
    tcpRxSubTaskConfPtr->sessConfPtr = sessConfPtr;

    memset(tcpRxSubTaskConfPtr->tcpRxSubTaskName, 0, TASK_NAME_LEN);
    sprintf(tcpRxSubTaskConfPtr->tcpRxSubTaskName, "tcpRxSub%d", tcpRxSubTaskConfPtr->subIndex);

    status = xTaskCreate(tcpServerSubTask,
                        tcpRxSubTaskConfPtr->tcpRxSubTaskName,
                        SESSION_TASK_TCP_SUB_TASK_STACK_SIZE,
                        (void *)tcpRxSubTaskConfPtr,
                        TCP_SERVER_SESSION_TASK_PRIORITY,
                        &tcpRxSubTaskConfPtr->tcpRxSubTaskHandler);
    if (status != pdPASS) {
        DM_DBG_ERR(" [%s] Task create fail(%s) !!!\n", __func__, tcpRxSubTaskConfPtr->tcpRxSubTaskName);

        errorCallback(DM_ERROR_TASK_CREATE_FAIL, "Task create fail");
        return DM_ERROR_TASK_CREATE_FAIL;
    } else {
        return ERR_OK;
    }
}

static int runTcpServer(session_config_t *sessConfPtr)
{
    int                    status;
    int                    sub_idx;
    int                    bindErrorCnt = 0;
    struct sockaddr_in    server_addr;
    struct sockaddr_in    client_addr;
    int                    client_addrlen = sizeof(struct sockaddr_in);
    struct timeval        sockopt_timeout;
    UINT                stopReason;
#ifdef    DM_DEBUG_INFO
    ULONG                peerPort;
    ULONG                peerIP;
#endif
    tcpRxSubTaskConf_t    *tcpRxSubTaskConfPtr;
    int                    client_sock;
    int                    waitCount = 0;
    UINT                createdSubTaskCount = 0;
    UINT                acceptedSockCount = 0;
    int                    save_socket = -1;

tcp_server_restart:

    for (int i = 0; i < DM_TCP_SVR_MAX_SESS; i++) {
        sessConfPtr->tcpSubInfo[i] = &dpmUserConfigs.tcpRxTaskConfig[i];
        dpmUserConfigs.tcpRxTaskConfig[i].runStatus = SESSION_STATUS_STOP;
    }

    // Create tcp socket for new connection
    sessConfPtr->socket = socket_dpm(sessConfPtr->dpmRegName, PF_INET, SOCK_STREAM, 0);
    if (sessConfPtr->socket < 0) {
        DM_DBG_ERR(RED "[%s] Failed to create listen socket \n" CLEAR, __func__);

        __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int)__LINE__);

        errorCallback(DM_ERROR_SOCKET_CREATE_FAIL, "Fail to tcp socket create");
        stopReason = DM_ERROR_SOCKET_CREATE_FAIL;
        goto tcpServerEnd;
    }
    DM_DBG_INFO(" [%s] Create socket(%d) \n", __func__, sessConfPtr->socket);

    if (sessConfPtr->supportSecure) {
        sockopt_timeout.tv_sec = 0;
        sockopt_timeout.tv_usec = TCP_SVR_RX_SECURE_TIMEOUT * 1000;
    } else {
        sockopt_timeout.tv_sec = 0;
        sockopt_timeout.tv_usec = TCP_SVR_RX_TIMEOUT * 1000;
    }

    status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_RCVTIMEO, &sockopt_timeout, sizeof(sockopt_timeout));
    if (status != 0) {
        DM_DBG_ERR(RED "[%s] Failed to set socket option - SO_RCVTIMEOUT(%d:%d) \n" CLEAR,
                                                                __func__, status, errno);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(sessConfPtr->sessionMyPort);

    status = bind(sessConfPtr->socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (status == -1) {
        DM_DBG_ERR(RED "[%s] Failed to bind socket \n" CLEAR, __func__);

        close(sessConfPtr->socket);
        shutdown(sessConfPtr->socket, SHUT_RDWR);
        sessConfPtr->socket = -1;

        errorCallback(DM_ERROR_SOCKET_BIND_FAIL, "Fail to tcp socket bind");
        stopReason = DM_ERROR_SOCKET_BIND_FAIL;
        if (++bindErrorCnt < 5) {
            vTaskDelay(50);
            goto tcp_server_restart;
        } else {
            __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int)__LINE__);

            goto tcpServerEnd;
        }
    }
    DM_DBG_INFO(" [%s] Bind socket(%d) \n", __func__, sessConfPtr->socket);
    bindErrorCnt = 0;

    status = listen(sessConfPtr->socket, TCP_SVR_DPM_BACKLOG);
    if (status != 0) {
        DM_DBG_ERR(RED "[%s] Failed to listen socket of tcp server(status:%d) \n" CLEAR,
                                                                        __func__, status);
        close(sessConfPtr->socket);
        sessConfPtr->socket = -1;

        errorCallback(DM_ERROR_DPM_SOCKET_LISTEN_FAIL, "Fail to tcp socket listen");
        stopReason = DM_ERROR_DPM_SOCKET_LISTEN_FAIL;
        goto tcpServerEnd;
    }
    DM_DBG_INFO(" [%s] Listen socket(%d) \n", __func__, sessConfPtr->socket);

    clearTcpSubTaskRunStatus();

    dpm_app_wakeup_done(sessConfPtr->dpmRegName);                // Noti to timer (Can receive timer ISR)
    dpm_app_data_rcv_ready_set(sessConfPtr->dpmRegName);        // Noti to umac(Can receive packet)

    initDoneSubTask();

wait_tcp_accept:

    memset(&client_addr, 0x00, sizeof(struct sockaddr_in));
    client_addrlen = sizeof(struct sockaddr_in);

    sessConfPtr->runStatus = SESSION_STATUS_WAIT_ACCEPT;

    client_sock = accept(sessConfPtr->socket, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);
    if (client_sock < 0) {
        clearNotActTcpSubTask();

        // Enable to enter DPM SLEEP for master task
        __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int)__LINE__);

        goto wait_tcp_accept;
    } else {
        ++acceptedSockCount;
    }

    __clr_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int)__LINE__);

    DM_DBG_INFO(" [%s] Connected client 0x%x (%d.%d.%d.%d:%d) socket:%d \n",
                            __func__,
                            ntohl(client_addr.sin_addr.s_addr),
                            (ntohl(client_addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(client_addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(client_addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(client_addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(client_addr.sin_port)),
                            client_sock);
#ifdef    DM_DEBUG_INFO
    peerIP = ntohl(client_addr.sin_addr.s_addr);
    peerPort = ntohs(client_addr.sin_port);
#endif

    DM_DBG_INFO(" [%s] peerIP:0x%x peerPort:%d socket:%d \n", __func__, peerIP, peerPort, client_sock);

    sub_idx = getSessionTcpSubIndexBySocket(client_sock);
    if (sub_idx < 0) {  // No matching index found
        sub_idx = getEmptyTcpSubIndex(sessConfPtr);
        if (sub_idx >= DM_TCP_SVR_MAX_SESS) {      // Not found empty index
            if (createdSubTaskCount < DM_TCP_SVR_MAX_SESS) {
                save_socket = client_sock;
                DM_DBG_INFO(" [%s] Saved socket id: %d \n", __func__, save_socket);
                goto wait_tcp_accept;
            }

change_tcp_sub_session:

            // Not found empty socket
#ifdef WAITING_ALLOC_EMPTY_SUB_IDX
            goto wait_tcp_accept;
#else
            DM_DBG_INFO(RED "[%s] Not found empty socket (save_socket: %d) \n" CLEAR,
                                                                __func__, save_socket);
            sub_idx = getOldestAccessTcpSubIndex(sessConfPtr);
            if ((sub_idx >= 0) || (sub_idx < DM_TCP_SVR_MAX_SESS )) {
                tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[sub_idx];
                tcpRxSubTaskConfPtr->requestCmd = CMD_REQUEST_STOP;
                DM_DBG_INFO(" [%s] Request stop oldest sub_idx : %d \n", __func__, sub_idx);
            } else {
                DM_DBG_ERR(RED "[%s] Not found oldest sub_idx : %d \n" CLEAR, __func__, sub_idx);
                goto wait_tcp_accept;
            }
wait_stop:
            vTaskDelay(30);
            if (tcpRxSubTaskConfPtr->runStatus != SESSION_STATUS_STOP) {
                if (++waitCount < 10) {
                    DM_DBG_INFO(" [%s] wait... \n", __func__);
                    goto wait_stop;
                } else {
                    DM_DBG_INFO( RED "[%s] Timeout!!! stop oldest sub : %d \n", __func__, sub_idx);
                    goto wait_tcp_accept;
                }
            } else {
                // Stoped oldest sub task
                vTaskDelay(10);
            }
#endif
        }
    }

    tcpRxSubTaskConfPtr = &dpmUserConfigs.tcpRxTaskConfig[sub_idx];
    tcpRxSubTaskConfPtr->clientSock = client_sock;

    inet_ntop(AF_INET,                                // af
              &client_addr.sin_addr.s_addr,           // src *
              &tcpRxSubTaskConfPtr->peerIpAddr[0],    // dst *
              DM_IP_LEN);                             // size
    tcpRxSubTaskConfPtr->peerPort = lwip_htons(client_addr.sin_port);
    tcpRxSubTaskConfPtr->runStatus = SESSION_STATUS_INIT;

    // Create new task for sub-socket
    DM_DBG_INFO(CYAN " [%s] Create sub-task_#%d \n" CLEAR, __func__, sub_idx);
    status = createtcpServerSubTask(sessConfPtr, sub_idx);
    if (status != ERR_OK) {
        DM_DBG_ERR(RED "[%s] Failed to create sub-task_#%d (status=0x%x)...\n" CLEAR,
                                                            __func__, sub_idx, status);
    } else {
        if ( (++createdSubTaskCount == DM_TCP_SVR_MAX_SESS) && (save_socket > 0)) {
            client_sock = save_socket;
            DM_DBG_INFO(CYAN "[%s] goto change_tcp_sub_session (client_sock: %d) \n" CLEAR, __func__, client_sock);
            goto change_tcp_sub_session;
        }
    }

    vTaskDelay(3);
    goto wait_tcp_accept;

tcpServerEnd:

    dpm_app_unregister(sessConfPtr->dpmRegName);
    DM_DBG_INFO(" [%s] Stop TCP server by reason:0x%x (session no:%d) \n",
                            __func__, stopReason, sessConfPtr->sessionNo);
    sessConfPtr->runStatus = SESSION_STATUS_STOP;

    return stopReason;
}

static void dpmTcpServerManager(void *sessionNo)
{
    session_config_t *sessConfPtr = NULL;

    if (!(UINT)sessionNo || (UINT)sessionNo > MAX_SESSION_CNT) {
        DM_DBG_ERR(RED "[%s] Session Start Error(sessionNo:%d) \n" CLEAR, __func__, sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "Session start fail");
        goto TS_taskEnd;
    }

    sessConfPtr = &dpmUserConfigs.sessionConfig[(UINT)sessionNo - 1];
    sessConfPtr->sessionNo = (UINT)sessionNo;

    if (initSessionCommon(sessConfPtr)) {
        DM_DBG_ERR(RED "[%s] TCP Server Start Error(sessionNo:%d) \n" CLEAR,
                                                __func__, (UINT)sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "TCP server start fail");
        goto TS_taskEnd;
    }

    DM_DBG_INFO(CYAN " [%s] Started dpmTcpServerManager session no:%d name:%s myPort:%d \n" CLEAR,
        __func__, (UINT)sessionNo, sessConfPtr->dpmRegName, (unsigned short)sessConfPtr->sessionMyPort);

    runTcpServer(sessConfPtr);

TS_taskEnd:
    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        // Delete TCP Filter for dpm
        dpm_tcp_port_filter_delete(sessConfPtr->sessionMyPort);    // del_dpm_tcp_port_filter
    }
    deinitSessionCommon(sessConfPtr);

    vTaskDelete(NULL);
}

//
// end dpm M TCP Server Manager
//
#endif    // !__LIGHT_DPM_MANAGER__

//
// start dpm TCP Client Manager
//
static int sendTcpClient(session_config_t *sessConfPtr, char *data_buf, UINT len_buf)
{
    int     status;
    UINT    ret;
    int     tcp_tx_sock = 0;
    SECURE_INFO_T    *config = NULL;
    int     retryCount = 0;

    extern    int chk_dpm_rcv_ready(char *mod_name);

    DM_DBG_INFO(CYAN " [%s] send TCP packet(data_buf:0x%x, len_buf:%d) \n" CLEAR,
        __func__, data_buf, len_buf);

    /* At this case,,, dpm_sleep operation was started already */
    if (chk_dpm_pdown_start()) {
        DM_DBG_ERR(RED "[%s] Error - In progressing DPM sleep !!!" CLR_COL, __func__);
        return ER_NOT_SUCCESSFUL;
    }

    setPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);

    if (!sessionRunConfirm(sessConfPtr, 200)) {  // 2 sec wait for running status
        DM_DBG_ERR(RED "[%s] Not ready to send \n" CLEAR, __func__);
        clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
        return ER_NOT_ENABLED;
    }

confirm:

    if (dpm_mode_is_enabled() && (!chk_dpm_rcv_ready(sessConfPtr->dpmRegName))) {
        if (++retryCount < 50) {
            vTaskDelay(1);
            goto confirm;
        } else {
            DM_DBG_ERR(RED "[%s] send FAIL(%s) \n" CLEAR, __func__, sessConfPtr->dpmRegName);
            clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
            return ER_NOT_ENABLED;    // Not Enabled
        }
    }

    tcp_tx_sock = sessConfPtr->socket;
    if (tcp_tx_sock < 0) {
        DM_DBG_ERR(RED "[%s] Failed TCP send packet(socket: %d) \n" CLEAR, __func__, tcp_tx_sock);
        clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
        return ER_NOT_ENABLED;
    }

    DM_DBG_INFO(CYAN " [%s] send Packet(len_buf:%d) \n" CLEAR, __func__, len_buf);
#ifdef FOR_DEBUGGING_PKT_DUMP
    hexa_dump_print("send Packet ", data_buf, len_buf, 0, 2);
#endif

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        DM_DBG_INFO(CYAN "[%s] Send TLS Packet(data_buf:%s, len_buf:%d) \n" CLEAR, __func__, data_buf, len_buf);
        config = &sessConfPtr->secureConfig;

        if (config->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
#ifdef __SUPPORT_DPM_TCP_KEEPALIVE__
            if ((data_buf == NULL) && (len_buf == 0xFFFFFFFF)) {      // Keepalive
                status = (UINT)send_tcp_keepalive_dpm(tcp_tx_sock);
                DM_DBG_INFO(CYAN " [%s] Send keepalive Packet(status:%d) \n" CLEAR, __func__, status);
                clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
                return ER_SUCCESS;
            }
#endif

            status = (int)dmSecureSend(config, data_buf, len_buf, TCP_CLIENT_DEF_WAIT_TIME);
            if (status) {
                DM_DBG_ERR(RED "[%s] Failed TCP send packet(buf:0x%x len:%d s_fd:%d) \n"
                                            CLEAR, __func__, data_buf, len_buf, tcp_tx_sock);

                sessConfPtr->requestCmd = CMD_REQUEST_RESTART;    // request restart
            } else {
                ret = tls_save_ssl(sessConfPtr->sessTaskName, config);
                if (ret) {
                    DM_DBG_ERR(" [%s] Failed to save tls session(0x%02x)\n", __func__, ret);
                }
            }
        } else {
            DM_DBG_ERR(RED "[%s] Not yet handshake \n" CLEAR, __func__, data_buf, len_buf);
            status = ER_NOT_SUCCESSFUL;
        }

        clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
        return status;
    }

#ifdef __SUPPORT_DPM_TCP_KEEPALIVE__
    if ((data_buf == NULL) && (len_buf == 0xFFFFFFFF)) {      // Keepalive
        status = send_tcp_keepalive_dpm(tcp_tx_sock);
        DM_DBG_INFO(CYAN " [%s] Send keepalive packet(status:%d) \n" CLEAR, __func__, status);
        clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
        return ER_SUCCESS;
    }
#endif

    // Send data to TCP server
    status = send(tcp_tx_sock, data_buf, len_buf, 0);
    if (status <= 0) {  // NOT CONNECT
        DM_DBG_ERR(RED "[%s] tcp socket send error (0x%x)\n" CLEAR, __func__, status);

        close(tcp_tx_sock);
        errorCallback(DM_ERROR_SEND_FAIL, "Fail to tcp client socket send");

        clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
        return ER_NOT_CONNECTED;
    }

    clrPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT);
    return ER_SUCCESS;
}

void dpmTcpClientKeepaliveCallback(UINT sessionNo, char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    BaseType_t    xHigherPriorityTaskWoken, xResult;
    UINT        event;

    if (sessionNo == SESSION1) {
        event = DPM_EVENT_TCP_C_CB_TIMER1;
    } else if (sessionNo == SESSION2) {
        event = DPM_EVENT_TCP_C_CB_TIMER2;
    } else if (sessionNo == SESSION3) {
        event = DPM_EVENT_TCP_C_CB_TIMER3;
    } else if (sessionNo == SESSION4) {
        event = DPM_EVENT_TCP_C_CB_TIMER4;
    } else {
        DM_DBG_ERR(RED "[%s] Session no error \n" CLEAR, __func__);
        return;
    }

    if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
        DM_DBG_INFO(" [%s] Call TCP Client CB timer event:0x%x (FromISR) \n", __func__, event);
        xHigherPriorityTaskWoken = pdFALSE;

        xResult = xEventGroupSetBitsFromISR(dpmManagerEventGroup, event, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
              * switch should be requested. The macro used is port specific and will
              * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
              * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        DM_DBG_INFO(" [%s] Call TCP Client CB timer event:0x%x \n", __func__, event);
        xEventGroupSetBits(dpmManagerEventGroup, event);
    }
}

void dpm_tcpc_ka_cb_sess0(char *timer_name)
{
    dpmTcpClientKeepaliveCallback(SESSION1, timer_name);
}

void dpm_tcpc_ka_cb_sess1(char *timer_name)
{
    dpmTcpClientKeepaliveCallback(SESSION2, timer_name);
}

void dpm_tcpc_ka_cb_sess2(char *timer_name)
{
    dpmTcpClientKeepaliveCallback(SESSION3, timer_name);
}

void dpm_tcpc_ka_cb_sess3(char *timer_name)
{
    dpmTcpClientKeepaliveCallback(SESSION4, timer_name);
}

static void dpmTcpCliKaCbSess(UINT sessionNo)
{
    UINT  sessionIndex = sessionNo-1;
    session_config_t    *sessConfPtr = &dpmUserConfigs.sessionConfig[sessionIndex];
    int   tcpc_sock = sessConfPtr->socket;

    DM_DBG_INFO(YELLOW_COLOR " [%s] Session_#%d Keep-Alive \n" CLEAR_COLOR, __func__, sessionNo);

    if (tcpc_sock < 0) {
        DM_DBG_ERR(RED_COLOR " [%S] tcpc_sock is abnormal (%d) \n" CLEAR_COLOR, __func__, tcpc_sock);
        return;
    }

    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        // Set flag to permit goto DPM sleep mode
        __clr_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
    }

#ifdef __SUPPORT_DPM_TCP_KEEPALIVE__
    // Send TCP keep-alive packet
    sendTcpClient(sessConfPtr, NULL, 0xFFFFFFFF);        // send keepalive
    DM_DBG_INFO(YELLOW_COLOR " [%s] Send Keep-Alive Session_#%d \n" CLEAR_COLOR, __func__, sessionNo);
    //vTaskDelay(5);    // FOR_PRINT
#endif

    // Set flag to permit goto DPM sleep mode
    __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
}

static int runTcpClient(session_config_t *sessConfPtr)
{
    int         rx_data_len;
    char        *ip_addr_str;
    ULONG       tcp_server_ip_addr;
    int         tcp_server_port;
    int         tcp_local_port;
    int         status = ER_NOT_CREATED;
    UINT        connWaitTime = DEFAULT_CONN_WAIT_TIME;
    char        *ipStr;
    char        peerIp[DM_IP_LEN + 1];
    int         retryCount;
    int         stopReason = 0;
    SECURE_INFO_T    *config = &sessConfPtr->secureConfig;
    ULONG       dns_query_wait_option = 400;
    UINT        dm_tcp_no_packet_count = 0;
    struct timeval        sockopt_timeout;
    struct sockaddr_in    local_addr;
    struct sockaddr_in    srv_addr;
    UINT        delayCountBySendingFlag = 0;
    struct netif    *netif;
    int         bindErrorCnt = 0;

tcp_client_restart:

    config->sessionConfPtr = sessConfPtr;

    ip_addr_str = sessConfPtr->sessionServerIp;
    tcp_server_port = (int)sessConfPtr->sessionServerPort;
    tcp_local_port = (int)sessConfPtr->localPort;

    dpm_app_register(sessConfPtr->dpmRegName, sessConfPtr->sessionServerPort);

    // Allocate RX data buffer
    if (sessConfPtr->rxDataBuffer == NULL) {
        sessConfPtr->rxDataBuffer = pvPortMalloc(TCP_CLIENT_RX_BUF_SIZE+4);
        if (sessConfPtr->rxDataBuffer == NULL) {
            DM_DBG_ERR(RED "[%s] rxDataBuffer memory alloc fail ...\n" CLEAR, __func__);

            errorCallback(DM_ERROR_RX_BUFFER_ALLOC_FAIL, "TCP client Rx buffer memory alloc fail");
            stopReason = DM_ERROR_RX_BUFFER_ALLOC_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto tcpClientEnd;
        }
    }

    // Check IP address resolution status
    while (check_net_ip_status(interface_select)) {
        vTaskDelay(1);
    }

    DM_DBG_INFO(CYAN "[%s] TCP client start (name:%s svrIp:%s svrPort:%d local_port:%d) \n" CLEAR,
                      __func__,
                      sessConfPtr->dpmRegName,
                      sessConfPtr->sessionServerIp,
                      sessConfPtr->sessionServerPort,
                      tcp_local_port);

    if (!lwip_isdigit(ip_addr_str[0])) {  // Get ip address by domain
        retryCount = 0;
retryQuery:
        ipStr = dns_A_Query((char *)ip_addr_str, dns_query_wait_option);
        DM_DBG_INFO(" [%s] dns_A_quary by domain (%s => %s) \n", __func__, ip_addr_str, ipStr);

        if (ipStr == NULL) {
            if (retryCount++ < 3) {
                vTaskDelay(100);
                goto retryQuery;
            } else {
                DM_DBG_ERR(RED "[%s] Error: Not found TCP server address(%s)\n" CLEAR,
                        __func__, ip_addr_str);
            }
        } else {
            memset(peerIp, 0, DM_IP_LEN + 1);
            strcpy(peerIp, ipStr);
            ip_addr_str = peerIp;
        }
    }

    tcp_server_ip_addr = (ULONG)iptolong(ip_addr_str);

    sessConfPtr->socket = socket_dpm(sessConfPtr->dpmRegName, PF_INET, SOCK_STREAM, 0);
    if (sessConfPtr->socket < 0) {
        DM_DBG_ERR(RED "[%s] Failed to create socket \n" CLEAR, __func__);

        errorCallback(DM_ERROR_SOCKET_CREATE_FAIL, "TCP client socket creat fail");
        stopReason = DM_ERROR_SOCKET_CREATE_FAIL;
        sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
        goto tcpClientEnd;
    }

    DM_DBG_INFO(" [%s] Create socket(%d) \n", __func__, sessConfPtr->socket);

    if (sessConfPtr->supportSecure) {
        sockopt_timeout.tv_sec = 0;
        sockopt_timeout.tv_usec = TCP_CLI_RX_SECURE_TIMEOUT * 1000;
    } else {
        sockopt_timeout.tv_sec = 0;
        sockopt_timeout.tv_usec = TCP_CLI_RX_TIMEOUT * 1000;
    }

    status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_RCVTIMEO, &sockopt_timeout, sizeof(sockopt_timeout));
    if (status != 0) {
        DM_DBG_ERR(" [%s] Failed to set socket option - SO_RCVTIMEOUT(%d:%d) \n", __func__, status, errno);
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons((u16_t)tcp_local_port);

    status = bind(sessConfPtr->socket, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if (status == -1) {
        DM_DBG_ERR(RED "[%s] Failed to bind socket \n" CLEAR, __func__);

        errorCallback(DM_ERROR_SOCKET_BIND_FAIL, "TCP client socket bind error");
        if (++bindErrorCnt < 5) {
            vTaskDelay(50);
            goto tcp_client_restart;
        } else {
            stopReason = DM_ERROR_SOCKET_BIND_FAIL;
            goto tcpClientEnd_act;
        }
    }
    bindErrorCnt = 0;

    // To receive packets when DPM wakeup > TCP restore fail > new session
    dpm_app_data_rcv_ready_set(sessConfPtr->dpmRegName);        // Noti to umac(Can receive packet)

    // Connect to peer tcp server
    retryCount = 0;

    if (sessConfPtr->sessionConnWaitTime) {
        connWaitTime = 100 * sessConfPtr->sessionConnWaitTime;
    }

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        config->sessionType = sessConfPtr->sessionType;
        config->localPort = sessConfPtr->localPort;

        netif = netif_get_by_index(2);    // WLAN0:2, WLAN1:3
        sprintf(config->localIp, "%d.%d.%d.%d",
                                    ip4_addr1_16(&netif->ip_addr),
                                    ip4_addr2_16(&netif->ip_addr),
                                    ip4_addr3_16(&netif->ip_addr),
                                    ip4_addr4_16(&netif->ip_addr));

        DM_DBG_INFO(" [%s] type:%d local address: %s:%d \n",
                    __func__, config->sessionType, config->localIp, config->localPort);

        config->timeout = TCP_CLIENT_DEF_WAIT_TIME;

        ssl_ctx_clear(config);

        status = (int)dmSecureInit(config);    // TCP CLIENT
        if (status) {
            DM_DBG_ERR(" [%s] Failed to init ssl(0x%02x) \n", __func__, status);

            errorCallback(DM_ERROR_SECURE_INIT_FAIL, "TCP client secure init fail");
            stopReason = DM_ERROR_SECURE_INIT_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto tcpClientEnd_act;
        }

        setupSecureCallback(sessConfPtr, config);    // call back : secure config setup

        status = (int)tls_restore_ssl(sessConfPtr->sessTaskName, config);
        if (status) {
            DM_DBG_ERR(" [%s:%d] Failed to restore ssl(0x%02x)\n", __func__, __LINE__);

            errorCallback(DM_ERROR_RESTORE_SSL_FAIL, "TCP client restore ssl fail");
            stopReason = DM_ERROR_RESTORE_SSL_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto tcpClientEnd_act;
        }

        mbedtls_ssl_set_bio(config->ssl_ctx,
                            (void *)config,
                            dmSecureSendCallback,
                            dmSecureRecvCallback,
                            NULL);

        DM_DBG_INFO(" [%s] set_bio init done \n", __func__);
    }

conn_retry:

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(ip_addr_str);
    srv_addr.sin_port = htons((u16_t)tcp_server_port);

    DM_DBG_INFO(" [%s] Request connect to %s:%d \n", __func__, ip_addr_str, tcp_server_port);
    status = connect(sessConfPtr->socket, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in));
    if (status < ERR_OK) {
        connectCallback(sessConfPtr, DM_ERROR_SOCKET_CONNECT_FAIL);
        if ((++retryCount < (int)sessConfPtr->sessionConnRetryCnt) && (sessConfPtr->requestCmd == CMD_NONE)) {
            vTaskDelay(connWaitTime);

            goto conn_retry;
        }

        DM_DBG_ERR(RED "[%s] tcp socket connect fail (%d)\n" CLEAR, __func__, status);

        errorCallback(DM_ERROR_SOCKET_CONNECT_FAIL, "TCP client connect fail");
        stopReason = DM_ERROR_SOCKET_CONNECT_FAIL;
        sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
        goto tcpClientEnd_act;
    }

    sessConfPtr->runStatus = SESSION_STATUS_CONNECTED;
    connectCallback(sessConfPtr, 0);    // Connected
    DM_DBG_INFO("[%s] Connected server_ip:%s server_port:%d ka_interval:%d socket:%d \n",
                 __func__,
                 sessConfPtr->sessionServerIp,
                 sessConfPtr->sessionServerPort,
                 sessConfPtr->sessionKaInterval,
                 sessConfPtr->socket);

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        status = (int)dmSecureHandshake(config);    // TCP Client handshake
        if (status) {
            DM_DBG_ERR(RED "[%s] Failed to establish tls session. Retry connecting(0x%02x) \n" CLEAR,
                           __func__, status);

            dmSecureShutdown(config);

            vTaskDelay(TCP_CLIENT_DEF_WAIT_TIME);

            goto conn_retry;
        } else {
            //DM_DBG_INFO(BLUE " [%s:%d] Success TLS handshake \n" CLEAR, __func__, __LINE__);
        }

        status = (int)tls_save_ssl(sessConfPtr->sessTaskName, config);
        if (status) {
            DM_DBG_ERR(" [%s:%d] Failed to save tls session(0x%02x)\n", __func__, __LINE__, status);
        }
    }

    if (sessConfPtr->sessionKaInterval) {
        // For TCP Keep-Alive ...
        status = 0;
        status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&status, sizeof(status));
        if (status < ERR_OK) {
            DM_DBG_ERR(RED "[%s] Failed to set TCP keep-alive ...\n" CLEAR, __func__);
        }

        if (dpm_mode_is_enabled() == DPM_ENABLED) {
            // Register timer to send TCP keep-alive
            // Create RTC timer for TCP keep-alive on DPM mode
            sessConfPtr->sessionKaTid = rtcRegistTcpClientKeepaliveTimer(sessConfPtr);
            if (sessConfPtr->sessionKaTid == 0) {
                DM_DBG_ERR(" [%s] Failed to create ka timer \n", __func__);
                errorCallback(DM_ERROR_TIMER_CREATE, "Fail to create ka timer");
                stopReason = DM_ERROR_TIMER_CREATE;
                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

                goto tcpClientEnd_act;
            }

            // Save TCP keep-alive timer id to RTM area
#if defined ( SUPPORT_RTM_TCP_KEEPALIVE )
            dpm_tcp_socket_keep_alive_delete(sessConfPtr->dpmRegName, sessConfPtr->sessionKaTid);
            dpm_tcp_socket_keep_alive_set(sessConfPtr->dpmRegName, sessConfPtr->sessionKaTid);
#endif    // SUPPORT_RTM_TCP_KEEPALIVE
        }
    }

    // Notice initialize done to DPM module
    dpm_app_wakeup_done(sessConfPtr->dpmRegName);        // Noti to timer (Can receive timer ISR)
    dpm_app_data_rcv_ready_set(sessConfPtr->dpmRegName); // Noti to umac(Can receive packet)

    ///Main TCP Client loop
    while (1) {
        if (dpm_sleep_is_started()) {
            DM_DBG_INFO(" [%s] Wait dpm_sleep_is_started) \n", __func__);
            vTaskDelay(10);    // 10 ticks : 100 msec
            continue;
        }

        sessConfPtr->runStatus = SESSION_STATUS_RUN;

        if (sessConfPtr->supportSecure) {
            status = dmSecureRecv(config, sessConfPtr->rxDataBuffer, SSL_READ_TIMEOUT);
            if (status == ERR_TIMEOUT) {
                rx_data_len = -1;
            } else if (status == ERR_CONN) {
                rx_data_len = 0;
            } else if (status == ERR_UNKNOWN) {
                rx_data_len = 0;
            } else {
                rx_data_len = status;
            }
        } else {
            /* Two task share one TCP Socket
             * For synchronization
             */
#ifdef SESSION_MUTEX
            xSemaphoreTake(sessConfPtr->socketMutex, portMAX_DELAY);
#endif
            rx_data_len = recv(sessConfPtr->socket, sessConfPtr->rxDataBuffer, TCP_CLIENT_RX_BUF_SIZE, 0);
#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
        }

        if (rx_data_len > 0) {
            // Clear flag to protect goto DPM sleep mode
            __clr_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);

            dm_tcp_no_packet_count = 0;

            if (rx_data_len > TCP_CLIENT_RX_BUF_SIZE) {
                DM_DBG_ERR(" [%s] Failed to packet size (size:%d)\n", __func__, rx_data_len);
                errorCallback(DM_ERROR_RECV_SIZE_FAIL, "TCP client rx_data size fail");
                stopReason = DM_ERROR_RECV_SIZE_FAIL;
                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
                goto tcpClientEnd_act;
            }

            *(sessConfPtr->rxDataBuffer + rx_data_len) = 0;
            receiveCallback(sessConfPtr,
                        sessConfPtr->rxDataBuffer,
                        (UINT)rx_data_len,
                        tcp_server_ip_addr,
                        sessConfPtr->sessionServerPort);
        } else if (rx_data_len == 0) {  // DISCONNECT
            // Clear flag to protect goto DPM sleep mode
            __clr_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);

            DM_DBG_ERR(RED "[%s] NOT_CONNECTED (%s)\n" CLEAR, __func__, sessConfPtr->dpmRegName);
        
            connectCallback(sessConfPtr, DM_ERROR_SOCKET_NOT_CONNECT);        // Disconnected

            stopReason = DM_ERROR_SOCKET_NOT_CONNECT;    // case of restart -- if (sessConfPtr->sessionAutoReconn)
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto tcpClientEnd_act;
        } else {  // no packet, timeout
            vTaskDelay(1);

            if (dm_tcp_no_packet_count ++ > 30) {
                //DM_DBG_INFO(" [%s] ER_NO_PACKET (%d) \n" , __func__, rx_data_len);
                dm_tcp_no_packet_count = 0;
            }

            if (sessConfPtr->requestCmd == CMD_REQUEST_STOP) {
                DM_DBG_ERR(RED "[%s] request Stop Session \n" CLEAR, __func__);

                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
                goto tcpClientEnd_act;
            }

            if (sessConfPtr->requestCmd == CMD_REQUEST_RESTART) {
                DM_DBG_ERR(RED "[%s] Request restart session \n" CLEAR, __func__);

                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
                stopReason = DM_ERROR_SOCKET_NOT_CONNECT;        // case of restart
                goto tcpClientEnd_act;
            }
        }

        if (   (getPacketSendingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT))
            || (getPacketReceivingStatus(sessConfPtr, REG_TYPE_TCP_CLIENT)) ) {
            if (delayCountBySendingFlag++ < 100) {
                continue;
            } else {
                DM_DBG_ERR(RED "[%s] send/recv pending \n" CLEAR, __func__);
            }
            delayCountBySendingFlag = 0;
        }

        // Save dtls session
        if (sessConfPtr->supportSecure) {
            status = (int)tls_save_ssl(sessConfPtr->sessTaskName, config);
            if (status) {
                DM_DBG_ERR(" [%s:%d] Failed to save tls session(0x%02x)\n", __func__, __LINE__, status);
            }
        }

        // Set flag to permit goto DPM sleep mode
        __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
    }

tcpClientEnd_act:
    close(sessConfPtr->socket);
    shutdown(sessConfPtr->socket, SHUT_RDWR);
    sessConfPtr->socket = -1;

    if (sessConfPtr->supportSecure) {
        dmSecureShutdown(config);
        dmSecureDeinit(config);
        tls_clear_ssl(sessConfPtr->sessTaskName, config);
    }

tcpClientEnd:
    if (sessConfPtr->rxDataBuffer != NULL) {
        vPortFree(sessConfPtr->rxDataBuffer);
        sessConfPtr->rxDataBuffer = NULL;
    }

    if (sessConfPtr->sessionKaInterval) {
        // Delete TCP keep-alive timer
        rtc_cancel_timer(sessConfPtr->sessionKaTid);
        sessConfPtr->sessionKaTid = 0;
        DM_DBG_INFO(" [%s] Cancel timer(id:%d) \n", __func__, sessConfPtr->sessionKaTid);
    }

    __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
    DM_DBG_INFO("[%s] Stop TCP client by reason:0x%x (session no:%d) \n",
                 __func__, stopReason, sessConfPtr->sessionNo);
    sessConfPtr->runStatus = SESSION_STATUS_STOP;

    return stopReason;
}

static void dpmTcpClientManager(void *sessionNo)
{
    int    status = 0;
    session_config_t *sessConfPtr = NULL;

    if (!(UINT)sessionNo || (UINT)sessionNo > MAX_SESSION_CNT) {
        DM_DBG_ERR(RED "[%s] Session Start Error(sessionNo:%d) \n" CLEAR, __func__, (UINT)sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "Session start fail");
        goto TC_taskEnd;
    }

    sessConfPtr = &dpmUserConfigs.sessionConfig[(UINT)sessionNo - 1];
    sessConfPtr->sessionNo = (UINT)sessionNo;

    if (initSessionCommon(sessConfPtr)) {
        DM_DBG_ERR(RED "[%s] TCP Client Start Error(sessionNo:%d) \n" CLEAR,
                        __func__, (UINT)sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "TCP client start fail");
        goto TC_taskEnd;
    }

    DM_DBG_INFO(CYAN "[%s] Started dpmTcpClientManager session no:%d \n" CLEAR,
                      __func__, (UINT)sessionNo);
    if (sessConfPtr->sessionServerPort && iptolong(sessConfPtr->sessionServerIp)) {
loop:
        status = runTcpClient(sessConfPtr);

        if (sessConfPtr->requestCmd == CMD_REQUEST_STOP) {
#ifdef CHANGE_TCP_LOCAL_PORT_BY_RESTART
            sessConfPtr->localPort = 0;                   // change local port number by stop/start
#endif
            sessConfPtr->requestCmd = CMD_NONE;
        } else if (sessConfPtr->sessionAutoReconn) {
            if (status == DM_ERROR_SOCKET_NOT_CONNECT) {  // case of conn->disconn
                vTaskDelay(RECONN_WAIT_TIME);    // Waiting to connect : 5 seconds
                goto loop;
            }
        }

        DM_DBG_ERR(RED "[%s] Stopped dpmTcpClientManager(sessNo:%d stopReason:%d) \n" CLEAR,
                        __func__, (UINT)sessionNo, status);
    }
    else
    {
        DM_DBG_ERR(RED "[%s] Stopped dpmTcpClientManager by abnormal config (sess:%d ip:0x%x port:%d) \n"
                        CLEAR, __func__, (UINT)sessionNo,
                                                sessConfPtr->sessionServerPort, sessConfPtr->sessionServerIp);
    }

    deinitSessionCommon(sessConfPtr);

TC_taskEnd:

    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        // Delete TCP Filter for dpm
        dpm_tcp_port_filter_delete((unsigned short)sessConfPtr->localPort);    // del_dpm_tcp_port_filter
        dpm_app_unregister(sessConfPtr->dpmRegName);
    }

    vTaskDelete(NULL);
}

//
// end dpm TCP Client Manager
//


#if !defined (__LIGHT_DPM_MANAGER__)
//
// start dpm UDP Server Manager
//
int sendUdpServer(session_config_t *sessConfPtr, int sub_idx, char *data_buf, UINT len_buf, ULONG ip, ULONG port)
{
    UINT      ret;
    UINT      status = ERR_OK;
    UINT      waitSendTime = 0;
    ULONG     udp_peer_ip = ip;
    USHORT    udp_peer_port = port;
    struct    sockaddr_in    peer_addr;
    SECURE_INFO_T        *config = NULL;
    int       udp_tx_sock;
    udpSubSessConf_t    *udpSubSessInfoPtr;
    UINT      preRunStatus;

    if (sessConfPtr->supportSecure) {
        DM_DBG_INFO(CYAN "[%s] Start send UDP packet(sub_idx:%d buf:0x%x len:%d ip:0x%x) \n" CLEAR,
                          __func__, sub_idx, data_buf, len_buf, udp_peer_ip);

        /* At this case,,, dpm_sleep operation was started already */
        if (chk_dpm_pdown_start()) {
            DM_DBG_ERR(RED "[%s] Error - In progressing DPM sleep !!!" CLR_COL, __func__);
            return ER_NOT_SUCCESSFUL;
        }

        if (!sessionRunConfirm(sessConfPtr, 200)) {  // 2 sec wait for running status
            DM_DBG_ERR(RED "[%s] Not ready to send \n" CLEAR, __func__);
            clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
            return ER_NOT_ENABLED;
        }

        udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[sub_idx];

        if (udpSubSessInfoPtr->allocFlag != pdTRUE) {
            DM_DBG_ERR(RED "[%s] Not alloc \n" CLEAR, __func__);
            return ER_NOT_ENABLED;
        }

        setPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
        preRunStatus = udpSubSessInfoPtr->runStatus;


        if (udpSubSessInfoPtr->runStatus == SESSION_STATUS_STOP) {  // not yet received
            DM_DBG_INFO(" [%s] Not yet init \n", __func__);
            udpSubSessInfoPtr->runStatus = SESSION_STATUS_INIT;

            tlsRestoreSubSessInfo(udpSubSessInfoPtr);

            config = &udpSubSessInfoPtr->secureConfig;

            if (config->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
                memset(&config->timer, 0x00, sizeof(SECURE_TIMER));
                mbedtls_ssl_set_timer_cb(config->ssl_ctx,
                                        &config->timer,
                                        dmDtlsTimerStart,
                                        dmDtlsTimerGetStatus);

                config->client_ctx->fd = sessConfPtr->socket;

                udpSubSessInfoPtr->clientSock = config->client_ctx->fd;
                DM_DBG_INFO(" [%s] Create socket(udpSubSessInfoPtr->clientSock:%d) \n",
                                                __func__, udpSubSessInfoPtr->clientSock);

                status = mbedtls_ssl_set_client_transport_id(config->ssl_ctx,
                                                            (unsigned char *)udpSubSessInfoPtr->peerIpAddr,
                                                            DM_IP_LEN);
                if (status) {
                    DM_DBG_INFO(" [%s] Failed to set client trasport id(0x%x) \n", __func__, -status);
                }

                mbedtls_ssl_set_bio(config->ssl_ctx,
                                    (void *)config,
                                    dmSecureSendCallback,
                                    NULL,
                                    dmSecureRecvTimeoutCallback);

                DM_DBG_INFO(" [%s] set_bio init done \n", __func__);

                udpSubSessInfoPtr->runStatus = SESSION_STATUS_RUN;
                goto secure_send;
            } else {
                tlsClearSubSessInfo(udpSubSessInfoPtr);

                udpSubSessInfoPtr->runStatus = SESSION_STATUS_STOP;
                clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
                DM_DBG_INFO(" [%s] Not yet handshake \n", __func__);
                return ER_NOT_ENABLED;
            }
        } else {

retry_check:
            if (udpSubSessInfoPtr->runStatus != SESSION_STATUS_RUN) {
                if (++waitSendTime <= 100) {
                    vTaskDelay(2);
                    goto retry_check;
                } else {
                    DM_DBG_ERR(RED "[%s] Not ready to send (runStatus:%d) \n" CLEAR,
                                                    __func__, udpSubSessInfoPtr->runStatus);

                    clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
                    return ER_NOT_ENABLED;
                }
            }
            preRunStatus = udpSubSessInfoPtr->runStatus;

            DM_DBG_INFO(CYAN " [%s] Send TLS Packet(ip:0x%x port:%d len_buf:%d preRunStatus:%d) \n" CLEAR,
                                            __func__, udp_peer_ip, udp_peer_port, len_buf, preRunStatus);
#ifdef FOR_DEBUGGING_PKT_DUMP
            hexa_dump_print("sendto Packet ", data_buf, len_buf, 0, 2);
#endif
            config = &udpSubSessInfoPtr->secureConfig;
            if (config->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
                goto secure_send;
            } else {
                DM_DBG_ERR(RED "[%s] Not yet handshake \n" CLEAR, __func__, data_buf, len_buf);
                status = ER_NOT_SUCCESSFUL;
            }
        }

secure_send:
        status = dmSecureSend(config, data_buf, len_buf, UDP_SERVER_DEF_WAIT_TIME);
        if (status) {
            DM_DBG_ERR(RED "[%s] Failed UDP send packet(buf:0x%x len:%d) \n"
                           CLEAR, __func__, data_buf, len_buf);
        } else {
            udpSubSessInfoPtr->lastAccessRTC = RTC_GET_COUNTER() >> 15;
            ret = tls_save_ssl(udpSubSessInfoPtr->udpSubSessName, config);
            if (ret)
            {
                DM_DBG_ERR("[%s:%d] Failed to save tls session(0x%02x)\n", __func__, __LINE__, ret);
            }
        }

        clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);

        udpSubSessInfoPtr->runStatus = preRunStatus;

        if (udpSubSessInfoPtr->runStatus == SESSION_STATUS_STOP) {
            vPortFree(config->listen_ctx);
            config->listen_ctx = 0;

            vPortFree(config->client_ctx);
            config->client_ctx = 0;

            tlsDeinitSubSessInfo(udpSubSessInfoPtr);
        }

        DM_DBG_INFO(CYAN "[%s] End send UDP packet(sub_idx:%d buf:0x%x len:%d runStatus:%d) \n" CLEAR,
                          __func__, sub_idx, data_buf, len_buf, udpSubSessInfoPtr->runStatus);

        return status;
    } else {
        DM_DBG_INFO(CYAN "[%s] send UDP packet(ip:0x%x, port:%d) \n" CLEAR,
                          __func__, udp_peer_ip, udp_peer_port);

        /* At this case,,, dpm_sleep operation was started already */
        if (chk_dpm_pdown_start()) {
            DM_DBG_ERR(RED "[%s] Error - In progressing DPM sleep !!!" CLR_COL, __func__);
            return ER_NOT_SUCCESSFUL;
        }

        setPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);

        if (!sessionRunConfirm(sessConfPtr, 200)) {  // 2 sec wait for running status
            DM_DBG_ERR(RED "[%s] Not ready to send \n" CLEAR, __func__);
            clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
            return ER_NOT_ENABLED;
        }

        udp_tx_sock = sessConfPtr->socket;

        DM_DBG_INFO(CYAN " [%s] sendto Packet(len_buf:%d, ip:0x%x, port:%d) \n" CLEAR,
                                        __func__, len_buf, udp_peer_ip, udp_peer_port);
#ifdef FOR_DEBUGGING_PKT_DUMP
        hexa_dump_print("sendto Packet ", data_buf, len_buf, 0, 2);
#endif
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr.s_addr = htonl(udp_peer_ip);
        peer_addr.sin_port = htons(udp_peer_port);

        // Send data to UDP client
        status = sendto(udp_tx_sock, data_buf, len_buf, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        if (status != len_buf) {
            DM_DBG_ERR(RED "[%s] Failed to data send (0x%02x)\n" CLEAR, __func__, status);
            errorCallback(DM_ERROR_SEND_FAIL, "Fail to udp server socket send");

            clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
            return status;
        }

        clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER);
        return ERR_OK;
    }
}

void printSessionUdpSubIndex()    // UDP Server Only
{
    int    index;
    udpSubSessConf_t    *udpSubSessInfoPtr;

    PRINTF(" << udpSubSession config>> \n");
    udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[0];
    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {

        PRINTF(" %d) allocFlag:%d runSt:%d No:%d name:%s peerP:%d peerIp:%s socket:%d la_tick:%d \n",
                index,
                udpSubSessInfoPtr->allocFlag,
                udpSubSessInfoPtr->runStatus,
                udpSubSessInfoPtr->subIndex,
                udpSubSessInfoPtr->udpSubSessName,
                udpSubSessInfoPtr->peerPort,
                udpSubSessInfoPtr->peerIpAddr,
                udpSubSessInfoPtr->clientSock,
                udpSubSessInfoPtr->lastAccessRTC);

        udpSubSessInfoPtr++;
    }
}

void tlsShutdownSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr)
{
    SECURE_INFO_T        *config;
    config = &udpSubSessInfoPtr->secureConfig;

    DM_DBG_INFO(GREEN " [%s] Start shutdown sub_idx (sub_idx:%d run_status:%d) \n" CLEAR, __func__,
                udpSubSessInfoPtr->subIndex, udpSubSessInfoPtr->runStatus);

    if (udpSubSessInfoPtr->runStatus == SESSION_STATUS_STOP) {
        tlsRestoreSubSessInfo(udpSubSessInfoPtr);
    }

    dmSecureShutdown(config);
    close(config->client_ctx->fd);
    shutdown(config->client_ctx->fd, SHUT_RDWR);

    tls_clear_ssl(udpSubSessInfoPtr->udpSubSessName, config);

    DM_DBG_INFO(GREEN "[%s] End shutdown sub_idx (sub_idx:%d) \n" CLEAR, __func__, udpSubSessInfoPtr->subIndex);
}

void tlsClearSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr)
{
    DM_DBG_INFO(GREEN "[%s] Start clear sub_idx (sub_idx:%d run_status:%d) \n" CLEAR, __func__,
                udpSubSessInfoPtr->subIndex, udpSubSessInfoPtr->runStatus);

    memset(udpSubSessInfoPtr, 0, sizeof(udpSubSessConf_t));
    DM_DBG_INFO(GREEN "[%s] End clear sub_idx (sub_idx:%d) \n" CLEAR, __func__, udpSubSessInfoPtr->subIndex);
}

void tlsDeinitSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr)
{
    SECURE_INFO_T        *config;
    config = &udpSubSessInfoPtr->secureConfig;

    DM_DBG_INFO(GREEN "[%s] Start deinit sub_idx (sub_idx:%d run_status:%d) \n" CLEAR, __func__,
                       udpSubSessInfoPtr->subIndex, udpSubSessInfoPtr->runStatus);

    dmSecureDeinit(config);

    DM_DBG_INFO(GREEN "[%s] End deinit sub_idx (sub_idx:%d) \n" CLEAR, __func__, udpSubSessInfoPtr->subIndex);
}

void tlsRestoreSubSessInfo(udpSubSessConf_t *udpSubSessInfoPtr)
{
    int     status = ERR_OK;
    session_config_t *sessConfPtr = udpSubSessInfoPtr->sessConfPtr;
    SECURE_INFO_T    *config = &udpSubSessInfoPtr->secureConfig;
    struct netif     *netif;

    config->sessionConfPtr = sessConfPtr;
    config->subIndex = udpSubSessInfoPtr->subIndex;
    config->sessionType = sessConfPtr->sessionType;    // REG_TYPE_UDP_SERVER
    config->localPort = sessConfPtr->sessionMyPort;
    config->timeout = UDP_SERVER_DEF_WAIT_TIME;

    netif = netif_get_by_index(2);    // WLAN0:2, WLAN1:3
    sprintf(config->localIp, "%d.%d.%d.%d",
                ip4_addr1_16(&netif->ip_addr),
                ip4_addr2_16(&netif->ip_addr),
                ip4_addr3_16(&netif->ip_addr),
                ip4_addr4_16(&netif->ip_addr));

    DM_DBG_INFO(" [%s] Sub session name:%s sub_index:%d type:%d local address: %s:%d \n",
                    __func__, udpSubSessInfoPtr->udpSubSessName, udpSubSessInfoPtr->subIndex,
                    config->sessionType, config->localIp, config->localPort);

    udpSubSessInfoPtr->runStatus = SESSION_STATUS_INIT;

    ssl_ctx_clear(config);
    status = dmSecureInit(config);        // UDP SERVER    
    if (status) {
        DM_DBG_ERR(" [%s] Failed to init ssl(0x%02x) \n", __func__, status);
        return;
    }

    setupSecureCallback(sessConfPtr, config);    // call back : secure config setup

    status = tls_restore_ssl(udpSubSessInfoPtr->udpSubSessName, config);
    if (status) {
        DM_DBG_ERR(" [%s] Failed to restore ssl(0x%02x)\n", __func__, status);
    }
}

void clearNotActUdpSubTask()
{
    int    index;
    udpSubSessConf_t    *udpSubSessInfoPtr;

    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
        udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[index];

        if (udpSubSessInfoPtr->runStatus == SESSION_STATUS_STOP) {
            memset(udpSubSessInfoPtr, 0, sizeof(udpSubSessConf_t));
        }
    }
}

int getSessionUdpSubIndexByPort(ULONG ip, ULONG port)    // UDP Server Only
{
    DA16X_UNUSED_ARG(ip);

    int    index;
    udpSubSessConf_t    *udpSubSessInfoPtr;

    DM_DBG_INFO(" [%s] Start sub index search (ip:0x%x port:%d)\n" CLEAR, __func__, ip, port);

    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
        udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[index];
        if (udpSubSessInfoPtr->allocFlag == pdTRUE) {
            if (udpSubSessInfoPtr->peerPort == port) {
                DM_DBG_INFO(" [%s] Find sub index %d (port:%d)\n" CLEAR, __func__, index, port);
                return index;
            }
        }
    }

    DM_DBG_INFO(" [%s] Not found sub index (ip:0x%x port:%d)\n", __func__, ip, port);

    return -1;
}

int getSessionUdpSubIndexBySocket(int socket)    // UDP Server Only
{
    int    index;
    udpSubSessConf_t    *udpSubSessInfoPtr;

    DM_DBG_INFO(" [%s] Start sub index search (socket: %d)\n" CLEAR, __func__, socket);
    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
        udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[index];
        if (udpSubSessInfoPtr->allocFlag == pdTRUE) {
            if (udpSubSessInfoPtr->clientSock == socket) {
                DM_DBG_INFO(" [%s] Find sub index %d (socket:%d)\n" CLEAR, __func__, index, socket);
                return index;
            }
        }
    }

    DM_DBG_INFO(" [%s] Not found sub index (socket:%d)\n", __func__, socket);

    return -1;
}

static int getEmptyUdpSubIndex(session_config_t *sessConfPtr)
{
    int    index;

    // Find empty index
    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
        if (sessConfPtr->udpSubInfo[index]->allocFlag == pdFALSE) {
            break;
        }
    }

    return index;
}

static int getOldestAccessUdpSubIndex(session_config_t *sessConfPtr)
{
    int            index;
    ULONG        lastAccessRTC = 0xFFFFFFFF;

    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
        if (lastAccessRTC > sessConfPtr->udpSubInfo[index]->lastAccessRTC) {
            lastAccessRTC = sessConfPtr->udpSubInfo[index]->lastAccessRTC;
        }
    }

    DM_DBG_INFO(" [%s] lastAccessRTC :%d \n", __func__, lastAccessRTC);
    for (index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
        if (lastAccessRTC == sessConfPtr->udpSubInfo[index]->lastAccessRTC)
        {
            break;
        }
    }

    if (index != DM_UDP_SVR_MAX_SESS) {
        DM_DBG_INFO(" [%s] return sub_idx:%d lastAccessRTC :%d \n", __func__,
                    index, sessConfPtr->udpSubInfo[index]->lastAccessRTC);
    } else {
        index = -1;
    }

    return index;
}

UINT dmSecureUdpServerHandshake(udpSubSessConf_t *udpSubSessInfoPtr)
{
    SECURE_INFO_T *config = &udpSubSessInfoPtr->secureConfig;
    int  status = ERR_OK;
    int  addr_len;
    struct timeval        tv;
    struct sockaddr_in    peer_addr;
    char  buf[1] = { 0 };

    config->client_ctx->fd = udpSubSessInfoPtr->clientSock;

    if (startHandshaking()) {
        DM_DBG_ERR(RED "[%s] Failed to wait_timeout \n" CLEAR, __func__);
        return DM_ERROR_HANDSHAKE_FAIL;
    }
    DM_DBG_INFO("[%s] Start DTLS handshake (client_ctx->fd:%d peer_ip:%s peer_port:%d) \n",
                 __func__,
                 config->client_ctx->fd,
                 udpSubSessInfoPtr->peerIpAddr,
                 udpSubSessInfoPtr->peerPort);

wait_accept:

    // set socket timeout
    tv.tv_sec = 0;
    tv.tv_usec = UDP_SVR_SECURE_ACCEPT_TIMEOUT * 1000;
    status = setsockopt(config->client_ctx->fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d (fd:%d) \n" CLEAR,
                    __func__, __LINE__, status, config->client_ctx->fd);
    }

    buf[0] = 0;
    addr_len = sizeof(peer_addr);
    status = (int)recvfrom(config->client_ctx->fd,
                            buf,
                            sizeof(buf),
                            MSG_PEEK,
                            (struct sockaddr *)&peer_addr,
                            (socklen_t *)&addr_len);
    if (status < 0) {
        status = DM_ERROR_HANDSHAKE_FAIL;
        goto end_of_udp_server_handshake;
    }

    DM_DBG_INFO(GREEN " [%s] Received socket:%d \n" CLEAR, __func__, config->client_ctx->fd);

    if (connect(config->client_ctx->fd, (struct sockaddr *)&peer_addr, addr_len) != 0) {
        status = DM_ERROR_HANDSHAKE_FAIL;
        goto end_of_udp_server_handshake;
    }

    longtoip(htonl(peer_addr.sin_addr.s_addr), (char *)udpSubSessInfoPtr->peerIpAddr);
    udpSubSessInfoPtr->peerPort = htons(peer_addr.sin_port);

    DM_DBG_INFO(GREEN " [%s] listen_ctx->fd:%d client_ctx->fd:%d peer_ip:%s peer_port:%d \n" CLEAR,
                                                    __func__,
                                                    config->listen_ctx->fd,
                                                    config->client_ctx->fd,
                                                    udpSubSessInfoPtr->peerIpAddr,
                                                    udpSubSessInfoPtr->peerPort);
    //Set client ip address
    status = mbedtls_ssl_set_client_transport_id(config->ssl_ctx,
                                                (unsigned char *)udpSubSessInfoPtr->peerIpAddr,
                                                DM_IP_LEN);
    if (status)
    {
        DM_DBG_INFO(" [%s] Failed to set client trasport id(0x%x) \n", __func__, -status);
        goto end_of_udp_server_handshake;
    }

    mbedtls_ssl_set_bio(config->ssl_ctx,
                        config->client_ctx,
                        mbedtls_net_send,
                        NULL,
                        mbedtls_net_recv_timeout);

    DM_DBG_INFO(" [%s] set_bio init done \n", __func__);

    while ((status = mbedtls_ssl_handshake(config->ssl_ctx)) != 0) {  // UDP SERVER HANDSHAKE
        if ((status == MBEDTLS_ERR_SSL_WANT_READ) || (status == MBEDTLS_ERR_SSL_WANT_WRITE)) {
            continue;
        }

        if (status == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
            DM_DBG_INFO(" [%s] hello verification requested \n", __func__);
            dmSecureShutdown(config);
            goto wait_accept;
        } else {
            DM_DBG_ERR(RED "[%s] Failed to do handshake(0x%x) sub_idx:%d \n" CLEAR,
                                                    __func__, -status, config->subIndex);
            mbedtls_net_free(config->client_ctx);
            dmSecureShutdown(config);

            status = DM_ERROR_HANDSHAKE_FAIL;
            goto end_of_udp_server_handshake;
        }
    }

    tv.tv_sec = 0;
    tv.tv_usec = UDP_SVR_ACCEPT_TIMEOUT * 1000;
    status = setsockopt(config->client_ctx->fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
    }

    DM_DBG_INFO(BLUE " [%s] Success DTLS handshake(sub_idx:%d) \n" CLEAR, __func__, config->subIndex);

    status = ERR_OK;

end_of_udp_server_handshake:

    endHandshaking();
    return status;
}

void dmSecurePrepare(udpSubSessConf_t *udpSubSessInfoPtr)
{
    int   status = ERR_OK;
    SECURE_INFO_T  *config = &udpSubSessInfoPtr->secureConfig;

    DM_DBG_INFO(GREEN "[%s] HANDSHAKE_OVER peer_ip:%s peer_port:%d \n" CLEAR,
                       __func__,
                       udpSubSessInfoPtr->peerIpAddr,
                       udpSubSessInfoPtr->peerPort);

    //Set client ip address
    status = mbedtls_ssl_set_client_transport_id(config->ssl_ctx,
                                                (unsigned char *)udpSubSessInfoPtr->peerIpAddr,
                                                DM_IP_LEN);
    if (status) {
        DM_DBG_INFO(" [%s] Failed to set client trasport id(0x%x) \n", __func__, -status);
        return;
    }

    config->client_ctx->fd = udpSubSessInfoPtr->clientSock;

    DM_DBG_INFO(BLUE " [%s:%d] clientSock fd:%d \n" CLEAR,
            __func__, __LINE__, udpSubSessInfoPtr->clientSock);

    mbedtls_ssl_set_bio(config->ssl_ctx, (void *)config, dmSecureSendCallback, NULL, dmSecureRecvTimeoutCallback);

    DM_DBG_INFO(" [%s] set_bio init done \n", __func__);
}

UINT udpServerSubFunction(void *pvParameters)    // sub function
{
    int    status = ERR_OK;
    UINT   ret;
    UINT   stopReason = ERR_OK;
    struct timeval        tv;
    udpSubSessConf_t    *udpSubSessInfoPtr;
    session_config_t    *sessConfPtr;
    SECURE_INFO_T        *config;
#if defined ( KEEPALIVE_UDP_CLI )
    UCHAR    dpm_udp_tid = 0;
#endif    // KEEPALIVE_UDP_CLI

    // Set the pointer.
    udpSubSessInfoPtr = (udpSubSessConf_t *)pvParameters;
    sessConfPtr = udpSubSessInfoPtr->sessConfPtr;
    config = &udpSubSessInfoPtr->secureConfig;

    config->sessionConfPtr = sessConfPtr;
    config->subIndex = udpSubSessInfoPtr->subIndex;

    DM_DBG_INFO(CYAN "[%s] Started sub_idx:%d by client(%s:%d) clientSock:%d local address(%s:%d)\n" CLEAR,
                      __func__, udpSubSessInfoPtr->subIndex, udpSubSessInfoPtr->peerIpAddr,
                      udpSubSessInfoPtr->peerPort, udpSubSessInfoPtr->clientSock,
                      config->localIp, config->localPort);

    if (udpSubSessInfoPtr->runStatus == SESSION_STATUS_RUN) {
        config->client_ctx->fd = udpSubSessInfoPtr->clientSock;
        goto handshake_over;
    }

    memset(&config->timer, 0x00, sizeof(SECURE_TIMER));
    mbedtls_ssl_set_timer_cb(config->ssl_ctx, &config->timer, dmDtlsTimerStart, dmDtlsTimerGetStatus);

    // Notice initialize done to DPM module
    if (config->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
        dmSecurePrepare(udpSubSessInfoPtr);

        goto handshake_over;
    }

    if (dmSecureUdpServerHandshake(udpSubSessInfoPtr)) {
        udpSubSessInfoPtr->runStatus = SESSION_STATUS_STOP;
        DM_DBG_ERR(RED "[%s] Failed to dtls handshake dtls \n" CLEAR, __func__);
        goto udpRxSubTaskEnd;
    } else {    // HANDSHAKE OK
        dmSecurePrepare(udpSubSessInfoPtr);

        status = tls_save_ssl(udpSubSessInfoPtr->udpSubSessName, config);
        if (status) {
            DM_DBG_ERR(RED "[%s:%d] Failed to save dtls session(0x%02x)\n" CLEAR,
                                                    __func__, __LINE__, status);
        }
    }

handshake_over:

    DM_DBG_INFO(" [%s] after handshake (listen_fd:%d client_fd:%d) clientSock:%d \n",
            __func__, config->listen_ctx->fd, config->client_ctx->fd, udpSubSessInfoPtr->clientSock);

    tv.tv_sec = 0;
    tv.tv_usec = UDP_SVR_RX_SECURE_TIMEOUT * 1000;

    status = setsockopt(udpSubSessInfoPtr->clientSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
    }

    udpSubSessInfoPtr->runStatus = SESSION_STATUS_RUN;
    DM_DBG_INFO(" [%s] STATUS_RUN sub_idx:%d \n", __func__, udpSubSessInfoPtr->subIndex);

    status = dmSecureRecv(config, sessConfPtr->rxDataBuffer, SSL_READ_TIMEOUT);
    if (status == ERR_TIMEOUT) {
        DM_DBG_ERR(RED "[%s:%d] dmSecureRecv timeout error (%d) \n" CLEAR, __func__, __LINE__, status);
        errorCallback(DM_ERROR_RECEIVE_TIMEOUT, "Recv timeout fail");
        stopReason = DM_ERROR_RECEIVE_TIMEOUT;
    } else if (status == ERR_CONN) {
        DM_DBG_ERR(RED "[%s] Failed to not_connect (%d)\n", __func__, status);
        errorCallback(DM_ERROR_SOCKET_NOT_CONNECT, "Connect fail");
        stopReason = DM_ERROR_SOCKET_NOT_CONNECT;
    } else if (status == ERR_UNKNOWN) {
        DM_DBG_ERR(RED "[%s] Failed to unknown receive error (%d)\n", __func__, status);
        errorCallback(DM_ERROR_SECURE_UNKNOWN_FAIL, "Unaware failure");
        stopReason = DM_ERROR_SECURE_UNKNOWN_FAIL;
    } else if (status >= 0) {      // OK
        DM_DBG_INFO(CYAN " [%s:%d] recv packet OK (prt:0x%x socket:%d size:%d) \n" CLEAR,
                        __func__, __LINE__, udpSubSessInfoPtr, udpSubSessInfoPtr->clientSock, status);

        *((sessConfPtr->rxDataBuffer)+status) = 0;
        receiveCallback(sessConfPtr,
                        sessConfPtr->rxDataBuffer,
                        status,
                        iptolong(udpSubSessInfoPtr->peerIpAddr),
                        udpSubSessInfoPtr->peerPort);

        udpSubSessInfoPtr->lastAccessRTC = RTC_GET_COUNTER() >> 15;
    }

    if (   (getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER))
        || (getPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_SERVER)) ) {
            DM_DBG_ERR(RED "[%s] Pending send/recv \n" CLEAR, __func__);
    }

    ret = tls_save_ssl(udpSubSessInfoPtr->udpSubSessName, config);
    if (ret) {
        DM_DBG_ERR(" [%s] Failed to save dtls sess(0x%02x)\n", __func__, ret);
    }

udpRxSubTaskEnd:

    DM_DBG_INFO("[%s] End UDP server sub function (stopReson:0x%x session no:%d name:%s allocFlag:%d) \n",
                 __func__, stopReason, sessConfPtr->sessionNo,
                 udpSubSessInfoPtr->udpSubSessName, udpSubSessInfoPtr->allocFlag);

    udpSubSessInfoPtr->runStatus = SESSION_STATUS_STOP;

    return stopReason;
}

void initUdpSubSessInfo(session_config_t *sessConfPtr, int sub_idx, char *peerIpAddr, ULONG peerPort)
{
    udpSubSessConf_t    *udpSubSessInfoPtr;

    udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[sub_idx];

    strcpy((char *)udpSubSessInfoPtr->peerIpAddr, peerIpAddr);
    udpSubSessInfoPtr->peerPort = peerPort;
    udpSubSessInfoPtr->subIndex = sub_idx;
    udpSubSessInfoPtr->sessConfPtr = sessConfPtr;

    memset(udpSubSessInfoPtr->udpSubSessName, 0, TASK_NAME_LEN);
    sprintf(udpSubSessInfoPtr->udpSubSessName, "udpRxSub%d", udpSubSessInfoPtr->subIndex);

    udpSubSessInfoPtr->allocFlag = pdTRUE;
}

int udpServerSocketCreate(struct sockaddr *local_addr)
{
    int listen_socket;
    int optval = 1;

    listen_socket = (int) socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_socket < 0) {
        DM_DBG_ERR(RED "[%s:%d] Error - socket() \n" CLEAR, __func__, __LINE__);
        errorCallback(DM_ERROR_SOCKET_CREATE_FAIL, "UDP socket create fail");
        return DM_ERROR_SOCKET_CREATE_FAIL;
    }
    DM_DBG_INFO(" [%s] Create socket(listen_socket: %d) \n", __func__, listen_socket);

    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) != 0) {
        close(listen_socket);
        errorCallback(DM_ERROR_SETSOCKOPT_FAIL, "UDP setsockopt fail");
        return DM_ERROR_SETSOCKOPT_FAIL;
    }

    if (bind(listen_socket, local_addr, sizeof(struct sockaddr_in))) {
        close(listen_socket);
        errorCallback(DM_ERROR_SOCKET_BIND_FAIL, "UDP server bind fail");
        return DM_ERROR_SOCKET_BIND_FAIL;
    }

    return listen_socket;
}

static int runUdpServer(session_config_t *sessConfPtr)
{
    int    status = ERR_OK;
    UINT   stopReason;
    int    optval = 1;
    struct sockaddr_in    local_addr;
    struct sockaddr_in    peer_addr;
    ULONG  udp_peer_ip;
    UINT   udp_peer_port;
    size_t addr_len;
    struct timeval    tv;
    UINT   delayCountBySendingFlag = 0;
#if defined ( KEEPALIVE_UDP_CLI )
    UCHAR  dpm_udp_tid = 0;
#endif    // KEEPALIVE_UDP_CLI

    unsigned char client_ip[16] = {0x00,};    // int
    size_t client_ip_len = 4;
    char   peerIpAddr[DM_IP_LEN];        // string
#ifdef ACCEPT_RETRY
    int    retryCount = 0;
#endif
    int    listen_socket;
    int    client_socket;
    int    sub_idx;
    udpSubSessConf_t    *udpSubSessInfoPtr;
    char   str_port[8] = {0x00,};    // create socket & bind
    struct sockaddr_storage client_addr;
    char   buf[1] = { 0 };
    int    waitCount = 0;

    DM_DBG_INFO(" [%s] Start \n", __func__);

    // Allocate RX data buffer
    if (sessConfPtr->rxDataBuffer == NULL) {
        sessConfPtr->rxDataBuffer = pvPortMalloc(UDP_SERVER_RX_BUF_SIZE);
        if (sessConfPtr->rxDataBuffer == NULL) {
            DM_DBG_ERR(RED "[%s] rxDataBuffer memory alloc fail \n" CLEAR, __func__);

            errorCallback(DM_ERROR_RX_BUFFER_ALLOC_FAIL, "UDP server Rx buffer memory alloc fail");
            stopReason = DM_ERROR_RX_BUFFER_ALLOC_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_STOP;
            return stopReason;
        }
    }

    memset(&local_addr, 0x00, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(sessConfPtr->sessionMyPort);

    dpm_app_wakeup_done(sessConfPtr->dpmRegName);                // Noti to timer (Can receive timer ISR)

    if (sessConfPtr->supportSecure) {      // Secure UDP Server
        for (int index = 0; index < DM_UDP_SVR_MAX_SESS; index++) {
            dpmUserConfigs.udpRxTaskConfig[index].runStatus = SESSION_STATUS_STOP;
            sessConfPtr->udpSubInfo[index] = &dpmUserConfigs.udpRxTaskConfig[index];
        }

        snprintf(str_port, sizeof(str_port), "%d", sessConfPtr->sessionMyPort);

        listen_socket = udpServerSocketCreate((struct sockaddr *)&local_addr);
        if (listen_socket < 0) {
            DM_DBG_ERR(RED "[%s:%d] Error - socket() \n" CLEAR, __func__, __LINE__);
            errorCallback(listen_socket, "UDP socket create fail");
            stopReason = listen_socket;
            return stopReason;
        }
        sessConfPtr->socket = listen_socket;
        DM_DBG_INFO(" [%s] Create socket(listen_socket: %d) \n", __func__, listen_socket);
        
        sessConfPtr->runStatus = SESSION_STATUS_RUN;

wait_udp_accept:
        // set socket timeout
        tv.tv_sec = 0;
        tv.tv_usec = UDP_SVR_SECURE_ACCEPT_TIMEOUT * 1000;
        status = setsockopt(listen_socket, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
        if (status) {
            DM_DBG_ERR(RED "[%s:%d] setsockopt error %d (fd:%d) \n" CLEAR, __func__, __LINE__, status, listen_socket);
        }

        dpm_app_data_rcv_ready_set(sessConfPtr->dpmRegName);        // Noti to umac(Can receive packet)

        // UDP: wait for a message, but keep it in the queue
        buf[0] = 0;
        addr_len = sizeof(client_addr);
        status = (int)recvfrom(listen_socket,
                                buf,
                                sizeof(buf),
                                MSG_PEEK,
                                (struct sockaddr *)&client_addr,
                                (socklen_t *)&addr_len);
        if (status < 0) {
#ifdef ACCEPT_RETRY
            if ((++retryCount >= 2) && (!(getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER))))
#else
            if (!(getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER)))
#endif
            {
                __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
            }

            //DM_DBG_INFO(" [%s] Failed to do recvfrom(%d) \n", __func__, status);

            vTaskDelay(2);
            goto wait_udp_accept;
        }

        DM_DBG_INFO(GREEN " [%s] Received listen_socket:%d \n" CLEAR, __func__, listen_socket);

        if (connect(listen_socket, (struct sockaddr *)&client_addr, addr_len) != 0) {
#ifdef ACCEPT_RETRY
            DM_DBG_ERR(RED "[%s] Failed to do connect(0x%x) cnt:%d \n" CLEAR, __func__, -status, retryCount);
            if ((++retryCount > 2) && (!(getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER))))
#else
            if (!(getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER)))
#endif
            {
                __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
            }

            goto wait_udp_accept;
        }

        client_socket = listen_socket;
        listen_socket = -1;    // In case we exit early
        DM_DBG_INFO(GREEN " [%s] listen_socket ==> client_socket(%d) \n" CLEAR, __func__, client_socket);

        listen_socket = udpServerSocketCreate((struct sockaddr *)&local_addr);
        if (listen_socket < 0) {
            DM_DBG_ERR(RED "[%s:%d] Error - socket() \n" CLEAR, __func__, __LINE__);
            errorCallback(listen_socket, "UDP socket create fail");
            stopReason = listen_socket;
            return stopReason;
        }
        sessConfPtr->socket = listen_socket;
        DM_DBG_INFO(" [%s] Create socket(listen_socket: %d) \n", __func__, listen_socket);

        if (client_addr.ss_family == AF_INET) {
            struct sockaddr_in *addr4 = (struct sockaddr_in *)&client_addr;
            client_ip_len = sizeof(addr4->sin_addr.s_addr);

            if (sizeof(client_ip) < client_ip_len) {
                DM_DBG_ERR(RED "[%s] ip length error \n" CLEAR, __func__);
                goto udpServerMsEnd;
            }

            memcpy(client_ip, &addr4->sin_addr.s_addr, client_ip_len);
        } else {
            DM_DBG_ERR(RED "[%s] Not serviced (%d) \n" CLEAR, __func__, client_addr.ss_family);
        }

#ifdef ACCEPT_RETRY
        retryCount = 0;
#endif

        lwip_getpeername(client_socket, (struct sockaddr *)&peer_addr, (socklen_t *)&addr_len);

        longtoip(htonl(peer_addr.sin_addr.s_addr), (char *)peerIpAddr);
        udp_peer_port = htons(peer_addr.sin_port);

        DM_DBG_INFO(GREEN "[%s] Connected client_socket:%d peer_ip:%s(0x%x) peer_port:%d \n" CLEAR,
                           __func__,
                           client_socket,
                           peerIpAddr,
                           htonl(*((int *)client_ip)),
                           udp_peer_port);

        sub_idx = getSessionUdpSubIndexByPort(*((int *)client_ip), udp_peer_port);
        if (sub_idx < 0) {  // Not found index
            sub_idx = getEmptyUdpSubIndex(sessConfPtr);
            if (sub_idx >= DM_UDP_SVR_MAX_SESS) {      // Not found empty index
#ifdef WAITING_ALLOC_EMPTY_SUB_IDX
                goto wait_udp_accept;
#else
                DM_DBG_INFO(BLUE " [%s] Not found empty index \n" CLEAR, __func__);

                // search oldest sub index
                sub_idx = getOldestAccessUdpSubIndex(sessConfPtr);
                if ((sub_idx >= 0) || (sub_idx < DM_UDP_SVR_MAX_SESS )) {
                    udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[sub_idx];

                    // clean sub info
                    tlsShutdownSubSessInfo(udpSubSessInfoPtr);
                    tlsClearSubSessInfo(udpSubSessInfoPtr);
                    DM_DBG_INFO(" [%s] Clear sub_idx(%d) info \n", __func__, sub_idx);
                } else {
                    DM_DBG_ERR(RED "[%s] Not found oldest sub_idx : %d \n" CLEAR, __func__, sub_idx);
                    goto wait_udp_accept;
                }
#endif
            }

            // New one
            DM_DBG_INFO(GREEN "[%s] Alloc new one sub_idx:%d sock:%d peer_ip:%s peer_port:%d \n" CLEAR,
                               __func__, sub_idx, client_socket, peerIpAddr, udp_peer_port);

            initUdpSubSessInfo(sessConfPtr, sub_idx, peerIpAddr, udp_peer_port);
        }

        // Found sub_idx
        udpSubSessInfoPtr = &dpmUserConfigs.udpRxTaskConfig[sub_idx];

wait_stop:
        if (++waitCount < 50) {
            if (udpSubSessInfoPtr->runStatus != SESSION_STATUS_STOP) {
                vTaskDelay(2);
                goto wait_stop;
            } else {
                waitCount = 0;
                DM_DBG_ERR(CYAN " [%s] goto Sub-function (waitCount:%d) \n" CLEAR, __func__, waitCount);
                // goto udpServerSubFunction
            }
        } else {
            waitCount = 0;
            DM_DBG_ERR(RED "[%s] goto Wait accept by wait_timeout(status:%d) \n" CLEAR,
                                                __func__, udpSubSessInfoPtr->runStatus);
            goto wait_udp_accept;
        }

        udpSubSessInfoPtr->runStatus = SESSION_STATUS_INIT;
        udpSubSessInfoPtr->clientSock = client_socket;

        tlsRestoreSubSessInfo(udpSubSessInfoPtr);

        // sub function
        DM_DBG_INFO(CYAN " [%s] Call Sub-function (#%d) \n" CLEAR, __func__, sub_idx);
        status = udpServerSubFunction(udpSubSessInfoPtr);        // udpServerSubFunction
        if (status != ERR_OK) {
            DM_DBG_ERR(RED "[%s] Failed to return by sub-function #%d (status=0x%x)...\n" CLEAR,
                                                                    __func__, sub_idx, status);
        }
        tlsDeinitSubSessInfo(udpSubSessInfoPtr);

        goto wait_udp_accept;

udpServerMsEnd:

        close(sessConfPtr->socket);
        shutdown(sessConfPtr->socket, SHUT_RDWR);
        sessConfPtr->socket = -1;

        if (sessConfPtr->rxDataBuffer != NULL) {
            vPortFree(sessConfPtr->rxDataBuffer);
            sessConfPtr->rxDataBuffer = NULL;
        }

        dpm_app_unregister(sessConfPtr->dpmRegName);
        DM_DBG_INFO(" [%s] Stop UDP server by reason:0x%x (session no:%d) \n",
                            __func__, stopReason, sessConfPtr->sessionNo);
        sessConfPtr->runStatus = SESSION_STATUS_STOP;

        return stopReason;
    }

udp_server_restart:

    sessConfPtr->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sessConfPtr->socket == -1) {
        DM_DBG_ERR(RED "[%s:%d] Error - socket() \n" CLEAR, __func__, __LINE__);
        errorCallback(DM_ERROR_SOCKET_CREATE_FAIL, "UDP socket create fail");
        stopReason = DM_ERROR_SOCKET_CREATE_FAIL;
        return stopReason;
    }
    DM_DBG_INFO(" [%s] Create socket(%d) \n", __func__, sessConfPtr->socket);

    status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
    }

    status = bind(sessConfPtr->socket, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if (status < 0) {
        DM_DBG_ERR(RED "[%s:%d] Error - bind(%d) \n" CLEAR, __func__, __LINE__, status);
        errorCallback(DM_ERROR_SOCKET_BIND_FAIL, "UDP socket bind fail");
        stopReason = DM_ERROR_SOCKET_BIND_FAIL;
        goto udpServerEnd;
    }

    DM_DBG_INFO(" [%s] Bind socket(%d) \n", __func__, sessConfPtr->socket);

    tv.tv_sec = 0;
    tv.tv_usec = UDP_SVR_RX_TIMEOUT * 1000;

    status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
    }

    dpm_app_data_rcv_ready_set(sessConfPtr->dpmRegName);        // Noti to umac(Can receive packet)

    while (1) {
        /* If dpm power down scheme was already started,
         * skip next udp rx operataion.
         */
        if (dpm_sleep_is_started()) {
            vTaskDelay(10);
            continue;
        }

        sessConfPtr->runStatus = SESSION_STATUS_RUN;

        status = recvfrom(sessConfPtr->socket,
                    sessConfPtr->rxDataBuffer,
                    UDP_CLIENT_RX_BUF_SIZE,
                    0,
                    (struct sockaddr *)&peer_addr,
                    (socklen_t *)&addr_len);

        udp_peer_ip = htonl(peer_addr.sin_addr.s_addr);
        udp_peer_port = htons(peer_addr.sin_port);

        //DM_DBG_INFO(" [%s:%d] recvfrom (peer_ip:0x%x peer_port:%d status:%d \n",
        //                    __func__, __LINE__, udp_peer_ip, udp_peer_port, status);

        // Disable to enter DPM SLEEP
        __clr_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int)__LINE__);

        if (status >= 0) {      // OK
            DM_DBG_INFO(CYAN " [%s:%d] recv packet OK (prt:0x%x socket:%d size:%d) \n" CLEAR,
                                                __func__, __LINE__, sessConfPtr, sessConfPtr->socket, status);

            *(sessConfPtr->rxDataBuffer+status) = 0;
            receiveCallback(sessConfPtr,
                            sessConfPtr->rxDataBuffer,
                            status,
                            udp_peer_ip,
                            udp_peer_port);
        } else if (status == -1) {      // no packet, timeout
            vTaskDelay(1);

            if (sessConfPtr->requestCmd == CMD_REQUEST_STOP) {
                DM_DBG_ERR(" [%s] Stop UDP Server by request \n", __func__, status);

                stopReason = ER_DELETED;
                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
                goto udpServerEnd;
            }
        } else if (status == ERR_UNKNOWN) {
            DM_DBG_ERR(RED "[%s] Failed to unknown receive error (0x%x)\n", __func__, status);

            close(sessConfPtr->socket);
            shutdown(sessConfPtr->socket, SHUT_RDWR);
            sessConfPtr->socket = -1;

            errorCallback(DM_ERROR_SECURE_UNKNOWN_FAIL, "Unaware failure");
            stopReason = DM_ERROR_SECURE_UNKNOWN_FAIL;

            goto udp_server_restart;

        } else {       // ??????
            DM_DBG_ERR(RED "[%s:%d] Unknown recvfrom error status:%d \n" CLEAR,
                           __func__, __LINE__, status);
        }

        if (   (getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_SERVER))
            || (getPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_SERVER)) ) {
            if (delayCountBySendingFlag++ < 100) {
                //DM_DBG_INFO(" [%s] send/recv pending (%d) \n", __func__, delayCountBySendingFlag);
                continue;
            } else {
                DM_DBG_ERR(RED "[%s] send/recv pending \n" CLEAR, __func__);
            }
            delayCountBySendingFlag = 0;
        }

        // Set flag to permit goto DPM sleep mode
        __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
    }

udpServerEnd:

    close(sessConfPtr->socket);
    shutdown(sessConfPtr->socket, SHUT_RDWR);
    sessConfPtr->socket = -1;

    if (sessConfPtr->rxDataBuffer != NULL) {
        vPortFree(sessConfPtr->rxDataBuffer);
        sessConfPtr->rxDataBuffer = NULL;
    }

    dpm_app_unregister(sessConfPtr->dpmRegName);
    DM_DBG_INFO("[%s] Stop UDP server by reason:0x%x (session no:%d) \n",
                 __func__, stopReason, sessConfPtr->sessionNo);
    sessConfPtr->runStatus = SESSION_STATUS_STOP;

    return stopReason;
}

static void dpmUdpServerManager(void *sessionNo)
{
    session_config_t *sessConfPtr;

    if (!(UINT)sessionNo || (UINT)sessionNo > MAX_SESSION_CNT) {
        DM_DBG_ERR(RED "[%s] Session Start Error(sessionNo:%d) \n" CLEAR, __func__, sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "Session start fail");
        goto US_taskEnd;
    }

    sessConfPtr = &dpmUserConfigs.sessionConfig[(UINT)sessionNo - 1];
    sessConfPtr->sessionNo = (UINT)sessionNo;

    if (initSessionCommon(sessConfPtr)) {
        DM_DBG_ERR(RED "[%s] UDP Server Start Error(sessionNo:%d) \n" CLEAR,
                       __func__, (UINT)sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "UDP server start fail");
        goto US_taskEnd;
    }

    DM_DBG_INFO(CYAN "[%s] Started dpmUdpServerManager session no:%d \n" CLEAR,
                      __func__, (UINT)sessionNo);
    runUdpServer(sessConfPtr);

    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        // Delete UDP Filter for dpm
        dpm_udp_port_filter_delete(sessConfPtr->sessionMyPort);        // del_dpm_udp_port_filter
    }

    deinitSessionCommon(sessConfPtr);

US_taskEnd:
    DM_DBG_ERR(RED "[%s] Stoped dpmUdpServerManager session no:%d \n" CLEAR, __func__, (UINT)sessionNo);

    vTaskDelete(NULL);
}
//
// end dpm UDP Server Manager
//
#endif    // !__LIGHT_DPM_MANAGER__

//
// start dpm UDP Client Manager
//
static int sendUdpClient(session_config_t *sessConfPtr, char *data_buf, UINT len_buf)
{
    UINT    ret;
    int        status = ERR_OK;
    ULONG    udp_peer_ip;
    USHORT    udp_peer_port;
    struct    sockaddr_in    peer_addr;
    SECURE_INFO_T    *config = NULL;

    udp_peer_ip = (ULONG)iptolong(sessConfPtr->sessionServerIp);
    udp_peer_port = (USHORT)sessConfPtr->sessionServerPort;

    DM_DBG_INFO(CYAN " [%s] send UDP packet ip:0x%x(%s), port:%d len:%d \n" CLEAR,
                __func__, udp_peer_ip, sessConfPtr->sessionServerIp, udp_peer_port, len_buf);

    /* At this case,,, dpm_sleep operation was started already */
    if (chk_dpm_pdown_start()) {
        DM_DBG_ERR(RED "[%s] Error - In progressing DPM sleep !!!" CLR_COL, __func__);
        return ER_NOT_SUCCESSFUL;
    }

    setPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);

    if (!sessionRunConfirm(sessConfPtr, 200)) {  // 2 sec wait for running status
        DM_DBG_ERR(RED "[%s] Not ready to send \n" CLEAR, __func__);
        clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);
        return ER_NOT_ENABLED;
    }

#ifdef FOR_DEBUGGING_PKT_DUMP
    hexa_dump_print("sendto Packet ", data_buf, len_buf, 0, 2);
#endif

    if (sessConfPtr->supportSecure) {
        DM_DBG_INFO(CYAN "[%s] Send TLS Packet(data_buf:%s, len_buf:%d) \n" CLEAR,
                          __func__, data_buf, len_buf);
        config = &sessConfPtr->secureConfig;

        if (config->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
            status = (int)dmSecureSend(config, data_buf, len_buf, UDP_CLIENT_DEF_WAIT_TIME);
            if (status) {
                DM_DBG_ERR(RED "[%s] Failed UDP send packet(buf:0x%x len:%d) \n" CLEAR,
                                                                        __func__, data_buf, len_buf);
            } else {
                ret = tls_save_ssl(sessConfPtr->sessTaskName, config);
                if (ret) {
                    DM_DBG_ERR(" [%s:%d] Failed to save tls session(0x%02x)\n", __func__, __LINE__, ret);
                }
            }
        } else {
            DM_DBG_ERR(RED "[%s] Not yet handshake \n" CLEAR, __func__, data_buf, len_buf);
            status = ER_NOT_SUCCESSFUL;
        }

        clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);        // clear by Callback function
        return status;
    }

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = htonl(udp_peer_ip);
    peer_addr.sin_port = htons(udp_peer_port);

    // Send data to UDP server
    status = sendto(sessConfPtr->socket, data_buf, len_buf, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    if (status != (int)len_buf) {
        DM_DBG_ERR(RED "[%s] Failed to data send (0x%02x)\n" CLEAR, __func__, status);
        errorCallback(DM_ERROR_SEND_FAIL, "Fail to udp client socket send");

        clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);
        return status;
    }

    clrPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT);
    return ERR_OK;
}

#if defined ( KEEPALIVE_UDP_CLI )
static void dpm_udp_to_cb(char *timer_name)
{
    UCHAR    dpm_udp_tid = 0;

    dpm_udp_tid = rtc_register_timer(UDP_PERIOD_SEC * 1000, sessConfPtr->dpmRegName, dpm_udp_tid, 0, dpm_udp_to_cb); //msec
}
#endif    // KEEPALIVE_UDP_CLI

UINT dmSecureUdpClientHandshake(SECURE_INFO_T *config)        // UDP CLIENT HANDSHAKE
{
    int  status;
    struct timeval   tv;

    if (startHandshaking()) {
        DM_DBG_ERR(RED "[%s] Failed to wait_timeout \n" CLEAR, __func__);
        return DM_ERROR_HANDSHAKE_FAIL;
    }
    DM_DBG_INFO(" [%s] Start DTLS handshake \n", __func__);

    tv.tv_sec = 0;
    tv.tv_usec = UDP_CLI_RX_SECURE_TIMEOUT * 1000;
    status = setsockopt(config->server_ctx->fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
        status = DM_ERROR_HANDSHAKE_FAIL;
        goto end_of_udp_client_handshake;
    }

    while ((status = mbedtls_ssl_handshake(config->ssl_ctx)) != 0) {
        if (status == MBEDTLS_ERR_NET_CONN_RESET) {
            DM_DBG_ERR("\n [%s] Peer closed the connection(0x%x)\n", __func__, -status);
            status = DM_ERROR_HANDSHAKE_FAIL;
            goto end_of_udp_client_handshake;
        }

        if ((status != MBEDTLS_ERR_SSL_WANT_READ) && (status != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            DM_DBG_ERR(RED "[%s] Failed to do DTLS handshake(0x%x)\n" CLEAR, __func__, -status);
            status = DM_ERROR_HANDSHAKE_FAIL;
            goto end_of_udp_client_handshake;
        }
    }

    DM_DBG_INFO(BLUE " [%s] Success DTLS handshake \n" CLEAR, __func__);

    status = ERR_OK;

end_of_udp_client_handshake:
    endHandshaking();

    return (UINT)status;
}

static int runUdpClient(session_config_t *sessConfPtr)
{
    int     status = ERR_OK;
    int     ret;
    int     stopReason = 0;
    int     optval = 1;
    struct sockaddr_in    local_addr;
    struct sockaddr_in    peer_addr;
    ULONG   udp_peer_ip;
    UINT    udp_peer_port;
    int     addr_len;
    struct  timeval    tv;
    UINT    delayCountBySendingFlag = 0;
    UINT    countByUnknownError = 0;
    SECURE_INFO_T    *config = &sessConfPtr->secureConfig;
    struct sockaddr_in    srv_addr;
    char   *ip_addr_str;
    int    udp_server_port;

#if defined ( KEEPALIVE_UDP_CLI )
    UCHAR    dpm_udp_tid = 0;
#endif    // KEEPALIVE_UDP_CLI

    DM_DBG_INFO(" [%s] Start \n", __func__);

udp_client_restart:

    ip_addr_str = sessConfPtr->sessionServerIp;
    udp_server_port = (int)sessConfPtr->sessionServerPort;

    config->sessionConfPtr = sessConfPtr;

    sessConfPtr->runStatus = SESSION_STATUS_INIT;

    // Allocate RX data buffer
    if (sessConfPtr->rxDataBuffer == NULL) {
        sessConfPtr->rxDataBuffer = pvPortMalloc(UDP_CLIENT_RX_BUF_SIZE);
        if (sessConfPtr->rxDataBuffer == NULL) {
            DM_DBG_ERR(RED "[%s] rxDataBuffer memory alloc fail \n" CLEAR, __func__);

            errorCallback(DM_ERROR_RX_BUFFER_ALLOC_FAIL, "UDP client Rx buffer memory alloc fail");
            stopReason = DM_ERROR_RX_BUFFER_ALLOC_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_STOP;
            goto udpClientEnd;
        }
    }

    DM_DBG_INFO(CYAN "[%s] UDP client start (name:%s svrIp:%s svrPort:%d local_port:%d) \n" CLEAR,
                      __func__,
                      sessConfPtr->dpmRegName,
                      sessConfPtr->sessionServerIp,
                      sessConfPtr->sessionServerPort,
                      sessConfPtr->localPort);

    // SUPPORT_SECURE
    if (sessConfPtr->supportSecure) {
        struct netif *netif;

        config->sessionType = sessConfPtr->sessionType;
        config->localPort = sessConfPtr->localPort;

        netif = netif_get_by_index(2);    // WLAN0:2, WLAN1:3
        sprintf(config->localIp, "%d.%d.%d.%d",
                                    ip4_addr1_16(&netif->ip_addr),
                                    ip4_addr2_16(&netif->ip_addr),
                                    ip4_addr3_16(&netif->ip_addr),
                                    ip4_addr4_16(&netif->ip_addr));

        DM_DBG_INFO(" [%s] type:%d local address: %s:%d \n",
                __func__, config->sessionType, config->localIp, config->localPort);

        config->timeout = UDP_CLIENT_DEF_WAIT_TIME;

        ssl_ctx_clear(config);

        status = (int)dmSecureInit(config);    // UDP CLIENT
        if (status) {
            DM_DBG_ERR(" [%s] Failed to init ssl(0x%02x) \n", __func__, status);

            stopReason = status;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto udpClientEnd_act;
        }

        config->server_ctx->fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (config->server_ctx->fd == -1) {
            DM_DBG_ERR(RED "[%s:%d] Error - socket() \n" CLEAR, __func__, __LINE__);
            errorCallback(DM_ERROR_SOCKET_CREATE_FAIL, "UDP socket create fail");
            stopReason = DM_ERROR_SOCKET_CREATE_FAIL;
            return stopReason;
        }
        DM_DBG_INFO(" [%s] Create socket(%d) \n", __func__, config->server_ctx->fd);

        srv_addr.sin_family = AF_INET;
        srv_addr.sin_addr.s_addr = inet_addr(ip_addr_str);
        srv_addr.sin_port = htons((u16_t)udp_server_port);

        DM_DBG_INFO(" [%s] Request connect to %s:%d \n", __func__, ip_addr_str, udp_server_port);

        if (connect(config->server_ctx->fd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr) ) != 0 ) {
            DM_DBG_ERR(RED "[%s:%d] Error - connect() \n" CLEAR, __func__, __LINE__);
            errorCallback(DM_ERROR_SOCKET_CONNECT_FAIL, "UDP socket connect fail");
            stopReason = DM_ERROR_SOCKET_CONNECT_FAIL;
            return stopReason;
        }

        sessConfPtr->socket = config->server_ctx->fd;
        DM_DBG_INFO(" [%s] Secure init done (socket:%d) \n", __func__, sessConfPtr->socket);

        memset(&local_addr, 0x00, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons((u16_t)sessConfPtr->localPort);

        DM_DBG_INFO(" [%s:%d] bind(local_port:%d) \n" CLEAR, __func__, __LINE__, sessConfPtr->localPort);
        status = bind(sessConfPtr->socket, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
        if (status < 0) {
            DM_DBG_ERR(RED "[%s:%d] Error - bind(%d) \n" CLEAR, __func__, __LINE__, status);

            errorCallback(DM_ERROR_SOCKET_BIND_FAIL, "UDP client socket bind fail");
            stopReason = DM_ERROR_SOCKET_BIND_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;

            goto udpClientEnd_act;
        }

        setupSecureCallback(sessConfPtr, config);    // call back : secure config setup

        status = (int)tls_restore_ssl(sessConfPtr->sessTaskName, config);
        if (status) {
            DM_DBG_ERR(" [%s] Failed to restore ssl(0x%02x)\n", __func__, status);

            stopReason = status;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto udpClientEnd_act;
        }

        memset(&config->timer, 0x00, sizeof(SECURE_TIMER));
        mbedtls_ssl_set_timer_cb(config->ssl_ctx,
                                &config->timer,
                                dmDtlsTimerStart,
                                dmDtlsTimerGetStatus);

        mbedtls_ssl_set_bio(config->ssl_ctx,
                            (void *)config,
                            dmSecureSendCallback,
                            NULL,
                            dmSecureRecvTimeoutCallback);

        DM_DBG_INFO(" [%s] set_bio init done \n", __func__);
    } else {
        sessConfPtr->socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (sessConfPtr->socket == -1) {
            DM_DBG_ERR(RED "[%s:%d] Error - socket() \n" CLEAR, __func__, __LINE__);
            errorCallback(DM_ERROR_SOCKET_CREATE_FAIL, "UDP client socket create fail");
            stopReason = DM_ERROR_SOCKET_CREATE_FAIL;
            goto udpClientEnd;
        }
        DM_DBG_INFO(" [%s] Create socket(%d) \n", __func__, sessConfPtr->socket);

        status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
        if (status) {
            DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
        }

        memset(&local_addr, 0x00, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons((u16_t)sessConfPtr->localPort);

        status = bind(sessConfPtr->socket, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
        if (status < 0) {
            DM_DBG_ERR(RED "[%s:%d] Error - bind(%d) \n" CLEAR, __func__, __LINE__, status);

            errorCallback(DM_ERROR_SOCKET_BIND_FAIL, "UDP client socket bind fail");
            stopReason = DM_ERROR_SOCKET_BIND_FAIL;
            goto udpClientEnd_act;
        }

        DM_DBG_INFO(" [%s] Bind socket(%d) \n", __func__, sessConfPtr->socket);
    }

    // Notice initialize done to DPM module
    dpm_app_wakeup_done(sessConfPtr->dpmRegName);                // Noti to timer (Can receive timer ISR)
    DM_DBG_INFO(" [%s] Init done (socket:%d) \n", __func__, sessConfPtr->socket);
    dpm_app_data_rcv_ready_set(sessConfPtr->dpmRegName);        // Noti to umac(Can receive packet)
    DM_DBG_INFO(" [%s] Ready receive \n", __func__);

#if defined ( KEEPALIVE_UDP_CLI )

    // RTC Timer
    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        dpm_udp_tid = rtc_register_timer(UDP_PERIOD_SEC * 1000, sessConfPtr->dpmRegName, dpm_udp_tid, 0, dpm_udp_to_cb); //msec
    }

#endif    // KEEPALIVE_UDP_CLI

    if (sessConfPtr->supportSecure) {
        if (dmSecureUdpClientHandshake(config)) {
            stopReason = DM_ERROR_HANDSHAKE_FAIL;
            sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
            goto udpClientEnd_act;
        } else {
            status = (int)tls_save_ssl(sessConfPtr->sessTaskName, config);
            if (status) {
                DM_DBG_ERR(" [%s:%d] Failed to save tls session(0x%02x)\n", __func__, __LINE__, status);
                stopReason = DM_ERROR_SAVE_SSL_FAIL;
                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
                goto udpClientEnd_act;
            }
        }
    }

#if defined ( __SUPPORT_HOLEPUNCH__ )
    //////////// HOLEPUNCH ///////////////
    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        ULONG        udp_peer_ip;
        unsigned long    peer_ip_addr = 0;
        unsigned char    *peer_ip_ptr = (unsigned char *)&peer_ip_addr;

        udp_peer_ip = iptolong(sessConfPtr->sessionServerIp);

        if (udp_peer_ip && sessConfPtr->sessionServerPort) {

            *peer_ip_ptr++ = (udp_peer_ip >> 24) & 0x0ff;
            *peer_ip_ptr++ = (udp_peer_ip >> 16) & 0x0ff;
            *peer_ip_ptr++ = (udp_peer_ip >>  8) & 0x0ff;
            *peer_ip_ptr++ = (udp_peer_ip >>  0) & 0x0ff;

            //set udp hole punching
            dpm_udp_hole_punch(1,
                            peer_ip_addr,
                            (unsigned short)sessConfPtr->sessionMyPort,
                            (unsigned short)sessConfPtr->sessionServerPort);
            DM_DBG_INFO(" Set holepunch (UDP Client) [my_port:%d server_ip:0x%x server_port:%d] \n",
                        sessConfPtr->sessionMyPort, peer_ip_addr, sessConfPtr->sessionServerPort);
        }
    }

    //////////// HOLEPUNCH ///////////////
#endif    // __SUPPORT_HOLEPUNCH__

    udp_peer_ip = (ULONG)iptolong(sessConfPtr->sessionServerIp);
    udp_peer_port = sessConfPtr->sessionServerPort;

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = htonl(udp_peer_ip);
    peer_addr.sin_port = htons((u16_t)udp_peer_port);

    if (sessConfPtr->supportSecure) {
        tv.tv_sec = 0;
        tv.tv_usec = UDP_CLI_RX_SECURE_TIMEOUT * 1000;
    } else {
        tv.tv_sec = 0;
        tv.tv_usec = UDP_CLI_RX_TIMEOUT * 1000;
    }

    status = setsockopt(sessConfPtr->socket, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (status) {
        DM_DBG_ERR(RED "[%s:%d] setsockopt error %d \n" CLEAR, __func__, __LINE__, status);
    }

    if (sessConfPtr->supportSecure) {
        DM_DBG_INFO(CYAN "[%s] Start loop sock:%d(%d) local_port:%d(%d) server_port:%d server_ip:%s \n"
                          CLEAR,
                          __func__,
                          sessConfPtr->socket,
                          config->server_ctx->fd,
                          sessConfPtr->localPort,
                          config->localPort,
                          sessConfPtr->sessionServerPort,
                          sessConfPtr->sessionServerIp);
    }

    while (1) {
        /* If dpm power down scheme was already started,
         * skip next udp rx operataion.
         */
        if (dpm_sleep_is_started()) {
            vTaskDelay(10);
            continue;
        }

        sessConfPtr->runStatus = SESSION_STATUS_RUN;

        if (sessConfPtr->supportSecure) {
            status = dmSecureRecv(config, sessConfPtr->rxDataBuffer, SSL_READ_TIMEOUT);
            if (status == ERR_TIMEOUT) {
                status = -1;
            } else if (status == ERR_CONN) {
                status = 0;
            }
        } else {
            //memset(sessConfPtr->rxDataBuffer, 0, UDP_CLIENT_RX_BUF_SIZE);
            status = recvfrom(sessConfPtr->socket,
                            sessConfPtr->rxDataBuffer,
                            UDP_CLIENT_RX_BUF_SIZE,
                            0,
                            (struct sockaddr *)&peer_addr,
                            (socklen_t *)&addr_len);
        }

        // Disable to enter DPM SLEEP
        __clr_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int)__LINE__);

        if (status > 0) {   // OK
            countByUnknownError = 0;
            DM_DBG_INFO(CYAN " [%s:%d] recv packet OK (size:%d) \n" CLEAR, __func__, __LINE__, status);

            *(sessConfPtr->rxDataBuffer+status) = 0;
            receiveCallback(sessConfPtr, sessConfPtr->rxDataBuffer, (UINT)status, udp_peer_ip, udp_peer_port);
        } else if (status == -1) {      // no packet, timeout
            vTaskDelay(1);

            if (sessConfPtr->requestCmd == CMD_REQUEST_STOP) {
                sessConfPtr->requestCmd = CMD_NONE;
                DM_DBG_ERR(" [%s] Stop UDP Client by request \n", __func__, status);

                stopReason = DM_ERROR_STOP_REQUEST;
                sessConfPtr->runStatus = SESSION_STATUS_GOING_STOP;
                goto udpClientEnd_act;
            }
        } else {  // ??????
            DM_DBG_ERR(RED "[%s:%d] Unknown recvfrom error status:%d \n" CLEAR,
                            __func__, __LINE__, status);
            if (sessConfPtr->supportSecure) {
                close(sessConfPtr->socket);
                shutdown(sessConfPtr->socket, SHUT_RDWR);
                sessConfPtr->socket = -1;

                dmSecureDeinit(config);
                tls_clear_ssl(sessConfPtr->sessTaskName, config);
                goto udp_client_restart;
            }

            if (++countByUnknownError > 10) {
                close(sessConfPtr->socket);
                shutdown(sessConfPtr->socket, SHUT_RDWR);
                sessConfPtr->socket = -1;

                goto udp_client_restart;
            }
        }

        if (   (getPacketSendingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT))
            || (getPacketReceivingStatus(sessConfPtr, REG_TYPE_UDP_CLIENT)) ) {
            if (delayCountBySendingFlag++ < 100) {
                continue;
            } else {
                DM_DBG_ERR(RED "[%s] send/recv pending \n" CLEAR, __func__);
            }

            delayCountBySendingFlag = 0;
        }

        // Save dtls session
        if (sessConfPtr->supportSecure) {
            ret = (int)tls_save_ssl(sessConfPtr->sessTaskName, config);
            if (ret) {
                DM_DBG_ERR(" [%s] Failed to save dtls sess(0x%02x)\n", __func__, ret);
            }
        }

        // Set flag to permit goto DPM sleep mode
        __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
    }

udpClientEnd_act:
    close(sessConfPtr->socket);
    shutdown(sessConfPtr->socket, SHUT_RDWR);
    sessConfPtr->socket = -1;

    if (sessConfPtr->supportSecure) {
        dmSecureDeinit(config);
        tls_clear_ssl(sessConfPtr->sessTaskName, config);
    }

    if (sessConfPtr->rxDataBuffer != NULL) {
        vPortFree(sessConfPtr->rxDataBuffer);
        sessConfPtr->rxDataBuffer = NULL;
    }

udpClientEnd:
    DM_DBG_INFO("[%s] Stop UDP client by reason:0x%x (session no:%d) \n",
                 __func__, stopReason, sessConfPtr->sessionNo);

    __set_dpm_sleep(sessConfPtr->dpmRegName, (char *)__func__, (int) __LINE__);
    sessConfPtr->runStatus = SESSION_STATUS_STOP;

    return stopReason;
}

static void dpmUdpClientManager(void *sessionNo)
{
    int    status;
    session_config_t *sessConfPtr = NULL;

    if (!(UINT)sessionNo || (UINT)sessionNo > MAX_SESSION_CNT) {
        DM_DBG_ERR(RED "[%s] Session Start Error(sessionNo:%d) \n" CLEAR,
                        __func__, (UINT)sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "Session start fail");
        goto UC_taskEnd;
    }

    sessConfPtr = &dpmUserConfigs.sessionConfig[(UINT)sessionNo - 1];
    sessConfPtr->sessionNo = (UINT)sessionNo;

    if (initSessionCommon(sessConfPtr)) {
        DM_DBG_ERR(RED "[%s] UDP Client Start Error(sessionNo:%d) \n" CLEAR,
                        __func__, (UINT)sessionNo);
        errorCallback(DM_ERROR_SESSION_START_FAIL, "UDP client start fail");
        goto UC_taskEnd;
    }

    DM_DBG_INFO(CYAN "[%s] Started dpmUdpClientManager session no:%d \n" CLEAR,
                      __func__, (UINT)sessionNo);
loop:
    status = runUdpClient(sessConfPtr);

    if ((status == DM_ERROR_STOP_REQUEST) || (status == DM_ERROR_RX_BUFFER_ALLOC_FAIL)) {  // End
#ifdef CHANGE_UDP_LOCAL_PORT_BY_RESTART
        sessConfPtr->localPort = 0;        // change local port number by stop/start
#endif
        deinitSessionCommon(sessConfPtr);
    } else {  // restart
        vTaskDelay(500);
        goto loop;
    }

UC_taskEnd:
    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        dpm_udp_port_filter_delete((unsigned short)sessConfPtr->localPort);        // del_dpm_udp_port_filter
        dpm_app_unregister(sessConfPtr->dpmRegName);
    }
    DM_DBG_INFO(CYAN "[%s] Stoped dpmUdpClientManager session no:%d \n" CLEAR,
                      __func__, (UINT)sessionNo);

    vTaskDelete(NULL);
}
//
// end dpm UDP Client Manager
//

static void dpmManagerTimer1Callback(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    BaseType_t xHigherPriorityTaskWoken, xResult;

    if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
        DM_DBG_INFO(" [%s] Call timer1 event FromISR \n", __func__);
        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(dpmManagerEventGroup, DPM_EVENT_TIMER1, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
              * switch should be requested. The macro used is port specific and will
              * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
              * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        DM_DBG_INFO(" [%s] Call timer1 event \n", __func__);
        xEventGroupSetBits(dpmManagerEventGroup, DPM_EVENT_TIMER1);
    }
}

static void dpmManagerTimer2Callback(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    BaseType_t xHigherPriorityTaskWoken, xResult;

    if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
        DM_DBG_INFO(" [%s] Call timer2 event FromISR \n", __func__);
        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(dpmManagerEventGroup, DPM_EVENT_TIMER2, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
              * switch should be requested. The macro used is port specific and will
              * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
              * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        DM_DBG_INFO(" [%s] Call timer2 event \n", __func__);
        xEventGroupSetBits(dpmManagerEventGroup, DPM_EVENT_TIMER2);
    }
}

static void dpmManagerTimer3Callback(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    BaseType_t xHigherPriorityTaskWoken, xResult;

    if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
        DM_DBG_INFO(" [%s] Call timer3 event FromISR \n", __func__);
        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(dpmManagerEventGroup, DPM_EVENT_TIMER3, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
              * switch should be requested. The macro used is port specific and will
              * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
              * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        DM_DBG_INFO(" [%s] Call timer3 event \n", __func__);
        xEventGroupSetBits(dpmManagerEventGroup, DPM_EVENT_TIMER3);
    }
}

static void dpmManagerTimer4Callback(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    BaseType_t xHigherPriorityTaskWoken, xResult;

    if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
        DM_DBG_INFO(" [%s] Call timer4 event FromISR \n", __func__);
        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(dpmManagerEventGroup, DPM_EVENT_TIMER4, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
              * switch should be requested. The macro used is port specific and will
              * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
              * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        DM_DBG_INFO(" [%s] Call timer4 event \n", __func__);
        xEventGroupSetBits(dpmManagerEventGroup, DPM_EVENT_TIMER4);
    }
}

static void startSessTask(UINT type, UINT sessionNo)
{
    VOID    (*functionPtr)(void * arg) = NULL;
    session_config_t    *sessConfPtr;

    UINT taskPriority = tskIDLE_PRIORITY;

    sessConfPtr = &dpmUserConfigs.sessionConfig[sessionNo - 1];

    if (sessConfPtr->is_force_stopped) {
        DM_DBG_INFO(" [%s] This session is forcibly stopped(type:%d, sessionNo:%d)\n", __func__,
                type, sessionNo);
        return ;
    }

    if (sessConfPtr->sessionStackSize == 0) {
#if !defined (__LIGHT_DPM_MANAGER__)
        if (type == REG_TYPE_TCP_SERVER) {
            sessConfPtr->sessionStackSize = SESSION_TASK_TCP_SVR_STACK_SIZE;
        } else if (type == REG_TYPE_UDP_SERVER) {
            sessConfPtr->sessionStackSize = SESSION_TASK_UDP_SVR_STACK_SIZE;
        } else
#endif    // !__LIGHT_DPM_MANAGER__
		{
            sessConfPtr->sessionStackSize = SESSION_TASK_STACK_SIZE;
		}
    }

    sessConfPtr->sessionNo = sessionNo;
    memset(sessConfPtr->dpmRegName, 0, TASK_NAME_LEN);
    sprintf(sessConfPtr->dpmRegName, "DPM_SESS%d_TASK", sessConfPtr->sessionNo);

    memset(sessConfPtr->sessTaskName, 0, 20);
    sessConfPtr->runStatus = SESSION_STATUS_READY_TO_CREATE;

#if !defined (__LIGHT_DPM_MANAGER__)
    if (type == REG_TYPE_TCP_SERVER) {
        sprintf(sessConfPtr->sessTaskName, "dpmTcpSvr_%d", sessionNo);
        functionPtr = dpmTcpServerManager;
        taskPriority = TCP_SERVER_SESSION_TASK_PRIORITY;

        // Register DPM module & filter port
        if (dpm_mode_is_enabled() == DPM_ENABLED) {
            dpm_app_register(sessConfPtr->dpmRegName, sessConfPtr->sessionMyPort);
            DM_DBG_INFO(" [%s] Reg DPM SLEEP %s \n", __func__, sessConfPtr->dpmRegName);

            //Set TCP Filter for dpm
            dpm_tcp_port_filter_set(sessConfPtr->sessionMyPort);    // set_dpm_tcp_port_filter
            DM_DBG_INFO(" [%s] Set TCP Filter Port %d \n",
                    __func__, (unsigned short)sessConfPtr->sessionMyPort);
        }
    } else if (type == REG_TYPE_UDP_SERVER) {
        sprintf(sessConfPtr->sessTaskName, "dpmUdpSvr_%d", sessionNo);
        functionPtr = dpmUdpServerManager;
        taskPriority = UDP_SERVER_SESSION_TASK_PRIORITY;

        // Register DPM module & filter port
        if (dpm_mode_is_enabled() == DPM_ENABLED) {
            dpm_app_register(sessConfPtr->dpmRegName, sessConfPtr->sessionMyPort);
            DM_DBG_INFO(CYAN " [%s] Reg DPM SLEEP %s \n" CLEAR, __func__, sessConfPtr->dpmRegName);

            //Set UDP Filter for dpm
            dpm_udp_port_filter_set(sessConfPtr->sessionMyPort);        // set_dpm_udp_port_filter
            DM_DBG_INFO(CYAN " [%s] Set UDP Filter Port %d \n" CLEAR,
                    __func__, (unsigned short)sessConfPtr->sessionMyPort);
        }
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    if (type == REG_TYPE_TCP_CLIENT) {
        sprintf(sessConfPtr->sessTaskName, "dpmTcpCli_%d", sessionNo);
        functionPtr = dpmTcpClientManager;
        taskPriority = TCP_CLIENT_SESSION_TASK_PRIORITY;

        if (sessConfPtr->sessionMyPort) {
            sessConfPtr->localPort = sessConfPtr->sessionMyPort;
        } else {
            if (!sessConfPtr->localPort) {
                sessConfPtr->localPort = generateClientLocalPort(sessConfPtr->sessionType);
            }
        }

        // Register DPM module & filter port
        if (dpm_mode_is_enabled() == DPM_ENABLED) {
            dpm_app_register(sessConfPtr->dpmRegName, sessConfPtr->localPort);
            DM_DBG_INFO(CYAN " [%s] Reg DPM SLEEP %s \n" CLEAR, __func__, sessConfPtr->dpmRegName);

            //Set TCP Filter for dpm
            dpm_tcp_port_filter_set((unsigned short)sessConfPtr->localPort);    // set_dpm_tcp_port_filter
            DM_DBG_INFO(CYAN " [%s] Set TCP Filter Port %d \n" CLEAR,
                    __func__, (unsigned short)sessConfPtr->localPort);
        }
    } else if (type == REG_TYPE_UDP_CLIENT) {
        sprintf(sessConfPtr->sessTaskName, "dpmUdpCli_%d", sessionNo);
        functionPtr = dpmUdpClientManager;
        taskPriority = UDP_CLIENT_SESSION_TASK_PRIORITY;

        if (sessConfPtr->sessionMyPort) {
            sessConfPtr->localPort = sessConfPtr->sessionMyPort;
        } else {
            if (!sessConfPtr->localPort) {
                sessConfPtr->localPort = generateClientLocalPort(sessConfPtr->sessionType);
            }
        }

        // Register DPM module & filter port
        if (dpm_mode_is_enabled() == DPM_ENABLED) {
            dpm_app_register(sessConfPtr->dpmRegName, sessConfPtr->localPort);
            DM_DBG_INFO(CYAN " [%s] Reg DPM SLEEP %s \n" CLEAR, __func__, sessConfPtr->dpmRegName);

            dpm_udp_port_filter_set((unsigned short)sessConfPtr->localPort);        // set_dpm_udp_port_filter
            DM_DBG_INFO(CYAN " [%s] Set UDP Filter Port %d \n" CLEAR,
                    __func__, (unsigned short)sessConfPtr->localPort);
        }
    }

    DM_DBG_INFO(" [%s] Create session task(name:%s, sessNo:%d) \n",
                __func__, sessConfPtr->sessTaskName, sessionNo);

    xTaskCreate(functionPtr,
                (const char *)(sessConfPtr->sessTaskName),
                (const configSTACK_DEPTH_TYPE)sessConfPtr->sessionStackSize,
                (void *)sessionNo,
                taskPriority,
                NULL);
}

static void startSessionTask(void)
{
    UINT sessIndex;

    dpm_arp_enable(1, 1);
    DM_DBG_INFO(CYAN " [%s] Set GARP \n" CLEAR, __func__);

    for (sessIndex = 0; sessIndex < MAX_SESSION_CNT; sessIndex++) {
        if (dpmUserConfigs.sessionConfig[sessIndex].sessionType) {
            startSessTask(dpmUserConfigs.sessionConfig[sessIndex].sessionType, sessIndex + 1);
        }
    }
}

static void registTimer(void)
{
    int    period = 0;
    int    timerNo;
    int    timerBaseID = TIMER1_ID;
    int    status = 0;
    VOID    (*functionPtr)();

    for (timerNo = 0; timerNo < MAX_TIMER_CNT; timerNo++) {
        if (dpmUserConfigs.timerConfig[timerNo].timerType) {
            if (dpmUserConfigs.timerConfig[timerNo].timerType == TIMER_TYPE_PERIODIC) {
                period = 1;
            } else if (dpmUserConfigs.timerConfig[timerNo].timerType == TIMER_TYPE_ONETIME) {
                period = 0;
            } else {
                DM_DBG_ERR(RED "[%s] Register a timer: %s,%d \n" CLEAR, __func__,
                        DPM_EVENT_REG_NAME, dpmUserConfigs.timerConfig[timerNo].timerType);

                errorCallback(DM_ERROR_TIMER_TYPE, "Timer type error");

                return;
            }

            if (timerNo == 0) {
                functionPtr = dpmManagerTimer1Callback;
            } else if (timerNo == 1) {
                functionPtr = dpmManagerTimer2Callback;
            } else if (timerNo == 2) {
                functionPtr = dpmManagerTimer3Callback;
            } else if (timerNo == 3) {
                functionPtr = dpmManagerTimer4Callback;
            }

            status = rtc_register_timer(dpmUserConfigs.timerConfig[timerNo].timerInterval * 1000,
                                        DPM_EVENT_REG_NAME,
                                        timerBaseID + timerNo,
                                        period,
                                        functionPtr); //msec

            if (status < 0) {
                DM_DBG_ERR(RED "[%s] Failed to register timer(status:%d) \n" CLEAR,
                                                                __func__, status);
                status = ER_NOT_SUCCESSFUL;
            } else {
                DM_DBG_INFO("[%s] Register a timer - name:%s, id:%d(%d) type:%d, period:%d interval:%d \n",
                             __func__,
                             DPM_EVENT_REG_NAME,
                             timerBaseID + timerNo,
                             status,
                             dpmUserConfigs.timerConfig[timerNo].timerType,
                             period,
                             dpmUserConfigs.timerConfig[timerNo].timerInterval);
            }
        }
    }
}

#ifdef __ENABLE_UNUSED__    //for future work.
static UCHAR getDpmWakeupMode(void)
{
    unsigned char    flag = 0x00;
    UINT32        wakeupMode = dpm_mode_get_wakeup_source();
    int            wakeup_type = dpm_mode_get_wakeup_type();
    void        *paramData[2] = {NULL, };

    DM_DBG_INFO(YELLOW" Wake mode [0x%x], type [%d] \n" CLEAR, wakeupMode, wakeup_type);

    if (wakeupMode & WAKEUP_SOURCE_EXT_SIGNAL) {
        executeUserCallback(dpmUserConfigs.externWakeupCallback, 1, paramData, pdTRUE);
    }

    switch (wakeup_type) {
    case DPM_UNKNOWN_WAKEUP:
        DM_DBG_INFO(" Wake up .... Boot \n");
        flag |= DM_BOOT_WAKEUP;
        break;

    case DPM_RTCTIME_WAKEUP:
        DM_DBG_INFO(" Wake up .... RTC timer \n");
        flag |= DM_RTC_WAKEUP;
        break;

    case DPM_PACKET_WAKEUP:
        DM_DBG_INFO(" Wake up .... Recv Packet \n");
        flag |= DM_RECV_WAKEUP;
        break;

    case DPM_USER_WAKEUP:
        DM_DBG_INFO(" Wake up .... User Request \n");
        flag |= DM_SENSOR_WAKEUP;
        break;

    case DPM_NOACK_WAKEUP:
        DM_DBG_INFO(" Wake up .... No Ack \n");
        break;

    case DPM_DEAUTH_WAKEUP:
        DM_DBG_INFO(" Wake up .... De Auth \n");
        break;

    case DPM_TIM_ERR_WAKEUP:
        DM_DBG_INFO(" Wake up .... Tim err \n");
        break;

    default:
        DM_DBG_ERR(" Wake up .... Type err \n");
        break;
    }

    return flag;
}
#endif    //__ENABLE_UNUSED__

static void dpmWakeupManager(void)
{
#ifdef __ENABLE_UNUSED__    //for future work.
    UCHAR statusFlag = getDpmWakeupMode();
#endif  //__ENABLE_UNUSED__
    return;
}

static void dpmControlManager(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;
    void    *paramData[2] = {NULL, };
    int        ret = 0;

    DM_DBG_INFO(" [%s] Start DPM (Control)Manager \n", __func__);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

#if defined ( FOR_DEBUGGING_HOLD_DPM )
    if (dpm_mode_is_enabled()) {
        hold_dpm_sleepd();
    }
#endif    // FOR_DEBUGGING_HOLD_DPM

#if defined (FOR_DEBUGGING_HOLD_ABNORMAL_CHECK)
    if (dpm_mode_is_enabled()) {
        holdDpmAbnormalCheck();
    }
#endif    // FOR_DEBUGGING_HOLD_ABNORMAL_CHECK

    if (dpm_mode_is_enabled()) {
#ifdef __DPM_MNG_SAVE_RTM__
        dpm_daemon_regist_sleep_cb(dpm_mng_save_rtm);
#endif
        dpm_app_register(DPM_MANAGER_REG_NAME, 0);
        DM_DBG_INFO(" [%s] reg %s \n", __func__, DPM_MANAGER_REG_NAME);
    }

#ifdef DM_MUTEX
    dmMutex = xSemaphoreCreateMutex();
    if ( dmMutex == NULL ) {
        DM_DBG_ERR(" [%s] semaphore create error \n", __func__);
        da16x_sys_watchdog_unregister(sys_wdog_id);
        vTaskDelete(NULL);
        return;
    }
#endif

    da16x_sys_watchdog_notify(sys_wdog_id);

    if (!dpm_mode_is_wakeup()) {  // POR or Boot
boot:
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        ret = initDpmConfig();    // init dpmUserConfigs: get user DPM configuration

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        if (ret) {
            vTaskDelay(10);
            goto boot;
        } else {
            DM_DBG_INFO(" [%s] Initialized user DPM config \n", __func__);
        }

        if (dpm_mode_is_enabled()) {
            if (assignDpmRtmArea(DPM_CONFIG_NAME, (void **)&rtmDpmConfigPtr, sizeof(dpm_user_config_t)) == pdTRUE) {
#ifndef __DPM_MNG_SAVE_RTM__
                DM_DBG_INFO(" [%s] SAVE to RTM \n", __func__);
                saveDpmConfigToRtm();
#endif    // !__DPM_MNG_SAVE_RTM__
            } else {
                DM_DBG_ERR(RED "[%s] assignDpmRtmArea Error \n" CLEAR, __func__);
            }
        }
    } else {
        // get DPM configuration from RTM
        ret = readFromRtm(DPM_CONFIG_NAME, (void *)&dpmUserConfigs, (void **)&rtmDpmConfigPtr, sizeof(dpm_user_config_t));

        if (!ret) {
            //printDpmConfig();
        } else {
            DM_DBG_ERR(RED "[%s] read from RTM Error \n" CLEAR, __func__);
        }

        if (dpmUserConfigs.magicCode != DPM_MAGIC_CODE) {  // for RTM Check
            DM_DBG_ERR(RED "[%s] RTM is abnormal(MAGIC_CODE:0x%x)! \n" CLEAR,
                    __func__, dpmUserConfigs.magicCode);
            goto boot;
        }
    }

    if (rtmDpmConfigPtr) {
        //printDpmConfigRtm();
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    // get userParams RTM_DATA from RTM
    if (!dpm_mode_is_wakeup()) {
        if (dpmUserConfigs.bootInitCallback) {
            executeUserCallback(dpmUserConfigs.bootInitCallback, 1, paramData, pdTRUE);
        } else {
            errorCallback(DM_ERROR_NO_BOOT_INIT_CALLBACK, "Not registered bootInitCallback");
        }

        if (dpm_mode_is_enabled()) {
            if (assignDpmRtmArea(DPM_USER_PARAMS_NAME,
                                 (void **)&rtmUserParamsPtr,
                                 dpmUserConfigs.sizeOfRetentionMemory) == pdTRUE) {
#ifndef __DPM_MNG_SAVE_RTM__
                DM_DBG_INFO(" [%s] SAVE to RTM \n", __func__);
                saveUserParamsToRtm();
#endif    // !__DPM_MNG_SAVE_RTM__
            } else {
                DM_DBG_ERR(RED "[%s] assignDpmRtmArea Error \n" CLEAR, __func__);
            }
        }

        initDpmConfig();    // init dpmUserConfigs: get user DPM configuration

        for (int i = 0; i < MAX_SESSION_CNT; i++) {
            dpmUserConfigs.sessionConfig[i].sessionConnStatus = 0xFF;
        }

        registTimer();
        startSessionTask();
    } else {  // DPM Wake-up
        readFromRtm(DPM_USER_PARAMS_NAME,
                    (void *)dpmUserConfigs.ptrDataFromRetentionMemory,
                    (void **)&rtmUserParamsPtr,
                    dpmUserConfigs.sizeOfRetentionMemory);

        if (dpmUserConfigs.wakeupInitCallback) {
            executeUserCallback(dpmUserConfigs.wakeupInitCallback, 1, paramData, pdTRUE);
        } else {
            errorCallback(DM_ERROR_NO_WAKEUP_INIT_CALLBACK, "Not registered wakeupInitCallback");
        }

        initSessionConfigByWakeup();    // Clear malloc address case of DPM wake-up
        startSessionTask();

        dpmWakeupManager();
    }

    dpmControlManagerDoneFlag = 1;

#ifndef __DPM_MNG_SAVE_RTM__
    if (!dpm_mode_is_wakeup()) {  // POR or Boot
        vTaskDelay(50);

        if (dpm_mode_is_enabled()) {
            saveDpmConfigToRtm();    // save to RTM user dpm config
            saveUserParamsToRtm();    // save to RTM user params
        }
    }
#endif    // !__DPM_MNG_SAVE_RTM__

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

static session_config_t *getSessionInfoPtr(UINT sessionNo)
{
    if (sessionNo == SESSION1) {
        return (&dpmUserConfigs.sessionConfig[0]);
    } else if (sessionNo == SESSION2) {
        return (&dpmUserConfigs.sessionConfig[1]);
    } else if (sessionNo == SESSION3) {
        return (&dpmUserConfigs.sessionConfig[2]);
    } else if (sessionNo == SESSION4) {
        return (&dpmUserConfigs.sessionConfig[3]);
    } else {
        return 0;
    }
}

//
// user APIs
//

void dpm_mng_print_session_info(void)
{
    session_config_t    *sessConfPtr;

    PRINTF(" < DPM Session Info > \n");

    for (UINT sessionNo = 1; sessionNo <= MAX_SESSION_CNT; sessionNo++) {
        sessConfPtr = getSessionInfoPtr(sessionNo);

        if (sessConfPtr) {
            PRINTF("  * Session %d :", sessionNo);
#if !defined (__LIGHT_DPM_MANAGER__)
            if (   (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER)
                || (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER)) {
                PRINTF("  session_type:%s \n",
                        sessConfPtr->sessionType == REG_TYPE_TCP_SERVER ? "TCP_SERVER" : "UDP_SERVER");
                PRINTF("  session_my_port:%6d ", sessConfPtr->sessionMyPort);
                PRINTF("  peer_port:%6d ", sessConfPtr->peerPort);
                PRINTF("  peer_ip_addr:%s \n", sessConfPtr->peerIpAddr);
            }
            else
#endif    // !__LIGHT_DPM_MANAGER__

            if (   (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT)
                || (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT)) {
                    PRINTF("  session_type:%s \n",
                           sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT ? "TCP_CLIENT" : "UDP_CLIENT");
                    PRINTF("  session_serverIp:%s ", sessConfPtr->sessionServerIp);
                    PRINTF("  server_port_no:%6d ", sessConfPtr->sessionServerPort);
                    PRINTF("  local_port:%6d \n", sessConfPtr->localPort);
            }
        }
    }

    return;
}

void dpm_mng_print_session_config(UINT sessionNo)
{
    session_config_t    *sessConfPtr;
    char    type[12];
    PRINTF(" < DPM Session %d Config > \n", sessionNo);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        memset(type, 0, 12);

#if !defined (__LIGHT_DPM_MANAGER__)
        if (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER) {
            strcpy(type, "TCP SERVER");
        } else if (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER) {
            strcpy(type, "UDP SERVER");
        } else
#endif    // !__LIGHT_DPM_MANAGER__
        {
            if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
                strcpy(type, "TCP CLIENT");
            } else if (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT) {
                strcpy(type, "UDP CLIENT");
            }
        }

        PRINTF("  session_type                : %s \n", type);
        PRINTF("  session_my_port             : %d \n", sessConfPtr->sessionMyPort);
        PRINTF("  peer_port                   : %d \n", sessConfPtr->peerPort);
        PRINTF("  peer_ip_addr                : %s \n", sessConfPtr->peerIpAddr);
        PRINTF("  local_port                  : %d \n", sessConfPtr->localPort);
        PRINTF("  session_serverIp            : %s \n", sessConfPtr->sessionServerIp);
        PRINTF("  server_port_no              : %d \n", sessConfPtr->sessionServerPort);
        PRINTF("  Keep alive interval         : %d \n", sessConfPtr->sessionKaInterval);
        PRINTF("  Keep alive tid              : %d \n", sessConfPtr->sessionKaTid);
        PRINTF("  Window size                 : %d \n", sessConfPtr->sessionWindowSize);
        PRINTF("  Connect retry count         : %d \n", sessConfPtr->sessionConnRetryCnt);
        PRINTF("  Connect wait time           : %d \n", sessConfPtr->sessionConnWaitTime);
        PRINTF("  Auto reconnect              : %d \n", sessConfPtr->sessionAutoReconn);
        PRINTF("  session_connectCallback addr: 0x%x \n", sessConfPtr->sessionConnectCallback);
        PRINTF("  session_recvCallback addr   : 0x%x \n", sessConfPtr->sessionRecvCallback);
        PRINTF("  DPM register Name           : %s \n", sessConfPtr->dpmRegName);
        PRINTF("  session task name           : %s \n", sessConfPtr->sessTaskName);
        PRINTF("  session socket address      : 0x%x \n", sessConfPtr->socket);
    }
}

int dpm_mng_send_to_session(UINT sessionNo, ULONG ip, ULONG port, char *buf, UINT size)
{
    session_config_t    *sessConfPtr;
#if !defined (__LIGHT_DPM_MANAGER__)
    int    sub_idx;
#endif    // !__LIGHT_DPM_MANAGER__
    int        sendStatus = 0;
    UINT    wait_count = 0;

    if ((sessionNo <= 0) || (sessionNo > MAX_SESSION_CNT)) {
        DM_DBG_ERR(" [%s] Session No is out of range \n", __func__);
        return DM_ERROR_OUT_OF_RANGE;
    }

    sessConfPtr = getSessionInfoPtr(sessionNo);

    if (sessConfPtr->is_force_stopped) {
        DM_DBG_INFO(" [%s] This session is forcibly stopped(sessionNo:%d)\n", __func__, sessionNo);
        return DM_ERROR_FORCE_STOPPED;
    }

wait_to_start_session:
    if (sessConfPtr->runStatus == SESSION_STATUS_STOP) {
        if (wait_count < 50) {
            vTaskDelay(2);
            goto wait_to_start_session;
        }
        DM_DBG_ERR(RED "[%s] Not yet started (session:%d) \n" CLEAR, __func__, sessionNo);
        return DM_ERROR_SESSION_START_FAIL;
    }

#ifdef SESSION_MUTEX
    xSemaphoreTake(sessConfPtr->socketMutex, portMAX_DELAY);
#endif

#if !defined (__LIGHT_DPM_MANAGER__)
    if (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER) {
        DM_DBG_INFO(" [%s] session:%d ip:0x%x port:%d buf:0x%x size:%d \n",
                    __func__, sessionNo, ip, port, buf, size);

        sub_idx = getSessionTcpSubIndexByPort(ip, port);
        if (sub_idx >= 0) {
            sendStatus = sendTcpServer(sessConfPtr, sub_idx, buf, size);
            if (sendStatus) {
                DM_DBG_ERR(RED "[%s] Error sendTcpServer(sess:%d idx:%d status:0x%x) \n"
                                CLEAR, __func__, sessionNo, sub_idx, sendStatus);

#ifdef SESSION_MUTEX
                xSemaphoreGive(sessConfPtr->socketMutex);
#endif
                return sendStatus;
            }
        } else {
            DM_DBG_ERR(RED "[%s] Error : Not found TCP sub_session index (sess:%d) \n" CLEAR,
                                                                        __func__, sessionNo);
            errorCallback(DM_ERROR_NOT_FOUND_SUB_SESSION, "Not found sub session index");

#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
            return DM_ERROR_NOT_FOUND_SUB_SESSION;
        }
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER) {
        DM_DBG_INFO(" [%s] session:%d ip:0x%x port:%d buf:0x%x size:%d \n",
                    __func__, sessionNo, ip, port, buf, size);

        if (sessConfPtr->supportSecure) {
            sub_idx = getSessionUdpSubIndexByPort(ip, port);
            if (sub_idx >= 0) {
                sendStatus = sendUdpServer(sessConfPtr, sub_idx, buf, size, ip, port);
                if (sendStatus) {
                    DM_DBG_ERR(RED "[%s] Error sendUdpServer(sess:%d idx:%d status:0x%x) \n"
                                                CLEAR, __func__, sessionNo, sub_idx, sendStatus);

#ifdef SESSION_MUTEX
                    xSemaphoreGive(sessConfPtr->socketMutex);
#endif
                    return sendStatus;
                }
            } else {
                DM_DBG_ERR(RED "[%s] Error : Not found UDP sub_session index(sess:%d) \n" CLEAR,
                                                                            __func__, sessionNo);
                errorCallback(DM_ERROR_NOT_FOUND_SUB_SESSION, "Not found sub session index");

#ifdef SESSION_MUTEX
                xSemaphoreGive(sessConfPtr->socketMutex);
#endif
                return DM_ERROR_NOT_FOUND_SUB_SESSION;
            }
        } else {
            sendStatus = sendUdpServer(sessConfPtr, 0, buf, size, ip, port);
            if (sendStatus) {
                DM_DBG_ERR(RED "[%s] Error sendUdpServer(sess:%d status:0x%x) \n" CLEAR,
                                                                    __func__, sessionNo, sendStatus);

#ifdef SESSION_MUTEX
                xSemaphoreGive(sessConfPtr->socketMutex);
#endif
                return sendStatus;
            }
        }
    } else
#endif    // !__LIGHT_DPM_MANAGER__
    if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
        DM_DBG_INFO(" [%s] session:%d ip:0x%x port:%d buf:0x%x size:%d \n",
                    __func__, sessionNo, ip, port, buf, size);

        if (sessConfPtr->sessionConnStatus) {
            DM_DBG_ERR(RED "[%s] Not yet connected (session:%d) \n" CLEAR, __func__, sessionNo);

#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
            return DM_ERROR_SOCKET_NOT_CONNECT;
        }

        sendStatus = sendTcpClient(sessConfPtr, buf, size);
        if (sendStatus) {
            DM_DBG_ERR(RED "[%s] Error sendTcpClient(sess:%d status:0x%x) \n" CLEAR,
                    __func__, sessionNo, sendStatus);

#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
            return sendStatus;
        }
    } else if (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT) {
        DM_DBG_INFO(" [%s] session:%d ip:0x%x port:%d buf:0x%x size:%d \n",
                    __func__, sessionNo, ip, port, buf, size);

        sendStatus = sendUdpClient(sessConfPtr, buf, size);
        if (sendStatus) {
            DM_DBG_ERR(RED "[%s] Error sendUdpClient(sess:%d status:0x%x) \n" CLEAR,
                    __func__, sessionNo, sendStatus);

#ifdef SESSION_MUTEX
            xSemaphoreGive(sessConfPtr->socketMutex);
#endif
            return sendStatus;
        }
    } else {
        DM_DBG_ERR(RED "[%s] Error unknown type(sess:%d ip:0x%x port:%d) \n" CLEAR,
                __func__, sessionNo, ip, port);

#ifdef SESSION_MUTEX
        xSemaphoreGive(sessConfPtr->socketMutex);
#endif
        return ER_PARAMETER_ERROR;
    }

#ifdef SESSION_MUTEX
    xSemaphoreGive(sessConfPtr->socketMutex);
#endif
    return ERR_OK;
}

int dpm_mng_stop_session(UINT sessionNo)
{
    session_config_t    *sessConfPtr;
    const UINT    maxWaitCount = 100;
    UINT        waitCount = 0;
    UINT waitTick = 2;

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (!sessConfPtr) {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return ER_PARAMETER_ERROR;
    }

    if (sessConfPtr->is_force_stopped || sessConfPtr->runStatus == SESSION_STATUS_STOP) {
        DM_DBG_INFO("[%s:%d]It's already stopped\n", __func__, __LINE__);
        return ERR_OK;
    }

    while ((sessConfPtr->runStatus == SESSION_STATUS_READY_TO_CREATE) && (waitCount < maxWaitCount)) {
        vTaskDelay(waitTick);
        waitCount++;
    }

    if (sessConfPtr->runStatus == SESSION_STATUS_READY_TO_CREATE) {
        DM_DBG_ERR("[%s:%d] Session is not created yet(runStatus:%d, waitCount:%d, maxWaitCount:%d, waitTick:%d)\n",
                __func__, __LINE__, sessConfPtr->runStatus, waitCount, maxWaitCount, waitTick);
        return ER_NOT_CREATED;
    }

    DM_DBG_INFO(" [%s] Request to stop the task \"%s\" status:0x%x \n",
                        __func__, sessConfPtr->sessTaskName, sessConfPtr->sessionConnStatus);

    sessConfPtr->requestCmd = CMD_REQUEST_STOP;

    while ((sessConfPtr->runStatus != SESSION_STATUS_STOP) && (waitCount < maxWaitCount)) {
        vTaskDelay(2);
        waitCount++;
    }

    if (sessConfPtr->runStatus == SESSION_STATUS_STOP) {
#if !defined (__LIGHT_DPM_MANAGER__)
        if (sessConfPtr->sessionType != REG_TYPE_TCP_SERVER)
#endif // (__LIGHT_DPM_MANAGER__)
        {
            DM_DBG_INFO(" [%s:%d] Terminate session task(%s)\n",
                        __func__, __LINE__, sessConfPtr->sessTaskName);
        }
        sessConfPtr->is_force_stopped = pdTRUE;
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to stop session \n" CLEAR, __func__, __LINE__);
        sessConfPtr->requestCmd = CMD_NONE;
        return    ER_WAIT_ERROR;
    }

    return ERR_OK;
}

int dpm_mng_start_session(UINT sess_idx)
{
    session_config_t    *sessConfPtr;

    sessConfPtr = getSessionInfoPtr(sess_idx);
    if (!sessConfPtr) {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return ER_ARGUMENT;
    }

    if (sessConfPtr->runStatus > SESSION_STATUS_STOP) {
        DM_DBG_INFO("[%s:%d]Session is already started\n", __func__, __LINE__);
        return ERR_OK;
    }

    DM_DBG_INFO("[%s] Request to start the task (type:%d sessionNo:%d) \n" CLEAR,
            __func__, sessConfPtr->sessionType, sess_idx);

    sessConfPtr->is_force_stopped = pdFALSE;

    startSessTask(sessConfPtr->sessionType, sess_idx);

    if (!sessionRunConfirm(sessConfPtr, 500)) {  // 5 sec wait for running status
        DM_DBG_ERR(RED "[%s:%d] Failed to start session \n" CLEAR, __func__, __LINE__);
        return ER_NOT_ENABLED;
    }

    return ERR_OK;
}

int dpm_mng_set_session_info(UINT sessionNo, ULONG type, ULONG myPort, char *peerIp,
                             ULONG peerPort, ULONG kaInterval, void (*connCb)(), void (*recvCb)())
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO("[%s] session:%d myPort:%d peerPort:%d peerIp:%s kaInterval:%d \n",
                __func__, sessionNo, myPort, peerPort, peerIp, kaInterval);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
#if !defined (__LIGHT_DPM_MANAGER__)
        if (type == REG_TYPE_TCP_SERVER) {
            sessConfPtr->sessionType = type;
            sessConfPtr->sessionMyPort = myPort;

            sessConfPtr->sessionKaInterval = kaInterval;

            sessConfPtr->sessionConnectCallback = connCb;
            sessConfPtr->sessionRecvCallback = recvCb;
        } else if (type == REG_TYPE_UDP_SERVER) {
            sessConfPtr->sessionType = type;
            sessConfPtr->sessionMyPort = myPort;

            strcpy(sessConfPtr->peerIpAddr, peerIp);
            sessConfPtr->peerPort = peerPort;

            sessConfPtr->sessionRecvCallback = recvCb;
        } else
#endif    // !__LIGHT_DPM_MANAGER__
        if (type == REG_TYPE_TCP_CLIENT) {
            sessConfPtr->sessionType = type;
            sessConfPtr->localPort = myPort;
            strcpy(sessConfPtr->sessionServerIp, peerIp);
            sessConfPtr->sessionServerPort = peerPort;

            sessConfPtr->sessionKaInterval = kaInterval;

            sessConfPtr->sessionConnectCallback = connCb;
            sessConfPtr->sessionRecvCallback = recvCb;
        } else if (type == REG_TYPE_UDP_CLIENT) {
            sessConfPtr->sessionType = type;
            sessConfPtr->localPort = myPort;
            strcpy(sessConfPtr->sessionServerIp, peerIp);
            sessConfPtr->sessionServerPort = peerPort;

            sessConfPtr->sessionRecvCallback = recvCb;
        } else {
            DM_DBG_ERR(RED "[%s:%d] type(%d) error \n" CLEAR, __func__, __LINE__, type);
            return 1;
        }

        return 0;
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr (sessionNo:%d)\n"
                CLEAR, __func__, __LINE__, sessionNo);
        return 1;
    }
}

int dpm_mng_set_session_info_my_port_no(UINT sessionNo, ULONG port)        // for Server
{
#ifdef __LIGHT_DPM_MANAGER__
    DA16X_UNUSED_ARG(sessionNo);
    DA16X_UNUSED_ARG(port);
    return 0;
#else
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d port:%d \n", __func__, sessionNo, port);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (   (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER)
            || (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER)) {
            sessConfPtr->sessionMyPort = port;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only Server service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }

#endif    // __LIGHT_DPM_MANAGER__
}

int dpm_mng_set_session_info_peer_port_no(UINT sessionNo, ULONG port)    // for Server
{
#if defined (__LIGHT_DPM_MANAGER__)
    DA16X_UNUSED_ARG(sessionNo);
    DA16X_UNUSED_ARG(port);
    return 0;
#else
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d port:%d \n", __func__, sessionNo, port);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (   (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER)
            || (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER)) {
            sessConfPtr->peerPort = port;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only Server service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }

#endif    // !__LIGHT_DPM_MANAGER__
}

int dpm_mng_set_session_info_peer_ip_addr(UINT sessionNo, char *ip)        // for Server
{
#if defined (__LIGHT_DPM_MANAGER__)
    DA16X_UNUSED_ARG(sessionNo);
    DA16X_UNUSED_ARG(ip);
    return 0;
#else
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d ip:%s \n", __func__, sessionNo, ip);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (   (sessConfPtr->sessionType == REG_TYPE_UDP_SERVER)
            || (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER)) {
            strcpy(sessConfPtr->peerIpAddr, ip);
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only Server service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
#endif    // !__LIGHT_DPM_MANAGER__
}

int dpm_mng_set_session_info_server_ip_addr(UINT sessionNo, char *ip)    // for Client
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d ip:%s \n", __func__, sessionNo, ip);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (   (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT)
            || (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT)) {
            strcpy(sessConfPtr->sessionServerIp, ip);
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only Client service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_session_info_server_port_no(UINT sessionNo, ULONG port)    // for Client
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d port:%d \n", __func__, sessionNo, port);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (   (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT)
            || (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT)) {
            sessConfPtr->sessionServerPort = port;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only Client service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_session_info_local_port(UINT sessionNo, ULONG port)        // for Client
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d port:%d \n", __func__, sessionNo, port);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (   (sessConfPtr->sessionType == REG_TYPE_UDP_CLIENT)
            || (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT)) {
            sessConfPtr->localPort = port;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only Client service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_session_info_window_size(UINT sessionNo, UINT windowSize)        // for TCP
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d window_size:%d \n", __func__, sessionNo, windowSize);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (
#if !defined (__LIGHT_DPM_MANAGER__)
               (sessConfPtr->sessionType == REG_TYPE_TCP_SERVER)
            ||
#endif    // !__LIGHT_DPM_MANAGER__
               (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT)) {
            sessConfPtr->sessionWindowSize = windowSize;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only TCP service \n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_session_info_conn_retry_count(UINT sessionNo, UINT connRetryCount)        // for TCP Client
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d conn_retry_count:%d \n", __func__, sessionNo, connRetryCount);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
            sessConfPtr->sessionConnRetryCnt = connRetryCount;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only TCP client service\n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_session_info_conn_wait_time(UINT sessionNo, UINT connWaitTime)        // for TCP Client
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d conn_wait_time:%d \n", __func__, sessionNo, connWaitTime);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
            sessConfPtr->sessionConnWaitTime = connWaitTime;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only TCP client service\n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_session_info_auto_reconnect(UINT sessionNo, UINT autoReconnect)        // for TCP Client
{
    session_config_t    *sessConfPtr;

    DM_DBG_INFO(" [%s] session:%d auto_reconnect:%d \n", __func__, sessionNo, autoReconnect);

    sessConfPtr = getSessionInfoPtr(sessionNo);
    if (sessConfPtr) {
        if (sessConfPtr->sessionType == REG_TYPE_TCP_CLIENT) {
            sessConfPtr->sessionAutoReconn = autoReconnect;
            return 0;
        } else {
            DM_DBG_ERR(RED "[%s:%d] Only TCP client service\n" CLEAR, __func__, __LINE__);
            return 1;
        }
    } else {
        DM_DBG_ERR(RED "[%s:%d] Failed to get session info ptr \n" CLEAR, __func__, __LINE__);
        return 1;
    }
}

int dpm_mng_set_DPM_timer(UINT id, UINT type, UINT interval, void (*timerCallback)())
{
    int            period = 0;
    UINT        timerID;
    UINT        timerIndex;
    timer_config_t    *timerConfPtr;
    VOID    (*functionPtr)();

    if (!id || (id > MAX_TIMER_CNT)) {
        DM_DBG_ERR(RED "[%s] Timer ID Error(%d) \n" CLEAR, __func__, id);
        return 1;
    }

    timerIndex = id - 1;
    timerID = TIMER1_ID + id - 1;

    timerConfPtr = &dpmUserConfigs.timerConfig[timerIndex];

    timerConfPtr->timerInterval = interval;
    timerConfPtr->timerCallback = timerCallback;

    if (type == TIMER_TYPE_PERIODIC) {
        period = 1;
    } else if (type == TIMER_TYPE_ONETIME) {
        period = 0;
    } else {
        DM_DBG_ERR(RED "[%s] Register a timer: %d,%d \n" CLEAR, __func__, id, type);
        return 1;
    }

    if (id == 1) {
        functionPtr = dpmManagerTimer1Callback;
    } else if (id == 2) {
        functionPtr = dpmManagerTimer2Callback;
    } else if (id == 3) {
        functionPtr = dpmManagerTimer3Callback;
    } else if (id == 4) {
        functionPtr = dpmManagerTimer4Callback;
    }

    rtc_cancel_timer((int)timerID);

    rtc_register_timer(timerConfPtr->timerInterval * 1000, DPM_EVENT_REG_NAME, (int)timerID, period, functionPtr); //msec

    DM_DBG_INFO(" [%s] Register a timer: %s,%d \n", __func__, DPM_MANAGER_REG_NAME, timerConfPtr->timerInterval);

    return 0;
}

int dpm_mng_unset_DPM_timer(UINT id)
{
    UINT        timerID;
    UINT        timerIndex;
    timer_config_t    *timerConfPtr;

    if (!id || (id > MAX_TIMER_CNT)) {
        DM_DBG_ERR(RED "[%s] Timer ID Error(%d) \n" CLEAR, __func__, id);
        return 1;
    }

    timerIndex = id - 1;
    timerID = TIMER1_ID + id - 1;

    timerConfPtr = &dpmUserConfigs.timerConfig[timerIndex];

    timerConfPtr->timerInterval = 0;
    timerConfPtr->timerCallback = NULL;

    rtc_cancel_timer((int)timerID);

    DM_DBG_INFO(" [%s] Stop the registered timer:%s id:%d(%d) \n", __func__, DPM_MANAGER_REG_NAME, id, timerID);

    return 0;
}

int dpm_mng_save_to_RTM(void)
{
    DM_DBG_INFO(" [%s] Called dpm_mng_save_to_RTM \n", __func__);

    if (dpm_mode_is_enabled()) {
        saveUserParamsToRtm();    // save to RTM user params
    } else {
        DM_DBG_ERR(RED "[%s] Error: Not dpm mode \n" CLEAR, __func__);
        return 1;
    }

    return 0;
}

int dpm_mng_job_start(void)
{
    DM_DBG_INFO(" [%s] Called dpm_mng_job_start \n", __func__);

    __clr_dpm_sleep(DPM_MANAGER_REG_NAME, (char *)__func__, (int) __LINE__);
    return 0;
}

int dpm_mng_job_done(void)
{
    DM_DBG_INFO(" [%s] Called dpm_mng_job_done \n", __func__);

#if !defined ( __DPM_MNG_SAVE_RTM__ )
    if (dpm_mode_is_enabled()) {
        saveDpmConfigToRtm();    // save to RTM user dpm config
        saveUserParamsToRtm();    // save to RTM user params
    }
#endif // !__DPM_MNG_SAVE_RTM__

    __set_dpm_sleep(DPM_MANAGER_REG_NAME, (char *)__func__, (int) __LINE__);
    return 0;
}

int dpm_mng_regist_config_cb(void (*regConfigFunction)())
{
    initDpmConfigFunction = regConfigFunction;
    return 0;
}

int dpm_mng_init_done(void)
{
    return dpmControlManagerDoneFlag;
}

#ifdef __DPM_MNG_SAVE_RTM__
void dpm_mng_save_rtm(void)
{
#define CHG_PRIO    31
    UBaseType_t    save_prio;

    DM_DBG_INFO(YELLOW " [%s] Called && save to rtm : 0x%x(%d) 0x%x(%d) \n" CLEAR,
                        __func__, rtmDpmConfigPtr, sizeof(dpm_user_config_t), 
                        rtmUserParamsPtr, dpmUserConfigs.sizeOfRetentionMemory);

    portDISABLE_INTERRUPTS();
    save_prio = uxTaskPriorityGet(NULL); 
    vTaskPrioritySet(NULL, CHG_PRIO);

    if (dpm_mode_is_enabled()) {
        if (dpm_mng_init_done()) {
            if ((rtmDpmConfigPtr != NULL) && (rtmUserParamsPtr != NULL)) {
                memcpy(rtmDpmConfigPtr, &dpmUserConfigs, sizeof(dpm_user_config_t));
                memcpy(rtmUserParamsPtr, dpmUserConfigs.ptrDataFromRetentionMemory, dpmUserConfigs.sizeOfRetentionMemory);
            }
        }
    }

    vTaskPrioritySet(NULL, save_prio);
    portENABLE_INTERRUPTS();

    DM_DBG_INFO(YELLOW " [%s] Done save to rtm      : 0x%x(%d) 0x%x(%d) \n" CLEAR,
                        __func__, rtmDpmConfigPtr, sizeof(dpm_user_config_t), 
                        rtmUserParamsPtr, dpmUserConfigs.sizeOfRetentionMemory);
}
#endif

#endif    // __SUPPORT_DPM_MANAGER__

int dpm_mng_start(void)
{
#if defined ( __SUPPORT_DPM_MANAGER__ )
    int    status;

    DM_DBG_INFO(" [%s] Start DPM Manager \n", __func__);

    // Create dpm control manager Task
    status = xTaskCreate(dpmControlManager,
                        DPM_MANAGER_NAME,
                        DPM_CTRL_MNG_STK,
                        (void *)NULL,
                        DPM_CONTROL_MANAGER_PRI,
                        &dpmControlManagerHandle);
    if (status != pdPASS) {
        DM_DBG_ERR(" [%s] Task create fail(%s) !!!\n", __func__, DPM_MANAGER_NAME);

        errorCallback(DM_ERROR_TASK_CREATE_FAIL, "Task create fail");
        return status;
    }

    // Create dpm event manager Task
    status = xTaskCreate(dpmEventManager,
                        DPM_EVENT_NAME,
                        DPM_EVENT_MNG_STK,
                        (void *)NULL,
                        DPM_EVENT_MANAGER_PRI,
                        &dpmEventManagerHandle);

    if (status != pdPASS) {
        DM_DBG_ERR(" [%s] Task create fail(%s) !!!\n", __func__, DPM_EVENT_NAME);

        errorCallback(DM_ERROR_TASK_CREATE_FAIL, "Task create fail");
        return status;
    }
#endif    // __SUPPORT_DPM_MANAGER__

    return ERR_OK;
}

// EOF
