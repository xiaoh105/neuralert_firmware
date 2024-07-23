/**
 ****************************************************************************************
 *
 * @file da16x_initialize.c
 *
 * @brief Initialize system Wi-Fi features
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

#include <stdio.h>
#include <stdlib.h>

#include "da16x_system.h"
#include "da16x_oops.h"

#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "lwip/ip_addr.h"

#include "cal.h"
#include "crypto_primitives.h"

#include "common_def.h"
#include "iface_defs.h"

#include "schd_system.h"
#include "schd_idle.h"
#include "schd_trace.h"
#include "sys_cfg.h"
#include "nvedit.h"
#include "environ.h"

#include "dpmpower.h"
#include "dpmsche_patch.h"

#include "command.h"
#include "common_uart.h"

#include "user_dpm_api.h"
#include "da16x_image.h"
#include "project_config.h"

#include "ramsymbols.h"

#if defined ( __SUPPORT_DPM_ABN_RTC_TIMER_WU__ )
#include "da16x_dpm_regs.h"
#endif // __SUPPORT_DPM_ABN_RTC_TIMER_WU__

#if defined ( __CONSOLE_CONTROL__ )
#include "sys_feature.h"
#include "da16x_dpm_regs.h"
#include "user_dpm_api.h"
#endif /* __CONSOLE_CONTROL__ */

#include "util_api.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

/******************************************************************************
 * Macro
 ******************************************************************************/
#define SUPPORT_RETMEM_PTIM        // PTIM Scenario
#define SUPPORT_RETMEM_RALIB    // RALIB Scenario

#define ENVIRON_DEVICE              NVEDIT_SFLASH
#define SFCOPY_BULKSIZE             0x1000

#define LOAD_FAIL                   1
#define CHK_ABNORMAL                2

#define IMG_NORMAL0                 0
#define IMG_NORMAL1                 1
#define IMG_ABNORMAL0               2
#define IMG_ABNORMAL1               3

#define LOWEST_PRIORITY_KICKING_TASK
#define WDOG_RESCALE_TIME           10      // sec
#define KICKING_INTERVAL            500     // msec

#define SUPPORT_OTA

#ifdef SUPPORT_OTA
#define IMAGE0                      "DA16200_IMAGE_ZERO"
#define IMAGE1                      "DA16200_IMAGE_ONE"
#define SFLASH_BOOT_OFFSET          SFLASH_BOOT_INDEX_BASE
#define SFLASH_BOOT_OFFSET_LEN      0x20
#define IMAGE0_RTOS                 DA16X_FLASH_RTOS_MODEL0
#define IMAGE1_RTOS                 DA16X_FLASH_RTOS_MODEL1
#define WAKEUP_SOURCE               0xF80000
#define RAMLIB_JUMP_ADDR            0xF802D0
#define PTIM_JUMP_ADDR              0xF802D4
#define FULLBOOT_JUMP_ADDR          0xF802D8

#define IMAGE0_RAMLIB               (IMAGE0_RTOS + 0x180000)
#define IMAGE1_RAMLIB               (IMAGE1_RTOS + 0x180000)

#undef UE_OTA_DEBUG                 /* FOR_DEBUGGING */
#ifdef UE_OTA_DEBUG
#define DBG_PRINTF(...)             Printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)             //
#endif

static void start_da16x(void);

#ifdef BUILD_OPT_DA16200_MAC
extern int  chk_ext_int_exist(void);
#endif //BUILD_OPT_DA16200_MAC
#ifdef  ISR_SEPERATE
extern void RxIsrHandler( void *pvParameters);
extern void TxIsrHandler( void *pvParameters);
#endif
extern int  config_pin_mux(void);
extern void clear_retmem_gpio_pullup_info(void);
extern void da16x_secure_module_init(void);
extern void dpm_init_retmemory(int mode , unsigned int wakeup_src);
extern int  get_boot_mode(void);
extern int  da16x_network_main_set_sysmode(int mode);
extern int  ptim_manager_check(UINT32 storeaddr, UINT32 force);
extern void ramlib_clear(void);

#if defined ( __CONSOLE_CONTROL__ )
extern UINT    console_ctrl;
#endif /* __CONSOLE_CONTROL__ */

extern UCHAR    dpm_slp_time_reduce_flag;


/* global variable */
int bootoffset;
int image0_ng_flag;
int image1_ng_flag;
int check_img;
HANDLE gsflash;
UINT32 *bootaddr;
#endif /* SUPPORT_OTA */

unsigned char fast_connection_sleep_flag = pdFALSE;


TaskHandle_t setup_task_handler = NULL;
TaskHandle_t wdt_kick_task_handler = NULL;

#ifdef  ISR_SEPERATE
TaskHandle_t tx_isr_handler = NULL;
TaskHandle_t rx_isr_handler = NULL;
#endif


/******************************************************************************
 * Types
 ******************************************************************************/

typedef struct _thd_info_ {
    char            *name;
    VOID             (*entry_function)(ULONG);
    UINT            stksize;
    UINT            priority;
    UINT            preempt;
    ULONG            tslice;
    UINT            autorun;
} THD_INFO_TYPE;

typedef struct _run_thd_list_    {
    THD_INFO_TYPE            *info;
    struct _run_thd_list_    *prev;
} RUN_THD_LIST_TYPE;

/******************************************************************************
 * Functions
 ******************************************************************************/
extern void    init_system_step2(void);
extern void    init_system_step3(void);
extern void    init_system_step4(UINT32 rtosmode);
extern void    init_system_post(void);
extern int     user_main(char init_state);
extern int     getSysMode(void);
extern void    set_run_mode(int mode);
extern void    set_sys_run_feature(void);
extern void    da16200_trace(int index, int value);
extern void    DA16x_mem_init(void);
extern void    check_and_restore_nvram(void);
extern void    dpm_save_wakeup_source( int wakeupsource );
extern void    dpm_power_up_step0(unsigned long powerup_mode, unsigned long reserved);
extern void    dpm_power_up_step1_patch(unsigned long prep_bit, unsigned long calen);
extern void    dpm_power_up_step3_patch(unsigned long prep_bit, unsigned long calen, unsigned long cal_addr);
extern int     da16x_get_fast_connection_mode(void);

void system_launcher(void *pvParameters);

/******************************************************************************
 *  da16x_get_boot_status()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
UINT32 da16x_get_boot_status(UINT32 wakeup_src)
{
    /* Need to review boot scenarios depending on the application */
    /* Do not reference DA16200 boot scenarios. */
    switch (wakeup_src) {
    case WAKEUP_RESET:
    case WAKEUP_SOURCE_EXT_SIGNAL:    //Added Ext wakeup only
    case WAKEUP_SOURCE_WAKEUP_COUNTER:        //added for sleep mode 2 wakeup, without retention
    case WAKEUP_SOURCE_POR:
    case WAKEUP_SOURCE_POR_EXT_SIGNAL:
    case WAKEUP_WATCHDOG:
    case WAKEUP_WATCHDOG_EXT_SIGNAL:
    case WAKEUP_SENSOR_WATCHDOG:
    case WAKEUP_SENSOR_EXT_WATCHDOG:
    case WAKEUP_WATCHDOG_WITH_RETENTION:
    case WAKEUP_WATCHDOG_EXT_SIGNAL_WITH_RETENTION:
    case WAKEUP_SENSOR_WATCHDOG_WITH_RETENTION:
    case WAKEUP_SENSOR_EXT_WATCHDOG_WITH_RETENTION:
    case WAKEUP_RESET_WITH_RETENTION:
    case WAKEUP_SOURCE_UNKNOWN:
    case WAKEUP_SENSOR:
    case WAKEUP_SENSOR_EXT_SIGNAL:
    case WAKEUP_SENSOR_WAKEUP_COUNTER:
    case WAKEUP_SENSOR_EXT_WAKEUP_COUNTER:
        return pdTRUE;

    default:
        break;
    }

    return pdFALSE;
}

int chk_jtag_load(void)
{
    if ( (DA16X_FLASH_RALIB_OFFSET == DA16X_PHYSICAL_FLASH_RALIB0_OFFSET) ||
        (DA16X_FLASH_RALIB_OFFSET == DA16X_PHYSICAL_FLASH_RALIB1_OFFSET)) {
        return 0;
    }
    return 1;
}

#ifdef __TIM_LOAD_FROM_IMAGE__
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Start RAM LOAD PTIM //////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define HDR_MAGIC 0xFC915249    
typedef    struct    img_hdr {
    UINT    magic;
    UINT    size;
    UINT    addr;
    UINT    crc;
} IMG_HDR;

int rlib_manager_load_from_image(UINT32 loadaddr, UINT32 storeaddr )
{
    UINT32  size;
    IMG_HDR *hdr;

    hdr = (IMG_HDR *)loadaddr;

#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(" [%s] loadaddr:0x%x storeaddr:0x%x size:0x%x \n",
            __func__, loadaddr, storeaddr, hdr->size);
#endif

    if (ntohl(hdr->magic) == HDR_MAGIC) {
        size = hdr->size + sizeof(IMG_HDR);
        memcpy((void *)storeaddr, (void *)loadaddr, size);
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
        Printf(" [%s] Load image(storeaddr:0x%x <<< loadaddr:0x%x size:0x%x) \n",
                                            __func__, storeaddr, loadaddr, size);
#endif
    } else {
        Printf(RED_COLOR " [%s] Error magic (0x%x 0x%x) \n" CLEAR_COLOR,
                                            __func__, hdr->magic, HDR_MAGIC);
        return pdFALSE;
    }

    return size;
}

int ptim_manager_load_from_image(UINT32 loadaddr, UINT32 storeaddr )
{
    UINT32      size;
    IMG_HDR     *hdr;

    hdr = (IMG_HDR *)loadaddr;

#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(" [%s] loadaddr:0x%x storeaddr:0x%x size:0x%x \n", __func__, loadaddr, storeaddr, hdr->size);
#endif

    if (ntohl(hdr->magic) == HDR_MAGIC) {
        size = hdr->size + sizeof(IMG_HDR);
        memcpy((void *)storeaddr, (void *)loadaddr, size);
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
        Printf(" [%s] Load image(storeaddr:0x%x <<< loadaddr:0x%x size:0x%x) \n", __func__, storeaddr, loadaddr, size);
#endif
    } else {
        Printf(RED_COLOR " [%s] Error magic (0x%x 0x%x) \n" CLEAR_COLOR, __func__, hdr->magic, HDR_MAGIC);
        return pdFALSE;
    }

    return size;
}

static int load_rlib(void)
{
#undef    RLIB_LOAD_SRAM_FROM_RETM    // load from retention memory to ram
    UINT32    retm_addr;
    UINT32    rlib_image_addr;
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    UINT32    entrypoint;
#endif //__PRINT_TIM_LOAD_FROM_IMAGE__
    UINT32    ssize;

#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(CYAN_COLOR " [%s] Start load RLIB \n" CLEAR_COLOR, __func__);
#endif

    // At POR Boot time, it will be prevented to access RamLIB
    // in unknown value filled SRAM.
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode())){
        ramlib_clear();
    }

    retm_addr = DA16X_RETM_RALIB_OFFSET;
    rlib_image_addr = (unsigned int) &rlib_image[0];

    // load from cache image to retention memory
    ssize = rlib_manager_load_from_image(rlib_image_addr, retm_addr);
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(" >>> RLIB size image(%08x) \n", ssize);
#endif

#ifdef    RLIB_LOAD_SRAM_FROM_RETM    // load from retention memory to ram
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(" >>> RLIB load from retm(size:%08x, ep:0x%x) \n", ssize, entrypoint);
#endif
    manager_load_from_retm(retm_addr);

    if (ramlib_manager_check() == TRUE) {
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
        Printf(" >>> R.LIB is relocated to RETMEM (%08x) \n", retm_addr);
#endif
    } else {
        Printf(RED_COLOR " >>> Error R.LIB relocate (%08x) \n" CLEAR_COLOR, retm_addr);
    }
#endif  /* RLIB_LOAD_SRAM_FROM_RETM    */

#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(CYAN_COLOR " [%s] End load RLIB \n" CLEAR_COLOR, __func__);
#endif

    return TRUE;
}

static int load_ptim(void)
{
#undef    PTIM_LOAD_SRAM_FROM_RETM    // load from retention memory to ram
    UINT32    retm_addr;
    UINT32    ptim_image_addr;
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    UINT32    entrypoint;
#endif //__PRINT_TIM_LOAD_FROM_IMAGE__
    UINT32    ssize;

#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(CYAN_COLOR " [%s] Start load TIM \n" CLEAR_COLOR, __func__);
#endif

    /* PTIM image is loaded in the case of POR, Reset, WDOG */
    if (!da16x_get_boot_status(da16x_boot_get_wakeupmode())) {
        return TRUE;
    }

    retm_addr = DA16X_RETM_PTIM_OFFSET;
    ptim_image_addr = (unsigned int) &tim_image[0];
    
    // load from cache image to retention memory
    ssize = ptim_manager_load_from_image(ptim_image_addr, retm_addr);
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(" >>> PTIM size image(%08x) \n", ssize);
#endif

    // check ptim 
    if (ptim_manager_check(retm_addr, FALSE) != FALSE) {
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
        Printf(" >>> P.TIM is relocated to RETMEM (%08x) \n", retm_addr);
#endif
    } else {
        Printf(RED_COLOR " >>> Error P.TIM relocate (%08x) \n" CLEAR_COLOR, retm_addr);
    }

#ifdef    PTIM_LOAD_SRAM_FROM_RETM    // load from retention memory to ram
#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(" >>> PTIM load from retm(size:%08x, ep:0x%x) \n", ssize, entrypoint);
#endif
    manager_load_from_retm(retm_addr);
#endif /* PTIM_LOAD_SRAM_FROM_RETM */

#ifdef __PRINT_TIM_LOAD_FROM_IMAGE__
    Printf(CYAN_COLOR " [%s] End load TIM \n" CLEAR_COLOR, __func__);
#endif

    return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// End RAM LOAD PTIM ////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif /* __TIM_LOAD_FROM_IMAGE__ */

static void exception_setting()
{
    UINT32 oopsctrl;
    OOPS_CONTROL_TYPE *poopsctrl;

    oopsctrl = da16x_boot_get_offset(BOOT_IDX_OOPS);
    poopsctrl = (OOPS_CONTROL_TYPE *)&oopsctrl;

    poopsctrl->active = 1;

#ifdef PRINT_EXCEPTION_INFO
    poopsctrl->boot = 0;
    poopsctrl->stop = 1;
#else
    poopsctrl->boot = 1;
    poopsctrl->stop = 0;
#endif /* PRINT_EXCEPTION_INFO */

    #define DCU_EN0_OFFSET    (DA16X_ACRYPT_BASE|0x1E00UL)
    if ( (*((volatile UINT32 *)DCU_EN0_OFFSET)&0xC0000000L) != 0 ) {
        poopsctrl->dump   = 1;
    } else {
        poopsctrl->dump   = 0;
    }

    poopsctrl->save = 0;

    da16x_boot_set_lock(FALSE);
    da16x_boot_set_offset(BOOT_IDX_OOPS, oopsctrl);
    da16x_boot_set_lock(TRUE);
}

void da16x_boot_reset_wakeupmode(void)
{
    ((UINT32 *) RETMEM_MAGIC_BASE)[0] = 0;
}

static void sflash_pad_settings()
{
    if (SYSGET_SFLASH_INFO() == SFLASH_NONSTACK)  { // non stack
        DA16X_DIOCFG->Flash_PAD = 0x00; // 0x50001214
        DA16X_DIOCFG->EXT_FLASH_PAD = 0xBC000555; // 0x50001230. Fixed.
    } else { // stack
        // internal Flash PAD setting (0x50001214)
        switch (SYSGET_SFLASH_INFO()) {
            case SFLASH_STACK_0: // ISSI
                DA16X_DIOCFG->Flash_PAD = 0x4000; // no need pull-up
                break;
            case SFLASH_STACK_1: // ESMT
                DA16X_DIOCFG->Flash_PAD = 0x4c3f0000; // need pull-up
                break;
        }
        // In stack, external Flash PAD setting is dependent on PIN_MUX control. So lets make PIN MUX driver handle this?
        // pull enable and input enable control should be added.
    }
}

static char ramlib_ptim_init_status = pdTRUE;

int Create_Console_Task(void)
{
// Console_OUT's priority should be higher than system_launcher
// to prevent uart tx errors.
#ifdef CONSOLE_HIGH_PRIORITY    // print for loop_task
#define tskCONSOLE_OUT_PRIORITY       ((UBaseType_t) 24U)
#define tskCONSOLE_IN_PRIORITY        ((UBaseType_t) 23U)
#else
#define tskCONSOLE_OUT_PRIORITY       ((UBaseType_t) OS_TASK_PRIORITY_USER+3)
#define tskCONSOLE_IN_PRIORITY        ((UBaseType_t) OS_TASK_PRIORITY_USER+2)
#endif
    int stack_size;
    BaseType_t    xRtn;
    TaskHandle_t xHandle = NULL;

    extern void trace_scheduler(void* thread_input);
    extern void thread_console_in(void* thread_input);


    //Printf(">> Create Console Tasks...\r\n", __func__);

    /////////////////////////////////////
    // Console output thread
    /////////////////////////////////////
#ifdef    __ENABLE_PERI_CMD__
    stack_size = 256;        // highest stack : utils for secure boot, sys.sbrom
#else    //__ENABLE_PERI_CMD__
    stack_size = 128*2;    /* 128 * sizeof(int) */
#endif //__ENABLE_PERI_CMD__

    xRtn = xTaskCreate(trace_scheduler,
                            "Console_OUT",
                            stack_size,
                            (void *)NULL,
                            tskCONSOLE_OUT_PRIORITY,
                            &xHandle);
    if (xRtn != pdPASS) {
        Printf(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "Console_OUT");
    }

    /////////////////////////////////////
    // Console input thread
    /////////////////////////////////////

#if 1 //def    __ENABLE_PERI_CMD__
    stack_size = 256 * 6;        // highest stack : mbedtls testsuite
#else    //__ENABLE_PERI_CMD__
    stack_size = 128 * 5;        /* 640 * sizeof(int) */
#endif //__ENABLE_PERI_CMD__

#ifdef __SUPPORT_HTTP_CLIENT_FOR_CLI__
    stack_size = 256 * 10;        // highest stack : http-client
#endif // __SUPPORT_HTTP_CLIENT_FOR_CLI__

    xRtn = xTaskCreate(thread_console_in,
                            "Console_IN",
                            stack_size,
                            (void *)NULL,
                            tskCONSOLE_IN_PRIORITY,
                            &xHandle);

    if (xRtn != pdPASS) {
        Printf(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "Console_IN");
    }

    return TRUE ;
}

#ifdef WATCH_DOG_ENABLE
extern void watch_dog_kicking(UINT rescale_time);
extern int rescaleTime;
void wdt_kicking_task( void *pvParameters)
{
    DA16X_UNUSED_ARG(pvParameters);

    vTaskDelay(1);

    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        WDOG_HANDLER_TYPE *wdog;

        wdog = WDOG_CREATE(WDOG_UNIT_0);
        WDOG_INIT(wdog);
        WDOG_IOCTL(wdog, WDOG_SET_OFF, NULL);
        WDOG_CLOSE(wdog);
        PRINTF(RED_COLOR " [%s] WDOG off on Debugger mode. \r\n" CLEAR_COLOR, __func__);

        vTaskDelete(NULL);

        return;
    }

    while(1) {
        if (rescaleTime > 0) {
            watch_dog_kicking(rescaleTime);         // rescale_time
        } else {
            watch_dog_kicking(WDOG_RESCALE_TIME);   // default rescale_time
        }
        vTaskDelay(KICKING_INTERVAL/10);            // kicking interval
    }
}
#endif /* WATCH_DOG_ENABLE */


/* Set the External Wakeup in rtc_control(0x50091010) */
#define MEM_LONG_READ(addr, data)    *(data) = *((volatile UINT32 *)(addr))
#define MEM_LONG_OR_WRITE32(addr, data)        *((volatile UINT *)addr) = *((volatile UINT *)addr) | data

static void da16x_set_ext_wakeup_rtc (void)
{
    UINT32 val;

    MEM_LONG_READ(0x50091010, &val);

    if (!(val & WAKEUP_PIN_ENABLE(1))) {
        MEM_LONG_OR_WRITE32(0x50091010, WAKEUP_PIN_ENABLE(1));
    }

    return;
}

extern unsigned char    ble_combo_ref_flag;

#ifdef BUILD_OPT_DA16200_MAC
static void cfg_dpm_mode_by_boot_scenario(int wakeup_type)
{
    extern int get_dpm_mode(void);
    extern void disable_dpm_wakeup(void);
    extern void disable_dpm_mode(void);
    extern void enable_dpm_wakeup(void);
    extern int get_dpm_mode_from_nvram(void);

    // sleep mode2 wakeup
    if ( ((wakeup_type & 0x80) == 0) && ((wakeup_type & 0x1f) != 0)) {
        if (get_dpm_mode()) {
            disable_dpm_wakeup();
            DBG_PRINTF(" > BootType %x : Disable DPM wakeup...\r\n" , wakeup_type);
        }
    // reset
    } else if (wakeup_type == WAKEUP_RESET_WITH_RETENTION || wakeup_type == WAKEUP_RESET) {    // CMD Reboot
        if (get_dpm_mode_from_nvram()) {
            disable_dpm_wakeup();
            DBG_PRINTF(" > BootType %x : Disable DPM wakeup...\r\n" , wakeup_type);
        }
    // sleep mode3 wakeup
    } else {
        /* DPM mode (run or stop) check!! */
        /* Wakeup source 0x0a by TIM & RTC, Wakeup source 0x09 by External Interrupt */
        if (get_dpm_mode() == 0) {
            disable_dpm_mode();
            disable_dpm_wakeup();
            DBG_PRINTF(" > BootType %x : Disable DPM wakeup...\r\n" , wakeup_type);
        } else {
            /* Have to set DPM Wakeup state flag
             *    when wakeup as mode 0xa or DPM wakeup with external interrupt.
             */
            if (wakeup_type & 0x80) {
                DBG_PRINTF(" > BootType %x : Enable DPM wakeup...\r\n" , wakeup_type);
                enable_dpm_wakeup();
            }
        }
    }
}
#endif /* BUILD_OPT_DA16200_MAC */

static void boot_scenario(void)
{
    UINT32 wakeup_src;
    UINT32 force = FALSE;

    /* Wakeup source should be saved */
    /* If not dpm, Maybe always 4(POR) */
    wakeup_src = da16x_boot_get_wakeupmode();
    dpm_save_wakeup_source(wakeup_src);

    PRINTF("\nWakeup source is 0x%x \r\n", wakeup_src);

    if (da16x_get_boot_status(wakeup_src)) {
        force = TRUE;
    }

    if (ble_combo_ref_flag == pdTRUE) {
        /*
            In DA16600 Module (as of July 27, 2020), RTC_WAKE_UP1 is connected to UART-Tx
            of DA14531 (UART-Rx of DA16200). As BLE chip does not use RTC_WAKE_UP1 to
            wake up DA16200, disable it in SW.
        */
        MEM_LONG_OR_WRITE32(0x50091010, WAKEUP_PIN_ENABLE(0));
    } else {
        /** 9050 SLR Specific
         * If Wakeup source is related POR/Reset, check and set the Ext Wakeup bit in RTC_Control reg
         */
        if (force) {
            da16x_set_ext_wakeup_rtc();
        }
    }

#if defined ( __SUPPORT_DPM_ABN_RTC_TIMER_WU__ )
    if (dpm_mode_get_wakeup_source() == WAKEUP_COUNTER_WITH_RETENTION) {
        unsigned char dpm_wakeup_flag = 0;

        /* Backup dpm wakeup flag */
        dpm_wakeup_flag = RTM_FLAG_PTR->dpm_wakeup;

        /* Initialize RTM area for DPM operation following by "wakeup_src" */
        dpm_init_retmemory(force, wakeup_src);

        /* Restore dpm wakeup flag */
        RTM_FLAG_PTR->dpm_wakeup = dpm_wakeup_flag;

        PRINTF("\nDPM Wakeup source is 0x%x\n", wakeup_src);
        cfg_dpm_mode_by_boot_scenario(wakeup_src);
    } else
#endif  // __SUPPORT_DPM_ABN_RTC_TIMER_WU__
    {
        dpm_init_retmemory(force, wakeup_src);

#ifdef    BUILD_OPT_DA16200_MAC
        cfg_dpm_mode_by_boot_scenario(wakeup_src);
#endif //BUILD_OPT_DA16200_MAC
    }
}

#define DPM_HANG_DBG_TEST
/* Debugging Trace by Specific Retention mem */
char     *dpm_hang_test_chk_addr = (char *)0xf802e0;
/* During DPM RTOS Wakeup, Write step value to Specific Retention mem */
static void TEST_DPM_HANG_TRACE(char * addr, int data)
{
#ifdef DPM_HANG_DBG_TEST
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode()) == pdFALSE) 
        *((volatile UINT *)(addr)) = data;
#endif 
}

/* It checks and verifies that between the defifned system clock and running
 * clock are matched. 
 */
static void check_system_clock(void)
{
    UINT32 cur_clock = 0;
    UINT32 status, nv_clock = 0;
    UINT32 ioctldata[4], reboot = TRUE;

    _sys_clock_read(&cur_clock, sizeof(UINT32));

    ioctldata[0] = (UINT32)("boot.clk.bus");
    ioctldata[1] = (UINT32)NULL;

    da16x_environ_lock(TRUE);

    if (SYS_NVEDIT_IOCTL(NVEDIT_CMD_FIND, ioctldata) == TRUE) {
        if ((void *)(ioctldata[1]) != NULL) {
            status = SYS_NVEDIT_READ(ioctldata[1], &nv_clock, sizeof(UINT32));

            if (status && (nv_clock != cur_clock)) {    // nvram saved clock vs. current system clock
                // Check supported clocks.
                for (char clk = SYSCLK_DIV_160MHZ; clk < SYSCLK_MAX; clk++) {
                    if (nv_clock == (DA16X_PLL_CLOCK / clk)) {
                        PRINTF("[%s] cur_clock=%u --> nv_clock=%u\n", __func__,
                                cur_clock, nv_clock);

                        goto finish;
                    } else if (clk >= SYSCLK_MAX-1) {
                        PRINTF("Update NVRAM clock %d -> %d\n", nv_clock, cur_clock);

                        nv_clock = cur_clock;
                        ioctldata[2] = sizeof(UINT32);
                        ioctldata[3] = (UINT32)&nv_clock;

                        if (SYS_NVEDIT_WRITE(ioctldata[1], (VOID *)ioctldata[3], ioctldata[2]) != 0) {
                            ioctldata[0] = NVEDIT_SFLASH;
                            ioctldata[1] = SFLASH_UNIT_0;
                            ioctldata[2] = (UINT32)NULL;
                            SYS_NVEDIT_IOCTL(NVEDIT_CMD_SAVE, ioctldata);
                        } else {
                            PRINTF("Failed to write system clock\n");
                            reboot = FALSE;
                        }
                        break;
                    }
                }
                vTaskDelay(10);
            } else if (!status) {
                PRINTF("[%s] %d \"boot.clk.bus\" is not found\n", __func__, __LINE__);
                goto clock_notfound;
            } else {
                da16x_environ_lock(FALSE);
                return;
            }
        }
    } else {
        /* "boot.clk.bus" */
        PRINTF("Failed to find system clock in nvram (boot.clk.bus)\n");

clock_notfound:

        nv_clock = DA16X_SYSTEM_CLOCK;
        
        ioctldata[0] = (UINT32) "boot.clk";
        ioctldata[1] = (UINT32) "bus";
        ioctldata[2] = sizeof(UINT32);
        ioctldata[3] = (UINT32) &nv_clock;

        /* nvram add "boot.clk.bus" */
        if (SYS_NVEDIT_IOCTL(NVEDIT_CMD_ADD, ioctldata) == TRUE) {
            PRINTF("nvram add boot.clk.bus=%d completed\n", nv_clock);
            ioctldata[0] = NVEDIT_SFLASH;
            ioctldata[1] = SFLASH_UNIT_0;

            if (SYS_NVEDIT_IOCTL(NVEDIT_CMD_SAVE, ioctldata) == TRUE) {
                PRINTF("nvram save completed\n");
            } else {
                PRINTF("nvram save not completed\n");
                reboot = FALSE;
            }
        } else {
            PRINTF("Error: nvram add boot.clk.bus=%d\n", nv_clock);
            reboot = FALSE;
        }
    }

finish:
    da16x_environ_lock(FALSE);

    /* System will be reboot if system clock doesn't match 
     *  to the defined system clock.
     */
    if (reboot) {
        PRINTF("Rebooting for system clock...\n");
        reboot_func(SYS_REBOOT_POR);
        /* Wait for system-reboot */
        while (1) {
            vTaskDelay(10);
        }
    } else {
        PRINTF(RED_COLOR "Please, check system clock\n" CLEAR_COLOR);
    }
}

/******************************************************************************
 *  system_launcher()
 *
 *  Purpose : Daemon
 *  Input    :
 *  Output    :
 *  Return    :
 ******************************************************************************/
void system_launcher( void *pvParameters)
{
#ifdef USING_RAMLIB
    RAMLIB_SYMBOL_TYPE *ramsymbols = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;
#endif
    BaseType_t   xRtn;
    int          sys_run_mode;
    int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
    save_with_run_time("Start sys_launcher");
#endif

    // fcCSP Patch due to ROMBOOT
    if (!da16x_get_boot_status(da16x_boot_get_wakeupmode())) {
        unsigned long dcdc_st_ctrl = dpm_get_st_dcdc_st_ctrl(GET_DPMP());
        unsigned long uldo_cont = 0;
        RTC_IOCTL(RTC_GET_LDO_LEVEL_REG, &uldo_cont);
        uldo_cont &= 0xe0ffffff;
        uldo_cont |= ((dcdc_st_ctrl & 0x1f) << 24);
        RTC_IOCTL(RTC_SET_LDO_LEVEL_REG, &uldo_cont);
    }

    da16200_trace(0, 0x22220000);
    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 1);
    init_system_step2();
    
    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 2);

    da16200_trace(0, 0x22220001);

    init_system_step3();

    /* Exception setting enable */
    exception_setting();

    da16x_sys_watchdog_init();

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
    if (sys_wdog_id >= 0) {
        da16x_sys_wdog_id_set_system_launcher(sys_wdog_id);
    }

    init_environ();

    {
        static const CRYPTO_PRIMITIVE_TYPE sym_crypto_primitive = {
            trc_que_proc_dump,
            trc_que_proc_text,
            trc_que_proc_vprint,
            trc_que_proc_getchar,
            pvPortMalloc,
            vPortFree
        };

        init_crypto_primitives(&sym_crypto_primitive);
        mbedtls_platform_init(NULL);
        da16x_secure_module_init();    //Init cc312 with 0xffffffff
#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
        save_with_run_time("Init crypto");
#endif
    }

    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 3);
    dpm_power_up_step0(0, 0);

    if (da16x_boot_get_wakeupmode() == WAKEUP_RESET_WITH_RETENTION) {
        da16x_boot_reset_wakeupmode();
    }

    da16200_trace(0, 0x22220002);

#if defined(BUILD_OPT_DA16200_ASIC) && !defined(BUILD_OPT_DA16200_LIBALL)
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode())) {
        dpm_power_up_step1_patch(DPMSCHE_PREP_TX, 1);
    } else {
        dpm_power_up_step1_patch(DPMSCHE_PREP_TX, 0);
    }
#endif

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
    save_with_run_time("Init WDT Console");
#endif

    //init_system_step3();
    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 5);
#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT) && defined(PRINT_SAVE)
    printf_save_with_run_time();        /* This function takes about 100ms */
#endif

#if defined(__RUNTIME_CALCULATION__)
    printf_with_run_time("after init_system_step3");
#endif

    da16200_trace(0, 0x22220003);

#if defined(BUILD_OPT_DA16200_ASIC) && !defined(BUILD_OPT_DA16200_LIBALL)
    /* Ready CAL */
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode())) {
        dpm_power_up_step2(DPMSCHE_PREP_TX, 1);
    } else { /* NO CAL */
        dpm_power_up_step2(DPMSCHE_PREP_TX, 0);
    }
#endif
    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 6);
    da16200_trace(0, 0x22220004);

    /*
     * FIXME: UART clock enable.
     */
#ifdef __ENABLE_UART1_CLOCK__
    DA16X_CLOCK_SCGATE->Off_CAPB_UART1 = 0;
#endif /* __ENABLE_UART1_CLOCK__ */

    /*
     * FIXME: GPIO clock enable.
     */
#ifdef __ENABLE_GPIO_CLOCK__
    DA16X_CLOCK_SCGATE->Off_DAPB_GPIO0 = 0;
#endif /* __ENABLE_GPIO_CLOCK__ */

    da16200_trace(0, 0x22220005);

    init_system_step4(TRUE);

#if defined(__RUNTIME_CALCULATION__)
    printf_with_run_time("after init_system_step4");
#endif

    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 7);

    set_sys_run_feature();

    // Create Console Tasks
    Create_Console_Task();
    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 8);

    //==============================================
    // 1. LIB Management
    //==============================================

#ifdef __TIM_LOAD_FROM_IMAGE__
    
    /* PTIM/RAMLIB image is loaded in the case of POR, Reset, WDOG */
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode())) {

#if defined(__RUNTIME_CALCULATION__)
        printf_with_run_time("load_rlib");
#endif
        load_rlib();

#if defined(__RUNTIME_CALCULATION__)
        printf_with_run_time("load_ptim");
#endif
        load_ptim();

#if defined(__RUNTIME_CALCULATION__)
        printf_with_run_time("end --- rlib ptim");
#endif
    }

#endif  /* __TIM_LOAD_FROM_IMAGE__ */

    sflash_pad_settings();
    //TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 9);

    // 20190717 JJ
    // SUSPEND disable code
    {
        UINT32 force = TRUE;
        HANDLE sflash = SFLASH_CREATE(SFLASH_UNIT_0);

        SFLASH_IOCTL(sflash, SFLASH_SET_UNSUSPEND, &force);
    }

    //==============================================
    // 2. WatchDog & CPU Sleep
    //==============================================

    init_idle_management();

    system_schedule_preset(); // 1st WatchDog kicking if WATCH_DOG_ENABLE

    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 0xa);

    //==============================================
    // 3. Pool Management
    //==============================================

    sys_run_mode = getSysMode();
    set_run_mode(sys_run_mode);

    //   Finally, fast_connection_sleep_flag in user_system_features.c
    //   will be enable running mode.
    fast_connection_sleep_flag = da16x_get_fast_connection_mode();

    if (fast_connection_sleep_flag == pdTRUE) {
        dpm_slp_time_reduce_flag = pdTRUE;
    }

    //==============================================
    // 4. Post Initialization
    //==============================================

    init_system_post();

    //==============================================
    // 5. Thread Management
    //==============================================

    ASIC_DBG_TRIGGER(MODE_INI_STEP(0xF1110A));

#if defined(BUILD_OPT_DA16200_ASIC) && !defined(BUILD_OPT_DA16200_LIBALL)
    /* Setup CAL */
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode())) {
        dpm_power_up_step3_patch(DPMSCHE_PREP_TX, 1, (unsigned long) dpm_get_adc(GET_DPMP()));
    } else { /* Read CAL */
        dpm_power_up_step3_patch(DPMSCHE_PREP_TX, 0, (unsigned long) dpm_get_adc(GET_DPMP()));
    }
#endif

    {
        // LDO status check: XTAL40M_RDY, DIG_LDO_RDY, F_LDO_RDY & DCDC_RDY
        unsigned long ldo_status = 0;
        RTC_IOCTL(RTC_GET_LDO_STATUS, &ldo_status);

        if ((ldo_status & 0x1d00) != 0x1d00) {
            PRINTF("LDO Status failed: 0x08x\n", ldo_status);
        }
    }

    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 0xb);


    // Check NVRAM status and recover if crashed when POR boot.
    if (da16x_get_boot_status(da16x_boot_get_wakeupmode()) == pdTRUE) {
        check_and_restore_nvram();
    }

    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 0xc);
#if FOR_DEBUG
    extern void print_NVIC_priority_all();
    extern void set_NVIC_priority_all();

    PRINTF(CYAN_COLOR " [%s] configPRIO_BITS:0x%x \r\n" CLEAR_COLOR, __func__, configPRIO_BITS);
    PRINTF(CYAN_COLOR " [%s] configMAX_SYSCALL_INTERRUPT_PRIORITY:0x%x \r\n" CLEAR_COLOR, __func__, configMAX_SYSCALL_INTERRUPT_PRIORITY);

    print_NVIC_priority_all();
#endif

    //==============================================
    // System Monitor
    //==============================================
    ASIC_DBG_TRIGGER(MODE_INI_STEP(0xF1110D));

#if !defined(BUILD_OPT_DA16200_ROMALL)
    /* NOT VSIM */
    DA16X_TIMER_SCHEDULER();
#endif

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
    printf_with_run_time("Init system post");
#endif

#ifdef WATCH_DOG_ENABLE
    xRtn = xTaskCreate(wdt_kicking_task,
                            "wdt_kicking_task",
                            128,        /* 128 * sizeof(int) */
                            (void *)NULL,
#ifdef LOWEST_PRIORITY_KICKING_TASK
                            OS_TASK_PRIORITY_USER,
#else
                            OS_TASK_PRIORITY_HIGHEST,
#endif
                            &wdt_kick_task_handler);
    if (xRtn != pdPASS) {
        PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "wdt_kicking_task");
    }
#else
    DA16X_UNUSED_ARG(xRtn);
#endif // WATCH_DOG_ENABLE

#ifdef    ISR_SEPERATE
    xRtn = xTaskCreate(TxIsrHandler,
                            "TX_ISR_H",
                            128,        /* 128 * sizeof(int) */
                            (void *)NULL,
                            OS_TASK_PRIORITY_SYSTEM+9, //20 //25,
                            &tx_isr_handler);
    if (xRtn != pdPASS) {
        PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "TX_ISR_H");
    }

    xRtn = xTaskCreate(RxIsrHandler,
                            "RX_ISR_H",
                            128 * 2,        /* 128 * sizeof(int) */
                            (void *)NULL,
                            OS_TASK_PRIORITY_SYSTEM+14,
                            &rx_isr_handler);
    if (xRtn != pdPASS) {
        PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "RX_ISR_H");
    }
#endif
    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 0x10);
    DA16x_mem_init();

    TEST_DPM_HANG_TRACE(dpm_hang_test_chk_addr , 0x20);

    /* Wakeup source should be saved */
    //==============================================
    // Boot Scenario
    //==============================================
    boot_scenario();

#if defined ( __CONSOLE_CONTROL__ )
    if (dpm_mode_is_wakeup() == TRUE) {
        console_ctrl = RTM_FLAG_PTR->console_ctrl_rtm;
    } else { // dpm(x), dpm (o) && wakeup (x)
        read_nvram_int((const char *)NVR_KEY_DLC_CONSOLE_CTRL, (int *)&console_ctrl);
    }

    console_control(console_ctrl);
#endif /* __CONSOLE_CONTROL__    */

#if !defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #if defined ( __IGNORE_SYS_RUN_MODE__ )
    #error "System run-mode error: In case of Non-concurrent mode, DA16200 cannot support SYSMODE_STA_N_AP."
  #else
    /*
     * Validity check whether sys_run_mode is supported or not on Non-concurrent RTOS.
     */
    if (sys_run_mode > SYSMODE_AP_ONLY) {
        unsigned int status;

        Printf(RED_COLOR "\n\n>>> Unsupported System Mode (%d) !!!\n" CLEAR_COLOR, sys_run_mode);
        Printf(RED_COLOR "\n>>> Reset saved profile !!!\n\n" CLEAR_COLOR, sys_run_mode);

        status = da16x_clearenv(ENVIRON_DEVICE, "app"); /* Factory reset */
        if (status == FALSE) {
            status = (UINT)(recovery_NVRAM());
        }

        /* Set as STA only mode */
        da16x_network_main_set_sysmode(SYSMODE_STA_ONLY);
        set_run_mode(SYSMODE_STA_ONLY);
    }
  #endif // !__IGNORE_SYS_RUN_MODE__
#endif // !__SUPPORT_WIFI_CONCURRENT__

#ifdef BUILD_OPT_DA16200_MAC
    /* Case of External Interrupt,
     * let's do clear external int flag of wakeup source
     */
    if (chk_ext_int_exist()) {
        UINT32 ioctldata;
        // wakeup pin2
        RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &ioctldata);
        if (!(ioctldata & RTC_WAKEUP2_EN(1)))
            RTC_CLEAR_EXT_SIGNAL();
    }
#endif /* BUILD_OPT_DA16200_MAC */

    if (   (dpm_mode_get_wakeup_source() == WAKEUP_SOURCE_POR || dpm_mode_get_wakeup_source() == WAKEUP_RESET)
        && fast_connection_sleep_flag == pdFALSE
    ) {
        check_system_clock();
    }

    /*
     * Logo Display :
     *    NORMAL BOOT : 0,  FULL BOOT : 1
     */
    if (get_boot_mode() == 0) {
        version_display(0, NULL);
    }

    /* Let's reduce the booting time in Fast connection mode */     
    if (fast_connection_sleep_flag == pdTRUE) {
        PRINTF(YELLOW_COLOR "\n>>> Wi-Fi Fast_Connection mode ...\n" CLEAR_COLOR);
    }

    // Start a timer to select RTC's clock as crystal
    start_rtc_switch_xtal();

    // Initialize and run system application
    // and run user application if needed.
    start_da16x();

    da16x_sys_watchdog_suspend(sys_wdog_id);

    system_schedule(pvParameters);

    da16200_trace(0, 0x2222000F);

    da16x_sys_watchdog_unregister(sys_wdog_id);
	
	da16x_sys_wdog_id_set_system_launcher(DA16X_SYS_WDOG_ID_DEF_ID);

    vTaskDelete(NULL);
}

/******************************************************************************
 *    start_da16x()
 *
 *  Purpose : Start the main flow of the DA16200 FreeRTOS
 *  Input    :
 *  Output    :
 *  Return    :
 ******************************************************************************/

static void    start_da16x(void)
{
    ASIC_DBG_TRIGGER(MODE_INI_STEP(0xF12341));

    /* clear RETMEM_GPIO_PULLUP_INFO */
    clear_retmem_gpio_pullup_info();

    /* Configure Pin-Mux of DA16200 */
    config_pin_mux();

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
    printf_with_run_time("Start user_main");
#endif

#ifdef __BOOT_CONN_TIME_PRINT__
    log_boot_conn_with_run_time("Start main");
#endif

    /* Start DA16200 IoT system layer */
    user_main(ramlib_ptim_init_status);
}

/* EOF */
