/**
 ****************************************************************************************
 *
 * @file mqtt_msg_tbl_presvd.h
 *
 * @brief Header file - MQTT Preserved Message table to support CleanSession=0
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

#ifndef	__MQTT_MSG_TBL_PRESVD_H__
#define	__MQTT_MSG_TBL_PRESVD_H__

#include "mqtt_client.h"
#include "da16x_time.h"
#include "mosquitto_internal.h"
#include "memory_mosq.h"

#define MQTT_MSG_TBL_PRESVD_NAME 				"MQ_MSG_TBL"

#define SET_FLAG_MSK_MAGIC_CODE					0x01
#define SET_FLAG_MSK_HDR_TOTAL_RX_MSG_CNT		0x02
#define SET_FLAG_MSK_HDR_TOTAL_TX_MSG_CNT		0x04
#define SET_FLAG_MSK_HDR_LAST_RETRY_CHK_TIME	0x08
#define SET_FLAG_MSK_HDR_LAST_TX_MID_USED		0x10

#define SET_FLAG_MSK_ROW_MID			0x01
#define SET_FLAG_MSK_ROW_PAYLOAD		0x02
#define SET_FLAG_MSK_ROW_PAYLOAD_LEN	0x04
#define SET_FLAG_MSK_ROW_TOPIC			0x08
#define SET_FLAG_MSK_ROW_TOPIC_LEN		0x10
#define SET_FLAG_MSK_ROW_QOS			0x20
#define SET_FLAG_MSK_ROW_STATE			0x40
#define SET_FLAG_MSK_ROW_DIR			0x80
#define SET_FLAG_MSK_ROW_TIMESTAMP		0x100
#define SET_FLAG_MSK_ROW_DUP			0x200
#define SET_FLAG_MSK_ROW_RETAIN			0x400
#define SET_FLAG_MSK_ROW_IS_VALID		0x800

typedef struct {
	int total_rx_msg_cnt;
	int total_tx_msg_cnt;
	da16x_time_t last_retry_chk_time;
	uint16_t last_tx_mid_used;
} mq_msg_tbl_hdr_t;

typedef struct {
	uint16_t 						mid;
	char 							payload[MQTT_MSG_TBL_PRESVD_MAX_PLAYLOAD_LEN+1];
	int								payload_len;
	char 							topic[MQTT_TOPIC_MAX_LEN+1];
	int								topic_len;
	int 							qos;	// 1(qos1), 2(qos2)
	enum mosquitto_msg_state 		state;
	enum mosquitto_msg_direction	dir;
	da16x_time_t 					timestamp;
	bool 							dup;
	bool							retain;
	uint8_t							is_valid;
} mq_msg_tbl_row_t;

typedef struct {
	int					magic_code;
	mq_msg_tbl_hdr_t 	tbl_hdr;
	mq_msg_tbl_row_t 	tbl_row[MQTT_MSG_TBL_PRESVD_MAX_MSG_CNT];
} mq_msg_tbl_presvd_t;

int mq_msg_tbl_presvd_init(struct mosq_config* mqtt_cfg, struct mosquitto* mosq);

int mq_msg_tbl_presvd_add(struct mosquitto_message_all* msg, 
								enum mosquitto_msg_direction dir);

int mq_msg_tbl_presvd_del(struct mosquitto_message_all * msg, 
							enum mosquitto_msg_direction dir);

int mq_msg_tbl_presvd_update(enum mosquitto_msg_direction dir, uint16_t mid, 
									enum mosquitto_msg_state state, da16x_time_t timestamp);

int mq_msg_tbl_presvd_update_last_retry_chk(da16x_time_t timestamp);

int mq_msg_tbl_presvd_update_last_mid(uint16_t mid);

int mq_msg_tbl_presvd_is_full(void);

int mq_msg_tbl_presvd_is_enabled(void);

int mq_msg_tbl_presvd_init_is_done(void);

void mq_msg_tbl_presvd_init_done(void);

void mq_msg_tbl_presvd_modify_row(int msg_idx, mq_msg_tbl_row_t* msg_to_set);

int mq_msg_tbl_presvd_sync(void);

void cmd_mq_msg_tbl_test(int argc, char *argv[]);
void cmd_mq_msg_tbl_test_mq_emul_param_clr(void);

#endif // __MQTT_MSG_TBL_PRESVD_H__

/* EOF */
