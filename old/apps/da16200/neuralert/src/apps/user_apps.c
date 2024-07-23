/**
 ****************************************************************************************
 *
 * @file user_apps.c
 *
 * @brief Config table to start user applications
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
#include "application.h"
#include "common_def.h"

//#include "sample_defs.h" //JW Added this -- probably could combine
// by adding the following two lines, I think we can now remove sample_defs.h from our app
#define USER_READ_DATA						"USER_READ"
#define USER_TRANSMIT_DATA					"USER_TX"

/******************************************************************************
 * External global functions
 ******************************************************************************/
#if defined ( __SUPPORT_HELLO_WORLD__ )
extern void customer_hello_world_1(void *arg);
extern void customer_hello_world_2(void *arg);
#endif //( __SUPPORT_HELLO_WORLD__ )

#if defined ( __SUPPORT_WIFI_PROVISIONING__ )
extern void     softap_provisioning(void* arg);
#endif    // __SUPPORT_WIFI_PROVISIONING__

#if defined (__TCP_CLIENT_SLEEP2_SAMPLE__)
extern void	tcp_client_sleep2_sample(void *param);
extern void user_start_MQTT_client();
extern void user_terminate_transmit();
//extern void user_process_wifi_conn();
//extern void user_wifi_connection_complete_event();
//extern void user_terminate_transmission_event();
//extern void customer_hello_world_2(void *arg);
#endif // (__TCP_CLIENT_SLEEP2_SAMPLE__)

/******************************************************************************
 * External global variables
 ******************************************************************************/
#if defined ( __SUPPORT_WIFI_CONN_CB__ ) && !defined ( __ENABLE_SAMPLE_APP__ )

extern EventGroupHandle_t  evt_grp_wifi_conn_notify;

/* Station mode */
extern short wifi_conn_fail_reason;
extern short wifi_disconn_reason;

/* AP mode */
extern short ap_wifi_conn_fail_reason;
extern short ap_wifi_disconn_reason;
#endif  // __SUPPORT_WIFI_CONN_CB__ && !( __ENABLE_SAMPLE_APP__ )

/******************************************************************************
 * Local static functions
 ******************************************************************************/
#if defined ( __SUPPORT_WIFI_CONN_CB__ ) && !defined ( __ENABLE_SAMPLE_APP__ )
static void user_wifi_conn(void *arg);
static void user_wifi_conn_fail(void *arg);
static void user_wifi_disconn(void *arg);
#endif  // __SUPPORT_WIFI_CONN_CB__ && !( __ENABLE_SAMPLE_APP__ )

/******************************************************************************
 * Local variables
 ******************************************************************************/


/**********************************************************
 * Customer's application thread table
 **********************************************************/

const app_task_info_t    user_apps_table[] = {
/* name, func, stack_size, priority, net_chk_flag, dpm_flag, port_no, run_sys_mode */

/*
 * !!! Caution !!!
 *
 *     User applications should not affect the operation of the Sample code.
 *
 *     Do not remove "__ENABLE_SAMPLE_APP__" feature in this table.
 */

/*  Task Name,      Funtion,                Stack Size,      Task Priority, Network, DPM,  Net Port,  Run Sys_Mode  */
#if !defined ( __ENABLE_SAMPLE_APP__ )

#if defined ( __SUPPORT_WIFI_CONN_CB__ )
  { WIFI_CONN,          user_wifi_conn,         256, (OS_TASK_PRIORITY_USER+1), FALSE, FALSE, UNDEF_PORT, RUN_ALL_MODE    },
  { WIFI_CONN_FAIL,     user_wifi_conn_fail,    256, (OS_TASK_PRIORITY_USER+1), FALSE, FALSE, UNDEF_PORT, RUN_ALL_MODE    },
  { WIFI_DISCONN,       user_wifi_disconn,      256, (OS_TASK_PRIORITY_USER+1), FALSE, FALSE, UNDEF_PORT, RUN_ALL_MODE    },
#endif  // __SUPPORT_WIFI_CONN_CB__

#if defined (__TCP_CLIENT_SLEEP2_SAMPLE__) //JW: Task priority in 3220 was OS_TASK_PRIOTITY_USER + 3 ... not sure which it should be
    //{ SAMPLE_TCP_SLP2_CLI,				tcp_client_sleep2_sample,					2048, (OS_TASK_PRIORITY_USER + 6), FALSE, FALSE,	TCP_CLI_TEST_PORT,		RUN_ALL_MODE },
    //{ USER_READ_DATA,					tcp_client_sleep2_sample,					2048, (OS_TASK_PRIORITY_USER + 3), FALSE, FALSE,	UNDEF_PORT,		RUN_ALL_MODE },
  //JW: in testing, we've seen close to 2048 utilized -- I'm opting for a 50% margin.
  { USER_READ_DATA,					tcp_client_sleep2_sample,					3072, (OS_TASK_PRIORITY_USER + 3), FALSE, FALSE,	UNDEF_PORT,		RUN_ALL_MODE },
	//{ USER_TRANSMIT_DATA,				mqtt_transmit_task,					512, (OS_TASK_PRIORITY_USER + 7), TRUE, FALSE,	UNDEF_PORT,		RUN_ALL_MODE },
	//	{ HELLO_WORLD_2,				customer_hello_world_2,					512, (OS_TASK_PRIORITY_USER + 1), TRUE, FALSE,	UNDEF_PORT,		RUN_ALL_MODE },
#endif // (__TCP_CLIENT_SAMPLE__)

#if defined ( __SUPPORT_HELLO_WORLD__ )
  { HELLO_WORLD_1,      customer_hello_world_1, 512, (OS_TASK_PRIORITY_USER+1), FALSE, FALSE, UNDEF_PORT, RUN_ALL_MODE    },
  { HELLO_WORLD_2,      customer_hello_world_2, 512, (OS_TASK_PRIORITY_USER+1), TRUE,  FALSE, UNDEF_PORT, RUN_ALL_MODE    },
#endif  // __SUPPORT_HELLO_WORLD__

#endif    /* !__ENABLE_SAMPLE_APP__ */

#if defined (__SUPPORT_WIFI_PROVISIONING__)
  { CUSTOMER_PROVISION, softap_provisioning,    256, (OS_TASK_PRIORITY_USER+1), TRUE,  FALSE, UNDEF_PORT, (RUN_AP_MODE | RUN_STA_SOFTAP_MODE)    },
#endif    // __SUPPORT_WIFI_PROVISIONING__


    { NULL,    NULL,    0, 0, FALSE, FALSE, UNDEF_PORT, 0    }
};


/*============================================================
 *
 * Customer's applications ...
 *
 *============================================================*/

#if defined ( __SUPPORT_WIFI_CONN_CB__ ) && !defined ( __ENABLE_SAMPLE_APP__ )

///////////////////////////////////////////////////////////////////////////////
////  Customer call-back function to notify WI-Fi connection status
///////////////////////////////////////////////////////////////////////////////

static void user_wifi_conn(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    EventBits_t wifi_conn_ev_bits;

    while (TRUE) {
       wifi_conn_ev_bits = xEventGroupWaitBits(
                                       evt_grp_wifi_conn_notify,
                                       (WIFI_CONN_SUCC_STA | WIFI_CONN_SUCC_SOFTAP),
                                       pdTRUE,
                                       pdFALSE,
                                       WIFI_CONN_NOTI_WAIT_TICK);

       if (wifi_conn_ev_bits & WIFI_CONN_SUCC_STA) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_CONN_SUCC_STA);
            PRINTF("\n### User Call-back : Success to connect Wi-Fi ...\n");

#if defined(__TCP_CLIENT_SLEEP2_SAMPLE__)
            user_start_MQTT_client();
#endif

       } else if (wifi_conn_ev_bits & WIFI_CONN_SUCC_SOFTAP) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_CONN_SUCC_SOFTAP);

            PRINTF("\n### User Call-back : Success to connect Wi-Fi ...\n");
       } else {
           // Not proper event
       }
    }

    vTaskDelete(NULL);
}

static void user_wifi_conn_fail(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    EventBits_t wifi_conn_ev_bits;

    while (TRUE) {
       wifi_conn_ev_bits = xEventGroupWaitBits(
                                       evt_grp_wifi_conn_notify,
                                       (WIFI_CONN_FAIL_STA | WIFI_CONN_FAIL_SOFTAP),
                                       pdTRUE,
                                       pdFALSE,
                                       WIFI_CONN_NOTI_WAIT_TICK);

       if (wifi_conn_ev_bits & WIFI_CONN_FAIL_STA) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_CONN_FAIL_STA);

            PRINTF("\n### User Call-back : Failed to connect Wi-Fi ( reason_code = %d ) ...\n", wifi_conn_fail_reason);

       } else if (wifi_conn_ev_bits & WIFI_CONN_FAIL_SOFTAP) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_CONN_FAIL_SOFTAP);

            PRINTF("\n### User Call-back : Failed to connect Wi-Fi ( reason_code = %d ) ...\n", ap_wifi_conn_fail_reason);
       } else {
           // Not proper event
       }
    }

    vTaskDelete(NULL);
}

static void user_wifi_disconn(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    EventBits_t wifi_conn_ev_bits;

    while (TRUE) {
       wifi_conn_ev_bits = xEventGroupWaitBits(
                                       evt_grp_wifi_conn_notify,
                                       (WIFI_DISCONN_STA | WIFI_DISCONN_SOFTAP),
                                       pdTRUE,
                                       pdFALSE,
                                       WIFI_CONN_NOTI_WAIT_TICK);

       if (wifi_conn_ev_bits == WIFI_CONN_FAIL_STA) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_DISCONN_STA);

            PRINTF("\n### User Call-back : Wi-Fi disconnected ( reason_code = %d ) ...\n", wifi_disconn_reason);

       } else if (wifi_conn_ev_bits == WIFI_CONN_FAIL_SOFTAP) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_DISCONN_SOFTAP);

            PRINTF("\n### User Call-back : Disassociated because STA has left ( reason_code = %d ) ...\n", ap_wifi_disconn_reason);
       } else {
           // Not proper event
       }
    }

    vTaskDelete(NULL);
}

#endif // __SUPPORT_WIFI_CONN_CB__ && !( __ENABLE_SAMPLE_APP__ )

/* EOF */
