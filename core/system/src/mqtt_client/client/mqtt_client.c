/**
 *****************************************************************************************
 * @file     mqtt_client.c
 * @brief    MQTT Client (both Subscriber and Publisher) main controller
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

#if defined (__SUPPORT_MQTT__)

#include "mqtt_client.h"
#include "mosquitto_internal.h"
#include "memory_mosq.h"
#include "da16200_map.h"
#include "da16x_dns_client.h"
#include "da16x_sntp_client.h"
#include "da16x_network_main.h"
#include "da16x_dhcp_client.h"
#include "da16x_image.h"
#include "da16x_network_main.h"
#include "da16x_cert.h"
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
#include "mqtt_msg_tbl_presvd.h"
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
#include "util_func.h"

extern bool confirm_dpm_init_done(char *);
extern unsigned long da16x_get_wakeup_source(void);
extern void _mosquitto_set_broker_conn_state(UINT8 state);
extern int is_netif_down_ever_happpened(void);
extern void atcmd_asynchony_event(int index, char *buf);

extern struct mosquitto	*mosq_sub;

extern void (*mqtt_conn_user_cb)(void);
extern void (*mqtt_disconn_user_cb)(void);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
extern void (*mqtt_disconn2_user_cb)(void);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
extern void (*mqtt_msg_user_cb)(const char* buf, int len, const char *topic);
extern void (*mqtt_pub_user_cb)(int);
extern void (*mqtt_subscribe_user_cb)(void);
extern void (*mqtt_unsubscribe_user_cb)(void);

struct mosq_config mqtt_cfg;
int cfg_status = 0;
int new_local_port_allocated = FALSE;

UINT mqttThreadStatus = 0;
mqttParamForRtm *rtmMqttParamPtr = NULL;
mqttParamForRtm mqttParams;

mqtt_mutex_t mqtt_conf_mutex;
mqtt_mutex_t connect_mutex;

static UINT	dummyCount1 = 0;

void	id_number_output(char *id_num);
void mqtt_client_save_to_dpm_user_mem(void);
static int	mqtt_client_wait_stopping(void);

void mqtt_client_user_cb_set(void)
{
	if (mqtt_conn_user_cb) {
		MQTT_DBG_INFO("[%s] Set cb_fn(mqtt_conn_user_cb)\n", __func__);
		mosquitto_connect_callback_set(mosq_sub, mqtt_conn_user_cb);
	}

	if (mqtt_disconn_user_cb) {
		MQTT_DBG_INFO(" [%s] Set callback func(mqtt_disconn_user_cb)\n", __func__);
		mosquitto_disconnect_callback_set(mosq_sub, mqtt_disconn_user_cb);
	}
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	if (mqtt_disconn2_user_cb) {
		MQTT_DBG_INFO(" [%s] Set callback func(mqtt_disconn2_user_cb)\n", __func__);
		mosquitto_disconnect2_callback_set(mosq_sub, mqtt_disconn2_user_cb);
	}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	if (mqtt_msg_user_cb) {
		MQTT_DBG_INFO("[%s] Set cb_fn(mqtt_msg_user_cb)\n", __func__);
		mosquitto_message_callback_set(mosq_sub, mqtt_msg_user_cb);
	}

	if (mqtt_pub_user_cb) {
		MQTT_DBG_INFO("[%s] Set cb_fn (mqtt_pub_user_cb)\n", __func__);
		mosquitto_publish_callback_set(mosq_sub, mqtt_pub_user_cb);
	}	

	if (mqtt_subscribe_user_cb) {
		MQTT_DBG_INFO("[%s] Set cb_fn (mqtt_subscribe_user_cb)\n", __func__);
		mosquitto_subscribe_callback_set(mosq_sub, mqtt_subscribe_user_cb);
	}	

	if (mqtt_unsubscribe_user_cb) {
		MQTT_DBG_INFO("[%s] Set cb_fn (mqtt_unsubscribe_user_cb)\n", __func__);
		mosquitto_unsubscribe_callback_set(mosq_sub, mqtt_unsubscribe_user_cb);
	}
}

void mqtt_client_generate_port(void)
{
	UINT port;

generate_port:
	++dummyCount1;
	
	port = 30000 + ((rand() * xTaskGetTickCount()) % 10000) + dummyCount1;	
	
	if (dummyCount1 > 25000)
	{
		dummyCount1 = 0;
		goto generate_port;
	}

	MQTT_DBG_INFO("[%s] Generated port: %d \n", __func__, port);

	mqttParams.my_sub_port_no = port;
	new_local_port_allocated = TRUE;
	return;
}

#if defined (__TEST_SET_SLP_DELAY__)
int temp_val_wait;
#endif

void mqtt_client_dpm_regi_with_port(void)
{
	if (chk_dpm_reg_state(APP_MQTT_SUB) == DPM_NOT_REGISTERED)
	{
		dpm_app_register(APP_MQTT_SUB, mqttParams.my_sub_port_no);
	}
	else
	{
		dpm_app_unregister(APP_MQTT_SUB);
		dpm_app_register(APP_MQTT_SUB, mqttParams.my_sub_port_no);
	}
	
	MQTT_DBG_TEMP("mqtt_client dpm regi with %d \n", mqttParams.my_sub_port_no);
}

int mqtt_client_dpm_sleep(char *dpm_name, char const *func_name, int line)
{
	DA16X_UNUSED_ARG(func_name);
	DA16X_UNUSED_ARG(line);

	int status;

	if (!dpm_mode_is_enabled())
	{
		return DPM_SET_ERR;
	}

	mqtt_client_save_to_dpm_user_mem();

	MQTT_DBG_INFO(GREEN_COLOR " [%s] SET DPM SLEEP(%s) by %s:%d \n" CLEAR_COLOR,
				  __func__, dpm_name, func_name, line);
#if defined (__TEST_SET_SLP_DELAY__)
	if (temp_val_wait == 0) {
	} else {
		vTaskDelay(temp_val_wait);
	}
#endif

	status = dpm_app_sleep_ready_set(dpm_name);

	return status;
}

int mqtt_client_dpm_clear(char *dpm_name, char const *func_name, int line)
{
	DA16X_UNUSED_ARG(func_name);
	DA16X_UNUSED_ARG(line);

	int status;

	if (!dpm_mode_is_enabled())
	{
		return DPM_SET_ERR;
	}

	MQTT_DBG_INFO(GREEN_COLOR " [%s] CLR DPM SLEEP(%s) by %s:%d \n" CLEAR_COLOR,
				  __func__, dpm_name, func_name, line);
	status = dpm_app_sleep_ready_clear(dpm_name);

	return status;
}

extern char *dns_A_Query(char *domain_name, ULONG wait_option);

static int mqtt_client_check_broker_addr(void)
{
	char		*ip = NULL;
	int			retryCount = 0;

	if (strlen(mqttParams.host))
	{
		if (is_in_valid_ip_class(mqttParams.host))   /* check ip */
		{
			_mosquitto_free(mqtt_cfg.host);
			mqtt_cfg.host = _mosquitto_strdup(mqttParams.host);
		}
		else
		{
			if (strlen(mqttParams.host_ip))
			{
				if (is_in_valid_ip_class(mqttParams.host_ip))   /* check ip */
				{
					_mosquitto_free(mqtt_cfg.host);
					mqtt_cfg.host = _mosquitto_strdup(mqttParams.host_ip);
					MQTT_DBG_INFO(" [%s] Set Host ip configure %s \n", __func__, mqttParams.host_ip);
					return pdTRUE;
				}
			}

retryQuery:
			ip = dns_A_Query(mqttParams.host, 400);
			if (ip == NULL)
			{
				MQTT_DBG_ERR(RED_COLOR "DNS query failed.\n" CLEAR_COLOR);

				if (++retryCount <= 5)
				{
					vTaskDelay(100);
					goto retryQuery;
				}

				return pdFALSE;
			}

			_mosquitto_free(mqtt_cfg.host);
			mqtt_cfg.host = _mosquitto_strdup(ip);

			strcpy(mqttParams.host_ip, ip);
			MQTT_DBG_INFO(" [%s] Set Host ip configure %s->%s .\n", __func__, mqttParams.host, ip);
		}
	}
	else
	{
		MQTT_DBG_ERR(RED_COLOR "No Broker configure\n" CLEAR_COLOR);
		return pdFALSE;
	}

	return pdTRUE;
}

static UINT mqtt_client_assign_dpm_user_mem(void)
{
	UINT	len;
	UINT	status;
	int	sysmode;
	const ULONG wait_option = 100;


	if (!dpm_mode_is_wakeup())
	{
		sysmode = get_run_mode();
	}
	else
	{
		/* !!! Caution !!!
		 * Don't do NVRAM READ when dpm_wakeup.
		 */
		sysmode = SYSMODE_STA_ONLY;
	}

	if (sysmode != SYSMODE_STA_ONLY)
	{
		return pdFALSE;
	}

	len = dpm_user_mem_get(MQTT_PARAM_NAME, (UCHAR **)&rtmMqttParamPtr);
	if (len == 0)
	{
		status = dpm_user_mem_alloc(MQTT_PARAM_NAME, (VOID **)&rtmMqttParamPtr,
									sizeof(mqttParamForRtm), wait_option);
		if (status != ERR_OK)
		{
			MQTT_DBG_ERR(RED_COLOR "Failed to allocate rtm(0x%02x)\n" CLEAR_COLOR, status);
			return  pdFALSE;
		}
		
		memset(rtmMqttParamPtr, 0, sizeof(mqttParamForRtm));
		MQTT_DBG_INFO(" [%s] after RTM alloc, status=%d, rtmMqttParamPtr=%p\n",
					  __func__, status, rtmMqttParamPtr);
	}
	else
	{
		if (len != sizeof(mqttParamForRtm))
		{
			MQTT_DBG_ERR(RED_COLOR "Invalid alloc size ...\n" CLEAR_COLOR);
			return pdFALSE;
		}
	}

	return pdTRUE;
}

void mqtt_client_save_to_dpm_user_mem(void)
{
	int	sysmode;

	if (!dpm_mode_is_wakeup())
	{
		sysmode = get_run_mode();
	}
	else
	{
		/* !!! Caution !!!
		 * Do not read operation from NVRAM when dpm_wakeup.
		 */
		sysmode = SYSMODE_STA_ONLY;
	}

	if (sysmode != SYSMODE_STA_ONLY)
	{
		return;
	}

	if (rtmMqttParamPtr == NULL)
	{
		return;
	}

	ut_mqtt_sem_take_recur(&mqtt_conf_mutex, portMAX_DELAY);
	
	memcpy(rtmMqttParamPtr, &mqttParams, sizeof(mqttParamForRtm));
	xSemaphoreGiveRecursive(mqtt_conf_mutex.mutex_info);
}

int mqtt_client_is_cfg_dpm_mem_intact(void)
{
	return (rtmMqttParamPtr != NULL && rtmMqttParamPtr->mqtt_auto == MQTT_INIT_MAGIC);
}

int mqtt_client_cfg_sync_rtm(int cfg_str_param, char* str_val, 
									   int cfg_int_param, int num_val)
{
	int res = 0;
	
	if (mqtt_client_is_cfg_dpm_mem_intact()) {
		if (str_val) {
			switch (cfg_str_param) {
				case DA16X_CONF_STR_MQTT_BROKER_IP:
					strcpy(mqttParams.host, str_val);
					break;
				case DA16X_CONF_STR_MQTT_PUB_TOPIC:
					strcpy(mqttParams.topic, str_val);
					break;
				case DA16X_CONF_STR_MQTT_SUB_CLIENT_ID:
					strcpy(mqttParams.sub_client_id, str_val);
					break;
				case DA16X_CONF_STR_MQTT_USERNAME:
					strcpy(mqttParams.username, str_val);
					break;
				case DA16X_CONF_STR_MQTT_PASSWORD:
					strcpy(mqttParams.password, str_val);
					break;
				case DA16X_CONF_STR_MQTT_WILL_TOPIC:
					strcpy(mqttParams.will_topic, str_val);
					break;
				case DA16X_CONF_STR_MQTT_WILL_MSG:
					strcpy(mqttParams.will_payload, str_val);
					break;
				default:
					return -1;
			}
		} else {
		
			switch (cfg_int_param) {
				case DA16X_CONF_INT_MQTT_PORT:
					mqttParams.port = num_val;
					break;
				case DA16X_CONF_INT_MQTT_QOS:
					mqttParams.qos = num_val;
					break;
				case DA16X_CONF_INT_MQTT_TLS:
					mqttParams.insecure = num_val;
					break;
				case DA16X_CONF_INT_MQTT_PING_PERIOD:
					mqttParams.keepalive = num_val;
					break;
				case DA16X_CONF_INT_MQTT_VER311:
					mqttParams.protocol_version = num_val;
					break;
				case DA16X_CONF_INT_MQTT_WILL_QOS:
					mqttParams.will_qos = num_val;
					break;
				case DA16X_CONF_INT_MQTT_AUTO:
					mqttParams.mqtt_auto = num_val;
					break;
				case DA16X_CONF_INT_MQTT_CLEAN_SESSION:
					mqttParams.clean_session = num_val;
					break;
				default:
					return -1;
			}
			
		}
		mqtt_client_save_to_dpm_user_mem();
	} else {
		res = -1;
	}

	return res;
}

int mqtt_client_cert_read(void)
{
    int ret = pdFAIL;
	HANDLE flash_handler;

    int cert_err = DA16X_CERT_ERR_OK;
    int format = DA16X_CERT_FORMAT_NONE;

	flash_handler = flash_image_open((sizeof(UINT32) * 8),
									 (UINT32)SFLASH_ROOT_CA_ADDR1,
									 (VOID *)&da16x_environ_lock);
	if (flash_handler == NULL)
	{
		PRINTF("[%s:%d] Failed to access memory\n", __func__, __LINE__);
		return pdFAIL;
	}


    //Read ca certificate
    mqtt_cfg.cacert_len = mqtt_cfg.cacert_buflen;

    cert_err = da16x_cert_read_no_fopen(flash_handler,
                                        DA16X_CERT_MODULE_MQTT,
                                        DA16X_CERT_TYPE_CA_CERT,
                                        &format,
                                        (unsigned char *)mqtt_cfg.cacert_ptr,
                                        &mqtt_cfg.cacert_len);
    if (cert_err == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        mqtt_cfg.cacert_len = 0;
    } else if (cert_err != DA16X_CERT_ERR_OK) {
		PRINTF("[%s:%d] CA Read error(%d)\n", __func__, __LINE__, cert_err);
        goto end;
    }

    //Read certificate
    mqtt_cfg.cert_len = mqtt_cfg.cert_buflen;

    cert_err = da16x_cert_read_no_fopen(flash_handler,
                                        DA16X_CERT_MODULE_MQTT,
                                        DA16X_CERT_TYPE_CERT,
                                        &format,
                                        (unsigned char *)mqtt_cfg.cert_ptr,
                                        &mqtt_cfg.cert_len);
    if (cert_err == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        mqtt_cfg.cert_len = 0;
    } else if (cert_err != DA16X_CERT_ERR_OK) {
		PRINTF("[%s:%d] CERT Read error(%d)\n", __func__, __LINE__, cert_err);
        goto end;
    }

    //Read private key
    mqtt_cfg.priv_key_len = mqtt_cfg.priv_key_buflen;

    cert_err = da16x_cert_read_no_fopen(flash_handler,
                                        DA16X_CERT_MODULE_MQTT,
                                        DA16X_CERT_TYPE_PRIVATE_KEY,
                                        &format,
                                        (unsigned char *)mqtt_cfg.priv_key_ptr,
                                        &mqtt_cfg.priv_key_len);
    if (cert_err == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        mqtt_cfg.priv_key_len = 0;
    } else if (cert_err != DA16X_CERT_ERR_OK) {
		PRINTF("[%s:%d] CERT Read error(%d)\n", __func__, __LINE__, cert_err);
        goto end;
    }

    //Read dh param
    mqtt_cfg.dh_param_len = mqtt_cfg.dh_param_buflen;

    cert_err = da16x_cert_read_no_fopen(flash_handler,
                                        DA16X_CERT_MODULE_MQTT,
                                        DA16X_CERT_TYPE_DH_PARAMS,
                                        &format,
                                        (unsigned char *)mqtt_cfg.dh_param_ptr,
                                        &mqtt_cfg.dh_param_len);
    if (cert_err == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        mqtt_cfg.dh_param_len = 0;
    } else if (cert_err != DA16X_CERT_ERR_OK) {
		PRINTF("[%s:%d] CERT Read error(%d)\n", __func__, __LINE__, cert_err);
        goto end;
    }
	
    ret = pdPASS;

end:
	
    if (flash_handler) {
	    flash_image_close(flash_handler);
        flash_handler = NULL;
    }
	
	return ret;
}
int mqtt_client_save_to_cfg(void)
{
	char tempId[MQTT_DEVICE_ID_MAX_LEN + 1 + 8];
	char id_num[5] = {0, };

	if (mqtt_cfg.cacert_ptr)
	{
		memset(mqtt_cfg.cacert_ptr, 0, mqtt_cfg.cacert_buflen);
        mqtt_cfg.cacert_len = 0;
	}
	if (mqtt_cfg.cert_ptr)
	{
		memset(mqtt_cfg.cert_ptr, 0, mqtt_cfg.cert_buflen);
        mqtt_cfg.cert_len = 0;
	}
	if (mqtt_cfg.priv_key_ptr)
	{
		memset(mqtt_cfg.priv_key_ptr, 0, mqtt_cfg.priv_key_buflen);
        mqtt_cfg.priv_key_len = 0;
	}
    if (mqtt_cfg.dh_param_ptr)
    {
		memset(mqtt_cfg.dh_param_ptr, 0, mqtt_cfg.dh_param_buflen);
        mqtt_cfg.dh_param_len = 0;
    }
	
	if (mqtt_client_check_broker_addr() != pdTRUE)
	{
		return pdFALSE;
	}

	mqtt_cfg.port	= mqttParams.port;
	mqtt_cfg.qos    = mqttParams.qos;

	if (strlen(mqttParams.will_topic))
	{
		_mosquitto_free(mqtt_cfg.will_topic);
		mqtt_cfg.will_topic = _mosquitto_strdup(mqttParams.will_topic);
	}

	if (strlen(mqttParams.will_payload))
	{
		_mosquitto_free(mqtt_cfg.will_payload);
		mqtt_cfg.will_payload	= _mosquitto_strdup(mqttParams.will_payload);
		mqtt_cfg.will_payloadlen	= strlen(mqtt_cfg.will_payload);
	}

	mqtt_cfg.will_qos			= mqttParams.will_qos;
	mqtt_cfg.keepalive			= mqttParams.keepalive;
	mqtt_cfg.max_inflight		= mqttParams.max_inflight;
	mqtt_cfg.protocol_version	= mqttParams.protocol_version;
	mqtt_cfg.insecure			= mqttParams.insecure;

	if (mqtt_cfg.insecure == 1)
	{
		if (mqtt_cfg.cacert_ptr == NULL)
		{
            mqtt_cfg.cacert_buflen = CERT_MAX_LENGTH;
            mqtt_cfg.cacert_len = 0;
			mqtt_cfg.cacert_ptr = _mosquitto_malloc(mqtt_cfg.cacert_buflen);
		}
		if (mqtt_cfg.cert_ptr == NULL)
		{
            mqtt_cfg.cert_buflen = CERT_MAX_LENGTH;
            mqtt_cfg.cert_len = 0;
			mqtt_cfg.cert_ptr = _mosquitto_malloc(mqtt_cfg.cert_buflen);
		}
		if (mqtt_cfg.priv_key_ptr == NULL)
		{
            mqtt_cfg.priv_key_buflen = CERT_MAX_LENGTH;
            mqtt_cfg.priv_key_len = 0;
			mqtt_cfg.priv_key_ptr = _mosquitto_malloc(mqtt_cfg.priv_key_buflen);
		}
        if (mqtt_cfg.dh_param_ptr == NULL)
        {
            mqtt_cfg.dh_param_buflen = CERT_MAX_LENGTH;
            mqtt_cfg.dh_param_len = 0;
			mqtt_cfg.dh_param_ptr = _mosquitto_malloc(mqtt_cfg.dh_param_buflen);
        }
		
		// read certificates
		if (mqtt_client_cert_read() != pdPASS)
        {
            return pdFALSE;
        }

        if (mqtt_cfg.cacert_len == 0)
        {
			_mosquitto_free(mqtt_cfg.cacert_ptr);
			mqtt_cfg.cacert_ptr = NULL;
            mqtt_cfg.cacert_buflen = 0;
            mqtt_cfg.cacert_len = 0;
        }

        if (mqtt_cfg.cert_len == 0)
        {
			_mosquitto_free(mqtt_cfg.cert_ptr);
			mqtt_cfg.cert_ptr = NULL;
            mqtt_cfg.cert_buflen = 0;
            mqtt_cfg.cert_len = 0;
        }

        if (mqtt_cfg.priv_key_len== 0)
        {
			_mosquitto_free(mqtt_cfg.priv_key_ptr);
			mqtt_cfg.priv_key_ptr = NULL;
            mqtt_cfg.priv_key_buflen = 0;
            mqtt_cfg.priv_key_len = 0;
        }

        if (mqtt_cfg.dh_param_len == 0)
        {
			_mosquitto_free(mqtt_cfg.dh_param_ptr);
			mqtt_cfg.dh_param_ptr = NULL;
            mqtt_cfg.dh_param_buflen = 0;
            mqtt_cfg.dh_param_len = 0;
        }

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
		if (mqttParams.tls_alpn_cnt)
		{
			int i;

			if (mqtt_cfg.alpn)
			{
				for (i = 0; i < mqtt_cfg.alpn_count; i++)
				{
					_mosquitto_free(mqtt_cfg.alpn[i]);
				}
			}
		
			for (i = 0; i < mqttParams.tls_alpn_cnt; i++)
			{
				char *tmp_str;
		
				tmp_str = mqttParams.tls_alpn[i];
				if (strlen(tmp_str))
				{
					mqtt_cfg.alpn = _mosquitto_realloc(mqtt_cfg.alpn, (i + 1) * sizeof(char *));
					mqtt_cfg.alpn[i]	= _mosquitto_strdup(tmp_str);
				}
			}
			mqtt_cfg.alpn = _mosquitto_realloc(mqtt_cfg.alpn, (i + 1) * sizeof(char *));
			mqtt_cfg.alpn[i] = NULL;
			mqtt_cfg.alpn_count = mqttParams.tls_alpn_cnt;
		}

		if (strlen(mqttParams.tls_sni))
		{
			_mosquitto_free(mqtt_cfg.sni);
			mqtt_cfg.sni = _mosquitto_strdup(mqttParams.tls_sni);
		}

		if (mqttParams.tls_csuits_cnt)
		{
			if (mqtt_cfg.cipher_suits)
			{
				_mosquitto_free(mqtt_cfg.cipher_suits);
			}

			mqtt_cfg.cipher_suits = _mosquitto_malloc(sizeof(int) * (mqttParams.tls_csuits_cnt + 1));
			memcpy(mqtt_cfg.cipher_suits, mqttParams.tls_csuits,
					sizeof(int) * mqttParams.tls_csuits_cnt);
			mqtt_cfg.cipher_suits[mqttParams.tls_csuits_cnt] = 0;
			mqtt_cfg.cipher_suits_count = mqttParams.tls_csuits_cnt;
		}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	}

	if (strlen(mqttParams.username))
	{
		_mosquitto_free(mqtt_cfg.username);
		mqtt_cfg.username = _mosquitto_strdup(mqttParams.username);
	}

	if (strlen(mqttParams.password))
	{
		_mosquitto_free(mqtt_cfg.password);
		mqtt_cfg.password = _mosquitto_strdup(mqttParams.password);
	}

	mqtt_cfg.clean_session = mqttParams.clean_session;

	if (strlen(mqttParams.sub_client_id))
	{
		_mosquitto_free(mqtt_cfg.sub_id);
		mqtt_cfg.sub_id = _mosquitto_strdup(mqttParams.sub_client_id);
	}
	else if (strlen(mqttParams.device_id))
	{
		memset(tempId, 0, MQTT_DEVICE_ID_MAX_LEN + 1 + 8);
		memset(id_num, 0, 5);

		id_number_output(id_num);
		sprintf(tempId, "%s_%s", mqttParams.device_id, id_num);
		_mosquitto_free(mqtt_cfg.sub_id);
		mqtt_cfg.sub_id = _mosquitto_strdup(tempId);
	}

	if (strlen(mqttParams.topic))
	{
		_mosquitto_free(mqtt_cfg.topic);
		mqtt_cfg.topic = _mosquitto_strdup(mqttParams.topic);
	}

	if (mqttParams.topic_count >= 1)
	{
		if (mqtt_cfg.topics)
		{
			for (int i = 0; i < mqtt_cfg.topic_count; i++)
			{
				_mosquitto_free(mqtt_cfg.topics[i]);
			}
		}

		for (int i = 0; i < mqttParams.topic_count; i++)
		{
			char *tmp_str;

			tmp_str = mqttParams.topics[i];

			if (strlen(tmp_str))
			{
				if (mosquitto_sub_topic_check(tmp_str) == MOSQ_ERR_INVAL)
				{
					MQTT_DBG_ERR(RED_COLOR "Error: Invalid topic (%s)\n" CLEAR_COLOR, tmp_str);
					return pdFALSE;
				}

				mqtt_cfg.topics		= _mosquitto_realloc(mqtt_cfg.topics, (i + 1) * sizeof(char *));
				mqtt_cfg.topics[i]	= _mosquitto_strdup(tmp_str);
			}
		}
	}

	mqtt_cfg.topic_count = mqttParams.topic_count;
	return pdTRUE;
}

static int mqtt_client_read_from_nvram(void)
{
	char ret_str[MQTT_PASSWORD_MAX_LEN] = {0, }; // password has the max length in str type
	int ret_num = 0;

	mqttParamForRtm *paramsPtr = &mqttParams;

	memset(paramsPtr, 0, sizeof(mqttParamForRtm));

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_AUTO, &ret_num))
	{
		paramsPtr->mqtt_auto = ret_num;
	}
	else
	{
		paramsPtr->mqtt_auto = MQTT_CONFIG_AUTO_DEF;
	}

	strcpy(paramsPtr->device_id, "da16x");

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_BROKER_IP, ret_str))
	{
		strcpy(paramsPtr->host, ret_str);
	}
	else
	{
		MQTT_DBG_ERR(RED_COLOR "DA16X_CONF_STR_MQTT_BROKER_IP read failed.\n" CLEAR_COLOR);
		return pdFALSE;
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_PORT, &ret_num))
	{
		paramsPtr->port = ret_num;
	}
	else
	{
		paramsPtr->port = MQTT_CONFIG_PORT_DEF;
	}

#if defined (__TEST_SET_SLP_DELAY__)
	extern int temp_val_wait;
	read_nvram_int("temp_val_wait", &ret_num);
	temp_val_wait = paramsPtr->temp_val_wait = ret_num;
#endif

	ret_num = -1;

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, ret_str))
	{
		if (mosquitto_pub_topic_check(ret_str) == MOSQ_ERR_INVAL)
		{
			MQTT_DBG_ERR(RED_COLOR "Invalid publish topic '%s', does it contain '+' or '#'?\n"
						 CLEAR_COLOR, ret_str);
			return pdFALSE;
		}

		strcpy(paramsPtr->topic, ret_str);
	}

	if ((!da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_TOPIC, ret_str) || 
		!read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &ret_num)))
	{
		if (ret_num == -1)
		{
			if (mosquitto_sub_topic_check(ret_str) == MOSQ_ERR_INVAL)
			{
				MQTT_DBG_ERR(RED_COLOR "Invalid subscription topic '%s', are all '+' and '#' wildcards correct?\n" CLEAR_COLOR,  ret_str);
				return pdFALSE;
			}

			paramsPtr->topic_count++;
			strcpy(paramsPtr->topics[paramsPtr->topic_count - 1], ret_str);
		}
		else if (ret_num >= 1)
		{
			for (int i = 0; i < ret_num; i++)
			{
				char topics[16] = {0, };
				char *tmp_str;
				sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
				tmp_str = read_nvram_string(topics);

				if (strlen(tmp_str))
				{
					if (mosquitto_sub_topic_check(tmp_str) == MOSQ_ERR_INVAL)
					{
						MQTT_DBG_ERR(RED_COLOR "Invalid subscription topic '%s', are all '+' and '#' wildcards correct?\n" CLEAR_COLOR, ret_str);
						return pdFALSE;
					}

					paramsPtr->topic_count++;
					strcpy(paramsPtr->topics[paramsPtr->topic_count - 1], tmp_str);
				}
				else
				{
					MQTT_DBG_ERR(RED_COLOR "Error: Check Subscriber topics!\n" CLEAR_COLOR);
					return pdFALSE;
				}

				if (paramsPtr->topic_count >= MQTT_MAX_TOPIC)
				{
					MQTT_DBG_INFO(RED_COLOR " [%s] Max topic...!\n" CLEAR_COLOR, __func__);
					break;
				}
			}
		}
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_QOS, &ret_num))
	{
		paramsPtr->qos = ret_num;
	}
	else
	{
		paramsPtr->qos = MQTT_CONFIG_QOS_DEF;
	}

	if (!read_nvram_int(MQTT_NVRAM_CONFIG_TLS, &ret_num))
	{
		paramsPtr->insecure = ret_num;
	}

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_USERNAME, ret_str))
	{
		strcpy(paramsPtr->username, ret_str);
	}

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_PASSWORD, ret_str))
	{
		strcpy(paramsPtr->password, ret_str);
	}

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_WILL_TOPIC, ret_str))
	{
		if (mosquitto_pub_topic_check(ret_str) == MOSQ_ERR_INVAL)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid will topic '%s', does it contain '+' or '#'?\n" CLEAR_COLOR, ret_str);
			return pdFALSE;
		}

		strcpy(paramsPtr->will_topic, ret_str);
	}

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_WILL_MSG, ret_str))
	{
		strcpy(paramsPtr->will_payload, ret_str);
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_WILL_QOS, &ret_num))
	{
		paramsPtr->will_qos = ret_num;
	}
	else
	{
		paramsPtr->will_qos = MQTT_CONFIG_QOS_DEF;
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_PING_PERIOD, &ret_num))
	{
		paramsPtr->keepalive = ret_num;
	}
	else
	{
		paramsPtr->keepalive = MQTT_CONFIG_PING_DEF;
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_CLEAN_SESSION, &ret_num))
	{
		paramsPtr->clean_session = ret_num;
	}
	else
	{
		paramsPtr->clean_session = MQTT_CONFIG_CLEAN_SESSION_DEF;
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_VER311, &ret_num))
	{
		paramsPtr->protocol_version = MQTT_PROTOCOL_V31 + ret_num;
	}
	else
	{
		paramsPtr->protocol_version = MQTT_PROTOCOL_V31 + MQTT_CONFIG_VER311_DEF;
	}
	
	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_NO_TIME_CHK, &ret_num) && ret_num == 1)
	{
		paramsPtr->tls_time_check_option = pdFALSE;
	}
	else
	{
		paramsPtr->tls_time_check_option = pdTRUE;
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_AUTHMODE, &ret_num))
	{
		paramsPtr->tls_authmode = ret_num;
	}
	else
	{
		paramsPtr->tls_authmode = MBEDTLS_SSL_VERIFY_OPTIONAL;	//Default authmode
	}

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, ret_str))
	{
		strcpy(paramsPtr->sub_client_id, ret_str);
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_INCOMING, &ret_num))
	{
		paramsPtr->tls_incoming_size = ret_num;
	}
	else
	{
		paramsPtr->tls_incoming_size = MQTT_CONFIG_TLS_INCOMING_DEF;
	}

	if (!da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_OUTGOING, &ret_num))
	{
		paramsPtr->tls_outgoing_size = ret_num;
	}
	else
	{
		paramsPtr->tls_outgoing_size = MQTT_CONFIG_TLS_OUTGOING_DEF;
	}
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	if (!read_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, &ret_num))
	{
		char nvr_name[15] = {0, };
		char *tmp_str;

		for (int i = 0; i < ret_num; i++)
		{
			/* Sanity check */
			if (i > MQTT_TLS_MAX_ALPN)
			{
				MQTT_DBG_INFO(RED_COLOR "[%s] Exceeded max alpn number!\n" CLEAR_COLOR, __func__);
				break;
			}

			memset(nvr_name, 0, 15);
			sprintf(nvr_name, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
			tmp_str = read_nvram_string(nvr_name);
			strcpy(paramsPtr->tls_alpn[i], tmp_str);
			paramsPtr->tls_alpn_cnt++;
		}
	}

	if (!da16x_get_config_str(DA16X_CONF_STR_MQTT_TLS_SNI, ret_str))
	{
		strcpy(paramsPtr->tls_sni, ret_str);
	}

	if (!read_nvram_int(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM, &ret_num))
	{
		char *tmp_str, *pstr, *poffset = NULL;
		int len;

		tmp_str = read_nvram_string(MQTT_NVRAM_CONFIG_TLS_CSUITS);
		len = strlen(tmp_str);
		pstr = _mosquitto_malloc(len + 1);
		if (pstr == NULL)
		{
			MQTT_DBG_ERR(RED_COLOR "Fail to allocate memory\n" CLEAR_COLOR);
			return pdFALSE;
		}
		memset(pstr, 0, len + 1);
		strncpy(pstr, tmp_str, len);
		poffset = strtok(pstr, ",");

		paramsPtr->tls_csuits_cnt = 0;
		while (poffset != NULL)
		{
			paramsPtr->tls_csuits[paramsPtr->tls_csuits_cnt] = htoi(poffset);
			paramsPtr->tls_csuits_cnt++;
			poffset = strtok(NULL, ",");
		}

		if (pstr)
		{
			_mosquitto_free(pstr);
		}
	}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	paramsPtr->max_inflight = 20;

	mqtt_client_generate_port();

	if (mqtt_client_save_to_cfg() != pdTRUE)
		return pdFALSE;

	if (dpm_mode_is_enabled())
	{
        if (rtmMqttParamPtr && rtmMqttParamPtr->my_sub_port_no_prev != 0) {
            paramsPtr->my_sub_port_no_prev = rtmMqttParamPtr->my_sub_port_no_prev;
        }
		
		mqtt_client_save_to_dpm_user_mem();
	}

	return pdTRUE;
}

static int mqtt_client_read_from_dpm_user_mem(void)
{
	if (mqtt_client_assign_dpm_user_mem() == pdFALSE)
	{
		MQTT_DBG_ERR(RED_COLOR "Rtm assign ERROR \n" CLEAR_COLOR);
		return pdFALSE;
	}

	if (rtmMqttParamPtr->mqtt_auto != MQTT_INIT_MAGIC)
	{
		// rtm not found, then rtmMqttParamPtr is newly allocated.
		return pdTRUE;
	}
	
	/* Restore mqttParams values */
	memcpy(&mqttParams, rtmMqttParamPtr, sizeof(mqttParamForRtm));

#if defined (__TEST_SET_SLP_DELAY__)
	extern int temp_val_wait;
	temp_val_wait = mqttParams.temp_val_wait;
#endif

	// Register only at DPM wakeup
	mqtt_client_dpm_regi_with_port();

	if (mqtt_client_save_to_cfg() != pdTRUE)
	{
		MQTT_DBG_ERR(RED_COLOR "Copy ERROR\n" CLEAR_COLOR);
		return pdFALSE;
	}

	return pdTRUE;
}

static int mqtt_client_check_broker_info(void)
{
	if (mqttParams.mqtt_auto == MQTT_INIT_MAGIC && 
		(mqttParams.topic_count == 0 || strlen(mqttParams.topic) == 0))
	{
		MQTT_DBG_ERR(" No MQTT Topic\n");
		return -1;
	}

	if (strlen(mqttParams.host_ip) == 0 && strlen(mqttParams.host) == 0)
	{
		MQTT_DBG_ERR(" No Broker Address\n");
		return -1;
	}

	return 0;
}

int mqtt_client_get_qos(void)
{
	return mqtt_cfg.qos;
}

int mqtt_client_initialize_config(void)
{
	int	result = pdPASS;

	if (cfg_status) return result;

	ut_mqtt_sem_take_recur(&mqtt_conf_mutex, portMAX_DELAY);

	memset(&mqttParams, 0, sizeof(mqttParamForRtm));

	if (dpm_mode_is_wakeup())
	{
		if (mqtt_client_read_from_dpm_user_mem() == pdFALSE)
		{
			xSemaphoreGiveRecursive(mqtt_conf_mutex.mutex_info);
			return pdFAIL;
		}

		if (mqttParams.mqtt_auto != MQTT_INIT_MAGIC)
		{
			MQTT_DBG_ERR(RED_COLOR "RTM is abnormal or newly allocated!\n" CLEAR_COLOR);
			result = mqtt_client_read_from_nvram();
		}
	}
	else
	{
		if (dpm_mode_is_enabled() && mqtt_client_assign_dpm_user_mem() == pdFALSE)
		{
			MQTT_DBG_ERR(RED_COLOR "Assign ERROR \n" CLEAR_COLOR, __func__);
			xSemaphoreGiveRecursive(mqtt_conf_mutex.mutex_info);
			return pdFALSE;
		}
		
		result = mqtt_client_read_from_nvram();
	}

	xSemaphoreGiveRecursive(mqtt_conf_mutex.mutex_info);

	if (mqtt_client_check_broker_info())
	{
		result = pdFALSE;
	}
	
	if (result == pdTRUE) cfg_status = 1;
	
	return result;
}

void client_config_cleanup(struct mosq_config *cfg_p)
{
	int i = 0;

	if (cfg_status == 0) return;
	_mosquitto_free(cfg_p->sub_id);
	 cfg_p->sub_id = NULL;
	
	_mosquitto_free(cfg_p->host);
	
	_mosquitto_free(cfg_p->topic);
	
	_mosquitto_free(cfg_p->bind_address);
	
	_mosquitto_free(cfg_p->username);

	_mosquitto_free(cfg_p->password);

	_mosquitto_free(cfg_p->will_topic);

	_mosquitto_free(cfg_p->will_payload);

	_mosquitto_free(cfg_p->cacert_ptr);
    cfg_p->cacert_buflen = 0;
    cfg_p->cacert_len = 0;

	_mosquitto_free(cfg_p->cert_ptr);
    cfg_p->cert_buflen = 0;
    cfg_p->cert_len = 0;

	_mosquitto_free(cfg_p->priv_key_ptr);
    cfg_p->priv_key_buflen = 0;
    cfg_p->priv_key_len = 0;

	_mosquitto_free(cfg_p->dh_param_ptr);
    cfg_p->dh_param_buflen = 0;
    cfg_p->dh_param_len = 0;

	if (cfg_p->topics)
	{
		for (i = 0; i < cfg_p->topic_count; i++)
		{
			_mosquitto_free(cfg_p->topics[i]);
		}

		_mosquitto_free(cfg_p->topics);
	}
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	if (cfg_p->alpn_count) {
		for (i = 0; i < cfg_p->alpn_count; i++) {
			_mosquitto_free(cfg_p->alpn[i]);
		}
		
		_mosquitto_free(cfg_p->alpn);
	}

	_mosquitto_free(cfg_p->sni);

	if (cfg_p->cipher_suits) {
		_mosquitto_free(cfg_p->cipher_suits);
	}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__

	memset(cfg_p, 0x00, sizeof(struct mosq_config));
	cfg_status = 0;
}

int client_opts_set(struct mosquitto *mosq, struct mosq_config *cfg)
{
	if (cfg->will_topic && mosquitto_will_set(mosq, cfg->will_topic, cfg->will_payloadlen, cfg->will_payload, cfg->will_qos, cfg->will_retain))
	{
		MQTT_DBG_ERR(RED_COLOR "Problem setting will.\n" CLEAR_COLOR);
		return 1;
	}

	if (cfg->username && mosquitto_username_pw_set(mosq, cfg->username, cfg->password))
	{
		MQTT_DBG_ERR(RED_COLOR "Problem setting username and password.\n" CLEAR_COLOR);
		return 1;
	}

	if (cfg->insecure)
	{
		if (mosquitto_tls_insecure_set(mosq, true))
		{
			MQTT_DBG_ERR(RED_COLOR "Failed to set insecure\n" CLEAR_COLOR);
			return 1;
		}

		if (cfg->cacert_ptr)
		{
			if (mosquitto_tls_ca_cert_set(mosq, cfg->cacert_ptr, cfg->cacert_len))
			{
				MQTT_DBG_ERR(RED_COLOR "Failed to set CA cert\n" CLEAR_COLOR);
				return 1;
			}
		}

		if (cfg->cert_ptr)
		{
			if (mosquitto_tls_cert_set(mosq, cfg->cert_ptr, cfg->cert_len))
			{
				MQTT_DBG_ERR(RED_COLOR "Failed to set cert\n" CLEAR_COLOR);
				return 1;
			}
		}

		if (cfg->priv_key_ptr)
		{
			if (mosquitto_tls_private_key_set(mosq, cfg->priv_key_ptr, cfg->priv_key_len))
			{
				MQTT_DBG_ERR(RED_COLOR "Failed to set private key\n" CLEAR_COLOR);
				return 1;
			}
		}

		if (cfg->dh_param_ptr)
		{
			if (mosquitto_tls_dh_param_set(mosq, cfg->dh_param_ptr, cfg->dh_param_len))
			{
				MQTT_DBG_ERR(RED_COLOR "Failed to set dh param\n" CLEAR_COLOR);
				return 1;
			}
		}

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
		if (cfg->alpn && cfg->alpn_count) {
			if (mosquitto_tls_alpn_set(mosq, cfg->alpn, cfg->alpn_count)) {
				MQTT_DBG_ERR(RED_COLOR "Failed to set ALPN\n" CLEAR_COLOR);
				return 1;
			}
		}

		if (cfg->sni) {
			if (mosquitto_tls_sni_set(mosq, cfg->sni)) {
				MQTT_DBG_ERR(RED_COLOR "Failed to set SNI\n" CLEAR_COLOR);
				return 1;
			}
		}

		if (cfg->cipher_suits_count && cfg->cipher_suits) {
			if (mosquitto_tls_cipher_suits_set(mosq, cfg->cipher_suits)) {
				MQTT_DBG_ERR(RED_COLOR "Failed to set cipher suits\n" CLEAR_COLOR);
				return 1;
			}
		}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	}

	mosquitto_max_inflight_messages_set(mosq, cfg->max_inflight);
	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &(cfg->protocol_version));
	return MOSQ_ERR_SUCCESS;
}

void id_number_output(char *id_num)
{
	ULONG macmsw, maclsw;
	getMacAddrMswLsw(0, &macmsw, &maclsw);

	sprintf(id_num, "%02lX%02lX", ((maclsw >> 8) & 0x0ff), ((maclsw >> 0) & 0x0ff));
}

int client_connect(struct mosquitto *mosq, struct mosq_config *cfg)
{
	int rc;

	MQTT_DBG_INFO(" [%s] Bind info---host:%s port:%d keepalive:%d(s) (%s)\n",
				  __func__, cfg->host, cfg->port, cfg->keepalive, mosq->mqtt_pub_or_sub == 1 ? "Pub" : "Sub");

	ut_mqtt_sem_take_recur(&connect_mutex, portMAX_DELAY);
	rc = mosquitto_connect_bind(mosq, cfg->host, cfg->port, cfg->keepalive, cfg->bind_address);
	xSemaphoreGiveRecursive(connect_mutex.mutex_info);

	if (rc > 0)
	{
		MQTT_DBG_ERR(RED_COLOR " Unable to connect (%s)\n" CLEAR_COLOR, mosquitto_strerror(rc));

		_mosquitto_set_broker_conn_state(FALSE);

		return rc;
	}

	return MOSQ_ERR_SUCCESS;
}

UINT mqtt_client_add_pub_topic(char *topic)
{
	if ((topic == NULL) || (strlen(topic) <= 0) || (strlen(topic) > MQTT_TOPIC_MAX_LEN))
	{
		MQTT_DBG_ERR(RED_COLOR "Topic length error (max_len=%d)\n" CLEAR_COLOR, MQTT_TOPIC_MAX_LEN);
		return CC_FAILURE_STRING_LENGTH;
	}
	return da16x_set_config_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, topic);
}

UINT mqtt_client_add_sub_topic(char *topic, int cache)
{
	UINT status = CC_SUCCESS;

	char *nvram_read_topic;
	char nvram_tag[16] = {0, };
	int sub_topic_num = 0;

	if ((topic == NULL) || (strlen(topic) <= 0) || (strlen(topic) > MQTT_TOPIC_MAX_LEN))
	{
		MQTT_DBG_ERR(RED_COLOR "Topic length error (max_len=%d)\n" CLEAR_COLOR, MQTT_TOPIC_MAX_LEN);
		status = CC_FAILURE_RANGE_OUT;
		goto finish;
	}

	if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &sub_topic_num) == CC_SUCCESS)
	{
		if (sub_topic_num >= MQTT_MAX_TOPIC)
		{
			MQTT_DBG_ERR(RED_COLOR "Cannot add topics anymore (max=%d)\n" CLEAR_COLOR, MQTT_MAX_TOPIC);
			status = CC_FAILURE_RANGE_OUT;
			goto finish;
		}

		for (int i = 0; i < sub_topic_num; i++)
		{
			memset(nvram_tag, 0x00, 16);
			sprintf(nvram_tag, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
			nvram_read_topic = read_nvram_string(nvram_tag);

			if (strcmp(nvram_read_topic, topic) == 0)
			{
				MQTT_DBG_ERR(RED_COLOR "Duplicate topic is not allowed.\n" CLEAR_COLOR);
				status = CC_FAILURE_INVALID;
				goto finish;
			}
		}

		sprintf(nvram_tag, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, sub_topic_num);
	}
	else
	{
		nvram_read_topic = read_nvram_string(MQTT_NVRAM_CONFIG_SUB_TOPIC);

		if (strlen(nvram_read_topic))
		{
			delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC);
		}

		sub_topic_num = 0;
		memset(nvram_tag, 0x00, 16);
		sprintf(nvram_tag, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, sub_topic_num);
	}

	sub_topic_num += 1;
	write_tmp_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, sub_topic_num);
	write_tmp_nvram_string(nvram_tag, topic);

	if (!cache)
	{
		save_tmp_nvram();
	}

finish:
	return status;
}

UINT mqtt_client_del_sub_topic(char *topic, int cache)
{
	char *checker = NULL;
	int i, tmp, result = 1;
	int tmp_result;

	if (!read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &tmp))
	{
		for (i = 0; i < tmp; i++)
		{
			char topics[16] = {0, };
			sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
			checker = read_nvram_string(topics);

			if (strcmp(topic, checker) == 0)
			{
				tmp_result = delete_tmp_nvram_env(topics);
				if (tmp_result != TRUE) {
					goto NVRAM_DRV_OPS_ERROR;
				} else {		
					result = 0;
					break;
				}
			}
		}

		if (result == 0)
		{
			// entry found ... 
		
			char topics[16] = {0, };

			for (i += 1; i < tmp; i++)
			{
				sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
				checker = read_nvram_string(topics);
				sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i - 1);
				tmp_result = write_tmp_nvram_string(topics, checker);
				if (tmp_result != 0) {
					goto NVRAM_DRV_OPS_ERROR;
				}
			}

			if (tmp > 1)
			{
				tmp_result = write_tmp_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, tmp - 1);
				if (tmp_result != 0) {
					goto NVRAM_DRV_OPS_ERROR;
				}
			}
			else
			{
				tmp_result = delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM);
				if (tmp_result != TRUE) {
					goto NVRAM_DRV_OPS_ERROR;
				}
			}

			sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, tmp - 1);
			tmp_result = delete_tmp_nvram_env(topics);
			if (tmp_result != TRUE) {
				goto NVRAM_DRV_OPS_ERROR;
			}
		}
	}
	else
	{
		checker = read_nvram_string(MQTT_NVRAM_CONFIG_SUB_TOPIC);

		if (strcmp(topic, checker) == 0)
		{
			tmp_result = delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC);
			if (tmp_result != TRUE) {
				goto NVRAM_DRV_OPS_ERROR;
			} else {
				result = 0;
			}
		}
	}
	
	if (result)
	{
		MQTT_DBG_ERR(RED_COLOR "No Topic to remove.\n" CLEAR_COLOR);
		return 100;
	}
	else if (!cache)
	{
		tmp_result = save_tmp_nvram();
		if (tmp_result != TRUE) {
			goto NVRAM_DRV_OPS_ERROR;
		}
	}

	return result;

NVRAM_DRV_OPS_ERROR:
	return 101;
}

void mqtt_client_delete_sub_topics(void)
{
	int tmp;

	if (!read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &tmp))
	{
		for (int i = 0; i < tmp; i++)
		{
			char topics[16] = {0, };
			sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
			delete_tmp_nvram_env(topics);
		}

		delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM);
	}

	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC, NULL);
	save_tmp_nvram();

}

#define UNSUB_WAIT_MAX_TIMEOUT 300 // 3 seconds

static int unsub_status;
static int unsub_wait_timeout = 0;
/* 
	1: UNSUBACK recv (x) 
	0: UNSUBACK recv (o), successful sync with local data
   -1: UNSUBACK recv (o), but some error in parsing
*/
int mqtt_client_unsub_status_get(void) 
{
	return unsub_status;
}

void mqtt_client_unsub_status_set(int status) 
{
	unsub_status = status;
}

void  mqtt_client_unsub_status_reset(void) 
{
	unsub_status = 0;
	unsub_wait_timeout = 0;
}

int mqtt_client_unsub_topic(char* top)
{
	int ret = MOSQ_ERR_SUCCESS;
	int status;

	if (top == NULL) {
		return MOSQ_ERR_INVAL;
	}

	if (!ut_mqtt_is_topic_existing(top)) {
		return MOSQ_ERR_NOT_FOUND;
	}

	if (mqtt_client_check_conn() == false) {
		return MOSQ_ERR_NO_CONN;
	}

	mqtt_client_unsub_status_set(1); // in progress
	// Send UNSUBSCRIBE
	ret = mosquitto_unsubscribe(mosq_sub, NULL, top);
	if (ret == MOSQ_ERR_SUCCESS) {
		// Tx Queuing success.. wait until UNSUBACK arrives ....
		while(mqtt_client_unsub_status_get() == 1) { 
			if (unsub_wait_timeout >= UNSUB_WAIT_MAX_TIMEOUT) {
				break;
			}
			vTaskDelay(1);
			unsub_wait_timeout++;
		}

		status = mqtt_client_unsub_status_get();
		if (status == -1 || 
			(status == 1 && unsub_wait_timeout >= UNSUB_WAIT_MAX_TIMEOUT)) {
			
			MQTT_DBG_ERR(RED_COLOR "[%s] err, unsub_status(%d), unsub_wait_timeout(%d), \n" CLEAR_COLOR, 
				__func__, mqtt_client_unsub_status_get(), unsub_wait_timeout);
			
			mqtt_client_unsub_status_reset();
			
			if (status == -1) {
				MQTT_DBG_ERR(RED_COLOR "UNSUBACK received, but PROTOCOL error in parsing \n"CLEAR_COLOR);
				return MOSQ_ERR_PROTOCOL;
			} else  {
				MQTT_DBG_ERR(RED_COLOR "UNSUBACK not come yet, timed out!\n"CLEAR_COLOR);
				return MOSQ_ERR_UNKNOWN; // timed-out ... 
			}
		}

		// unsub success ...
		
		// delete from mqttParams
		char* temp_buf = _mosquitto_malloc(MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN+1));
		if (temp_buf == NULL) {
			MQTT_DBG_ERR(RED_COLOR "[%s] malloc error ! \n" CLEAR_COLOR, 
			__func__);
			mqtt_client_unsub_status_reset(); // reset
			return MOSQ_ERR_NOMEM;
		}

		memset(temp_buf, 0x00, MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN+1));
		for (int i = 0; i < MQTT_MAX_TOPIC; i++) {
			if (mqttParams.topics[i][0] == (char)0) {
				continue;
			}
			
			if (strcmp(mqttParams.topics[i], top) == 0) {
				mqttParams.topics[i][0] = (char)0;
				mqttParams.topic_count--;

				// pack array contents (to sync with nvram index)
				int k = 0;
				for (int j=0; j < MQTT_MAX_TOPIC; j++) {
					if (mqttParams.topics[j][0] != 0) {
						strcpy(&(temp_buf[(k++)*(MQTT_TOPIC_MAX_LEN+1)]), mqttParams.topics[j]);
					}
				}
				memcpy(mqttParams.topics, temp_buf, MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN+1));
				mqtt_client_save_to_dpm_user_mem();
								
				break;
			}
		}
		_mosquitto_free(temp_buf);
		
	} else {
		MQTT_DBG_ERR(RED_COLOR "[%s] err, unsub fail(%d), \n" CLEAR_COLOR, 
			__func__, ret);
		mqtt_client_unsub_status_reset(); // reset
		return ret;
	}

	mqtt_client_unsub_status_reset();

	// delete from nvram
	ret = mqtt_client_del_sub_topic(top, 0);
	if (ret == 100) { 
		/* 
		   Topic not found in NVRAM, but the UNSUB procedure anyhow success, so it is 
		   considered success.
		*/
		ret = MOSQ_ERR_SUCCESS;
	}

	return ret;
}

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
void mqtt_client_delete_tls_alpns(void)
{
	int tmp;

	if (!read_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, &tmp)) {
		for (int i = 0; i < tmp; i++) {
			char items[15] = {0, };

			sprintf(items, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
			delete_tmp_nvram_env(items);
		}
		delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM);
	}

	save_tmp_nvram();
}

void mqtt_client_delete_cipher_suits(void)
{
	int tmp;

	if (!read_nvram_int(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM, &tmp)) {
		delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_TLS_CSUITS);
		delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM);
		save_tmp_nvram();
	}
}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__

int mqtt_client_config_initialize(void)
{
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_BROKER_IP, NULL);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_PORT, MQTT_CONFIG_PORT_DEF);
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, NULL);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_QOS, MQTT_CONFIG_QOS_DEF);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_TLS, MQTT_CONFIG_TLS_DEF);
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_USERNAME, NULL);
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_PASSWORD, NULL);
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_WILL_TOPIC, NULL);
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_WILL_MSG, NULL);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_WILL_QOS, MQTT_CONFIG_QOS_DEF);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_AUTO, 0);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_PING_PERIOD, MQTT_CONFIG_PING_DEF);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_SAMPLE, 0);
	da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_VER311, MQTT_CONFIG_VER311_DEF);
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, NULL);

	mqtt_client_delete_sub_topics();

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_TLS_SNI, NULL);
	mqtt_client_delete_tls_alpns();
	mqtt_client_delete_cipher_suits();
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	return 0;
}

static void print_usage(void)
{
	PRINTF("- Usage: MQTT Configuration\n");
	PRINTF("  DESCRIPTION\n");
	PRINTF("\tmqtt_config broker <Broker IP>\n");
	PRINTF("\tmqtt_config port <Port Number> (Default: %d)\n", MQTT_CONFIG_PORT_DEF);
	PRINTF("\tmqtt_config pub_topic <Topic>\n");
	PRINTF("\tmqtt_config sub_topic_add <Topic>\n");
	PRINTF("\tmqtt_config sub_topic_del <Topic>\n");
	PRINTF("\tmqtt_config sub_topic <Topic Number(1~%d)> <Topic#1> <Topic#2> ...\n",
		   MQTT_MAX_TOPIC);
	PRINTF("\tmqtt_config qos <QoS Level> (Default: %d)\n", MQTT_CONFIG_QOS_DEF);
	PRINTF("\tmqtt_config tls <1|0> (Default: %d)\n", MQTT_CONFIG_TLS_DEF);
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	PRINTF("\tmqtt_config tls_alpn <ALPN number(1~3)> <alpn#1> <alpn #2> <alpn#3>\n");
	PRINTF("\tmqtt_config tls_sni <SNI>\n");
	PRINTF("\tmqtt_config tls_cipher <cipher#1> <cipher#2> ...\n");
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	PRINTF("\tmqtt_config username <Username>\n");
	PRINTF("\tmqtt_config password <Password>\n");
	PRINTF("\tmqtt_config long_password \r\n");
	PRINTF("\tmqtt_config will_topic <Topic>\n");
	PRINTF("\tmqtt_config will_message <Message>\n");
	PRINTF("\tmqtt_config will_qos <QoS Level> (Default: %d)\n", MQTT_CONFIG_QOS_DEF);
	PRINTF("\tmqtt_config auto <1|0> (Default: 0)\n");
	PRINTF("\tmqtt_config ping_period <period> (Default: %d)\n", MQTT_CONFIG_PING_DEF);
	PRINTF("\tmqtt_config ver311 <1|0> (Default: 0)\n");
	PRINTF("\tmqtt_config reset (Delete All Configuration)\n");
	PRINTF("\tmqtt_config status\n");
	PRINTF("\tmqtt_config client_id <client_id>\n");
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	PRINTF("\tmqtt_config clean_session <1|0> (Default: %d)\n", MQTT_CONFIG_CLEAN_SESSION_DEF);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	PRINTF("\tmqtt_config no_cert_time_chk <1|0> (Default: 0, for debug)\n");
}

UINT mqtt_client_set_auto_start(int auto_start)
{
	return da16x_set_config_int(DA16X_CONF_INT_MQTT_AUTO, auto_start);
}

UINT mqtt_client_set_broker_info(char *broker_ip, int port)
{
	UINT status = CC_FAILURE_NO_VALUE;

	if ((broker_ip != NULL) && (strlen(broker_ip) > 0))
	{
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_BROKER_IP, broker_ip);

		if (status)
		{
			return status;
		}
	}

	if (port > 0)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_PORT, port);
	}

	return status;
}

UINT mqtt_client_config(int argc, char *argv[])
{
	int status = CC_SUCCESS;

	if (argc == 1)
	{
		print_usage();
		return MOSQ_ERR_INVAL;
	}

	if (strcmp(argv[1], "broker") == 0 && argc == 3)
	{
		status = mqtt_client_set_broker_info(argv[2], 0);
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid Broker Address\n" CLEAR_COLOR);
		}
	}
	else if (strcmp(argv[1], "port") == 0 && argc == 3)
	{
		status = mqtt_client_set_broker_info(NULL, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid Port Number\n" CLEAR_COLOR);
		}
	}
	else if (strcmp(argv[1], "pub_topic") == 0 && argc == 3)
	{
		status = mqtt_client_add_pub_topic(argv[2]);
	}
	else if (strcmp(argv[1], "sub_topic_add") == 0 && argc == 3)
	{
		status = mqtt_client_add_sub_topic(argv[2], 0);
	}
	else if (strcmp(argv[1], "sub_topic_del") == 0 && argc == 3)
	{
		status = mqtt_client_del_sub_topic(argv[2], 0);
	}
	else if (strcmp(argv[1], "sub_topic") == 0 && argc > 3)
	{
		if ((atoi(argv[2]) > MQTT_MAX_TOPIC) || (argc - 3 != atoi(argv[2])))
		{
			MQTT_DBG_ERR(RED_COLOR "Invalid Input\n" CLEAR_COLOR, MQTT_TOPIC_MAX_LEN);
			return MOSQ_ERR_INVAL;
		}

		for (int i = 0; i < atoi(argv[2]); i++)
		{
			if (strlen(argv[i + 3]) > MQTT_TOPIC_MAX_LEN)
			{
				MQTT_DBG_ERR(RED_COLOR "Topic length error (max_len=%d)\n" CLEAR_COLOR, MQTT_TOPIC_MAX_LEN);
				return MOSQ_ERR_INVAL;
			}
		}

		for (int i = 0; i < atoi(argv[2]) - 1; i++)
		{
			for (int j = 0; j < atoi(argv[2]) - 1; j++)
			{
				if (3 + i + 1 + j > atoi(argv[2]) + 3 - 1)
				{
					continue;
				}

				if (strcmp(argv[3 + i], argv[3 + i + 1 + j]) == 0)
				{
					MQTT_DBG_ERR(RED_COLOR "Duplicate topic is not allowed.\n" CLEAR_COLOR);
					return MOSQ_ERR_INVAL;
				}
			}
		}

		mqtt_client_delete_sub_topics();

		for (int i = 0; i < atoi(argv[2]); i++)
		{
			char topics[16] = {0, };
			sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
			write_tmp_nvram_string(topics, argv[i + 3]);
		}

		write_tmp_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, atoi(argv[2]));
		save_tmp_nvram();
		status = CC_SUCCESS;
	}
	else if (strcmp(argv[1], "qos") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_QOS, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT QoS value (0~2)\n" CLEAR_COLOR);
		}
	}
	else if (strcmp(argv[1], "tls") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_TLS, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT TLS value (0|1)\n" CLEAR_COLOR);
		}
	}
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	else if (strcmp(argv[1], "tls_alpn") == 0 && argc > 3) {
		if ((atoi(argv[2]) > MQTT_TLS_MAX_ALPN) || (argc - 3 != atoi(argv[2]))) {
			MQTT_DBG_ERR(RED_COLOR "Invalid Input(max_num=%d)\n" CLEAR_COLOR, MQTT_TLS_MAX_ALPN);
			return MOSQ_ERR_INVAL;
		}

		for (int i = 0; i < atoi(argv[2]); i++) {
			if (strlen(argv[i + 3]) > MQTT_TLS_ALPN_MAX_LEN) {
				MQTT_DBG_ERR(RED_COLOR "ALPN length error (max_len=%d)\n" CLEAR_COLOR, MQTT_TLS_ALPN_MAX_LEN);
				return MOSQ_ERR_INVAL;
			}
		}

		for (int i = 0; i < atoi(argv[2]) - 1; i++) {
			for (int j = 0; j < atoi(argv[2]) - 1; j++) {
				if (3 + i + 1 + j > atoi(argv[2]) + 3 - 1) {
					continue;
				}

				if (strcmp(argv[3 + i], argv[3 + i + 1 + j]) == 0) {
					MQTT_DBG_ERR(RED_COLOR "Duplicate ALPN is not allowed.\n" CLEAR_COLOR);
					return MOSQ_ERR_INVAL;
				}
			}
		}

		mqtt_client_delete_tls_alpns();

		for (int i = 0; i < atoi(argv[2]); i++) {
			char items[15] = {0, };
			sprintf(items, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
			write_tmp_nvram_string(items, argv[i + 3]);
		}

		write_tmp_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, atoi(argv[2]));
		save_tmp_nvram();
		status = CC_SUCCESS;
	}
	else if (strcmp(argv[1], "tls_sni") == 0 && argc == 3) {
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_TLS_SNI, argv[2]);
		if (status) {
			/* Assume that a length of SNI would be same or like broker name */
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid SNI (max_len=%d)\n" CLEAR_COLOR, MQTT_BROKER_MAX_LEN);
		}
	}
	else if (strcmp(argv[1], "tls_cipher") == 0 && argc > 3) {
		char *result_str_pos;
		char *res_str;
		int num_cipher_suits = 0;
		int arg_idx, alloc_bytes = 0;

		num_cipher_suits = argc - 2;
		if (num_cipher_suits > MQTT_TLS_MAX_CSUITS) {
			MQTT_DBG_ERR(RED_COLOR "Invalid Input(max_num=%d)\n" CLEAR_COLOR, MQTT_TLS_MAX_CSUITS);
			return MOSQ_ERR_INVAL;
		}

		for (int i = 0; i < num_cipher_suits; i++) {
			/* Cipher suite value should be hexdecimal that doesn't include the "0x" prefix.
			 * And, maximum length of the value should be 4 as an string.
			 */
			if (strlen(argv[i + 2]) > 4) {
				MQTT_DBG_ERR(RED_COLOR "ALPN length error (max_len=%d)\n" CLEAR_COLOR, 4);
				return MOSQ_ERR_INVAL;
			}
		}

		alloc_bytes = (num_cipher_suits - 1) + (4 * num_cipher_suits);
		res_str = _mosquitto_malloc(alloc_bytes + 1);
		if (res_str == NULL) {
			return MOSQ_ERR_NOMEM;
		}
		
		/* Delete the data stored */
		mqtt_client_delete_cipher_suits();
		
		result_str_pos = res_str;
		memset(res_str, 0, alloc_bytes + 1);
		write_tmp_nvram_int(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM, num_cipher_suits);

		arg_idx = 2;
		sprintf(result_str_pos, "%s", argv[arg_idx]);
		result_str_pos += strlen(argv[arg_idx++]);
		
		for (int i = 0; i < argc - 3; i++, arg_idx++) {
			sprintf(result_str_pos, ",%s", argv[arg_idx]);
			result_str_pos += (strlen(argv[arg_idx]) + 1);
		}
		
		write_tmp_nvram_string(MQTT_NVRAM_CONFIG_TLS_CSUITS, res_str);		
		save_tmp_nvram();
		_mosquitto_free(res_str);

	}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	else if (strcmp(argv[1], "username") == 0 && argc == 3)
	{
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_USERNAME, argv[2]);
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT username (max_len=%d)\n" CLEAR_COLOR, MQTT_USERNAME_MAX_LEN);
		}
	}
	else if (strcmp(argv[1], "password") == 0 && argc == 3)
	{
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_PASSWORD, argv[2]);
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT password (max_len=%d)\n" CLEAR_COLOR, MQTT_PASSWORD_MAX_LEN);
		}
	}
	else if (strcmp(argv[1], "long_password") == 0 && argc == 2)
	{
		extern int make_message(char *title, char *buf, size_t buflen);
		char *buffer = NULL;
		int ret;
		
		buffer = _mosquitto_calloc(MQTT_MSG_MAX_LEN + 1, sizeof(char));
		if (buffer == NULL)
		{
			PRINTF("[%s] Failed to alloc memory for password\n", __func__);
			status = MOSQ_ERR_NOMEM;
		}
		ret = make_message("password", buffer, MQTT_PASSWORD_MAX_LEN + 1);
		PRINTF("\n");
		if (ret > 0)
		{
			status = da16x_set_config_str(DA16X_CONF_STR_MQTT_PASSWORD, buffer);
			if (status)
			{
				MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT password (max_len=%d)\n" CLEAR_COLOR, MQTT_PASSWORD_MAX_LEN);
			}
		}
		else
		{
			PRINTF("Invalid message input\n");
			status = MOSQ_ERR_INVAL;
		}
		
		_mosquitto_free(buffer);
	}
	else if (strcmp(argv[1], "client_id") == 0 && argc == 3)
	{
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, argv[2]);
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT client_id (max_len=%d)\n" CLEAR_COLOR, MQTT_CLIENT_ID_MAX_LEN);
		}
	}
	else if (strcmp(argv[1], "will_topic") == 0 && argc == 3)
	{
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_WILL_TOPIC, argv[2]);
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT Will Topic (max_len=%d)\n" CLEAR_COLOR, MQTT_TOPIC_MAX_LEN);
		}
	}
	else if (strcmp(argv[1], "will_message") == 0 && argc == 3)
	{
		status = da16x_set_config_str(DA16X_CONF_STR_MQTT_WILL_MSG, argv[2]);
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT Will Message (max_len=%d)\n" CLEAR_COLOR, MQTT_WILL_MSG_MAX_LEN);
		}
	}
	else if (strcmp(argv[1], "will_qos") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_WILL_QOS, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT Will QoS value (0~2)\n" CLEAR_COLOR);
		}
	}
	else if (strcmp(argv[1], "auto") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_AUTO, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT_AUTO value (0|1)\n" CLEAR_COLOR);
		}
	}
	else if (strcmp(argv[1], "ping_period") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_PING_PERIOD, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid MQTT PING period (0~86400)\n" CLEAR_COLOR);
		}
	}
	else if (strcmp(argv[1], "ver311") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_VER311, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid value (0|1)\n" CLEAR_COLOR);
		}
	}
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	else if (strcmp(argv[1], "clean_session") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_CLEAN_SESSION, atoi(argv[2]));
		if (status)
		{
			MQTT_DBG_ERR(RED_COLOR "Error: Invalid value (0|1)\n" CLEAR_COLOR);
		}
	}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	else if (strcmp(argv[1], "no_cert_time_chk") == 0 && argc == 3) {
		/* mqtt_config no_cert_time_chk <1|0> */
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_TLS_NO_TIME_CHK,  atoi(argv[2]));
	}
	else if (strcmp(argv[1], "reset") == 0 && argc == 2)
	{
		status = mqtt_client_config_initialize();
	}
	else if (strcmp(argv[1], "tls_incoming") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_TLS_INCOMING, atoi(argv[2]));
	}
	else if (strcmp(argv[1], "tls_outgoing") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_TLS_OUTGOING, atoi(argv[2]));
	}
	else if (strcmp(argv[1], "tls_authmode") == 0 && argc == 3)
	{
		status = da16x_set_config_int(DA16X_CONF_INT_MQTT_TLS_AUTHMODE, atoi(argv[2]));
	}
	else if (strcmp(argv[1], "status") == 0 && argc == 2)
	{
		char ret_str[MQTT_PASSWORD_MAX_LEN + 1] = {0, };
		int ret_num = 0;

		PRINTF("MQTT Client Information:\n");

		da16x_get_config_int(DA16X_CONF_INT_MQTT_SUB, &ret_num);
		PRINTF("  - MQTT Status  : %s\n", ret_num ? "Running" : "Not Running");

		if (da16x_get_config_str(DA16X_CONF_STR_MQTT_BROKER_IP, ret_str))
		{
			strcpy(ret_str, "0.0.0.0");
		}

		PRINTF("  - Broker IP          : %s\n", ret_str);

		da16x_get_config_int(DA16X_CONF_INT_MQTT_PORT, &ret_num);
		PRINTF("  - Port               : %d\n", ret_num);

		if (da16x_get_config_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, ret_str))
		{
			strcpy(ret_str, "");
		}

		PRINTF("  - Pub. Topic         : %s\n", ret_str);

		PRINTF("  - Sub. Topic         : ");

		if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &ret_num) != 0)
		{
			if (da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_TOPIC, ret_str))
			{
				strcpy(ret_str, "");
			}

			PRINTF("%s\n", ret_str);
		}
		else
		{
			memset(ret_str, '\0', MQTT_TOPIC_MAX_LEN + 1);

			if (ret_num == 0)
			{
				PRINTF("\n");
			}

			for (int i = 0; i < ret_num; i++)
			{
				char topics[16] = {0, };
				char *tmp_str;

				sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
				tmp_str = read_nvram_string(topics);

				if (i != (ret_num - 1))
				{
					PRINTF("%s, ", tmp_str);
				}
				else
				{
					PRINTF("%s\n", tmp_str);
				}
			}
		}

		da16x_get_config_int(DA16X_CONF_INT_MQTT_QOS, &ret_num);
		PRINTF("  - QoS Level          : %d\n", ret_num);

		da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS, &ret_num);
		PRINTF("  - TLS                : %s\n", ret_num ? "Enable" : "Disable");

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		da16x_get_config_int(DA16X_CONF_INT_MQTT_CLEAN_SESSION, &ret_num);
		PRINTF("  - Clean Session      : %s\n", ret_num ? "Yes" : "No");
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
		read_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, &ret_num);
		PRINTF("  - TLS ALPN           : ");
		if (!(ret_num == 0 || ret_num == -1)) {
			for (int i = 0; i < ret_num; i++) {
				char items[15] = {0, };
				char *tmp_str;

				sprintf(items, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
				tmp_str = read_nvram_string(items);
				if (strlen(tmp_str)) {
					if (i != (ret_num - 1))
						PRINTF("%s, ", tmp_str);
					else
						PRINTF("%s", tmp_str);
				}
			}
		} else PRINTF("(None)");
		PRINTF("\r\n");

		if (da16x_get_config_str(DA16X_CONF_STR_MQTT_TLS_SNI, ret_str)) {
			strcpy(ret_str, "");
		}
		PRINTF("  - TLS SNI            : %s\r\n", strlen(ret_str) ? ret_str : "(None)");

		read_nvram_int(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM, &ret_num);
		PRINTF("  - TLS CIPHER SUIT    : ");
		if (!(ret_num == 0 || ret_num == -1)) { 
			char *tmp_str = NULL;

 			tmp_str = read_nvram_string(MQTT_NVRAM_CONFIG_TLS_CSUITS);
			if (strlen(tmp_str)) {
				PRINTF("%s", tmp_str);
			}
		} else PRINTF("(None)");
		PRINTF("\r\n");
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
		da16x_get_config_int(DA16X_CONF_INT_MQTT_PING_PERIOD, &ret_num);
		PRINTF("  - Ping Period        : %d\n", ret_num);

		da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_INCOMING, &ret_num);
		PRINTF("  - TLS Incoming buf   : %d(bytes)\n", ret_num);

		da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_OUTGOING, &ret_num);
		PRINTF("  - TLS Outgoing buf   : %d(bytes)\n", ret_num);

		da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS_AUTHMODE, &ret_num);
		PRINTF("  - TLS Auth mode      : %d\n", ret_num);

		if (da16x_get_config_str(DA16X_CONF_STR_MQTT_USERNAME, ret_str) == CC_SUCCESS)
		{
			PRINTF("  - User name          : %s\n", ret_str);
		}
		else
		{
			PRINTF("  - User name          : %s\n", "(None)");
		}
		
		if (da16x_get_config_str(DA16X_CONF_STR_MQTT_PASSWORD, ret_str) == CC_SUCCESS)
		{
			PRINTF("  - Password           : %s\n", ret_str);
		}
		else
		{
			PRINTF("  - Password           : %s\n", "(None)");
		}

		if (da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, ret_str) == CC_SUCCESS)
		{
			PRINTF("  - Client ID          : %s\n", ret_str);
		}
		else
		{
			// generate default cid if there's no cid stored in NVM
			char mac_id[5] = {0,};
			char def_cid[11] = {0,};
			id_number_output(mac_id);
			
			sprintf(def_cid, "%s_%s", "da16x", mac_id);	
		
			PRINTF("  - Client ID          : (default: %s)\n", def_cid);
		}
		
		if (da16x_get_config_int(DA16X_CONF_INT_MQTT_VER311, &ret_num) == CC_SUCCESS)
		{
			if (ret_num == 1)
				PRINTF("  - MQTT VER           : %s\n", MQTT_VER_311);
			else
				PRINTF("  - MQTT VER           : %s\n", MQTT_VER_31);
		}
		else
		{
			PRINTF("  - MQTT VER           : %s\n", MQTT_VER_31);
		}
		status = CC_SUCCESS;
	}
	else
	{
		print_usage();
		return MOSQ_ERR_INVAL;
	}

	if (status == CC_SUCCESS)
	{
		return MOSQ_ERR_SUCCESS;
	}
	else
	{
		return MOSQ_ERR_INVAL;
	}
}

UINT	mqttSubDpmPingCheckCnt		= 0;
UINT	subPingStatus				= PING_STS_NONE;

extern struct mosquitto	*mosq_sub;
UINT	runContinueSub = false;
UINT	force_mqtt_stop = false;

static int is_sub_ping()
{
	int ret;

	if (subPingStatus == PING_STS_NONE)
	{
		ret = 0;	/* OK */
	}
	else if (subPingStatus == PING_STS_RECEIVED)
	{
		ret = 0;	/* OK */
	}
	else if (subPingStatus == PING_STS_SENT)
	{
		MQTT_DBG_INFO(" [%s] PING Sended(%d) \n", __func__, subPingStatus);
		ret = 1;	/* Not yet */
	}
	else
	{
		MQTT_DBG_ERR(RED_COLOR "Error Ping status (%d) \n" CLEAR_COLOR, __func__, subPingStatus);
		ret = 0;	/* OK */
	}

	return ret;
}

static void mqtt_client_disconnect_forcibly(void)
{
	if (mosq_sub)
	{
		mosq_sub->state = mosq_cs_disconnect_cb;
	}
}

extern UCHAR get_last_abnormal_act(void);
static int is_wakeup_from_dpm_abn_type_3(void)
{
	return (dpm_mode_is_wakeup() && get_last_abnormal_act() == 3); // ARP Failure
}

#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
/* Max wait time (in TLS_DECRYPT_WAIT_TIME_MAX / 10 sec) for TLS to decrypt full packets 
in case only partial packets are received */
#define TLS_DECRYPT_WAIT_TIME_MAX 100

int tls_decrypt_wait_time;
extern int hold_sleep_due_to_tls_decrypt_in_progress;
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__
static void mqtt_client_check_keep_alive(void* arg)
{
	DA16X_UNUSED_ARG(arg);

	extern int subRetryCount;
	int sub_check = 0;
	int qos_check = 0;
	int time_work_around = MQTT_CLIENT_DPM_SLEEP_TIMEOUT * 10;
	unsigned long wk_src;

	if (!dpm_mode_is_wakeup() || is_wakeup_from_dpm_abn_type_3()) {
		vTaskDelete(NULL);
		return;
	}

	wk_src = da16x_get_wakeup_source();
	while (confirm_dpm_init_done(APP_MQTT_SUB) == pdFAIL) {
		vTaskDelay(1);
	}

	if (wk_src == DPM_UC) {
		if (mqttParams.qos >= 1) {
			vTaskDelay(50);
		} else {
			vTaskDelay(30);
		}
	} else if (wk_src == DPM_DEAUTH || 
			   wk_src == DPM_NO_BCN || 
			   wk_src == DPM_KEEP_ALIVE_NO_ACK) {
		if (wk_src == DPM_NO_BCN) {
            if (is_netif_down_ever_happpened()) {
                goto force_disconnect;
            } else {
                /* Wakeup by NO_BCN, but no link-down ever happened before mqtt app starts
                in this case, we do not restart mqtt */
    			vTaskDelay(50);
    			if (chk_network_ready(WLAN0_IFACE) == 0) {
                    goto force_disconnect;
    			}
            }
		} else {
force_disconnect:		
			mqtt_client_disconnect_forcibly();
			vTaskDelete(NULL);
			return;
		}		   
	} else {
		vTaskDelay(5);
	}

	while (1) {
		if (time_work_around < 0) {
			break;
		}
		
		if (chk_network_ready(WLAN0_IFACE) == 0) {
			mqtt_client_disconnect_forcibly();
			vTaskDelete(NULL);
			return;
		}

		if (!mosq_sub) {
			goto next_mqtt;
		} else if (mosq_sub->q2_proc) {
			qos_check = 1;

			if (is_sub_ping()) {
			   /* 
			   	just in case mqtt ping event happens 
			   	while waiting for qos2 (Rx: PUBREL)
			   */ 
			
				if (++mqttSubDpmPingCheckCnt < (MQTT_WAIT_COUNT_PING * 10)) {
					if (mqttSubDpmPingCheckCnt % 10 == 0) {
						MQTT_DBG_INFO(" [%s] mqttSubDpmPingCheckCnt is IN_OPER(cnt:%d) \n",
									  __func__, mqttSubDpmPingCheckCnt / 10);
					}
					goto next_mqtt;
				} else {
					goto ping_timeout;
				}
			}

			goto next_mqtt;
		} else if (qos_check && !mosq_sub->q2_proc) {
			vTaskDelay(50);
			time_work_around -= 5;
			qos_check = 0;
		}
		
		if (sub_check == 1) {
			goto next_mqtt;
		}

		if (is_sub_ping()) {
			if (++mqttSubDpmPingCheckCnt < (MQTT_WAIT_COUNT_PING * 10)) {
				if (mqttSubDpmPingCheckCnt % 10 == 0) {
					MQTT_DBG_INFO(" [%s] mqttSubDpmPingCheckCnt is IN_OPER(cnt:%d) \n",
								  __func__, mqttSubDpmPingCheckCnt / 10);
				}
				goto next_mqtt;
			} else {
ping_timeout:
				MQTT_DBG_ERR(RED_COLOR "Timeout -- mqttSubDpmPingCheckCnt is %d \n" CLEAR_COLOR,
							 mqttSubDpmPingCheckCnt / 10);
				mqttSubDpmPingCheckCnt = 0;
				mosq_sub->state = mosq_cs_disconnect_cb;
				sub_check = 1;
				vTaskDelay(50);
			}
		}
		
		if (subRetryCount == 0 && mqttParams.sub_connect_retry_count == 0) {
#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
			if (hold_sleep_due_to_tls_decrypt_in_progress == TRUE) {
				if (tls_decrypt_wait_time++ >= TLS_DECRYPT_WAIT_TIME_MAX) {
					MQTT_DBG_ERR(RED_COLOR "Max TLS decrypt time (%d secs) reached, entering DPM Sleep \n" CLEAR_COLOR,
									TLS_DECRYPT_WAIT_TIME_MAX/10);
					mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
				} else {
					MQTT_DBG_INFO("Delaying DPM Sleep due to TLS decrypt in progress ... \n");
				}				
			} else
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__
				mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
		}

		sub_check = 1;

next_mqtt:
		vTaskDelay(10);
		time_work_around--;
	}

	rtc_register_timer(5000, "MQTT_TEMP", 0, 0, NULL); //msec

	mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);

	vTaskDelete(NULL);
	return;
}

static int check_mqtt_auto_flag(void)
{
	int mqtt_auto;
	int ret = 0;

	if (dpm_mode_is_wakeup())
	{
		if (!dpm_user_mem_get(MQTT_PARAM_NAME, (UCHAR **)&rtmMqttParamPtr))
		{
			// rtm not found, then check nvram's auto-flag
			goto chk_auto_flag_from_nvm;
		}
		
		if (rtmMqttParamPtr->mqtt_auto != MQTT_INIT_MAGIC) 
		{
			// no auto-start
			ret = -1;
		}
	}
	else
	{
chk_auto_flag_from_nvm:
		if (da16x_get_config_int(DA16X_CONF_INT_MQTT_AUTO, &mqtt_auto))
		{
			ret = -1;
		}

		if (mqtt_auto == 0)
		{
			ret = -1;
		}
	}

	if (ret)
	{
		dpm_app_unregister(APP_MQTT_SUB);
	}

	return ret;
}

TaskHandle_t	mqttCheckThread = NULL;
#define	MQTT_CHECK_THREAD_STACK_SIZE	1024

void	mqtt_client_create_thread(void * pvParameters)
{
	DA16X_UNUSED_ARG(pvParameters);

	int sys_wdog_id = -1;
	int status;
	extern UCHAR sntp_support_flag;

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
	
	if (check_mqtt_auto_flag())
	{
		da16x_sys_watchdog_unregister(sys_wdog_id);

		vTaskDelete(NULL);
		return;
	}


	if (is_wakeup_from_dpm_abn_type_3() && 
			(da16x_network_main_get_netmode(WLAN0_IFACE) == DHCPCLIENT)) {

		da16x_sys_watchdog_notify(sys_wdog_id);

		da16x_sys_watchdog_suspend(sys_wdog_id);

		while(da16x_network_main_check_dhcp_state(WLAN0_IFACE) != DHCP_STATE_BOUND) {
			vTaskDelay(1);
		}

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
	}

	ut_mqtt_sem_create(&mqtt_conf_mutex);
	ut_mqtt_sem_create(&connect_mutex);

    da16x_sys_watchdog_notify(sys_wdog_id);
    da16x_sys_watchdog_suspend(sys_wdog_id);	
	// early initialize mqtt config
	status = mqtt_client_initialize_config();
    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

	if (status != pdTRUE)
	{
		MQTT_DBG_ERR(RED_COLOR "Failed MQTT Start (status:%d)!!! \n" CLEAR_COLOR, status);

        atcmd_asynchony_event(4, "0");

		if (dpm_mode_is_enabled())
		{
			mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
		}

		da16x_sys_watchdog_unregister(sys_wdog_id);

		vTaskDelete(NULL);
		return;
	}

	// re-register to DPM w/ valid port number
	if (dpm_mode_is_enabled())
	{
		if (!dpm_mode_is_wakeup())
		{
			mqtt_client_dpm_regi_with_port();
		}
		
		mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);
	}

	if (sntp_support_flag == pdTRUE)
	{
		int	i = 0;
		
		if (!dpm_mode_is_wakeup())
		{
			while (1)  	/* waiting for SNTP Sync */
			{
				da16x_sys_watchdog_notify(sys_wdog_id);

				if (get_sntp_use() == FALSE)
				{
					MQTT_DBG_INFO(">>> Not using SNTP \n");
					break;
				}

				if (is_sntp_sync() == TRUE)
				{
					break;
				}

				if ((++i % 50) == 0)  	/* 5 SEC */
				{
					MQTT_DBG_ERR(RED_COLOR ">>> Not Sync timer by SNTP \n" CLEAR_COLOR);
					break;
				}
				
				vTaskDelay(10);
			}
		}
	}

	// create mqtt_client monitor thread
	xTaskCreate(mqtt_client_check_keep_alive,
				"MQTT_TEMP",
				(MQTT_CHECK_THREAD_STACK_SIZE/4),
				NULL,
				(OS_TASK_PRIORITY_USER + 6),
				&mqttCheckThread);
	
	mqtt_client_start();

	da16x_sys_watchdog_unregister(sys_wdog_id);

	vTaskDelete(NULL);
}

bool	mqttRestartProcessing = false;

TimerHandle_t restartTimer = NULL;

void mqtt_restart(ULONG arg)
{
	DA16X_UNUSED_ARG(arg);

	MQTT_DBG_INFO(" [%s] Start \n", __func__);

	mqtt_client_wait_stopping();

	runContinueSub = true;
	mqttRestartProcessing = false;

	MQTT_DBG_INFO(" [%s] Request mqtt_auto_start \n", __func__);
	mqtt_auto_start(NULL);
}

int mqtt_client_wait_stopping(void)
{
	int	i = 0;

	runContinueSub = false;
	
	vTaskDelay(10);

waiting:
	if (mosq_sub != NULL)
	{
		if (++i < 50)
		{
			if (i % 5 == 0)
			{
				MQTT_DBG_ERR(RED_COLOR "Waiting for SUB/PUB stop(S:%d) \n" CLEAR_COLOR, mosq_sub);
			}

			runContinueSub = false;
			vTaskDelay(10);
			goto waiting;
		}
		else
		{
			MQTT_DBG_ERR(RED_COLOR "Timeout waiting (mosq_sub:%d) \n" CLEAR_COLOR, mosq_sub);
			return 0;	/* Timeout */
		}
	}

	return 1;	/* MQTT Stopped */
}

void mqtt_client_terminate_thread(void)
{
	runContinueSub = false;
}

int mqtt_client_get_topic_count(void)
{
	return mqttParams.topic_count;
}

int mqtt_client_get_pub_msg_id(void)
{
	return mqttParams.pub_msg_id;
}

void mqtt_client_dpm_port_filter_set(void)
{
    if (mqttParams.my_sub_port_no_prev != 0) {
        dpm_tcp_port_filter_delete((unsigned short)mqttParams.my_sub_port_no_prev);
    }
    
    dpm_tcp_port_filter_set((unsigned short)(mqttParams.my_sub_port_no));
    mqttParams.my_sub_port_no_prev = mqttParams.my_sub_port_no;
}

void mqtt_client_dpm_port_filter_clear(void)
{
    dpm_tcp_port_filter_delete(mqttParams.my_sub_port_no);
    
    if (mqttParams.my_sub_port_no_prev != 0) {
        dpm_tcp_port_filter_delete(mqttParams.my_sub_port_no_prev);
    }
    mqttParams.my_sub_port_no_prev = 0;
}

#else
#include "da16x_types.h"

void mqtt_client_create_thread(void * pvParameters) { DA16X_UNUSED_ARG(pvParameters); }
void mqtt_restart(unsigned long arg) { DA16X_UNUSED_ARG(arg); }
void mqtt_client_terminate_thread(void) { }

#endif // (__SUPPORT_MQTT__)

/* EOF */
