/**
 ****************************************************************************************
 *
 * @file mqtt_msg_tbl_presvd.c
 *
 * @brief MQTT Preserved Message table to support CleanSession=0
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

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)

#include "mqtt_client.h"
#include "mosquitto_internal.h"
#include "memory_mosq.h"
#include "messages_mosq.h"
#include "util_mosq.h"

#include "mqtt_msg_tbl_presvd.h"

// preserved mqtt msg table
static mq_msg_tbl_presvd_t  mqtt_msg_tbl_presvd;
static mq_msg_tbl_presvd_t* mqtt_msg_tbl_presvd_rtm;
static int 					mq_msg_tbl_presvd_enabled;
static int 					mq_msg_tbl_init_done;

static mqtt_mutex_t 		mqtt_tbl_mutex;

extern mqttParamForRtm mqttParams;
#if defined (__MQTT_EMUL_CMD__)
extern mqttParamForRtm *rtmMqttParamPtr;
#endif	//__MQTT_EMUL_CMD__

// find an empty row in preserved mqtt message table
static int _mq_msg_tbl_presvd_find_empty_slot(void)
{
	int i, ret = -1; // an empty slot not found

	for(i = 0; i < MQTT_MSG_TBL_PRESVD_MAX_MSG_CNT; i++) {
		if (mqtt_msg_tbl_presvd.tbl_row[i].is_valid == false) 
			return i;
	}

	return ret;
}

static void _mq_msg_tbl_presvd_mutex_take(void)
{
	if (mqtt_tbl_mutex.mutex_info == NULL) {
		mqtt_tbl_mutex.mutex_info = xSemaphoreCreateRecursiveMutex();
	}

	xSemaphoreTakeRecursive(mqtt_tbl_mutex.mutex_info, portMAX_DELAY);
}

static void _mq_msg_tbl_presvd_mutex_give(void)
{
	xSemaphoreGiveRecursive(mqtt_tbl_mutex.mutex_info);
}

static const char* const state_str[] = {"invalid", 
										"publish_qos0", 
										"publish_qos1", 
										"wait_for_puback", 
										"publish_qos2", 
										"wait_for_pubrec", 
										"resend_pubrel",
										"wait_for_pubrel",
										"resend_pubcomp",
										"wait_for_pubcomp",
										"send_pubrec",
										"queued"};
static const char* _mq_msg_tbl_presvd_state_2_string(enum mosquitto_msg_state state)
{
	return state_str[state];
}

static void _mq_msg_tbl_presvd_print_row(int msg_idx)
{
	DA16X_UNUSED_ARG(msg_idx);

	MQTT_DBG_INFO(YELLOW_COLOR "msg[%d]: \n  .is_valid=%u\n  .dir=%d\n  .dup=%d\n  .mid=%u\n  .payload_len=%d\n  .topic_len=%d\n  .retain=%d\n  .state=%s\n  .timestamp=%llu\n  .qos=%d\n  .topic=%s\n  .payload=%s\n" CLEAR_COLOR,
		 msg_idx, 
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].is_valid,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].dir,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].dup,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].mid,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload_len,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic_len,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].retain,
		 _mq_msg_tbl_presvd_state_2_string(mqtt_msg_tbl_presvd.tbl_row[msg_idx].state),
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].timestamp,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].qos,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic,
		 mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload);
}	

// build a Msg
static int _mq_msg_tbl_presvd_build_msg(int msg_idx, 
											struct mosquitto_message_all ** msg) 
{
	struct mosquitto_message_all* message;

	message = _mosquitto_calloc(1, sizeof(struct mosquitto_message_all));
	if (!message) {
		return MOSQ_ERR_NOMEM;
	}

	message->dup		= mqtt_msg_tbl_presvd.tbl_row[msg_idx].dup;
	message->msg.qos	= mqtt_msg_tbl_presvd.tbl_row[msg_idx].qos;
	message->msg.retain	= mqtt_msg_tbl_presvd.tbl_row[msg_idx].retain;

	message->msg.topic = _mosquitto_calloc(1, mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic_len+1);
	if (message->msg.topic == NULL) return MOSQ_ERR_NOMEM;
	strcpy(message->msg.topic, mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic);

	message->msg.payload = _mosquitto_calloc(1, mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload_len+1);
	if (message->msg.payload == NULL) return MOSQ_ERR_NOMEM;
	memcpy(message->msg.payload, mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload, 
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload_len);
	message->msg.payloadlen = mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload_len;

	message->msg.mid = mqtt_msg_tbl_presvd.tbl_row[msg_idx].mid;
	message->timestamp = mqtt_msg_tbl_presvd.tbl_row[msg_idx].timestamp;
	message->state = mqtt_msg_tbl_presvd.tbl_row[msg_idx].state;

	*msg = message;

	return MOSQ_ERR_SUCCESS;
}


static void _mq_msg_tbl_presvd_init_1(void)
{
	// magic init
	mqtt_msg_tbl_presvd.magic_code = MQTT_INIT_MAGIC;
}

static void _mq_msg_tbl_presvd_init_2(int len)
{
	int status;
	const unsigned long wait_option = 100;
	
	_mq_msg_tbl_presvd_init_1();

	if (len != 0) dpm_user_mem_free(MQTT_MSG_TBL_PRESVD_NAME);

	// rtm alloc
	status = dpm_user_mem_alloc(MQTT_MSG_TBL_PRESVD_NAME, (void **)&mqtt_msg_tbl_presvd_rtm,
											sizeof(mq_msg_tbl_presvd_t), wait_option);
	if (status == 0 /* SUCCESS */) {		
		// sync (sram -> rtm)
		memcpy(mqtt_msg_tbl_presvd_rtm, &mqtt_msg_tbl_presvd, sizeof(mq_msg_tbl_presvd_t));
		MQTT_DBG_INFO(YELLOW_COLOR "[%s] rtm alloc successful \n" CLEAR_COLOR, __func__);
	} else {
		MQTT_DBG_ERR(RED_COLOR "[%s] rtm alloc failed (0x%02x), proceed with sram only \n" CLEAR_COLOR, 
					__func__, status);
	}
}

static void _mq_msg_tbl_presvd_init_3(void)
{
	if (mqtt_msg_tbl_presvd_rtm->magic_code == MQTT_INIT_MAGIC) {
		// sync (rtm -> sram)
		memcpy(&mqtt_msg_tbl_presvd, mqtt_msg_tbl_presvd_rtm, sizeof(mq_msg_tbl_presvd_t));
		MQTT_DBG_INFO(YELLOW_COLOR "[%s] restore from rtm successful \n" CLEAR_COLOR, __func__);
	} else {
		// rtm not valid, free and re-alloc...
		MQTT_DBG_ERR(RED_COLOR "[%s] magic corrupted, to realloc ... \n" CLEAR_COLOR, __func__);
		_mq_msg_tbl_presvd_init_2(1);
	}
}

static int _mq_msg_tbl_presvd_init_is_done(void)
{
	return mq_msg_tbl_init_done;
}

// check if table is fully occupied
static void _mq_msg_tbl_presvd_init_done(void)
{
	mq_msg_tbl_init_done = true;
}


// Restore messages from preserved mqtt message table to Tx Msg Q and Rx Msg Q
static void _mq_msg_tbl_presvd_restore(struct mosq_config* mqtt_cfg, struct mosquitto* mosq) 
{
	DA16X_UNUSED_ARG(mqtt_cfg);

	int ret = MOSQ_ERR_SUCCESS;
  	unsigned int len;

	/*
		restore Tx Msg Q and Rx Msg Q from mqtt_msg_tbl_presvd
		restore other info
		
		case: DPM mode (o)
			check rtm
			case: rtm intact (o)
				case: init done (x)
					_mq_msg_tbl_presvd_init_3() // sram (magic init), copy (rtm->sram)
					init_done

			case: rtm intact (x)
				case: init done (x)
					_mq_msg_tbl_presvd_init_2() // sram (magic init), rtm (alloc), copy (sram->rtm)
					init_done

		case: DPM mode (x)
			case: init done (x)
				_mq_msg_tbl_presvd_init_1() // sram (magic init)
				init_done


		// for all case	
		case: msg count (tx+rx) > 0
			loop (sram.msg_q)
				add msg to msg q
				
			restore .last_retry_chk_time
			restore .last_tx_mid_used (if non-DPM mode)
	*/

	if (dpm_mode_is_enabled()) {
		len = dpm_user_mem_get(MQTT_MSG_TBL_PRESVD_NAME, (unsigned char **)&mqtt_msg_tbl_presvd_rtm);
		if (len == sizeof(mq_msg_tbl_presvd_t)) {
			if (!_mq_msg_tbl_presvd_init_is_done()) {
				_mq_msg_tbl_presvd_init_3();
				_mq_msg_tbl_presvd_init_done();
			}
		} else {
			if (!_mq_msg_tbl_presvd_init_is_done()) {
				_mq_msg_tbl_presvd_init_2(len);
				_mq_msg_tbl_presvd_init_done();
			}
		}
	} else {
		if (!_mq_msg_tbl_presvd_init_is_done()) {
			_mq_msg_tbl_presvd_init_1();
			_mq_msg_tbl_presvd_init_done();
			MQTT_DBG_INFO(YELLOW_COLOR "[%s] sram only init \n" CLEAR_COLOR, __func__);
		}
	}
	
	if (mqtt_msg_tbl_presvd.tbl_hdr.total_rx_msg_cnt + 
		mqtt_msg_tbl_presvd.tbl_hdr.total_tx_msg_cnt > 0) {
		int i, any_msg_restored=false;

		// restore messages
		for(i = 0; i < MQTT_MSG_TBL_PRESVD_MAX_MSG_CNT; i++) {
			struct mosquitto_message_all * message;
			
			if (mqtt_msg_tbl_presvd.tbl_row[i].is_valid == pdFALSE) continue;

			_mq_msg_tbl_presvd_print_row(i);
			
			ret = _mq_msg_tbl_presvd_build_msg(i, &message);
			if (ret == MOSQ_ERR_SUCCESS) {
				_mosquitto_message_queue(mosq, message, 
										mqtt_msg_tbl_presvd.tbl_row[i].dir);

				if (mqtt_msg_tbl_presvd.tbl_row[i].dir == mosq_md_in && 
					mqtt_msg_tbl_presvd.tbl_row[i].qos == 2) {
					mosq->q2_proc = 1;
				}
				MQTT_DBG_INFO(YELLOW_COLOR "[%s] restore successful \n" CLEAR_COLOR, __func__);
				any_msg_restored = true;
			} else {
				// message build error, cannot restore
				MQTT_DBG_ERR(RED_COLOR "[%s] restore failed (msg build error) \n" CLEAR_COLOR, __func__);
			}
		}

		if (!any_msg_restored) MQTT_DBG_INFO(YELLOW_COLOR "[%s] no msg restored !!! even though msg cnt > 0 \n" CLEAR_COLOR, __func__);
	}

	// restore other info
	mosq->last_retry_check = mqtt_msg_tbl_presvd.tbl_hdr.last_retry_chk_time;
	if (!dpm_mode_is_enabled()) {
		mosq->last_mid = mqtt_msg_tbl_presvd.tbl_hdr.last_tx_mid_used;
	} else {
		// in dpm mode, pub tx mid is fetched from mqttParams
	}
}

// remove all messages in preserved mqtt messge queue
static void _mq_msg_tbl_presvd_disable(void) 
{
	/*
	
		clear mqtt_msg_tbl_presvd (memset)
	
		case: DPM mode & rtm alloc exist
			 free rtm

		mqtt_msg_tbl_presvd_rtm = NULL;
	*/

	memset(&mqtt_msg_tbl_presvd, 0x00, sizeof(mq_msg_tbl_presvd_t));
	mqtt_msg_tbl_presvd.magic_code = MQTT_INIT_MAGIC;
	
	if (dpm_mode_is_enabled()) {
		if (dpm_user_mem_get(MQTT_MSG_TBL_PRESVD_NAME, 
							(unsigned char **)&mqtt_msg_tbl_presvd_rtm) != 0) {
				dpm_user_mem_free(MQTT_MSG_TBL_PRESVD_NAME);
		}
	}

	mqtt_msg_tbl_presvd_rtm = NULL;
	
	return;
}

// search and return the idx of the same message
static int _mq_msg_tbl_presvd_find(enum mosquitto_msg_direction dir, uint16_t mid)
{
	int i;
	for(i = 0; i < MQTT_MSG_TBL_PRESVD_MAX_MSG_CNT; i++) {
		if (mqtt_msg_tbl_presvd.tbl_row[i].is_valid == false) continue;
		if (mqtt_msg_tbl_presvd.tbl_row[i].dir != dir) continue;
		if (mqtt_msg_tbl_presvd.tbl_row[i].mid == mid) return i;
	}
	return -1; // not found
}

// dump table data from sram to rtm
static int _mq_msg_tbl_presvd_copy_to_rtm(void) 
{
	if (mqtt_msg_tbl_presvd_rtm) { 
		memcpy(mqtt_msg_tbl_presvd_rtm, &mqtt_msg_tbl_presvd, sizeof(mq_msg_tbl_presvd_t));
	}
	
	return true;
}

void _mq_msg_tbl_set_hdr_value(char* key, char* value, mq_msg_tbl_hdr_t* hdr, 
									int* magic_code, uint8_t* set_flag)
{
	if (strcmp(key, "magic_code") == 0) {
		*set_flag |= SET_FLAG_MSK_MAGIC_CODE;
		*magic_code = atoi(value);
		return;
	}

	if (strcmp(key, "total_rx_msg_cnt") == 0) {
		*set_flag |= SET_FLAG_MSK_HDR_TOTAL_RX_MSG_CNT;
		hdr->total_rx_msg_cnt = atoi(value);
		return;
	}	

	if (strcmp(key, "total_tx_msg_cnt") == 0) {
		*set_flag |= SET_FLAG_MSK_HDR_TOTAL_TX_MSG_CNT;
		hdr->total_tx_msg_cnt = atoi(value);
		return;
	}	
	
	if (strcmp(key, "last_retry_chk_time") == 0) {
		*set_flag |= SET_FLAG_MSK_HDR_LAST_RETRY_CHK_TIME;
		hdr->last_retry_chk_time = strtoull(value, 0, 10);
		return;
	}	
	if (strcmp(key, "last_tx_mid_used") == 0) {
		*set_flag |= SET_FLAG_MSK_HDR_LAST_TX_MID_USED;
		hdr->last_tx_mid_used = strtoul(value, 0, 10);
		return;
	}
}


void _mq_msg_tbl_set_row_value(char* key, char* value, 
							mq_msg_tbl_row_t* row, uint16_t* set_flag)
{
	if (strcmp(key, "mid") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_MID;
		row->mid = strtoul(value, 0, 10);
		return;
	}
	if (strcmp(key, "payload_len") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_PAYLOAD_LEN;
		row->payload_len = atoi(value);
		return;
	}
	if (strcmp(key, "topic_len") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_TOPIC_LEN;
		row->topic_len = atoi(value);
		return;
	}
	if (strcmp(key, "qos") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_QOS;
		row->qos = atoi(value);
		return;
	}
	if (strcmp(key, "state") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_STATE;
		row->state = (enum mosquitto_msg_state)atoi(value);
		return;
	}
	if (strcmp(key, "dir") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_DIR;
		row->dir = (enum mosquitto_msg_direction)atoi(value);
		return;
	}
	if (strcmp(key, "timestamp") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_TIMESTAMP;
		row->timestamp = strtoull(value, 0, 10);
		return;
	}
	if (strcmp(key, "dup") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_DUP;
		row->dup = strtoul(value, 0, 10);
		return;
	}
	if (strcmp(key, "retain") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_RETAIN;
		row->retain = strtoul(value, 0, 10);
		return;
	}
	if (strcmp(key, "is_valid") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_IS_VALID;
		row->is_valid = strtoul(value, 0, 10);
		return;
	}
	if (strcmp(key, "payload") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_PAYLOAD;
		strcpy(row->payload, value);
		return;
	}
	if (strcmp(key, "topic") == 0) {
		*set_flag |= SET_FLAG_MSK_ROW_TOPIC;
		strcpy(row->topic, value);
		return;
	}
}

// show the current status of the table (debug purpose)
static void _mq_msg_tbl_presvd_get_status(void)
{
	int i;
	/*
		fill in tbl_status
		print all messages
	*/

	PRINTF("msg_tbl : %s \n", mq_msg_tbl_presvd_enabled? "enabled" : "disabled");
	
	PRINTF("msg_tbl rtm : %s allocated \n\n", !mqtt_msg_tbl_presvd_rtm? "not" : "");

	PRINTF("mq_msg_tbl_presvd: \n  .magic_code=%d\n  .last_retry_chk_time=%llu\n  .last_tx_mid_used=%d\n  .total_rx_msg_cnt=%d\n  .total_tx_msg_cnt=%d\n\n\n", 
	mqtt_msg_tbl_presvd.magic_code,
	mqtt_msg_tbl_presvd.tbl_hdr.last_retry_chk_time,
	mqtt_msg_tbl_presvd.tbl_hdr.last_tx_mid_used,
	mqtt_msg_tbl_presvd.tbl_hdr.total_rx_msg_cnt,
	mqtt_msg_tbl_presvd.tbl_hdr.total_tx_msg_cnt);
	
	for(i = 0; i < MQTT_MSG_TBL_PRESVD_MAX_MSG_CNT; i++) {
		PRINTF("msg[%d]: \n  .is_valid=%d\n  .dir=%d\n  .dup=%d\n  .mid=%d\n  payload_len=%d\n  topic_len=%d\n  retain=%d\n  state=%s\n  timestamp=%llu\n  qos=%d\n  topic=%s\n  payload=%s\n",
			 i, 
			 mqtt_msg_tbl_presvd.tbl_row[i].is_valid,
			 mqtt_msg_tbl_presvd.tbl_row[i].dir,
			 mqtt_msg_tbl_presvd.tbl_row[i].dup,
			 mqtt_msg_tbl_presvd.tbl_row[i].mid,
			 mqtt_msg_tbl_presvd.tbl_row[i].payload_len,
			 mqtt_msg_tbl_presvd.tbl_row[i].topic_len,
			 mqtt_msg_tbl_presvd.tbl_row[i].retain,
			 _mq_msg_tbl_presvd_state_2_string(mqtt_msg_tbl_presvd.tbl_row[i].state),
			 mqtt_msg_tbl_presvd.tbl_row[i].timestamp,
			 mqtt_msg_tbl_presvd.tbl_row[i].qos,
			 mqtt_msg_tbl_presvd.tbl_row[i].topic,
			 mqtt_msg_tbl_presvd.tbl_row[i].payload);		
	}

	return;
}

static void _mq_msg_tbl_presvd_sync_to_rtm(int msg_idx, char* action)
{
	DA16X_UNUSED_ARG(action);

	_mq_msg_tbl_presvd_copy_to_rtm();
	
	MQTT_DBG_INFO(YELLOW_COLOR "[%s] %s successful! \n" CLEAR_COLOR, __func__, action);
	if(msg_idx != -1) _mq_msg_tbl_presvd_print_row(msg_idx);
}

static void _cmd_mq_msg_tbl_test_parse_hdr_param(char* params, int* magic_code,
									mq_msg_tbl_hdr_t* hdr, uint8_t* set_flag)
{
	uint8_t s_flg = 0;

	char* pair, *key, *value = NULL;
	char* cpy_param = NULL;
		
	cpy_param = _mosquitto_malloc(strlen(params)+1);
	memset(cpy_param, 0x00, strlen(params)+1);
	strcpy(cpy_param, params);
	
	// parse in pair
	pair = strtok(cpy_param, "=");
	while(pair != NULL) {
		PRINTF("key=%s\n", pair);
		key = pair;
		
		value = pair = strtok(NULL, ",");
		PRINTF("value=%s\n", pair);

		_mq_msg_tbl_set_hdr_value(key, value, hdr, magic_code, &s_flg);

		pair = strtok(NULL, "=");
	}
	_mosquitto_free(cpy_param);

	*set_flag = s_flg;
}

static void _cmd_mq_msg_tbl_test_parse_row_param(char* params,
									mq_msg_tbl_row_t* row, uint16_t* set_flag)
{
	uint16_t s_flg = 0;
	
	char* pair, *key, *value = NULL;
	char* cpy_param = NULL;
		
	cpy_param = _mosquitto_malloc(strlen(params)+1);
	memset(cpy_param, 0x00, strlen(params)+1);
	strcpy(cpy_param, params);
	
	// parse in pair
	pair = strtok(cpy_param, "=");
	while(pair != NULL) {
		PRINTF("key=%s\n", pair);
		key = pair;
		
		value = pair = strtok(NULL, ",");
		PRINTF("value=%s\n", pair);

		_mq_msg_tbl_set_row_value(key, value, row, &s_flg);

		pair = strtok(NULL, "=");
	}
	_mosquitto_free(cpy_param);

	*set_flag = s_flg;
}


int mq_msg_tbl_presvd_is_enabled(void)
{
	return mq_msg_tbl_presvd_enabled;
}

// initialize preserved mqtt message table
int mq_msg_tbl_presvd_init(struct mosq_config* mqtt_cfg, struct mosquitto* mosq) 
{
	_mq_msg_tbl_presvd_mutex_take();

	if (mosq->clean_session == false && mqtt_cfg->qos > 0) {
		_mq_msg_tbl_presvd_restore(mqtt_cfg, mosq);			
		mq_msg_tbl_presvd_enabled = true;
	} else {
		/* cs==0 && qos == 0 || cs==1 */
		_mq_msg_tbl_presvd_disable();
		mq_msg_tbl_presvd_enabled = false;
	}

	if (mq_msg_tbl_presvd_enabled)
		MQTT_DBG_PRINT(YELLOW_COLOR "  MQTT CleanSession=0 Support Mode enabled. \n" CLEAR_COLOR);

	_mq_msg_tbl_presvd_mutex_give();
	return MOSQ_ERR_SUCCESS;
}

// add a Msg to preserved mqtt message table
int mq_msg_tbl_presvd_add(struct mosquitto_message_all* msg, 
								enum mosquitto_msg_direction dir) 
{
	int ret = MOSQ_ERR_SUCCESS; // success
	int msg_idx;

	/* find an empty slot -> add to table */

	if (mq_msg_tbl_presvd_enabled == false) return MOSQ_ERR_UNKNOWN;

	_mq_msg_tbl_presvd_mutex_take();
	
	msg_idx = _mq_msg_tbl_presvd_find_empty_slot();
	if (msg_idx != -1) {	
		if (msg->msg.payloadlen > MQTT_MSG_TBL_PRESVD_MAX_PLAYLOAD_LEN) {
			ret = MOSQ_ERR_PAYLOAD_SIZE;
			MQTT_DBG_INFO(YELLOW_COLOR "[%s] payload to preserve exceeds the limit (> %d) !! \n" CLEAR_COLOR, 
				MQTT_MSG_TBL_PRESVD_MAX_PLAYLOAD_LEN, __func__);						
		} else {
			// add to table
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].dir = dir;
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].dup = msg->dup;
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].mid = msg->msg.mid;
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].qos = msg->msg.qos;
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].retain = msg->msg.retain;
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].state = msg->state;
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].timestamp = msg->timestamp;
			
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic_len = strlen(msg->msg.topic);
			memset(mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic, 0x00, MQTT_TOPIC_MAX_LEN+1);
			strcpy(mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic, msg->msg.topic);

			mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload_len = msg->msg.payloadlen;
			memset(mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload, 0x00, MQTT_MSG_TBL_PRESVD_MAX_PLAYLOAD_LEN+1);
			memcpy(mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload, msg->msg.payload, msg->msg.payloadlen);
			
			mqtt_msg_tbl_presvd.tbl_row[msg_idx].is_valid = true;

			if (dir == mosq_md_in)
				mqtt_msg_tbl_presvd.tbl_hdr.total_rx_msg_cnt++;
			else
				mqtt_msg_tbl_presvd.tbl_hdr.total_tx_msg_cnt++;

			_mq_msg_tbl_presvd_sync_to_rtm(msg_idx, "Add");
		}
	} else {
		ret = MOSQ_ERR_NOMEM;
		MQTT_DBG_INFO(YELLOW_COLOR "[%s] no empty slot !! \n" CLEAR_COLOR, __func__);
	}
	
	_mq_msg_tbl_presvd_mutex_give();
	
	return ret;
}

// delete a Msg in preserved mqtt message table
int mq_msg_tbl_presvd_del(struct mosquitto_message_all * msg, 
								enum mosquitto_msg_direction dir)
{
	int ret = MOSQ_ERR_SUCCESS;
	int msg_idx;

	/*
		find the message matched
		delete it
		delete it in rtm

	*/

	if (mq_msg_tbl_presvd_enabled == false) return MOSQ_ERR_UNKNOWN;

	_mq_msg_tbl_presvd_mutex_take();
	
	msg_idx = _mq_msg_tbl_presvd_find(dir, msg->msg.mid);
	if (msg_idx != -1) {
		mqtt_msg_tbl_presvd.tbl_row[msg_idx].is_valid = false; // deleted !
		
		if (dir == mosq_md_in)
			mqtt_msg_tbl_presvd.tbl_hdr.total_rx_msg_cnt--;
		else
			mqtt_msg_tbl_presvd.tbl_hdr.total_tx_msg_cnt--;
		
		_mq_msg_tbl_presvd_sync_to_rtm(msg_idx, "Delete");
	} else {
		ret = MOSQ_ERR_NOT_FOUND;
		MQTT_DBG_INFO(YELLOW_COLOR "[%s] a msg to delete not found, proceed ~ \n" CLEAR_COLOR, __func__);
	}

	_mq_msg_tbl_presvd_mutex_give();

	return ret;
}

// update the status of a Msg in preserved mqtt message table
int mq_msg_tbl_presvd_update(enum mosquitto_msg_direction dir, uint16_t mid, 
									enum mosquitto_msg_state state, da16x_time_t timestamp)
{
	int ret = MOSQ_ERR_SUCCESS;
	int msg_idx;

	/*
			find msg
			update msg
	*/

	if (mq_msg_tbl_presvd_enabled == false) return MOSQ_ERR_UNKNOWN;

	_mq_msg_tbl_presvd_mutex_take();

	msg_idx = _mq_msg_tbl_presvd_find(dir, mid);
	if (msg_idx != -1) {
		mqtt_msg_tbl_presvd.tbl_row[msg_idx].state = state;
		mqtt_msg_tbl_presvd.tbl_row[msg_idx].timestamp = timestamp;
		_mq_msg_tbl_presvd_sync_to_rtm(msg_idx, "Update");		
	} else {
		ret = MOSQ_ERR_NOT_FOUND;
		MQTT_DBG_INFO(YELLOW_COLOR "[%s] a msg to update not found, proceed ~ \n" CLEAR_COLOR, __func__);
	}

	_mq_msg_tbl_presvd_mutex_give();

	return ret;
}

int mq_msg_tbl_presvd_update_last_retry_chk(da16x_time_t timestamp)
{
	if (mq_msg_tbl_presvd_enabled == false) return MOSQ_ERR_UNKNOWN;

	_mq_msg_tbl_presvd_mutex_take();
	
	mqtt_msg_tbl_presvd.tbl_hdr.last_retry_chk_time = timestamp;
	// _mq_msg_tbl_presvd_copy_to_rtm(); // do this after break from mosquitto_loop_forever()

	_mq_msg_tbl_presvd_mutex_give();

	//MQTT_DBG_INFO("[%s] done \n", __func__);
	
	return MOSQ_ERR_SUCCESS;
}

int mq_msg_tbl_presvd_update_last_mid(uint16_t mid)
{
	if (mq_msg_tbl_presvd_enabled == false) return MOSQ_ERR_UNKNOWN;

	_mq_msg_tbl_presvd_mutex_take();
	
	mqtt_msg_tbl_presvd.tbl_hdr.last_tx_mid_used = mid;
	_mq_msg_tbl_presvd_copy_to_rtm();

	_mq_msg_tbl_presvd_mutex_give();
	
	MQTT_DBG_INFO(YELLOW_COLOR "[%s] done \n" CLEAR_COLOR, __func__);
	
	return MOSQ_ERR_SUCCESS;
}

// check if table is fully occupied
int mq_msg_tbl_presvd_is_full(void)
{
	return (_mq_msg_tbl_presvd_find_empty_slot() == -1)?true:false;
}

// modify row arbitrarily (debug purpose)
void mq_msg_tbl_presvd_modify_row(int msg_idx, mq_msg_tbl_row_t* msg_to_set)
{
	_mq_msg_tbl_presvd_mutex_take();

	memcpy(&(mqtt_msg_tbl_presvd.tbl_row[msg_idx]), msg_to_set, sizeof(mq_msg_tbl_row_t));
	_mq_msg_tbl_presvd_sync_to_rtm(-1, "modify arbitrarily");

	_mq_msg_tbl_presvd_mutex_give();
	
	return;
}

// dump table data from sram to rtm
int mq_msg_tbl_presvd_sync(void) 
{
	if (mq_msg_tbl_presvd_enabled == false) return MOSQ_ERR_UNKNOWN;

	_mq_msg_tbl_presvd_mutex_take();
	if (mqtt_msg_tbl_presvd_rtm) { 
		memcpy(mqtt_msg_tbl_presvd_rtm, &mqtt_msg_tbl_presvd, sizeof(mq_msg_tbl_presvd_t));
	}
	_mq_msg_tbl_presvd_mutex_give();
	
	return true;
}

void cmd_mq_msg_tbl_help(void)
{
	PRINTF("- mq_tbl\n\n");
	PRINTF("  Usage : mq_tbl [option]\n");
	PRINTF("	 option\n");
	PRINTF("\t<status>\t\t: show mqtt preserved msg table in detail\n");
	PRINTF("\t<mod> <hdr> <last_retry_chk_time=time,last_tx_mid_used=3,...>\n\t        \t\t: modify mqtt preserved msg table header directly\n");
	PRINTF("\t<mod> <row> <idx> <dir=0,mid=3,....>\n\t        \t\t: modify mqtt preserved msg table row directly\n");
	PRINTF("\t<del> <msg_idx>\t\t: delete the specified row\n");
}

void cmd_mq_emul_help(void)
{
	PRINTF("- mq_emul\n\n");
	PRINTF("  Usage : mq_emul [option]\n");
	PRINTF("	 option\n");
	PRINTF("\t<q2_rx> <no_pubrec_tx>\n\t\t: For PUB RX (qos=2). Disable pubrec tx. Undo when when mqtt restart\n");
	PRINTF("\t<q2_rx> <discard_pubrel_rx>\n\t\t: For PUB RX (qos=2). Discard pubrel rx. Undo when when mqtt restart\n");
	PRINTF("\t<q2_rx> <no_pubcomp_tx>\n\t\t: For PUB RX (qos=2). Disable pubcomp tx. Undo when when mqtt restart\n");
	PRINTF("\t<q1_tx> <discard_puback_rx>\n\t\t: For PUB TX (qos=1). Discard puback rx. Undo when when mqtt restart\n");
	PRINTF("\t<q2_tx> <discard_pubrec_rx>\n\t\t: For PUB TX (qos=2). Discard pubrec rx. Undo when when mqtt restart\n");
	PRINTF("\t<q2_tx> <no_pubrel_tx>\n\t\t: For PUB TX (qos=2). Disable pubrel tx. Undo when when mqtt restart\n");
}

void cmd_mq_msg_tbl_test(int argc, char *argv[])
{
	if (strcmp(argv[0], "mq_tbl") == 0) {
		
		if (strcmp(argv[1], "status") == 0) {
			/* mq_tbl <status> */
			_mq_msg_tbl_presvd_get_status();
		} else if (strcmp(argv[1], "mod") == 0) {
			/* 
				modify

				argc=4
				argv[0] argv[1] argv[2] argv[3]  <argv[4]>
				mq_tbl  <mod>   <hdr>   <last_retry_chk_time=time,last_tx_mid_used=3,....>
				mq_tbl	<mod>	<row>	<idx>    <dir=0,mid=3,....>		

				note: just for test purpose only thru console
					  for payload and topic, only string type should be used in console.
					  delimiter charater should not be used in topic and payload like
					  comma (,) and equal(=)
			*/

			if (strcmp(argv[2], "hdr") == 0 && argc == 4) {
				
				int magic_code;	mq_msg_tbl_hdr_t hdr; uint8_t set_flag = 0;
				
				// parse key=value pair argv[3]
				_cmd_mq_msg_tbl_test_parse_hdr_param(argv[3], &magic_code, &hdr, &set_flag);

				_mq_msg_tbl_presvd_mutex_take();
				// apply to table
				if (set_flag & SET_FLAG_MSK_MAGIC_CODE)
					mqtt_msg_tbl_presvd.magic_code = magic_code;
				
				if (set_flag & SET_FLAG_MSK_HDR_TOTAL_RX_MSG_CNT)
					mqtt_msg_tbl_presvd.tbl_hdr.total_rx_msg_cnt = hdr.total_rx_msg_cnt;
				
				if (set_flag & SET_FLAG_MSK_HDR_TOTAL_TX_MSG_CNT)
					mqtt_msg_tbl_presvd.tbl_hdr.total_tx_msg_cnt = hdr.total_tx_msg_cnt;

				if (set_flag & SET_FLAG_MSK_HDR_LAST_RETRY_CHK_TIME)
					mqtt_msg_tbl_presvd.tbl_hdr.last_retry_chk_time = hdr.last_retry_chk_time;

				if (set_flag & SET_FLAG_MSK_HDR_LAST_TX_MID_USED)
					mqtt_msg_tbl_presvd.tbl_hdr.last_tx_mid_used = hdr.last_tx_mid_used;

				_mq_msg_tbl_presvd_mutex_give();
			} else if (strcmp(argv[2], "row") == 0 && argc == 5) {
				mq_msg_tbl_row_t row; 
				uint16_t set_flag = 0;
				int msg_idx = atoi(argv[3]);

				// parse key=value pair argv[3]
				_cmd_mq_msg_tbl_test_parse_row_param(argv[4], &row, &set_flag);

				_mq_msg_tbl_presvd_mutex_take();
				// apply to table
				if (set_flag & SET_FLAG_MSK_ROW_MID)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].mid = row.mid;
				
				if (set_flag & SET_FLAG_MSK_ROW_PAYLOAD_LEN)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload_len = row.payload_len;
				
				if (set_flag & SET_FLAG_MSK_ROW_TOPIC_LEN)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic_len = row.topic_len;
				
				if (set_flag & SET_FLAG_MSK_ROW_QOS)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].qos = row.qos;
				
				if (set_flag & SET_FLAG_MSK_ROW_STATE)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].state = row.state;
				
				if (set_flag & SET_FLAG_MSK_ROW_DIR)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].dir = row.dir;
				
				if (set_flag & SET_FLAG_MSK_ROW_TIMESTAMP)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].timestamp = row.timestamp;
				
				if (set_flag & SET_FLAG_MSK_ROW_DUP)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].dup = row.dup;
				
				if (set_flag & SET_FLAG_MSK_ROW_RETAIN)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].retain = row.retain;

				if (set_flag & SET_FLAG_MSK_ROW_IS_VALID)
					mqtt_msg_tbl_presvd.tbl_row[msg_idx].is_valid = row.is_valid;

				if (set_flag & SET_FLAG_MSK_ROW_PAYLOAD)
					strcpy(mqtt_msg_tbl_presvd.tbl_row[msg_idx].payload, row.payload);
				
				if (set_flag & SET_FLAG_MSK_ROW_TOPIC)
					strcpy(mqtt_msg_tbl_presvd.tbl_row[msg_idx].topic, row.topic);

				_mq_msg_tbl_presvd_mutex_give();
			} else {
				PRINTF("Invalid command\n");
			}
						
		} else if (strcmp(argv[1], "del") == 0) {
			/* 
				delete

				argv[0]  argv[1] argv[2]
				mq_tbl   <del>   <msg_idx>
			*/
			_mq_msg_tbl_presvd_mutex_take();
			mqtt_msg_tbl_presvd.tbl_row[atoi(argv[2])].is_valid = false;
			_mq_msg_tbl_presvd_mutex_give();
			
		} else if (strcmp(argv[1], "add") == 0) {
		/*
				add a msg
					this cmd is just for debug purpose. without adding using mqtt send api
					wrong mqtt client state is expected. care must be taken.
			
		argv[0]  argv[1] argv[2]	3		4	 5		6			7			8    9
		mq_tbl   <del>   <msg_idx> <dir> <dup> <topic> <payload> <layload_len> <qos> <state>
		*/
			
			mq_msg_tbl_row_t* row;
			row = _mosquitto_malloc(sizeof(mq_msg_tbl_row_t));
			if (row == NULL) {
				PRINTF("MEM alloc fail !!!\n");
				return;
			}
			memset(row, 0x00, sizeof(mq_msg_tbl_row_t));
			row->dir = (enum mosquitto_msg_direction)atoi(argv[3]);
			row->dup = strtoul(argv[4], 0, 10);
			row->is_valid = 1;
			row->mid = mqttParams.pub_msg_id++;
			strcpy(row->payload, argv[6]);
			row->payload_len = atoi(argv[7]);
			row->qos = atoi(argv[8]);
			row->retain = 0;
			row->state = (enum mosquitto_msg_state)atoi(argv[9]);
			row->timestamp = mosquitto_time();
			strcpy(row->topic, argv[5]);
			row->topic_len = strlen(argv[5]);

			_mq_msg_tbl_presvd_mutex_take();
			memcpy((mq_msg_tbl_row_t*)&(mqtt_msg_tbl_presvd.tbl_row[atoi(argv[2])]), 
					row, sizeof(mq_msg_tbl_row_t));
			_mq_msg_tbl_presvd_mutex_give();

			_mosquitto_free(row);
		}else {
			cmd_mq_msg_tbl_help();
		}
	} 
#if defined (__MQTT_EMUL_CMD__)
		else if (strcmp(argv[0], "mq_emul") == 0) {
		extern struct mosquitto *mosq_sub;

		if (!mosq_sub || mosq_sub->state != mosq_cs_connected) {
			MQTT_DBG_ERR(RED_COLOR "mqtt client should be in connected state before this command is run \n" CLEAR_COLOR);
			return;
		}

		if (argc != 3) goto mq_emul_help;
		
		if (strcmp(argv[1], "q2_rx") == 0) {
			if (strcmp(argv[2], "no_pubrec_tx") == 0) {
				
				/* mq_emul <q2_rx> <no_pubrec_tx> : For PUB RX (qos=2). Disable pubrec tx. Undo when when mqtt restart */
				mqttParams.q2_rx_no_pubrec_tx = 1;
				if (rtmMqttParamPtr) rtmMqttParamPtr->q2_rx_no_pubrec_tx = 1;
				
			} else if (strcmp(argv[2], "discard_pubrel_rx") == 0) {
				
				/*mq_emul <q2_rx> <discard_pubrel_rx> 	For PUB RX (qos=2). Discard pubrel rx. Undo when when mqtt restart	*/
				mqttParams.q2_rx_discard_pubrel_rx = 1;
				if (rtmMqttParamPtr) rtmMqttParamPtr->q2_rx_discard_pubrel_rx = 1;

			} else if (strcmp(argv[2], "no_pubcomp_tx") == 0) {
				
				/* mq_emul <q2_rx> <no_pubcomp_tx> 		For PUB RX (qos=2). Disable pubcomp tx. Undo when when mqtt restart */
				mqttParams.q2_rx_no_pubcomp_tx = 1;
				if (rtmMqttParamPtr) rtmMqttParamPtr->q2_rx_no_pubcomp_tx = 1;
				
			} else {
				goto mq_emul_help;
			}
		} else if (strcmp(argv[1], "q2_tx") == 0) {

			if (strcmp(argv[2], "discard_pubrec_rx") == 0) {
				
				/* mq_emul <q2_tx> <discard_pubrec_rx> 	For PUB TX (qos=2). Discard pubrec rx. Undo when when mqtt restart */
				mqttParams.q2_tx_discard_pubrec_rx = 1;
				if (rtmMqttParamPtr) rtmMqttParamPtr->q2_tx_discard_pubrec_rx = 1;

			} else if (strcmp(argv[2], "no_pubrel_tx") == 0) {
				
				/* mq_emul <q2_tx> <no_pubrel_tx>			For PUB TX (qos=2). Disable pubrel tx. Undo when when mqtt restart */
				mqttParams.q2_tx_no_pubrel_tx = 1;
				if (rtmMqttParamPtr) rtmMqttParamPtr->q2_tx_no_pubrel_tx = 1;
				
			} else {
				goto mq_emul_help;
			}			
		} else if (strcmp(argv[1], "q1_tx") == 0 && 
					strcmp(argv[2], "discard_puback_rx") == 0) {			
			/* mq_emul <q1_tx> <discard_puback_rx> 	For PUB TX (qos=1). Discard puback rx. Undo when when mqtt restart */

				mqttParams.q1_tx_discard_puback_rx = 1;
				if (rtmMqttParamPtr) rtmMqttParamPtr->q1_tx_discard_puback_rx = 1;
				
		} else {
mq_emul_help:
			cmd_mq_emul_help();
		}
		
	} 
#endif // __MQTT_EMUL_CMD__
		else {
		PRINTF("Invalid command\n");
	}

}

#if defined (__MQTT_EMUL_CMD__)
void cmd_mq_msg_tbl_test_mq_emul_param_clr(void)
{
	mqttParamForRtm* params;
	int i;
	
	params = (mqttParamForRtm*)&mqttParams;
	for(i = 0; i < 2; i++) {
		if (params) {
			params->q2_rx_no_pubrec_tx		= 0;
			params->q2_rx_discard_pubrel_rx	= 0;
			params->q2_rx_no_pubcomp_tx		= 0;
			params->q1_tx_discard_puback_rx	= 0;
			params->q2_tx_discard_pubrec_rx	= 0;
			params->q2_tx_no_pubrel_tx		= 0;
		}
		params = rtmMqttParamPtr;
	}
	PRINTF("mq_emul: params all cleared \n");
}
#endif // __MQTT_EMUL_CMD__
#endif // (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)

/* EOF */
