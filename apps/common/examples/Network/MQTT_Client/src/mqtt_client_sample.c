/**
 ****************************************************************************************
 *
 * @file mqtt_client_sample.c
 *
 * @brief MQTT Client Sample Application
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


#if defined (__MQTT_CLIENT_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "net_bsd_sockets.h"
#include "da16x_network_common.h"
#include "user_dpm.h"
#include "user_dpm_api.h"
#include "common_def.h"
#include "da16200_ioconfig.h"
#include "command_net.h"
#include "mqtt_client.h"
#include "user_nvram_cmd_table.h"
#include "util_api.h"
#include "da16x_cert.h"

#define EVT_PUB_COMPLETE          0x01
#define EVT_PUB_ERROR             0x02
#define EVT_UNSUB_DONE            0x04
#define EVT_UNSUB_ERR             0x08
#define EVT_MQ_TERM_APP           0x10
#define EVT_MQ_REGI_RTC_DONE      0x20
#define EVT_MQ_REGI_RTC_FAIL      0x40
#define EVT_ANY  (EVT_PUB_COMPLETE|EVT_PUB_ERROR|EVT_UNSUB_DONE|EVT_UNSUB_ERR|EVT_MQ_TERM_APP|EVT_MQ_REGI_RTC_DONE|EVT_MQ_REGI_RTC_FAIL)

#define SAMPLE_MQTT_CLIENT                  "MQTT_CLIENT"
#define NAME_MY_APP_Q_HANDLER               "MY_APP_Q_HANDLER"
#define NAME_JOB_MQTT_TX_PERIODIC           "MQ_TX_PERIODIC"
#define NAME_JOB_MQTT_TX_REPLY              "MQ_TX_REPLY"
#define NAME_JOB_MQTT_UNSUB                 "MQ_UNSUB"
#define NAME_JOB_MQTT_PERIODIC_PUB_RTC_REGI "MQ_REGI_PRTC"
#define NAME_JOB_MY_APP_TERM                "MY_APP_TERM"

#define APP_UNSUB_HDR            "unsub:"
#define SAMPLE_DPM_TIMER_NAME    "timer1"

/// Wait count for mqtt send: in multiple of 100 ms
#define MQTT_PUB_MAX_WAIT_CNT    100

/// The period of MQTT Message Transmission
#define MQTT_PUB_MSG_PERIODIC    30

/// The Message Queue size of my application queue
#define MSG_Q_SIZE               5

/// If mqtt_client_send_message_with_qos() is used for mqtt publish, enable.
#undef USE_MQTT_SEND_WITH_QOS_API

/// User mqtt publish message buffer
#define USER_MSG_LEN             50
static char user_msg_buf[USER_MSG_LEN];

static EventGroupHandle_t my_app_event_group;
static int tx_periodic;
static int tx_reply;
static TimerHandle_t timer_handler;
static TaskHandle_t my_app_q_hdler;
static int dpm_timer_id = -100;
static int topic_count = 0;
int my_app_err_code;
QueueHandle_t my_app_q;
static int mqtt_sample_msg_id = 0;

typedef enum    {
    APP_MSG_PUBLISH    = 1,
    APP_MSG_UNSUB    ,
    APP_MSG_TERMINATE,
    APP_MSG_REGI_RTC,

    APP_MSG_MAX
} MY_APP_MSG;

static void my_app_mqtt_user_config(void);

static BaseType_t my_app_send_to_q(char *job_name, int *job_flag, const MY_APP_MSG msg_id, const char *buf)
{
    dpm_app_register(job_name, 0);
    dpm_app_sleep_ready_clear(job_name);

    memset(user_msg_buf, 0x00, USER_MSG_LEN);

    if (msg_id == APP_MSG_PUBLISH) {
        if (strcmp(job_name, NAME_JOB_MQTT_TX_REPLY) == 0) {
            sprintf(user_msg_buf, "DA16K status : Not bad (%d)",
                    dpm_mode_is_enabled() ? mqtt_client_get_pub_msg_id() : ++mqtt_sample_msg_id);
        } else if (strcmp(job_name, NAME_JOB_MQTT_TX_PERIODIC) == 0) {
            sprintf(user_msg_buf, "DA16K Periodic Message (%d)", ++mqtt_sample_msg_id);
        }

        if (job_flag) {
            *job_flag = pdTRUE;
        }
    } else if (msg_id == APP_MSG_UNSUB) {
        strcpy(user_msg_buf, buf + strlen(APP_UNSUB_HDR));
        PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Topic to unsub = %s\n" CLEAR_COLOR, user_msg_buf);
    }

    // message queue init if NULL
    if (!my_app_q) {
        my_app_q = xQueueCreate(MSG_Q_SIZE, sizeof(int));
        if (my_app_q == NULL) {
            PRINTF(RED_COLOR "[%s] Msg Q Create Error!" CLEAR_COLOR, __func__);
            return pdFAIL;
        }
    }

    return xQueueSendToBack(my_app_q, &msg_id, 10);
}


/**
 ****************************************************************************************
 * @brief mqtt_client sample callback function for processing PUBLISH messages \n
 * Users register a callback function to process a PUBLISH message. \n
 * In this example, when mqtt_client receives a message with payload "1",
 * it sends MQTT PUBLISH with payload "DA16K status : ..." to the broker connected.
 * @param[in] buf the message paylod
 * @param[in] len the message paylod length
 * @param[in] topic the topic this mqtt_client subscribed to
 ****************************************************************************************
 */
void my_app_mqtt_msg_cb(const char *buf, int len, const char *topic)
{
    DA16X_UNUSED_ARG(len);

    BaseType_t ret;

    PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Msg Recv: Topic=%s, Msg=%s \n" CLEAR_COLOR, topic, buf);

    if (strcmp(buf, "reply_needed") == 0) {
        if ((ret = my_app_send_to_q(NAME_JOB_MQTT_TX_REPLY, &tx_reply, APP_MSG_PUBLISH, NULL)) != pdPASS ) {
            PRINTF(RED_COLOR "[%s] Failed to add a message to Q (%d)\r\n" CLEAR_COLOR, __func__, ret);
        }
    } else if (strncmp(buf, APP_UNSUB_HDR, 6) == 0) {
        if ((ret = my_app_send_to_q(NAME_JOB_MQTT_UNSUB, NULL, APP_MSG_UNSUB, buf)) != pdPASS ) {
            PRINTF(RED_COLOR "[%s] Failed to add a message to Q (%d)\r\n" CLEAR_COLOR, __func__, ret);
        }
    } else {
        return;
    }
}

void my_app_mqtt_pub_cb(int mid)
{
    DA16X_UNUSED_ARG(mid);
    xEventGroupSetBits(my_app_event_group, EVT_PUB_COMPLETE);
}

void my_app_mqtt_conn_cb(void)
{
    topic_count = 0;
}

void my_app_mqtt_sub_cb(void)
{
    topic_count++;
    if (dpm_mode_is_enabled() && topic_count == mqtt_client_get_topic_count()) {
        my_app_send_to_q(NAME_JOB_MQTT_PERIODIC_PUB_RTC_REGI, NULL, APP_MSG_REGI_RTC, NULL);
    }
}

static void _my_app_mqtt_pub_send_periodic(TimerHandle_t xTimer)
{
    DA16X_UNUSED_ARG(xTimer);

    BaseType_t ret;

    if (!mqtt_client_is_running() && !is_mqtt_client_thd_alive()) {
        PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Mqtt_client is in terminated state, terminating my app ... \n" CLEAR_COLOR);

        if ((ret = my_app_send_to_q(NAME_JOB_MY_APP_TERM, NULL, APP_MSG_TERMINATE, NULL)) != pdPASS ) {
            PRINTF(RED_COLOR "[%s] Failed to add a message to Q (%d)\r\n" CLEAR_COLOR, __func__, ret);
        }

        return;
    } else if (!mqtt_client_is_running() && is_mqtt_client_thd_alive()) {
        PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Mqtt_client may be trying to reconnect ... canceling the job this time \n" CLEAR_COLOR);
        return;
    }

    if ((ret = my_app_send_to_q(NAME_JOB_MQTT_TX_PERIODIC, &tx_periodic, APP_MSG_PUBLISH, NULL)) != pdPASS ) {
        PRINTF(RED_COLOR "[%s] Failed to add a message to Q (%d)\r\n" CLEAR_COLOR, __func__, ret);
    }

    return;
}

static void my_app_mqtt_pub_send_periodic(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    _my_app_mqtt_pub_send_periodic(NULL);
}

int my_app_mqtt_pub_msg(void)
{
    int ret;
#if !defined (USE_MQTT_SEND_WITH_QOS_API)
    int wait_cnt = 0;

LBL_SEND_RETRY:
    ret = mqtt_client_send_message(NULL, user_msg_buf);
    if (ret != 0) {
        if (ret == -2) { // previous message Tx is not finished
            vTaskDelay(10);

            wait_cnt++;
            if (wait_cnt == MQTT_PUB_MAX_WAIT_CNT) {
                PRINTF(CYAN_COLOR "[MQTT_SAMPLE] System is busy (max wait=%d), try next time \n" CLEAR_COLOR, MQTT_PUB_MAX_WAIT_CNT);
            } else {
                goto LBL_SEND_RETRY;
            }
        }
    }
#else
    ret = mqtt_client_send_message_with_qos(NULL, user_msg_buf, MQTT_PUB_MAX_WAIT_CNT);
    if (ret != 0) {
        if (ret == -2) {
            PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Mqtt send not successfully delivered, timtout=%d \n" CLEAR_COLOR, MQTT_PUB_MAX_WAIT_CNT);
        }
    }
#endif
    return ret;
}

int my_app_dpm_timer_create_checker(int tid)
{
    switch ((int)tid) {
    case DPM_MODE_NOT_ENABLED     :
    case DPM_TIMER_SEC_OVERFLOW   :
    case DPM_TIMER_ALREADY_EXIST  :
    case DPM_TIMER_NAME_ERROR     :
    case DPM_UNSUPPORTED_RTM      :
    case DPM_TIMER_REGISTER_FAIL  :
    case DPM_TIMER_MAX_ERR        :
    case -100                     :
        return pdFAIL;
    default:
        return pdPASS;
    }
}

void my_app_q_handler(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int ret;
    MY_APP_MSG RecvVal;
    BaseType_t xStatus;

    // message queue init
    if (!my_app_q) {
        my_app_q = xQueueCreate(MSG_Q_SIZE, sizeof(int));
        if (my_app_q == NULL) {
            PRINTF(RED_COLOR "[%s] Msg Q Create Error!" CLEAR_COLOR, __func__);
            return;
        }
    }

    while (1) {
        xStatus = xQueueReceive(my_app_q, &RecvVal, portMAX_DELAY);
        if (xStatus != pdPASS) {
            PRINTF(RED_COLOR "[%s] Q recv error (%d)\r\n" CLEAR_COLOR, __func__, xStatus);
            vTaskDelete(NULL);
            return;
        }

        if (RecvVal == APP_MSG_PUBLISH) {
            ret = my_app_mqtt_pub_msg();
            if (ret != 0) {
                my_app_err_code = ret;
                if (my_app_err_code == -1 && dpm_mode_is_enabled()) {
                    dpm_timer_delete(SAMPLE_MQTT_CLIENT, SAMPLE_DPM_TIMER_NAME);
                    dpm_timer_id = -100;
                }
                xEventGroupSetBits(my_app_event_group, EVT_PUB_ERROR);
            }
        } else if (RecvVal == APP_MSG_UNSUB) {
            ret = mqtt_client_unsub_topic(user_msg_buf);
            if (ret != 0) {
                my_app_err_code = ret; // mqtt_client_unsub_topic
                xEventGroupSetBits(my_app_event_group, EVT_UNSUB_ERR);
            } else {
                xEventGroupSetBits(my_app_event_group, EVT_UNSUB_DONE);
            }
        } else if (RecvVal == APP_MSG_TERMINATE) {
            xEventGroupSetBits(my_app_event_group, EVT_MQ_TERM_APP);
        } else if (RecvVal == APP_MSG_REGI_RTC) {
            if (dpm_timer_id == -100) {
                dpm_timer_id = dpm_timer_create(SAMPLE_MQTT_CLIENT, SAMPLE_DPM_TIMER_NAME,
                                                my_app_mqtt_pub_send_periodic, MQTT_PUB_MSG_PERIODIC * 1000, MQTT_PUB_MSG_PERIODIC * 1000); //msec
                if (my_app_dpm_timer_create_checker(dpm_timer_id) == pdFAIL) {
                    PRINTF(RED_COLOR "[%s] Fail to create %s timer (err=%d)\n" CLEAR_COLOR, __func__,
                           SAMPLE_MQTT_CLIENT, (int)dpm_timer_id);
                    xEventGroupSetBits(my_app_event_group, EVT_MQ_REGI_RTC_FAIL);
                } else {
                    PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Periodic Publish scheduled (RTC). \r\n" CLEAR_COLOR);
                    xEventGroupSetBits(my_app_event_group, EVT_MQ_REGI_RTC_DONE);
                }
            } else { // already created
                xEventGroupSetBits(my_app_event_group, EVT_MQ_REGI_RTC_DONE);
            }
        } else {
            PRINTF(RED_COLOR "[%s] Q recv error, undefined (%d)\r\n" CLEAR_COLOR, __func__, RecvVal);
        }
    }

    vTaskDelete(NULL);
    return;
}

void my_app_deinit(void)
{
    if (dpm_mode_is_enabled()) {
        if (dpm_timer_id != -100) {
            dpm_timer_delete(SAMPLE_MQTT_CLIENT, SAMPLE_DPM_TIMER_NAME);
            dpm_timer_id = -100;
        }
    } else {
        if (timer_handler) {
            xTimerDelete(timer_handler, 0);
            timer_handler = NULL;
        }
    }

    if (my_app_q_hdler) {
        vTaskDelete(my_app_q_hdler);
        my_app_q_hdler = NULL;
    }

    if (my_app_q) {
        vQueueDelete(my_app_q);
        my_app_q = NULL;
    }

    if (my_app_event_group) {
        vEventGroupDelete(my_app_event_group);
        my_app_event_group = NULL;
    }
}

static void my_app_job_done(char *job_name, int *job_flag)
{
    if (job_flag) {
        *job_flag = pdFALSE;
    }
    dpm_app_sleep_ready_set(job_name);
    dpm_app_unregister(job_name);
}

static int my_app_init(void)
{
    int ret;
    int    timer_id = 777;
    BaseType_t    xRet;

    // event flag group init
    if (!my_app_event_group) {
        my_app_event_group = xEventGroupCreate();
        if (my_app_event_group == NULL) {
            PRINTF(RED_COLOR "[%s] Event group Create Error!" CLEAR_COLOR, __func__);
            return -1;
        }
    }

    // message queue init
    if (!my_app_q) {
        my_app_q = xQueueCreate(MSG_Q_SIZE, sizeof(int));
        if (my_app_q == NULL) {
            PRINTF(RED_COLOR "[%s] Msg Q Create Error!" CLEAR_COLOR, __func__);
            return -2;
        }
    }

    // timer init
    if (dpm_mode_is_enabled()) {
        if (!dpm_mode_is_wakeup()) {
            if (dpm_timer_id == -100) {
                dpm_timer_id = dpm_timer_create(SAMPLE_MQTT_CLIENT, SAMPLE_DPM_TIMER_NAME,
                                                my_app_mqtt_pub_send_periodic, MQTT_PUB_MSG_PERIODIC * 1000, MQTT_PUB_MSG_PERIODIC * 1000); //msec
                if (my_app_dpm_timer_create_checker(dpm_timer_id) == pdFAIL) {
                    PRINTF(RED_COLOR "[%s] Fail to create %s timer (err=%d)\n" CLEAR_COLOR, __func__,
                           SAMPLE_MQTT_CLIENT, (int)dpm_timer_id);
                    // Delay to display above message on console ...
                    vTaskDelay(1);

                    my_app_job_done(NAME_JOB_MQTT_PERIODIC_PUB_RTC_REGI, NULL);
                    return -3;
                }
                PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Periodic Publish scheduled (RTC). \r\n" CLEAR_COLOR);
            }
        }
        my_app_job_done(NAME_JOB_MQTT_PERIODIC_PUB_RTC_REGI, NULL);
    } else {

        // timer init
        timer_handler = xTimerCreate("mqtt_pub",
                                     pdMS_TO_TICKS(MQTT_PUB_MSG_PERIODIC * 1000), pdTRUE,
                                     (void *)&timer_id, _my_app_mqtt_pub_send_periodic);

        if (timer_handler == NULL) {
            PRINTF(RED_COLOR "[%s] Timer Create Error!" CLEAR_COLOR, __func__);
            return -3;
        }

        ret = xTimerStart(timer_handler, 0);
        if (ret != pdPASS) {
            PRINTF(RED_COLOR "[%s] Failed to start timer(%d)\r\n" CLEAR_COLOR, __func__, ret);
            return -4;
        }

        PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Periodic Publish scheduled. \r\n" CLEAR_COLOR);
    }

    // start Q message handler
    xRet = xTaskCreate(my_app_q_handler, NAME_MY_APP_Q_HANDLER, (2048 / 4), NULL,
                       (OS_TASK_PRIORITY_USER + 6), &my_app_q_hdler);
    if (xRet != pdPASS) {
        my_app_q_hdler = NULL;
        PRINTF(RED_COLOR "[%s] Failed to create %s (0x%02x)\n" CLEAR_COLOR, __func__,
               NAME_MY_APP_Q_HANDLER, xRet);
        return -5;
    }

    return 0;
}

static int my_app_mqtt_chk_connection(int timeout)
{
    int wait_cnt = 0;
    while (1) {
        if (!mqtt_client_check_conn()) {
            vTaskDelay(100);

            wait_cnt++;
            if (wait_cnt == timeout) {
                PRINTF(RED_COLOR "[%s] mqtt connection timeout!, check your configuration \r\n" CLEAR_COLOR,
                       __func__);
                return pdFALSE;
            }
        } else {
            return pdTRUE;
        }
    }
}

/**
 ****************************************************************************************
 * @brief mqtt_client sample description \n
 * How to run this sample :
 *   See Example UM-WI-055_DA16200_FreeRTOS_Example_Application_Manual
 ****************************************************************************************
 */
void mqtt_client_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    char    temp_str[16];
    EventBits_t events = 0;
    int    ret;

    dpm_app_sleep_ready_clear(SAMPLE_MQTT_CLIENT);

    mqtt_client_set_msg_cb(my_app_mqtt_msg_cb);
    mqtt_client_set_pub_cb(my_app_mqtt_pub_cb);
    mqtt_client_set_conn_cb(my_app_mqtt_conn_cb);
    mqtt_client_set_subscribe_cb(my_app_mqtt_sub_cb);

    if (!dpm_mode_is_wakeup()) {
        // Non-DPM or DPM POR ...

        memset(temp_str, 0x00, 16);
        if (da16x_get_config_str(DA16X_CONF_STR_MQTT_BROKER_IP, temp_str)) {
            my_app_mqtt_user_config();
            PRINTF(CYAN_COLOR "[MQTT_SAMPLE] MQTT Configuration is done. \r\n" CLEAR_COLOR);

            ret = sntp_wait_sync(10); // wait for 5 sec max
            if (ret != 0) {
                if (ret == -1) { // timeout
                    PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Pls check Internet conenction. Reboot to start over \n"CLEAR_COLOR);
                }
                goto LBL_MY_APP_TERM;
            }

            mqtt_client_start();
        } else {
            PRINTF(CYAN_COLOR "[MQTT_SAMPLE] MQTT Configuration exists. \r\n" CLEAR_COLOR);
        }

        // my app terminates if mqtt connection is not made within the specified time.
        if (my_app_mqtt_chk_connection(40) == pdFALSE) {
            goto LBL_MY_APP_TERM;
        }
    }

    ret = my_app_init();
    if (ret != 0) {
        goto LBL_MY_APP_TERM;
    }

    // init or wakeup done ...
    dpm_app_wakeup_done(SAMPLE_MQTT_CLIENT);
    dpm_app_sleep_ready_set(SAMPLE_MQTT_CLIENT);

    while (1) {
        events = xEventGroupWaitBits(my_app_event_group,
                                     EVT_ANY,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);

        if (events & EVT_PUB_COMPLETE) {

            if (tx_periodic == pdTRUE) {
                PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Sending a periodic message complete. \n" CLEAR_COLOR);
                my_app_job_done(NAME_JOB_MQTT_TX_PERIODIC, &tx_periodic);
            } else if (tx_reply == pdTRUE) {
                PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Sending a reply message complete. \n" CLEAR_COLOR);
                my_app_job_done(NAME_JOB_MQTT_TX_REPLY, &tx_reply);
            } else {
                PRINTF(RED_COLOR "Tx source unknown, debug needed \n" CLEAR_COLOR);
            }
        } else if (events & EVT_PUB_ERROR) {

            if (tx_periodic == pdTRUE) {
#if !defined (USE_MQTT_SEND_WITH_QOS_API)
                PRINTF(YELLOW_COLOR "[MQTT_SAMPLE] Sending a periodic message failed, err(%d), refer to mqtt_client_send_message() \n"
                       CLEAR_COLOR
                       , my_app_err_code);
#else
                PRINTF(YELLOW_COLOR "[MQTT_SAMPLE] Sending a periodic message failed, err(%d), refer to mqtt_client_send_message_with_qos() \n"
                       CLEAR_COLOR
                       , my_app_err_code);
#endif
                my_app_job_done(NAME_JOB_MQTT_TX_PERIODIC, &tx_periodic);
            } else if (tx_reply == pdTRUE) {

#if !defined (USE_MQTT_SEND_WITH_QOS_API)
                PRINTF(YELLOW_COLOR "[MQTT_SAMPLE] Sending a reply message failed, err(%d), refer to mqtt_client_send_message() \n" CLEAR_COLOR
                       , my_app_err_code);
#else
                PRINTF(YELLOW_COLOR "[MQTT_SAMPLE] Sending a reply message failed, err(%d), refer to mqtt_client_send_message_with_qos() \n"
                       CLEAR_COLOR
                       , my_app_err_code);
#endif
                my_app_job_done(NAME_JOB_MQTT_TX_REPLY, &tx_reply);
            }
        } else if (events & EVT_UNSUB_DONE) {
            PRINTF(CYAN_COLOR "[MQTT_SAMPLE] Unsubscribe complete. \n" CLEAR_COLOR);
            my_app_job_done(NAME_JOB_MQTT_UNSUB, NULL);

        } else if (events & EVT_MQ_TERM_APP) {
            my_app_job_done(NAME_JOB_MY_APP_TERM, NULL);
            goto LBL_MY_APP_TERM;

        } else if (events & EVT_UNSUB_ERR) {

            if (my_app_err_code == 101) { // nvram driver ops error
                PRINTF(RED_COLOR "[MQTT_SAMPLE] Unsubscribe failed, nvram driver operation error \n" CLEAR_COLOR);
            } else {
                PRINTF(RED_COLOR "[MQTT_SAMPLE] Unsubscribe failed, err(%d) , refer to mqtt_client_unsub_topic() \n" CLEAR_COLOR
                       , my_app_err_code);
            }
            my_app_job_done(NAME_JOB_MQTT_UNSUB, NULL);

        } else if (events & EVT_MQ_REGI_RTC_DONE) {
            my_app_job_done(NAME_JOB_MQTT_PERIODIC_PUB_RTC_REGI, NULL);

        } else if (events & EVT_MQ_REGI_RTC_FAIL) {
            my_app_job_done(NAME_JOB_MQTT_PERIODIC_PUB_RTC_REGI, NULL);
            goto LBL_MY_APP_TERM;
        } else {
            PRINTF(RED_COLOR "Unknown event! %d \n" CLEAR_COLOR, events);
        }
    }

LBL_MY_APP_TERM:
    my_app_deinit();
    dpm_app_sleep_ready_set(SAMPLE_MQTT_CLIENT);

    if (mqtt_client_is_running() == TRUE) {
        mqtt_client_force_stop();
        mqtt_client_stop();
    }

    PRINTF (CYAN_COLOR "[MQTT_SAMPLE] My sample app terminated.\n" CLEAR_COLOR);

    vTaskDelete(NULL);
}

static const char *cert_buffer0 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIID+TCCAuGgAwIBAgIJANqqHCazDkkOMA0GCSqGSIb3DQEBCwUAMIGSMQswCQYD\n"
    "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEUMBIGA1UEBwwLU2FudGEgQ2xh\n"
    "cmExFzAVBgNVBAoMDldpLUZpIEFsbGlhbmNlMR0wGwYDVQQDDBRXRkEgUm9vdCBD\n"
    "ZXJ0aWZpY2F0ZTEgMB4GCSqGSIb3DQEJARYRc3VwcG9ydEB3aS1maS5vcmcwHhcN\n"
    "MTMwMzExMTkwMjI2WhcNMjMwMzA5MTkwMjI2WjCBkjELMAkGA1UEBhMCVVMxEzAR\n"
    "BgNVBAgMCkNhbGlmb3JuaWExFDASBgNVBAcMC1NhbnRhIENsYXJhMRcwFQYDVQQK\n"
    "DA5XaS1GaSBBbGxpYW5jZTEdMBsGA1UEAwwUV0ZBIFJvb3QgQ2VydGlmaWNhdGUx\n"
    "IDAeBgkqhkiG9w0BCQEWEXN1cHBvcnRAd2ktZmkub3JnMIIBIjANBgkqhkiG9w0B\n"
    "AQEFAAOCAQ8AMIIBCgKCAQEA6TOCu20m+9zLZITYAhGmtxwyJQ/1xytXSQJYX8LN\n"
    "YUS/N3HG2QAQ4GKDh7DPDI13zhdc0yOUE1CIOXa1ETKbHIU9xABrL7KfX2HCQ1nC\n"
    "PqRPiW9/wgQch8Aw7g/0rXmg1zewPJ36zKnq5/5Q1uyd8YfaXBzhxm1IYlwTKMlC\n"
    "ixDFcAeVqHb74mAcdel1lxdagHvaL56fpUExm7GyMGXYd+Q2vYa/o1UwCMGfMOj6\n"
    "FLHwKpy62KCoK3016HlWUlbpg8YGpLDt2BB4LzxmPfyH2x+Xj75mAcllOxx7GK0r\n"
    "cGPpINRsr4vgoltm4Bh1eIW57h+gXoFfHCJLMG66uhU/2QIDAQABo1AwTjAdBgNV\n"
    "HQ4EFgQUCwPCPlSiKL0+Sd5y8V+Oqw6XZ4IwHwYDVR0jBBgwFoAUCwPCPlSiKL0+\n"
    "Sd5y8V+Oqw6XZ4IwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAsNxO\n"
    "z9DXb7TkNFKtPOY/7lZig4Ztdu6Lgf6qEUOvJGW/Bw1WxlPMjpPk9oI+JdR8ZZ4B\n"
    "9QhE+LZhg6SJbjK+VJqUcTvnXWdg0e8CgeUw718GNZithIElWYK3Kh1cSo3sJt0P\n"
    "z9CiJfjwtBDwsdAqC9zV9tgp09QkEkav84X20VxaITa3H1QuK/LWSn/ORrzcX0Il\n"
    "10YoF6Hz3ZWa65mUoMzd8DYtCyGtcbYrSt+NMCqRB186PDQn5XBCytgF8VuiCyyk\n"
    "Z04hqHLzAFc21P9yhwKGi3BHD/Sep8fvr9y4VpMIqHQm2jaFPxY1VxhPSV+UHoE1\n"
    "fCPitIJTp/iXi7uXTQ==\n"
    "-----END CERTIFICATE-----\n";
static const char *cert_buffer1 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEBTCCAu2gAwIBAgICEEYwDQYJKoZIhvcNAQELBQAwgZIxCzAJBgNVBAYTAlVT\n"
    "MRMwEQYDVQQIDApDYWxpZm9ybmlhMRQwEgYDVQQHDAtTYW50YSBDbGFyYTEXMBUG\n"
    "A1UECgwOV2ktRmkgQWxsaWFuY2UxHTAbBgNVBAMMFFdGQSBSb290IENlcnRpZmlj\n"
    "YXRlMSAwHgYJKoZIhvcNAQkBFhFzdXBwb3J0QHdpLWZpLm9yZzAeFw0xMzA1MTAy\n"
    "MzQ0NTFaFw0yMzA1MDgyMzQ0NTFaMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApD\n"
    "YWxpZm9ybmlhMRcwFQYDVQQKDA5XaS1GaSBBbGxpYW5jZTEfMB0GA1UEAwwWQ2xp\n"
    "ZW50IENlcnRpZmljYXRlIElETDEgMB4GCSqGSIb3DQEJARYRc3VwcG9ydEB3aS1m\n"
    "aS5vcmcwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDco7kQ4W2b/fBw\n"
    "2ZgSAXVWBCmJW0yax8K682kRiHlB1aJ5Im8rTEktZlPDQVhoO3Ur+Ij9Y8ukD3Hq\n"
    "CMa95T1a3Ly9lhDIME/VRqRgZRGa/FC/jkiK9u9SgIXPZqJk1s/JVG7a7deC8BvK\n"
    "iqIFhXoHl0N4nJxwVA8kag48dXbrTxrOH26C9qwU85iS/c1ozHJMmq052WPSQZII\n"
    "Sq8+VmxlCbbXxQ7kU2oZkxDqW3hMI3OPS80s8q4tMzuitvzFO0JvAHgh4IFyE7yg\n"
    "nIE7+lM9f3EwzECw9nEBdL7AvfnYLhlIEI8S8wZTUpnd8XA5Qs7Qa4rLNuqI273Z\n"
    "IWJLh9v/AgMBAAGjeDB2MA8GA1UdEwEB/wQFMAMCAQAwCwYDVR0PBAQDAgXgMBYG\n"
    "A1UdJQEB/wQMMAoGCCsGAQUFBwMCMB0GA1UdDgQWBBQtC2mx8POhZRfB+sj4wX1Z\n"
    "OzdrCzAfBgNVHSMEGDAWgBQLA8I+VKIovT5J3nLxX46rDpdngjANBgkqhkiG9w0B\n"
    "AQsFAAOCAQEAsvHJ+J2YTCsEA69vjSmsGoJ2iEXDuHI+57jIo+8qRVK/m1is1eiU\n"
    "AefExDtxxKTEPPtourlYO8A4i7oep9T43Be8nwVZdmxzfu14cdLKQrE+viCuHQTc\n"
    "BLSoAv6/SZa3MeIRkkDSPtCPTJ/Pp+VYPK8gPlc9pwEs/KLgFxK/Sq6RDNjTPs6J\n"
    "MChxi1iSdUES8mz3JDhQ2RQWVuibPorhgVqNTyXBjFUbYxTVl3hBCtK/Bd4IyFTX\n"
    "Li90XXHseNbj2sHu3PFBU7PG5mhKQMUOYqvQzEDIXT6SDA+PrepqrwXn/BL861K6\n"
    "GV4LcfKBg0HHdW9gJByZCN02igFTw56Kzg==\n"
    "-----END CERTIFICATE-----\n";
static const char *cert_buffer2 =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpQIBAAKCAQEA3KO5EOFtm/3wcNmYEgF1VgQpiVtMmsfCuvNpEYh5QdWieSJv\n"
    "K0xJLWZTw0FYaDt1K/iI/WPLpA9x6gjGveU9Wty8vZYQyDBP1UakYGURmvxQv45I\n"
    "ivbvUoCFz2aiZNbPyVRu2u3XgvAbyoqiBYV6B5dDeJyccFQPJGoOPHV2608azh9u\n"
    "gvasFPOYkv3NaMxyTJqtOdlj0kGSCEqvPlZsZQm218UO5FNqGZMQ6lt4TCNzj0vN\n"
    "LPKuLTM7orb8xTtCbwB4IeCBchO8oJyBO/pTPX9xMMxAsPZxAXS+wL352C4ZSBCP\n"
    "EvMGU1KZ3fFwOULO0GuKyzbqiNu92SFiS4fb/wIDAQABAoIBAQDcnbCc2mt5AM98\n"
    "Z3aQ+nhSy9Kkj2/njDqAKIc0ituEIpNUwEOcbaj2Bk1W/W3iuyEMGHURuMmUgAUN\n"
    "WD0w/5j705+9ieG56eTJgts1r5mM+SHch+6tVQAz5GLn4N4cKlaWHyDBM/S77k47\n"
    "lacwEijUkkFaxm3+O27woEMf3OxNl24KmRenMYBhqcsoT4BYBw3Bh8xe+XN95rXj\n"
    "2BdIbr5+RWGc9Zsz4o5Wmd4mL/JvbKeohrsecien4TZRzWFku93XV5kie1c1aJy1\n"
    "nJ85bGJk4focmP/2ToxQysTbPYCxHVTIHuADK/qf9SGHJ9F7EBHE7+0isuwBbqOD\n"
    "OzS8rHdRAoGBAPCXlaHumEkLIRv3enhpHPBYxnDndNCtT1T6+Cuit/vfo6K6oA7p\n"
    "iUaej/GPZsDKXhayeTiEaq7QMinUtGkiCgGlVtXghXuCZz6KrH19W6wzC6Pbokmq\n"
    "BZak4LQcvGavt3VzjliAKLcdn6nQt/+bp/jKDJOKVbvb30sjS035Ah4zAoGBAOrF\n"
    "BgE9UTEnfQHIh7pyiM1DAomBbdrlRos8maQl26cHqUHN3+wy1bGHLzOjYFFoAasx\n"
    "eizw7Gudgbae28WIP1yLGrpt15cqVAvlCYmBtZ3C98FuT3FYgEEZpWNmE8Om+5UM\n"
    "td+mtMjonWAPkCYC+alqUZzeIs+CZs5CHKYCDqcFAoGBAOfkQv38GV2102jARJPQ\n"
    "RGtINaRXApmrod43s4Fjac/kAzVyiZk18PFXHUhnvlMt+jgIN5yIzMbHtsHo2SbH\n"
    "/zsM4MBuklm0G80FHjIp5HT6EksSA77amF5VdptDYzfaP4p+IYIdrKCqddzYZrCA\n"
    "mArMvAhs+iuCRhuG3is+SZNPAoGAHs6r8w2w0dp0tP8zkGvnN8hLVO//EnJzx2G0\n"
    "Z63wHQMMWu5BLCWflSRANW6C/SvAzE450hvralPI6cX+4PT4G5TFdSFk4RlU3hq4\n"
    "Has/wewLxv5Kvnz2l5Rd96U1gr8u1GhOlYKyxop/3FMuf050pJ6nBwa/WquqAfb6\n"
    "+23ZrmECgYEA6l0GFHwMFBNnpPuxHgYgS5+4g3+8DhZZIDc7IflBCBWF/ZwbM+nH\n"
    "+JSxiYYjvD7zIBhndqERcZ+fvbZTQ8oymr3j5AESM0ZfAHbft6IFQWjDUC3IDUF/\n"
    "4F0cUidFC8smu6Wa2tjvSIz7DfvmDsn1l+7s9qQvDxdyPas0IkL/v8w=\n"
    "-----END RSA PRIVATE KEY-----\n";

#define MQTT_SAMPLE_BROKER_IP      "192.168.0.230"
#define MQTT_SAMPLE_BROKER_PORT    8883
#define MQTT_SAMPLE_QOS            2
#define MQTT_SAMPLE_TLS            1
#define MQTT_SAMPLE_TOPIC_SUB_1    "da16k1"
#define MQTT_SAMPLE_TOPIC_SUB_2    "da16k2"
#define MQTT_SAMPLE_TOPIC_SUB_3    "da16k3"
#define MQTT_SAMPLE_TOPIC_PUB      "_da16k"

/**
 ****************************************************************************************
 * @brief MQTT Basic Configuration Function \n
 * @subsection Parameters
 * - Broker IP Address
 * - Broker Port Number
 * - Qos
 * - Subscriber Topic
 * - Publisher Topic
 * - DPM Use
 * - SNTP Use (for TLS valid time)
 * - TLS Use
 * - TLS Root CA
 * - TLS Client Certificate
 * - TLS Client Private Key
 ****************************************************************************************
 */
static void my_app_mqtt_user_config(void)
{
    /* MQTT Setting */
    da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_AUTO, 1);
    da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_BROKER_IP, MQTT_SAMPLE_BROKER_IP);
    da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_PORT, MQTT_SAMPLE_BROKER_PORT);
    da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_QOS, MQTT_SAMPLE_QOS);
    da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_TLS, MQTT_SAMPLE_TLS);

    da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC_ADD, MQTT_SAMPLE_TOPIC_SUB_1);
    da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC_ADD, MQTT_SAMPLE_TOPIC_SUB_2);
    da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC_ADD, MQTT_SAMPLE_TOPIC_SUB_3);

    da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, MQTT_SAMPLE_TOPIC_PUB);
    da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_PING_PERIOD, 60);

    /* DPM after Rebooting */
    da16x_set_nvcache_int(DA16X_CONF_INT_DPM, 1);

    da16x_set_nvcache_int(DA16X_CONF_INT_SNTP_CLIENT, 1);
    da16x_nvcache2flash();

    /* Input Certificate & Private Key */
    cert_flash_write(SFLASH_ROOT_CA_ADDR1, (char *)cert_buffer0, strlen(cert_buffer0));
    cert_flash_write(SFLASH_CERTIFICATE_ADDR1, (char *)cert_buffer1, strlen(cert_buffer1));
    cert_flash_write(SFLASH_PRIVATE_KEY_ADDR1, (char *)cert_buffer2, strlen(cert_buffer2));
}

#endif    // (__MQTT_CLIENT_SAMPLE__)

/* EOF */
