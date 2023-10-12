/**
 *****************************************************************************************
 * @file     sub_client.c
 * @brief    MQTT Subscriber main handler
 * @todo		File name : sub_client.c -> mqtt_client_subscriber.c
 *****************************************************************************************
 */

/*
Copyright (c) 2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.

Copyright (c) 2019-2022 Modified by Renesas Electronics.
*/

#include "sdk_type.h"

#if defined	(__SUPPORT_MQTT__)

#include "mqtt_client.h"
#include "common_def.h"
#include "mosquitto_internal.h"
#include "memory_mosq.h"
#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#endif
#include "da16x_network_common.h"
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
#include "mqtt_msg_tbl_presvd.h"
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
#include "errno.h"

int subRetryCount	= 0;
struct mosquitto *mosq_sub = NULL;

extern UINT mqttThreadStatus;
extern UINT runContinueSub;
extern UINT force_mqtt_stop;
extern struct mosq_config mqtt_cfg;
extern mqttParamForRtm mqttParams;
extern int new_local_port_allocated;
extern void atcmd_asynchony_event(int index, char *buf);
extern void mqtt_client_dpm_regi_with_port(void);
extern void _mosquitto_set_broker_conn_state(UINT8 state);

extern mqtt_mutex_t mqtt_conf_mutex;
extern mqtt_mutex_t connect_mutex;

#if defined	(__MQTT_SELF_RECONNECT__)
static void sub_dummy_function(char *timer_name);
#endif // (__MQTT_SELF_RECONNECT__)

extern void mqtt_client_user_cb_set(void);
extern void atcmd_asynchony_event_for_mqttmsgpub(int result, int err_code);

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
static int mqtt_connect_ind_sent;
void mqtt_connect_ind_sent_set(int val)
{
	mqtt_connect_ind_sent = val;
}
int mqtt_connect_ind_is_sent(void)
{
	return mqtt_connect_ind_sent;
}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

#undef USER_CALLBACK_TEST
#if defined (USER_CALLBACK_TEST)
void mqtt_pub_user_cb_dummy(int mid)
{
	PRINTF(RED_COLOR "user cb: on_publish(mid=%d)\n" CLEAR_COLOR, mid);
}
void mqtt_conn_user_cb_dummy(void)
{
	PRINTF(RED_COLOR "user cb: on_connect \n" CLEAR_COLOR);
}
void mqtt_disconn_user_cb_dummy(void)
{
	PRINTF(RED_COLOR "user cb: on_disconnect \n" CLEAR_COLOR);
}
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
void mqtt_disconn2_user_cb_dummy(void)
{
	PRINTF(RED_COLOR "user cb: on_disconnect2 \n" CLEAR_COLOR);
	PRINTF(RED_COLOR "application needs to clear this invalid message by connecting with clean_session=1 \n" CLEAR_COLOR);
}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
void mqtt_msg_user_cb_dummy(const char* buf, int len, const char* topic)
{
	PRINTF(RED_COLOR "user cb: on_message, topic = %s, msg = %s, len = %d\n" CLEAR_COLOR, topic, buf, len);
}

void mqtt_subscribe_user_cb_dummy(void)
{
	PRINTF(RED_COLOR "user cb: on_subscribe \n" CLEAR_COLOR);
}

void mqtt_unsubscribe_user_cb_dummy(void)
{
	PRINTF(RED_COLOR "user cb: on_unsubscribe \n" CLEAR_COLOR);
}

void (*mqtt_pub_user_cb)(int mid) = mqtt_pub_user_cb_dummy;
void (*mqtt_conn_user_cb)(void) = mqtt_conn_user_cb_dummy;
void (*mqtt_disconn_user_cb)(void) = mqtt_disconn_user_cb_dummy;
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
void (*mqtt_disconn2_user_cb)(void) = mqtt_disconn2_user_cb_dummy;
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
void (*mqtt_msg_user_cb)(const char* buf, int len, const char* topic) = mqtt_msg_user_cb_dummy;
void (*mqtt_subscribe_user_cb)(void) = mqtt_subscribe_user_cb_dummy;
void (*mqtt_unsubscribe_user_cb)(void) = mqtt_unsubscribe_user_cb_dummy;
#else
void (*mqtt_pub_user_cb)(int mid) = NULL;
void (*mqtt_conn_user_cb)(void) = NULL;
void (*mqtt_disconn_user_cb)(void) = NULL;
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
void (*mqtt_disconn2_user_cb)(void) = NULL;
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
void (*mqtt_msg_user_cb)(const char* buf, int len, const char* topic) = NULL;
void (*mqtt_subscribe_user_cb)(void) = NULL;
void (*mqtt_unsubscribe_user_cb)(void) = NULL;
#endif // USER_CALLBACK_TEST

void mqtt_client_set_pub_cb(void (*user_pub_cb)(int))
{
	mqtt_pub_user_cb = user_pub_cb;

	if (mosq_sub != NULL) {
		mosquitto_publish_callback_set(mosq_sub, mqtt_pub_user_cb);
	}
}

void mqtt_client_set_conn_cb(void (*user_cb)(void))
{
	mqtt_conn_user_cb = user_cb;

	if (mosq_sub != NULL) {
		mosquitto_connect_callback_set(mosq_sub, mqtt_conn_user_cb);
	}
}

void mqtt_client_set_disconn_cb(void (*user_cb)(void))
{
	mqtt_disconn_user_cb = user_cb;
	
	if (mosq_sub != NULL) {
		mosquitto_disconnect_callback_set(mosq_sub, mqtt_disconn_user_cb);
	}
}

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
void mqtt_client_set_disconn2_cb(void (*user_cb)(void))
{
	mqtt_disconn2_user_cb = user_cb;
	
	if (mosq_sub != NULL) {
		mosquitto_disconnect2_callback_set(mosq_sub, mqtt_disconn2_user_cb);
	}
}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

void mqtt_client_set_msg_cb(void (*user_cb)(const char *buf, int len, const char *topic))
{
	mqtt_msg_user_cb = user_cb;

	if (mosq_sub != NULL)
	{
		mosquitto_message_callback_set(mosq_sub, mqtt_msg_user_cb);
	}
}

void mqtt_client_set_subscribe_cb(void (*user_cb)(void))
{
	mqtt_subscribe_user_cb = user_cb;

	if (mosq_sub != NULL) {
		mosquitto_subscribe_callback_set(mosq_sub, mqtt_subscribe_user_cb);
	}
}

void mqtt_client_set_unsubscribe_cb(void (*user_cb)(void))
{
	mqtt_unsubscribe_user_cb = user_cb;

	if (mosq_sub != NULL) {
		mosquitto_unsubscribe_callback_set(mosq_sub, mqtt_unsubscribe_user_cb);
	}
}

static int qos = 0;
static int mid_sent = 0;

char *mpub_message_buf	= NULL;
char *mpub_topic		= NULL;

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
static int invalid_msg_rx_in_cs_0;
int is_disconn_by_too_long_msg_rx_in_cs0(void) {return invalid_msg_rx_in_cs_0;}
void set_disconn_by_too_long_msg_rx_in_cs0(int onff) {invalid_msg_rx_in_cs_0=onff;}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

int my_publish(struct mosquitto *mosq, void *obj, int result)
{
	DA16X_UNUSED_ARG(obj);

	int rc = MOSQ_ERR_SUCCESS;

	if (!result)
	{
		if (mpub_message_buf == NULL)
		{
			return MOSQ_ERR_INVAL;
		}

		rc = mosquitto_publish(mosq, &mid_sent, mpub_topic, strlen(mpub_message_buf), mpub_message_buf, qos, 0);
		if (rc)
		{
			switch (rc)
			{
				case MOSQ_ERR_INVAL:
				{
					MQTT_DBG_ERR(RED_COLOR "ERR: Invalid input\n" CLEAR_COLOR);
				} break;

				case MOSQ_ERR_NOMEM:
				{
					MQTT_DBG_ERR(RED_COLOR "ERR: Out of memory\n" CLEAR_COLOR);
					mosq->state = mosq_cs_disconnect_cb;
				} break;

				case MOSQ_ERR_NO_CONN:
				{
					MQTT_DBG_ERR(RED_COLOR "ERR: No connection\n" CLEAR_COLOR);
					mosq->state = mosq_cs_disconnect_cb;
				} break;

				case MOSQ_ERR_PROTOCOL:
				{
					MQTT_DBG_ERR(RED_COLOR "ERR: Protocol error\n" CLEAR_COLOR);
					mosq->state = mosq_cs_disconnect_cb;
				} break;

				case MOSQ_ERR_PAYLOAD_SIZE:
				{
					MQTT_DBG_ERR(RED_COLOR "ERR: Payload is too long\n" CLEAR_COLOR);
				} break;
			}
		}
		else
		{
			MQTT_DBG_PRINT("(Tx: Len=%d,Topic=%s,Msg_ID=%d)\n", 
						strlen(mpub_message_buf), mpub_topic, mid_sent);
		}	

		if (mpub_message_buf)
		{
			_mosquitto_free(mpub_message_buf);
			mpub_message_buf = NULL;
		}

		if (mpub_topic)
		{
			_mosquitto_free(mpub_topic);
			mpub_topic = NULL;
		}

		mpub_topic = _mosquitto_strdup(mqtt_cfg.topic);

		if (rc == 0)
		{
			MQTT_DBG_PRINT("<< Mqtt Pub EnQ : SUCCESS >> \n");
		}
		else
		{
			MQTT_DBG_PRINT("<< Mqtt Pub EnQ : FAIL (%d) >> \n", rc);
		}

		if (!dpm_mode_is_enabled()) { // invoked by mqtt thread
			if (mqtt_cfg.qos == 0) {
				if (rc == 0) atcmd_asynchony_event_for_mqttmsgpub(1, 0);
				else atcmd_asynchony_event_for_mqttmsgpub(0, rc);
			}
		}
	}
	else
	{
		if (result)
		{
			MQTT_DBG_INFO(" [%s] %s\n", __func__, mosquitto_connack_string(result));
		}
	}
	
	return rc;
}

void mqtt_sub_on_connect(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct mosq_config *cfg;

	cfg = (struct mosq_config *)obj;

	if (!result)
	{
		for (i = 0; i < cfg->topic_count; i++)
		{
			mosquitto_subscribe(mosq, NULL, cfg->topics[i], cfg->qos);
		}
	}
	else
	{
		MQTT_DBG_INFO(" [%s] %s\n", __func__, mosquitto_connack_string(result));
	}
}

#if defined	(MQTT_USAGE)
static void print_usage(void)
{
	PRINTF("mosquitto_sub is a simple mqtt client that will subscribe to a single topic and print all messages it receives.\n");
	PRINTF("Usage: mosquitto_sub [-c] [-h host] [-k keepalive] [-p port] [-q qos] [-R] -t topic ...\n");
	PRINTF("                     [-A bind_address]\n");
	PRINTF("                     [-i id] [-I id_prefix]\n");
	PRINTF("                     [-d] [-N] [--quiet] [-v]\n");
	PRINTF("                     [-u username [-P password]]\n");
	PRINTF("                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]\n");
	PRINTF("       mosquitto_sub --help\n\n");
	PRINTF(" -A : bind the outgoing socket to this host/ip address. Use to control which interface\n");
	PRINTF("      the client communicates over.\n");
	PRINTF(" -c : disable 'clean session' (store subscription and pending messages when client disconnects).\n");
	PRINTF(" -h : mqtt host to connect to. Defaults to localhost.\n");
	PRINTF(" -i : id to use for this client. Defaults to mosquitto_sub_ appended with the process id.\n");
	PRINTF(" -I : define the client id as id_prefix appended with the process id. Useful for when the\n");
	PRINTF("      broker is using the clientid_prefixes option.\n");
	PRINTF(" -k : keep alive in seconds for this client. Defaults to 60.\n");
	PRINTF(" -N : do not add an end of line character when printing the payload.\n");
	PRINTF(" -p : network port to connect to. Defaults to 1883.\n");
	PRINTF(" -P : provide a password (requires MQTT 3.1 broker)\n");
	PRINTF(" -q : quality of service level to use for the subscription. Defaults to 0.\n");
	PRINTF(" -R : do not print stale messages (those with retain set).\n");
	PRINTF(" -t : mqtt topic to subscribe to. May be repeated multiple times.\n");
	PRINTF(" -T : topic string to filter out of results. May be repeated.\n");
	PRINTF(" -u : provide a username (requires MQTT 3.1 broker)\n");
	PRINTF(" -v : print published messages verbosely.\n");
	PRINTF(" -V : specify the version of the MQTT protocol to use when connecting.\n");
	PRINTF(" -x : stop mqtt_sub.\n");
	PRINTF("      Can be mqttv31 or mqttv311. Defaults to mqttv31.\n");
	PRINTF(" --help : display this message.\n");
	PRINTF(" --quiet : don't print error messages.\n");
	PRINTF(" --will-payload : payload for the client Will, which is sent by the broker in case of\n");
	PRINTF("                  unexpected disconnection. If not given and will-topic is set, a zero\n");
	PRINTF("                  length message will be sent.\n");
	PRINTF(" --will-qos : QoS level for the client Will.\n");
	PRINTF(" --will-retain : if given, make the client Will retained.\n");
	PRINTF(" --will-topic : the topic on which to publish the client Will.\n");
}
#endif // (MQTT_USAGE)

mqtt_client_thread mqtt_sub_thd_config = {0, };

USHORT is_mqtt_client_thd_alive(void)
{
	return (mqtt_sub_thd_config.thread)? pdTRUE:pdFALSE;
}

#if defined	(__MQTT_SELF_RECONNECT__)
extern bool  get_dpm_pwrdown_fail_max_trial_reached(void);
extern void  set_dpm_pwrdown_fail_max_trial_reached(bool value);
static TaskHandle_t mqtt_dpm_mon = NULL;
extern void mqtt_client_save_to_dpm_user_mem(void);

/* 
    The task (mqtt_client_restart_monitor) gets started when .... 
    - dpm mode is enabled
    - mqtt reconnect fails and if only ps-cmd max trial failure reached
*/
static void mqtt_client_restart_monitor(void* arg)
{
	DA16X_UNUSED_ARG(arg);

    while(1) {
        if (get_dpm_pwrdown_fail_max_trial_reached() == pdTRUE) {
            mqttParams.sub_connect_retry_count = 0;
            mqtt_client_save_to_dpm_user_mem();
            mqtt_client_start();
            set_dpm_pwrdown_fail_max_trial_reached(pdFALSE);
            break;
        }
        vTaskDelay(5);
    }
    
	vTaskDelete(NULL);
}
#endif /* __MQTT_SELF_RECONNECT__ */

static int mqtt_subscriber_main(void)
{
	int	rc;
	int	status;
    int mqtt_tcp_tls_conn_fail;

	subRetryCount = 0;

	if (runContinueSub == true)
	{
		MQTT_DBG_ERR("MQTT_Sub is already running.\n");
		return -1;
	}

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());
    da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_MQTT_Sub());
	status = mqtt_client_initialize_config();
	if (status != pdTRUE)
	{
		MQTT_DBG_ERR(RED_COLOR "[SUB] Failed to start (sts:%d) \n" CLEAR_COLOR, status);
		client_config_cleanup(&mqtt_cfg);
        da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_MQTT_Sub());
        atcmd_asynchony_event(4, "0");
		return 1;
	}
    da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_MQTT_Sub());

	da16x_secure_module_init(); //init cc312

restart:

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());

	runContinueSub = true;

	if (force_mqtt_stop == true) subRetryCount = MQTT_CONN_MAX_RETRY;

	mqtt_client_dpm_regi_with_port();

	MQTT_DBG_INFO("[%s] SUB-Start(subRetryCount:%d, restartCount:%d). \n",
				  __func__, subRetryCount, dpm_mode_is_enabled() ? mqttParams.sub_connect_retry_count : 0);

	qos = mqtt_cfg.qos;

	if (mpub_topic)
	{
		_mosquitto_free(mpub_topic);
		mpub_topic = NULL;
	}

	mpub_topic = _mosquitto_strdup(mqtt_cfg.topic);
	if (!mpub_topic)
	{
		MQTT_DBG_ERR(RED_COLOR "[SUB] ERR: No Topic.\n" CLEAR_COLOR);
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
		client_config_cleanup(&mqtt_cfg);
        atcmd_asynchony_event(4, "0");
		return 1;
	}

	mosq_sub = mosquitto_new(mqtt_cfg.sub_id, mqtt_cfg.clean_session, &mqtt_cfg);
	if (!mosq_sub)
	{
		MQTT_DBG_ERR(RED_COLOR "[SUB] ERR: mosquitto_new()\n" CLEAR_COLOR);
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
		client_config_cleanup(&mqtt_cfg);
        atcmd_asynchony_event(4, "0");
		return 1;
	}
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	mq_msg_tbl_presvd_init(&mqtt_cfg, mosq_sub);
#endif

	MQTT_DBG_INFO(" [%s] Done newing mosq_sub \n", __func__);

	mosq_sub->mqtt_pub_or_sub = CLIENT_SUB;

	if (client_opts_set(mosq_sub, &mqtt_cfg))
	{
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
		client_config_cleanup(&mqtt_cfg);
        atcmd_asynchony_event(4, "0");
		return 1;
	}
	mosq_sub->unsub_topic = mqtt_cfg.topic_count;

	MQTT_DBG_INFO(" [%s] Done set opts \n", __func__);

	mqtt_client_user_cb_set();

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());

    da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_MQTT_Sub());

	mqttThreadStatus |= THREAD_STS_SUB_INIT_DONE;
	rc = client_connect(mosq_sub, &mqtt_cfg);

    da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_MQTT_Sub());

	if (rc)
	{
	    mqtt_tcp_tls_conn_fail = 1;
		goto end_of_func;
	}

	MQTT_DBG_INFO("[%s] client_connect OK. \n", __func__);

    mqtt_tcp_tls_conn_fail = 0;

	if (dpm_mode_is_enabled())
	{
		dpm_app_wakeup_done(APP_MQTT_SUB);
		dpm_app_data_rcv_ready_set(APP_MQTT_SUB);
	}

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());

    da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_MQTT_Sub());

	rc = mosquitto_loop_forever(mosq_sub, -1, 1);

    da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_MQTT_Sub());

	MQTT_DBG_PRINT(" [%s] mosquitto_loop_forever exited (rc=%d, sock=%d, errno=%d, runContinueSub=%d) \n", 
				__func__, rc, mosq_sub->sock, errno, runContinueSub);

	// increment as soon as loop exit to prevent mqtt_client_check_keep_alive() from setting sleep
	subRetryCount++;

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	if (!mq_msg_tbl_presvd_is_enabled()) {
		if (mosq_sub->inflight_messages > 0 && mosq_sub->out_messages != 0)
			atcmd_asynchony_event_for_mqttmsgpub(0, MOSQ_ERR_CONN_LOST);
	}

	mq_msg_tbl_presvd_sync();

	mqtt_connect_ind_sent_set(FALSE);

	/* For disconnection by receiving a message exceeding max payload length 
	   when clean_session=0 is enabled */
	if (rc == MOSQ_ERR_PAYLOAD_SIZE && mq_msg_tbl_presvd_is_enabled()) {
		set_disconn_by_too_long_msg_rx_in_cs0(TRUE);
		
		/* 
			immediate stop. No re-connect should happen because otherwise 
			Broker will keep sending a wrong message
		*/
		subRetryCount = MQTT_CONN_MAX_RETRY;
		mqttParams.sub_connect_retry_count = MQTT_CONN_MAX_RETRY;
	}
	
#if defined (__MQTT_EMUL_CMD__)
	extern void cmd_mq_msg_tbl_test_mq_emul_param_clr(void);
	cmd_mq_msg_tbl_test_mq_emul_param_clr();
#endif // __MQTT_EMUL_CMD__

#else
	if (mosq_sub->inflight_messages > 0 && mosq_sub->out_messages != 0)
		atcmd_asynchony_event_for_mqttmsgpub(0, MOSQ_ERR_CONN_LOST);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	if (!is_disconn_by_too_long_msg_rx_in_cs0())
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
		{atcmd_asynchony_event(4, "0");}

	if (runContinueSub == false)
	{
		if (dpm_mode_is_enabled())
		{
            mqtt_client_dpm_port_filter_clear();            
			mqtt_client_generate_port();
		}
		goto exit;
	}

	// decrement for the original count value
	if (subRetryCount != MQTT_CONN_MAX_RETRY) {
	    subRetryCount--;
    }

end_of_func:

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());

	subPingStatus = PING_STS_NONE;

	if (dpm_mode_is_enabled())
	{
        mqtt_client_dpm_port_filter_clear();
		mqtt_client_generate_port();
	}

	if (runContinueSub == false)
	{
		mosquitto_pingreq_timer_unregister(MQTT_SUB_PERIOD_TIMER_ID);
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);

		goto exit;
	}

	if (dpm_mode_is_enabled())
	{
		mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);

		mosquitto_pingreq_timer_unregister(MQTT_SUB_PERIOD_TIMER_ID);

#if defined	(__MQTT_SELF_RECONNECT__)
		if ((++subRetryCount <= MQTT_RESTART_MAX_RETRY) && 
			!mqttParams.sub_connect_retry_count)
		{
			if (chk_network_ready(WLAN0_IFACE) != 1)
			{
				MQTT_DBG_ERR(" Wi-Fi Disconn -> Wait for Reconn ...\n");

                da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_MQTT_Sub());

				while (1)
				{
					if (chk_network_ready(WLAN0_IFACE) == 1)
					{
						break;
					}
					
                	vTaskDelay(100);
            	}

                da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_MQTT_Sub());
			}

			MQTT_DBG_ERR(" [SUB] Request mqtt_restart (cnt=%d)\n", subRetryCount);
		}
		else
		{
    		BaseType_t	ret_tmp;
			if (++mqttParams.sub_connect_retry_count <= MQTT_CONN_MAX_RETRY)
			{
				rtc_register_timer(MQTT_CONN_RETRY_PERIOD * 1000, APP_MQTT_SUB, 0, 0, sub_dummy_function); //msec
			}
			else
			{
				if (mqttParams.sub_connect_retry_count - 1)
				{
					MQTT_DBG_ERR(RED_COLOR " [SUB] MAX Retry (retry Count : %d). \n" CLEAR_COLOR,
								 mqttParams.sub_connect_retry_count - 1);
				}

				mqttParams.sub_connect_retry_count = 1;
			}

            // Before going to exit, run a thread to monitor power-down max trial failure
            ret_tmp = xTaskCreate(mqtt_client_restart_monitor,
                                  "mq_starter",
                                  (1024/4),
                                  NULL,
                                  (OS_TASK_PRIORITY_USER + 6),
                                  &mqtt_dpm_mon);
            if (ret_tmp != pdPASS) {
                MQTT_DBG_ERR(RED_COLOR "Failed to create mq_starter thread(0x%02x)\n" CLEAR_COLOR, ret_tmp);
            }
            
			goto exit;
		}
#else
		goto exit;
#endif // (__MQTT_SELF_RECONNECT__)
	}
	else
	{
		mqtt_client_generate_port();
#if defined	(__MQTT_SELF_RECONNECT__)
		if (mosq_sub->state != mosq_cs_disconnecting)
		{
			if (++subRetryCount < MQTT_CONN_MAX_RETRY)
			{
				MQTT_DBG_ERR(" [SUB] REQ mqtt_restart (count=%d)\n", subRetryCount);
			}
			else
			{
				MQTT_DBG_ERR(RED_COLOR "[SUB] MAX Retry (Retry Cnt=%d). \n" CLEAR_COLOR, 
							MQTT_CONN_MAX_RETRY);
				subRetryCount = 0;
				goto exit;
			}
		}
		else
		{
			goto exit;
		}
#else
		goto exit;
#endif	// (__MQTT_SELF_RECONNECT__)
	}

	if (runContinueSub == false)
	{
		goto exit;
	}

	mosquitto_disconnect(mosq_sub);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	mq_msg_tbl_presvd_sync();
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	mosquitto_destroy(mosq_sub);
	mosq_sub = NULL;

    da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_MQTT_Sub());

	vTaskDelay(300);

    da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_MQTT_Sub());

	goto restart;

exit:
	MQTT_DBG_PRINT(YELLOW_COLOR"[mqtt_client] terminated \r\n"CLEAR_COLOR);
	
	mosquitto_disconnect(mosq_sub);

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	if (is_disconn_by_too_long_msg_rx_in_cs0() || mqtt_tcp_tls_conn_fail == 1) {

        if (is_disconn_by_too_long_msg_rx_in_cs0()) {
    		ut_mqtt_sem_take_recur(&(mosq_sub->callback_mutex), portMAX_DELAY);
    		if (mosq_sub->on_disconnect2) {
    			mosq_sub->in_callback = true;
    			mosq_sub->on_disconnect2();
    			mosq_sub->in_callback = false;
    		}
    		xSemaphoreGiveRecursive(mosq_sub->callback_mutex.mutex_info);
        }

		atcmd_asynchony_event(4, "0");
	}
#else
    if (mqtt_tcp_tls_conn_fail == 1) {
        atcmd_asynchony_event(4, "0");
    }
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

    mqtt_tcp_tls_conn_fail = 0;

	mosquitto_destroy(mosq_sub);
	mosq_sub = NULL;

	client_config_cleanup(&mqtt_cfg);
	subRetryCount = 0;
	memset(&mqtt_cfg, 0x00, sizeof(struct mosq_config));

	da16x_secure_module_deinit(); 	//deinit cc312

	mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);

	if (dpm_mode_is_enabled())
	{
		dpm_app_unregister(APP_MQTT_SUB);
	}

	return rc;
}

void mqtt_client_thread_status_cleanup(void)
{
	mqtt_sub_thd_config.thread = NULL;
	runContinueSub = false;
	force_mqtt_stop = false;
}

static void mqtt_subscriber(void* arg)
{
    int sys_wdog_id = -1;

	DA16X_UNUSED_ARG(arg);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
    if (sys_wdog_id >= 0) {
        da16x_sys_wdog_id_set_MQTT_Sub(sys_wdog_id);
    }

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());

	mqtt_subscriber_main();

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_MQTT_Sub());
	
	mqtt_client_thread_status_cleanup();

    da16x_sys_watchdog_unregister(sys_wdog_id);

    da16x_sys_wdog_id_set_MQTT_Sub(DA16X_SYS_WDOG_ID_DEF_ID);

	vTaskDelete(NULL);
}

int mqtt_client_start(void)
{
	BaseType_t	xRet;
	
	mqtt_client_thread *mqtt_client_thd = &mqtt_sub_thd_config;

	if (mqtt_client_thd->thread)
	{
		MQTT_DBG_TEMP("mqtt_sub_thd_config->thread is running\n");
		return -1;
	}

	ut_mqtt_sem_create(&mqtt_conf_mutex);
	ut_mqtt_sem_create(&connect_mutex);
	
	xRet = xTaskCreate(mqtt_subscriber,
						MQTT_SUB_THREAD_NAME,
						(MQTT_CLIENT_THREAD_STACK_SIZE/4),
						NULL,
						(OS_TASK_PRIORITY_USER + 6),
						&mqtt_client_thd->thread);
	if (xRet != pdPASS)
	{
		MQTT_DBG_ERR(RED_COLOR "Failed to create MQTTC thread(0x%02x)\n" CLEAR_COLOR, xRet);
		goto error;
	}

	return 0;
	
error:
	mqtt_client_thread_status_cleanup();
	mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);

	return -1;
}

int mqtt_client_start_sub(void)
{
	UINT status;

	status = mqtt_client_start();

	return status;
}

int mqtt_client_stop(void)
{
	runContinueSub = false;

	if (mqtt_sub_thd_config.thread != NULL)
	{
	
		while(force_mqtt_stop == true) {
			vTaskDelay(50);
		}

		mosquitto_pingreq_timer_unregister(MQTT_SUB_PERIOD_TIMER_ID);
		
		_mosquitto_set_broker_conn_state(pdFALSE);

		return 0;
	}
	else
	{
		MQTT_DBG_INFO("[%s] No thread %s.\n", __func__, MQTT_SUB_THREAD_NAME);
		return -1;
	}
}

void mqtt_client_force_stop(void)
{
	force_mqtt_stop = true;
}

UINT8	mqtt_client_is_running(void)
{
	return runContinueSub;
}


int mqtt_client_stop_sub(void)
{
	int status;

	status = mqtt_client_stop();

	return status;
}

#if defined	(__MQTT_SELF_RECONNECT__)
static void sub_dummy_function(char *timer_name)
{
	DA16X_UNUSED_ARG(timer_name);

	MQTT_DBG_INFO("[%s] Called!!! \n", __func__);
}
#endif // (__MQTT_SELF_RECONNECT__)

int mqtt_client_send_message(char *top, char *publish)
{
	int ret = MOSQ_ERR_SUCCESS;

	if (!mosq_sub || mosq_sub->state != mosq_cs_connected)
	{
		MQTT_DBG_ERR(RED_COLOR "No MQTT_Sub session (Msg give-up)\n" CLEAR_COLOR);
		return -1;
	}

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	if (mq_msg_tbl_presvd_is_enabled() && 
			strlen(publish) > MQTT_MSG_TBL_PRESVD_MAX_PLAYLOAD_LEN) {
		MQTT_DBG_ERR(RED_COLOR "[clean_session=0 && qos>0] msg len should be <= %d\n" CLEAR_COLOR,
						MQTT_MSG_TBL_PRESVD_MAX_PLAYLOAD_LEN);
		return MOSQ_ERR_PAYLOAD_SIZE;
	}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	
	if (mosq_sub->inflight_messages > 0)
	{
		MQTT_DBG_ERR(RED_COLOR "Previous MSG is in-flight state\n" CLEAR_COLOR);
		return -2;
	}

	ut_mqtt_sem_take_recur(&(mosq_sub->send_msg_mutex), portMAX_DELAY);
	if (!dpm_mode_is_enabled())
		ut_mqtt_sem_take_recur(&(mosq_sub->callback_mutex), portMAX_DELAY);

	// allocate topic
	if (top != NULL)
	{
		if (mpub_topic)
		{
			_mosquitto_free(mpub_topic);
			mpub_topic = NULL;
		}

		mpub_topic = _mosquitto_strdup(top);
	}
	else
	{
		if (mqtt_cfg.topic == NULL || strlen(mqtt_cfg.topic) == 0)
		{
			MQTT_DBG_ERR(RED_COLOR "No topic to publish\n" CLEAR_COLOR);
			if (dpm_mode_is_enabled() == FALSE)
			{
				xSemaphoreGiveRecursive(mosq_sub->callback_mutex.mutex_info);

			}
			xSemaphoreGiveRecursive(mosq_sub->send_msg_mutex.mutex_info);
			return -3;
		}
	}

	mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);

	// allocate message
	if (mpub_message_buf)
	{
		_mosquitto_free(mpub_message_buf);
		mpub_message_buf = NULL;
	}

	mpub_message_buf = _mosquitto_strdup(publish);

	if (!dpm_mode_is_enabled()) {
		xSemaphoreGiveRecursive(mosq_sub->callback_mutex.mutex_info);
	}
	
	// In DPM mode only, mqtt_client_send_message() send a PUBLISH directly.
	if (dpm_mode_is_enabled()) {
		ret = my_publish(mosq_sub, NULL, 0);
	}
	
	xSemaphoreGiveRecursive(mosq_sub->send_msg_mutex.mutex_info);

	if (qos == 0)
	{
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
	}

	return ret;
}

int mqtt_client_send_message_with_qos(char *top, char *publish, ULONG timeout)
{
	extern struct mosquitto	*mosq_sub;
	int status;

	status = mqtt_pub_send_msg(top, publish);

	if (!status && qos >= 1)
	{
		while (timeout--)
		{

			if (mosq_sub == NULL)
			{
				return -3;	/* sub stop */
			}

			if (!mosq_sub->inflight_messages)
			{
				return 0;
			}

			vTaskDelay(10);
		}

		return -2;			/* timeout */
	}
	else if (!status && qos == 0)
	{
		return 0;
	}
	else
	{
		return -1;		/* error */
	}
}

int mqtt_client_check_conn(void)
{
	if (!mosq_sub ||  mosq_sub->state != mosq_cs_connected || mosq_sub->unsub_topic != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int mqtt_client_check_sub_conn(void)
{
	UINT status;

	status = mqtt_client_check_conn();

	return status;
}

#endif // (__SUPPORT_MQTT__)

/* EOF */
