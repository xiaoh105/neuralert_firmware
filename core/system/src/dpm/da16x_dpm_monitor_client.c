/**
 ****************************************************************************************
 *
 * @file da16x_dpm_monitor_client.c
 *
 * @brief DPM monitor client
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

#ifdef	__DA16X_DPM_MON_CLIENT__

#include "da16x_dpm.h"
#include "common_def.h"
#include "iface_defs.h"

#include "driver.h"

#include <stdio.h>
#include "application.h"
#include "nvedit.h"
#include "environ.h"
#include "user_dpm.h"
#include "common/defs.h"
#include "da16x_dpm_filter.h"
#include "os_con.h"
#include "crypto/dtls.h"
#include "utils/wpabuf.h"
#include "tls/dtlsv1_record_pool.h"
#include "command_net.h"
#include "da16x_time.h"
#include "json.h"

#undef	USE_DTLS
#undef	USE_DOMAIN
#undef	WAITING_FOR_RECEIVE
#undef	USE_MALLOC

#define	MAX_DATA_SIZE		512
#define	MAX_PARA_STRING		30

#define	NVR_KEY_MON_SVR_DOMAIN	"mon_svc_domain"
#define	NVR_KEY_MON_SVR_IP	"mon_svc_svr_ip"
#define	NVR_KEY_MON_SERVICE	"mon_service"

#define	MON_SVC_INFO		"mon_svc_info"
#define	MON_SVC_CLIENT_SOCKET	"MON_SVC_CLIENT_SKT"

/* CONFIGURATION - TEMPORARY */
#define	DFLT_MON_SERVICE	0
#define	DFLT_MON_SVR_PERIOD	0		/* SEC */
#define	DFLT_MON_SVR_DOMAIN	"a.a.a.a"	/* TEMPORARY */
#define	DFLT_MON_SERVER_IP	"10.20.30.40"	/* TEMPORARY */

#ifdef USE_DTLS
#define	MON_SVC_DTLS_CONN	"mon_svc_dtls_conn"
#define	MON_SVC_DTLS_RECORD	"mon_svc_dtls_record"
#endif /* USE_DTLS */

#define	MON_HANDSHAKE_MAX_RETRY	3
const UCHAR *MON_PSK_ID = "Client_identity";
const UCHAR *MON_PSK_KEY = "secretPSK";

typedef struct MON_jsonData {
	unsigned int	func;
	unsigned int 	statusCode;
	unsigned int 	para0;
	unsigned int 	para1;
	unsigned int 	para2;
	unsigned int 	para3;
	unsigned int 	para4;
	char 	paraStr[MAX_PARA_STRING];
	char		accessMethod[16];
} MON_jsonData_t;

MON_jsonData_t *MON_json_data;

typedef struct MON_rtm_info {
	ULONG	monService;
	ULONG	myPortNo;
	UINT	sendCommandDataNo;
	ULONG	recvCommandDataNo;
	char	svrIP[40];
	char	svrDomain[40];
	ULONG	totalRunCount;
	ULONG	currRunTick;
	ULONG	totalRunTick;
} MON_rtm_info_t;

#ifdef USE_DTLS
typedef struct MON_secure_config {
	void	*ssl_ctx;
	struct	dtls_connection *dtls_conn;
	struct	dtls_connection_params dtls_params;
} MON_secure_config_t;
#endif	/* USE_DTLS */

#undef	DBG_PRINT_INFO
#define	DBG_PRINT_ERR
#define	DBG_PRINT_TEMP
#undef	DETAIL_LOG

extern	int		check_net_ip_status(int iface);
extern	long	iptolong(char *ip);
extern	int		read_nvram_int(const char *name, int *_val);
extern	char	*read_nvram_string(const char *name);
extern	unsigned long	os_random(void);
extern	char	*dns_A_Query(char* domain_name, ULONG wait_option);
extern	int		set_dpm_init_done(char *mod_name);
extern	bool	confirm_dpm_init_done(char *mod_name);
extern	int		interface_select;
extern	user_dpm_supp_net_info_t *dpm_netinfo;
extern	int		get_run_mode(void);
extern	unsigned long	da16x_get_wakeup_source();


NX_PACKET_POOL	*MON_svcPool;
NX_UDP_SOCKET	monClientSocket;
ULONG		MON_svcServerIp;

UINT	MON_Service;
UINT	MON_statusCode;
UINT	MON_myPortNo;
UINT	MON_sendCommandDataNo = 0;
UINT	MON_recvCommandDataNo;
ULONG	MON_totalRunCount;
ULONG	MON_totalRunTick;
ULONG	MON_currRunTick;
UCHAR	MON_svrDomain[40];
CHAR	MON_svrIp[40];

#ifndef	USE_MALLOC
char	MON_json_data_buf[MAX_DATA_SIZE];
char	MON_sendPacketStr_buf[MAX_DATA_SIZE];
char	MON_recvPacketStr_buf[MAX_DATA_SIZE];
#endif

NX_UDP_SOCKET	*monClientSocketPtr;
char	*MON_sendPacketStr;
char	*MON_recvPacketStr;

#ifdef USE_DTLS
MON_secure_config_t monSecureConf;

UINT MON_init_secure(MON_secure_config_t *config);
void MON_deinit_secure(MON_secure_config_t *config);
UINT MON_shutdown_secure(MON_secure_config_t *config);
UINT MON_do_handshake(MON_secure_config_t *config);
UINT MON_send_handshake_msg(void *ssl_ctx, struct dtls_connection *dtls_conn, struct dtlsv1_record_pool *pool);

UINT MON_encrypt_packet(void *ssl_ctx, struct dtls_connection *dtls_conn, unsigned char *in_data, size_t in_len, NX_PACKET **out_data);
UINT MON_decrypt_packet(void *ssl_ctx, struct dtls_connection *dtls_conn, NX_PACKET *in_data, NX_PACKET **out_data);

UINT MON_save_secure(MON_secure_config_t *config);
UINT MON_restore_secure(MON_secure_config_t *config);
UINT MON_clear_secure(MON_secure_config_t *config);
#endif	/* USE_DTLS */

UINT MON_msgId_recv = 0;

#define	DPM_MON_CLIENT_STK_SIZE		1024		/* TEMPORARY */
#define	DPM_MON_CLIENT_PRIORITY		USER_PRI_APP(0)
#define	DPM_UDP_SVR_TEST_NAME		"DPM_UDP_RX"
#define	DPM_MON_CLIENT_NAME		"DPM_MON_CLIENT"

TX_THREAD	dpm_mon_client;
char	dpm_mon_client_stack[DPM_MON_CLIENT_STK_SIZE];

#define DA16X_MON_QUEUE_SIZE	24
#define	DA16X_MON_TX_QSIZE	_1_ULONG * 4 * DA16X_MON_QUEUE_SIZE

TX_QUEUE	mon_ts_msg_tx_queue;

typedef struct mon_tx_buf {
	UINT	status_code;
	UINT	para0;
	UINT	para1;
	UINT	para2;
	UINT	para3;
	UINT	para4;
	char	paraStr[MAX_PARA_STRING];
} mon_tx_buf_t;

typedef struct mon_tx_msg {
	mon_tx_buf_t	*mon_tx_buf;
} mon_tx_msg_t;

mon_tx_msg_t	*mon_rx_msg_p;
mon_tx_msg_t	*mon_tx_msg_p;
UCHAR		*mon_tx_ts_msg_queue_buf;

mon_tx_buf_t	MON_RX_DATA_BUFFER;
mon_tx_buf_t	MON_TX_DATA_BUFFER;

mon_tx_buf_t	MON_DATA_BUF;
mon_tx_msg_t	MON_RX_MSG_BUF;
mon_tx_msg_t	MON_TX_MSG_BUF;

int MON_jsonGeneratorClient(char *buf, MON_jsonData_t *json_input)
{
	cJSON *root;
	char json_ver[3];
	char json_msgType[2];
	char json_funcType[3];
	char json_sId[16];
	char json_tpId[32];
	char json_tId[32];
	char json_msgCode[16];
	char json_msgId[16];
	long long json_msgDate;
	char json_resCode[16];
	char json_resMsg[16];
	char json_dataFormat[64];
	char json_severity[2];
	char json_encType[2];
	char tmp_mac[18];

	cJSON *data;
	cJSON *arg_data;

	char *json_ret;

	if (json_input->func != FUNC_ERROR && json_input->func != FUNC_STATUS ) {
		PRINTF("[%s] Unknown message code (%d)\n", __func__, json_input->func);
		return -1;
	}

	strcpy(json_ver, "001");
	strcpy(json_sId, "SITEID0001");

#ifdef __TIME64__
	__time64_t now;
	da16x_time64(NULL, &now);
	json_msgDate = (long long)now;
#else
	time_t now;
	json_msgDate = (long long)da16x_time32(NULL);
#endif /* __TIME64__ */

	strcpy(json_dataFormat, "application/json");
	strcpy(json_severity, "0");
	strcpy(json_encType, "4");

	memset(json_tpId, 0, 32);
	memset(json_tId, 0, 32);
	memset(tmp_mac, 0, 18);

	getMACAddrStr(0, (char *)tmp_mac);
	sprintf(json_tpId, "DA16X.%s", tmp_mac);
	sprintf(json_tId, "%s", tmp_mac);

	/* JSON Generate */
	root = cJSON_CreateObject();
	data = cJSON_CreateObject();
	arg_data = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "version", json_ver);

	sprintf(json_msgCode, "MSGQ%d", json_input->func);
	strcpy(json_msgType, "Q");
	strcpy(json_funcType, "003");
	sprintf(json_msgId, "%d", ++MON_sendCommandDataNo);

	strcpy(json_resCode, "200");
	strcpy(json_resMsg, "OK");

	cJSON_AddNumberToObject(data, "func", json_input->func);
	cJSON_AddNumberToObject(arg_data, "statusCode", json_input->statusCode);
	cJSON_AddNumberToObject(arg_data, "para0", json_input->para0);
	cJSON_AddNumberToObject(arg_data, "para1", json_input->para1);
	cJSON_AddNumberToObject(arg_data, "para2", json_input->para2);
	cJSON_AddNumberToObject(arg_data, "para3", json_input->para3);
	cJSON_AddNumberToObject(arg_data, "para4", json_input->para4);
	cJSON_AddStringToObject(arg_data, "paraStr", json_input->paraStr);
	cJSON_AddItemToObject(data, "arg", arg_data);

	cJSON_AddStringToObject(root, "msgType", json_msgType);
	cJSON_AddStringToObject(root, "funcType", json_funcType);
	cJSON_AddStringToObject(root, "sId", json_sId);
	cJSON_AddStringToObject(root, "tpId", json_tpId);
	cJSON_AddStringToObject(root, "tId", json_tId);
	cJSON_AddStringToObject(root, "msgCode", json_msgCode);
	cJSON_AddStringToObject(root, "msgId", json_msgId);
	cJSON_AddNumberToObject(root, "msgDate", json_msgDate);
	cJSON_AddStringToObject(root, "resCode", json_resCode);
	cJSON_AddStringToObject(root, "resMsg", json_resMsg);
	cJSON_AddStringToObject(root, "dataFormat", json_dataFormat);
	cJSON_AddStringToObject(root, "severity", json_severity);
	cJSON_AddStringToObject(root, "encType", json_encType);
	cJSON_AddItemToObject(root, "data", data);

	/* JSON Object Print */
	json_ret = cJSON_Print(root);
	strcpy(buf, json_ret);

//	PRINTF("[%d] %s\n", strlen(buf), buf);

	/* Memory Free */
	vPortFree(json_ret);
	cJSON_Delete(root);

	return strlen(buf);
}

static  int     set_dpm_sleep_confirm(char *reg_name)
{
	UINT    dpm_retry_cnt = 0;
	int     dpm_sts = 0;

dpm_set_retry:
	if (dpm_retry_cnt++ < 5) {
		dpm_sts = set_dpm_sleep(reg_name);
		switch (dpm_sts) {
			case DPM_SET_ERR :
				PRINTF("[%s] set_dpm_sleep error!! (err=%d)\n", __func__, dpm_sts);
				tx_thread_sleep(1);
				goto dpm_set_retry;
				break;

			case DPM_SET_ERR_BLOCK :
				/* Don't need try continues */
				;
			case DPM_SET_OK :
				break;
		}
	}

	return dpm_sts;
}

static	unsigned int MON_makePortNo()
{
	UCHAR	tmp_str[12];
	UINT	port=0;
	int	i;

	getMACAddrStr(0, (char *)tmp_str);

	for(i=0;i<12;i++) {	/* make port number */
		if (i%2) {
			port += tmp_str[i]*16;
		}
		else {
			port += tmp_str[i];
		}
	}

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s] MAC Address = %c%c:%c%c:%c%c:%c%c:%c%c:%c%c \t port = %d \n",
			__func__,
			tmp_str[0], tmp_str[1],
			tmp_str[2], tmp_str[3],
			tmp_str[4], tmp_str[5],
			tmp_str[6], tmp_str[7],
			tmp_str[8], tmp_str[9],
			tmp_str[10], tmp_str[11],
			port);
#endif	/* DBG_PRINT_INFO */
	return (port+1);
}

static	UINT MON_SVC_receivePkt(NX_UDP_SOCKET *monClientSocketPtr,
				char *recvData,
				UINT *recvDataLen,
				UINT waitOption)
{
	UINT		status;
	NX_PACKET	*packet_ptr;
	ULONG		packetLen;

#ifdef	USE_DTLS
	void *ssl_ctx = monSecureConf.ssl_ctx;
	struct dtls_connection *dtls_conn = monSecureConf.dtls_conn;
	NX_PACKET *decrypted_packet_ptr = NULL;
#endif	/* USE_DTLS */

	status = nx_udp_socket_receive(monClientSocketPtr, &packet_ptr, waitOption);
	if (status == ER_NO_PACKET) {
#ifdef	DBG_PRINT_INFO
		PRINTF("%s:%d Timeout - MON_recv (status=0x%0x, waitoption=%d) \n",
					__func__, __LINE__, status, waitOption);
#endif
		return status;
	}
	else if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("%s:%d ERROR MON_recv (status=0x%0x) \n", __func__, __LINE__, status);
#endif
		if (chk_dpm_mode()) {
			clr_dpm_sleep(REG_NAME_MON_SERVICE);
		}

		return status;
	}

	if (chk_dpm_mode()) {
		clr_dpm_sleep(REG_NAME_MON_SERVICE);
	}

#ifdef USE_DTLS
	if (dtls_connection_established(ssl_ctx, dtls_conn)) {
		status = MON_decrypt_packet(ssl_ctx, dtls_conn,
				packet_ptr, &decrypted_packet_ptr);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to decrypt packet(0x%02x)\n",
				__func__, status);
#endif
			nx_packet_release(packet_ptr);
			return status;
		}

		nx_packet_release(packet_ptr);
		packet_ptr = decrypted_packet_ptr;
		decrypted_packet_ptr = NULL;
	}
#endif	/* USE_DTLS */

	nx_packet_data_retrieve(packet_ptr, MON_recvPacketStr, (unsigned long *)&packetLen);
	//PRINTF(" retrieve packetLen=%d nx_packet_length=%d\n", packetLen, packet_ptr->nx_packet_length);

	*recvDataLen = packetLen;

	nx_packet_release(packet_ptr);

	return status;
}

static	int MON_SVC_sendPkt(NX_UDP_SOCKET *monClientSocketPtr, void *sendPtr, ULONG packet_size)
{
	UINT		status = ERR_OK;
	NX_PACKET	*my_packet = NULL;

#ifdef USE_DTLS
	void *ssl_ctx = monSecureConf.ssl_ctx;
	struct dtls_connection *dtls_conn = monSecureConf.dtls_conn;

	if (dtls_connection_established(ssl_ctx, dtls_conn)) {
		status = MON_encrypt_packet(ssl_ctx, dtls_conn,
					sendPtr, packet_size,
					&my_packet);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[ERROR MON Client]Packet Encrypt(status=0x%02x)\n",
				status);
#endif
			return status;
		}
	}
	else {
#endif	/* USE_DTLS */
		/* Allocate a packet. */
		status = nx_packet_allocate(MON_svcPool,
				&my_packet,
				NX_UDP_PACKET,
				portMAX_DELAY);

		/* Check status. */
		if (status != ERR_OK) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[ERROR MON Client]Packet alloc(status=0x%02x)\n",
				status);
#endif
			return status;
		}

		/* Write ABCs into the packet payload! */
		/* Adjust the write pointer. */
		status = nx_packet_data_append(my_packet,
				sendPtr,
				packet_size,
				MON_svcPool,
				portMAX_DELAY);

		/* Check status. */
		if (status != ERR_OK) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[ERROR MON Client]Packet append(status=0x%02x)\n",
				status);
#endif
			return status;
		}
#ifdef USE_DTLS
	}
#endif	/* USE_DTLS */

	/* Send the packet out! */
	status = nx_udp_socket_send(monClientSocketPtr,
				my_packet,
				MON_svcServerIp,
				MON_SERVICE_PORT);

	/* Determine if the status is valid. */
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("ERROR MON Client MON_Send 0x%02x \n", status);
#endif
		nx_packet_release(my_packet);
		return status;
	}

	return status;
}


static void MON_packetPrint(char *buf, UINT send_recv)
{
#ifdef	DBG_PRINT_TEMP
	cJSON *json_recv_data = NULL;
	cJSON *child_object = NULL;
	char str_ret[64], str_msgCode[8];
	int num_ret = 0;

	if (chk_dpm_mode()) {
		set_dpm_rcv_ready(REG_NAME_MON_SERVICE);
	}

	PRINTF("%s Msg : ", send_recv ? " <<< Send " : " >>> Recv ");

	json_recv_data = cJSON_Parse(buf);

	memset(str_ret, 0, 64);
	child_object = cJSON_GetObjectItem(json_recv_data, "msgType");
	memcpy(str_ret, child_object->valuestring, 1);
	PRINTF("[%s] ", str_ret);

	memset(str_ret, 0, 64);
	child_object = cJSON_GetObjectItem(json_recv_data, "funcType");
	memcpy(str_ret, child_object->valuestring, 3);

	if (!strcmp(str_ret, "003" )) {
		PRINTF(CYAN_COLOR);
	}
	else if (!strcmp(str_ret, "004" )) {
		PRINTF(YELLOW_COLOR);
	}
	else if (!strcmp(str_ret, "005" )) {
		PRINTF(YELLOW_COLOR);
	}
	else if (!strcmp(str_ret, "006" )) {
		PRINTF(YELLOW_COLOR);
	}
	else {
		PRINTF(RED_COLOR);
	}

	PRINTF("Function Type: %s ", str_ret);

	memset(str_msgCode, 0, 64);
	child_object = cJSON_GetObjectItem(json_recv_data, "msgCode");
	memcpy(str_msgCode, child_object->valuestring, 7);
	PRINTF("[%s] ", str_msgCode);

	child_object = cJSON_GetObjectItem(json_recv_data, "msgId");
	num_ret = atoi(child_object->valuestring);
	PRINTF("msgID:%d ", num_ret);

	child_object = cJSON_GetObjectItem(json_recv_data, "msgDate");
	PRINTF("(%d) ", child_object->valueint);

	if (strcmp(str_msgCode, "MSGQ100") == 0) {
		child_object = cJSON_GetObjectItem(json_recv_data, "data");
		child_object = cJSON_GetObjectItem(child_object, "arg");
		child_object = cJSON_GetObjectItem(child_object, "statusCode");

		PRINTF("STATUS Info to Server (%d) ", child_object->valueint);
	}
	else if (strcmp(str_msgCode, "MSGQ200") == 0) {
		child_object = cJSON_GetObjectItem(json_recv_data, "data");
		child_object = cJSON_GetObjectItem(child_object, "arg");
		child_object = cJSON_GetObjectItem(child_object, "statusCode");

		PRINTF("STATUS Info to Server <%d> ", child_object->valueint);
	}
	else if (strcmp(str_msgCode, "MSGQ300") == 0) {
		child_object = cJSON_GetObjectItem(json_recv_data, "data");
		child_object = cJSON_GetObjectItem(child_object, "arg");
		child_object = cJSON_GetObjectItem(child_object, "statusCode");

		PRINTF("STATUS Info to Server <%d> ", child_object->valueint);
	}
	else if (strncmp(str_msgCode, "MSGA", 4) == 0) {
		PRINTF("No Data! (for ACK) ");
	}
	else {
		PRINTF(RED_COLOR "Unknown Packet (msgCode: %s) ", str_msgCode);
	}

	PRINTF(CLEAR_COLOR);
	PRINTF("\n");

	cJSON_Delete(json_recv_data);
#endif
	return;
}

static	MON_rtm_info_t *saveInfo_RTM()
{
	UINT status = ERR_OK;
	unsigned int len = 0;
	MON_rtm_info_t *data = NULL;
	const ULONG wait_option = 100;

	len = dpm_user_rtm_get(MON_SVC_INFO, (unsigned char **)&data);
	if (len == 0) {
		status = dpm_user_rtm_allocate(MON_SVC_INFO,
				(VOID **)&data,
				sizeof(MON_rtm_info_t),
				wait_option);
		if (status != ERR_OK) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s] failed to allocate memory\n", __func__);
#endif
			return NULL;
		}
	}
	else if (len != sizeof(MON_rtm_info_t)) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] invaild size(%ld)\n", __func__, len);
#endif
		return NULL;
	}

	return data;
}


static	MON_rtm_info_t *getSavedInfo_RTM()
{
	unsigned int len = 0;
	MON_rtm_info_t *data = NULL;

	len = dpm_user_rtm_get(MON_SVC_INFO, (unsigned char **)&data);
	if (len == sizeof(MON_rtm_info_t)) {
		return data;
	}
	else if (len == 0) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] failed to find sleep time\n", __func__);
#endif
	}
	else {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] invaild size(%ld)\n", __func__, len);
#endif
	}

	return NULL;
}

static	void saveMON_sendCommandDataNo_RTM(ULONG dataNo)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->sendCommandDataNo = dataNo;
	}

	return ;
}

static	void saveMON_service_RTM(ULONG monService)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->monService = monService;
	}

	return ;
}

static	void saveMON_myPortNo_RTM(ULONG myPortNo)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->myPortNo = myPortNo;
	}

	return ;
}

static	void saveMON_recvCommandDataNo_RTM(ULONG recvCommandDataNo)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->recvCommandDataNo = recvCommandDataNo;
	}

	return ;
}


static	void saveMON_svrIP_RTM(char *svrIP)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		strcpy(data->svrIP, svrIP);
	}

	return ;
}

static	void saveMON_svrDomain_RTM(char *svrDomain)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		strcpy(data->svrDomain, svrDomain);
	}

	return ;
}

static	void saveMON_totalRunTick_RTM(ULONG totalRunTick)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->totalRunTick = totalRunTick;
	}

	return ;
}

static	void saveMON_currRunTick_RTM(ULONG currRunTick)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->currRunTick = currRunTick;
	}

	return ;
}

static	void saveMON_totalRunCount_RTM(ULONG totalRunCount)
{
	MON_rtm_info_t *data = NULL;

	data = saveInfo_RTM();
	if (data != NULL) {
		data->totalRunCount = totalRunCount;
	}

	return ;
}

static	ULONG getMON_sendCommandDataNo_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->sendCommandDataNo;
	}

	return 0;
}

static	ULONG getMON_service_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->monService;
	}

	return 0;
}

static	ULONG getMON_myPortNo_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->myPortNo;
	}

	return 0;
}

static	ULONG getMON_recvCommandDataNo_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->recvCommandDataNo;
	}

	return 0;
}

static	char *getMON_svrIP_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->svrIP;
	}

	return 0;
}

static	char *getMON_svrDomain_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->svrDomain;
	}

	return 0;
}

static	ULONG getMON_totalRunCount_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->totalRunCount;
	}

	return 0;
}

static	ULONG getMON_currRunTick_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->currRunTick;
	}

	return 0;
}

static	ULONG getMON_totalRunTick_RTM()
{
	MON_rtm_info_t *data = NULL;

	data = getSavedInfo_RTM();
	if (data != NULL) {
		return data->totalRunTick;
	}

	return 0;
}

static void printInfoRTM(char *title)
{
#ifdef	DBG_PRINT_INFO
	PRINTF("%s\n"
		"\t>MON_Service:%d\n"
		"\t>MON_myPortNo:%d\n"
		"\t>MON_recvCommandDataNo:%d\n"
		"\t>MON_svrIp:%s\n"
		"\t>MON_svrDomain:%s\n",
		title,
		MON_Service,
		MON_myPortNo,
		MON_recvCommandDataNo,
		MON_svrIp,
		MON_svrDomain);
#endif
}

void clearStatusToMonitorServer()
{
	MON_totalRunCount = 0;
	MON_currRunTick = 0;
	MON_totalRunTick = 0;

	saveMON_totalRunCount_RTM(MON_totalRunCount);
	saveMON_currRunTick_RTM(MON_currRunTick);
	saveMON_totalRunTick_RTM(MON_totalRunTick);
}

void saveStatusToMonitorServer(ULONG curr_run_tick)
{
	MON_totalRunCount++;
	MON_currRunTick = curr_run_tick;
	MON_totalRunTick = MON_totalRunTick + curr_run_tick;

	saveMON_totalRunCount_RTM(MON_totalRunCount);
	saveMON_currRunTick_RTM(MON_currRunTick);
	saveMON_totalRunTick_RTM(MON_totalRunTick);
}

static	void mon_svc_save_all_rtm()
{
	saveMON_sendCommandDataNo_RTM(MON_sendCommandDataNo);
	saveMON_service_RTM(MON_Service);
	saveMON_myPortNo_RTM(MON_myPortNo);
	saveMON_recvCommandDataNo_RTM(MON_recvCommandDataNo);
	saveMON_svrIP_RTM((char *)MON_svrIp);
	saveMON_svrDomain_RTM((char *)MON_svrDomain);

	saveMON_totalRunCount_RTM(MON_totalRunCount);
	saveMON_currRunTick_RTM(MON_currRunTick);
	saveMON_totalRunTick_RTM(MON_totalRunTick);

	printInfoRTM("<SAVE INFO>");
}

static	void mon_svc_get_all_rtm()
{
	MON_sendCommandDataNo = getMON_sendCommandDataNo_RTM();
	MON_Service = getMON_service_RTM();
	MON_myPortNo = getMON_myPortNo_RTM();
	MON_recvCommandDataNo = getMON_recvCommandDataNo_RTM();

	MON_totalRunCount = getMON_totalRunCount_RTM();
	MON_currRunTick = getMON_currRunTick_RTM();
	MON_totalRunTick = getMON_totalRunTick_RTM();

	strcpy((char *)&MON_svrIp[0], (char *)getMON_svrIP_RTM());
	strcpy((char *)&MON_svrDomain[0], (char *)getMON_svrDomain_RTM());

	printInfoRTM("<GET INFO>");
}

int mon_queue_init()
{
	UINT	status;

	MON_RX_MSG_BUF.mon_tx_buf = &MON_RX_DATA_BUFFER;
	MON_TX_MSG_BUF.mon_tx_buf = &MON_TX_DATA_BUFFER;

	mon_rx_msg_p = &MON_RX_MSG_BUF;
	mon_tx_msg_p = &MON_TX_MSG_BUF;

	mon_tx_ts_msg_queue_buf = (UCHAR *)&MON_DATA_BUF;

	status = tx_queue_create(&mon_ts_msg_tx_queue, "mon_ts_msg_tx_queue",
				_1_ULONG,
				(void *)mon_tx_ts_msg_queue_buf,
				DA16X_MON_TX_QSIZE);

	if (status != ERR_OK) {
		PRINTF("[%s] : mon_msg_tx_queue create fail " "(stauts=%d)\n", __func__, status);
		return -1;
	}
	else {
		return 0;
	}
}

/* called by da16x status monitor --- send status to monitor server */
int	sendStatusToMonitorServer(UINT status_code,
				UINT para0,
				UINT para1,
				UINT para2,
				UINT para3,
				UINT para4,
				char *paraStr)
{
	UINT	status;

	if (confirm_dpm_init_done(REG_NAME_MON_SERVICE) == pdFAIL) {
		PRINTF(RED_COL "[%s] Not ready to send yet.\n" CLR_COL, __func__);
		return -1;
	}

	if (strlen(paraStr) >= MAX_PARA_STRING) {
		PRINTF(RED_COL "[%s] Range over input string(paraStr).\n" CLR_COL);
		return -1;
	}

	mon_tx_msg_p->mon_tx_buf->status_code = status_code;
	mon_tx_msg_p->mon_tx_buf->para0 = para0;
	mon_tx_msg_p->mon_tx_buf->para1 = para1;
	mon_tx_msg_p->mon_tx_buf->para2 = para2;
	mon_tx_msg_p->mon_tx_buf->para3 = para3;
	mon_tx_msg_p->mon_tx_buf->para4 = para4;
	strcpy(mon_tx_msg_p->mon_tx_buf->paraStr, paraStr);

	status = tx_queue_send(&mon_ts_msg_tx_queue, mon_tx_msg_p, portNO_DELAY);

	if (status != ERR_OK) {
#ifdef	DBG_PRINT_ERR
		PRINTF(RED_COL "[%s] stx_queue_send error (%d) \n" CLR_COL, __func__, status);
#endif
	}

	return status;
}

int	MON_SVC_send(UINT status_code, UINT para0, UINT para1, UINT para2, UINT para3, UINT para4, char *paraStr)
{
	UINT status;
	int size;

	if (chk_dpm_mode()) {
		clr_dpm_sleep(REG_NAME_MON_SERVICE);
	}

	if (confirm_dpm_init_done(REG_NAME_MON_SERVICE) == pdFAIL) {
		PRINTF(RED_COL "[%s] Not ready to send yet.\n" CLR_COL, __func__);
		if (chk_dpm_mode()) {
			status = set_dpm_sleep_confirm(REG_NAME_MON_SERVICE);
			if (status != DPM_SET_OK) {
				PRINTF(RED_COLOR "[%s] set_dpm_sleep error!! (err=%d)\n" CLEAR_COLOR, __func__, status);
			}
		}
		return 0;
	}

	memset(MON_json_data, 0, sizeof(MON_json_data));
	//MON_json_data->func = 102;
	//MON_json_data->func = status_code;
	MON_json_data->func = (status_code/100)*100;
	MON_json_data->statusCode = status_code;
	MON_json_data->para0 = para0;
	MON_json_data->para1 = para1;
	MON_json_data->para2 = para2;
	MON_json_data->para3 = para3;
	MON_json_data->para4 = para4;
	strcpy(MON_json_data->paraStr, paraStr);

	memset(MON_sendPacketStr, '\0', MAX_DATA_SIZE);
	size = MON_jsonGeneratorClient(MON_sendPacketStr, MON_json_data);
	if (size < 0) {
		if (chk_dpm_mode()) {
			status = set_dpm_sleep_confirm(REG_NAME_MON_SERVICE);
			if (status != DPM_SET_OK) {
				PRINTF(RED_COLOR "[%s] set_dpm_sleep error!! (err=%d)\n" CLEAR_COLOR, __func__, status);
			}
		}
		return -1;
	}

	status = MON_SVC_sendPkt(monClientSocketPtr, MON_sendPacketStr, size);
	if (!status)
		MON_packetPrint(MON_sendPacketStr, 1);

	if (chk_dpm_mode()) {
		mon_svc_save_all_rtm();

		status = set_dpm_sleep_confirm(REG_NAME_MON_SERVICE);
		if (status != DPM_SET_OK) {
			PRINTF(RED_COLOR "[%s] set_dpm_sleep error!! (err=%d)\n" CLEAR_COLOR, __func__, status);
		}
	}

	return status;
}

static	void mon_dpm_action()
{
	UINT status = ERR_OK;

	mon_svc_save_all_rtm();

#ifdef USE_DTLS
	status = MON_save_secure(&monSecureConf);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to save dtls session(0x%02x)\n",
			__func__, status);
#endif
	}
#endif	/* USE_DTLS */

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s]" GREEN_COLOR " SET_DPM %s & set timer(%d) \n" CLEAR_COLOR,
		__func__, REG_NAME_MON_SERVICE, MON_Service);
#endif
	status = set_dpm_sleep_confirm(REG_NAME_MON_SERVICE);
	if (status != DPM_SET_OK) {
		PRINTF(RED_COLOR "[%s] set_dpm_sleep error!! (err=%d)\n" CLEAR_COLOR, __func__, status);
	}
}

static	int mon_send()
{
	UINT	status;
	int size;
#ifdef WAITING_FOR_RECEIVE
	UINT	recvLen;
#endif	/* WAITING_FOR_RECEIVE */

	memset(MON_json_data, 0, sizeof(MON_json_data));
	MON_json_data->func = FUNC_STATUS;
	MON_json_data->statusCode = STATUS_POR;
	MON_json_data->para0 = da16x_get_wakeup_source();
	MON_json_data->para1 = 0;
	MON_json_data->para2 = 0;
	MON_json_data->para3 = 0;
	MON_json_data->para4 = 0;
	strcpy(MON_json_data->paraStr, "test");

	memset(MON_sendPacketStr, '\0', MAX_DATA_SIZE);
	size = MON_jsonGeneratorClient(MON_sendPacketStr, MON_json_data);
	if (size < 0)
		return -1;

	status = MON_SVC_sendPkt(monClientSocketPtr, MON_sendPacketStr, size);

	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("%s:%d ERROR MON Send (PKT_ID_reqToServer) (status=0x%02x) \n",
							__func__, __LINE__, status);
#endif
		return status;
	}
	else {
		MON_packetPrint(MON_sendPacketStr, 1);
	}

#ifdef WAITING_FOR_RECEIVE
	if (chk_dpm_mode()) {
		set_dpm_rcv_ready(REG_NAME_MON_SERVICE);
	}

	memset(MON_recvPacketStr, '\0', MAX_DATA_SIZE);
	status = MON_SVC_receivePkt(monClientSocketPtr, MON_recvPacketStr, &recvLen, 30);
	if (status == ER_NO_PACKET) {
		return ERR_OK;
	}
	else if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("%s:%d ERROR MON Receive (status=0x%02x) \n", __func__, __LINE__, status);
#endif
	}
	else {
		MON_packetPrint(MON_recvPacketStr, 0);
	}
#endif	/* WAITING_FOR_RECEIVE */

	return status;
}

static	int mon_recv()
{
	UINT	status;
	UINT	recvLen;

	cJSON *json_recv_data = NULL;
	cJSON *child_object = NULL;
	char str_msgCode[8];
	int size;

	if (chk_dpm_mode()) {
		set_dpm_rcv_ready(REG_NAME_MON_SERVICE);
	}

loop:
	memset(MON_recvPacketStr, '\0', MAX_DATA_SIZE);

	status = MON_SVC_receivePkt(monClientSocketPtr, MON_recvPacketStr, &recvLen, 10);	/* 100 ms */
	if (status == ER_NO_PACKET) {
		return ERR_OK;
	}
	else if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("%s:%d ERROR MON Receive (status=0x%02x) \n", __func__, __LINE__, status);
#endif
		return status;
	}
	else {
		MON_packetPrint(MON_recvPacketStr, 0);
	}

	json_recv_data = cJSON_Parse(MON_recvPacketStr);

	memset(str_msgCode, '\0', 8);
	child_object = cJSON_GetObjectItem(json_recv_data, "msgCode");
	memcpy(str_msgCode, child_object->valuestring, 7);

	child_object = cJSON_GetObjectItem(json_recv_data, "msgId");
	MON_msgId_recv = atoi(child_object->valuestring);

	if (strcmp(str_msgCode, "MSGQ200") == 0) {
		if (MON_recvCommandDataNo == MON_msgId_recv) { /* duplicated */
#ifdef DETAIL_LOG
			PRINTF(BLUE_COLOR "[%s] give up DUP MON Recv packet (dn=%d, rdn=%d) \n" CLEAR_COLOR,
						__func__,
						MON_recvCommandDataNo,
						MON_msgId_recv);
#endif
			cJSON_Delete(json_recv_data);
			return 0;	/* discard */
		}

		MON_recvCommandDataNo = MON_msgId_recv;

		memset(MON_json_data, 0, sizeof(MON_json_data));
		MON_json_data->func = FUNC_STATUS;

		memset(MON_sendPacketStr, '\0', MAX_DATA_SIZE);
		size = MON_jsonGeneratorClient(MON_sendPacketStr, MON_json_data);
		if (size < 0) {
			cJSON_Delete(json_recv_data);
			return -1;
		}

		status = MON_SVC_sendPkt(monClientSocketPtr, MON_sendPacketStr, size);
		if (!status) {
			MON_packetPrint(MON_sendPacketStr, 1);
		}
		cJSON_Delete(json_recv_data);
		goto loop;

	}
	else if (strcmp(str_msgCode, "MSGA100") == 0) {
#ifdef	DBG_PRINT_INFO
		PRINTF("Receiving ACK from Server! (msgCode: %s)\n", str_msgCode);
#endif
		cJSON_Delete(json_recv_data);
		goto loop;
	}
	else if (strcmp(str_msgCode, "MSGA200") == 0) {
#ifdef	DBG_PRINT_INFO
		PRINTF("Receiving ACK from Server! (msgCode: %s)\n", str_msgCode);
#endif
		//clearStatusToMonitorServer();
		cJSON_Delete(json_recv_data);
		goto loop;
	}
	else {
#ifdef	DBG_PRINT_INFO
		PRINTF("Receiving unknwon packet! (msgCode: %s)\n", str_msgCode);
#endif
		status = -1;
	}

	cJSON_Delete(json_recv_data);
	return status;
}

static	void MON_getConfigFromNvram()
{
	CHAR 	*tmpPtr = NULL;

	if (read_nvram_int((const char *)NVR_KEY_MON_SERVICE, (int *)&MON_Service)) {
		MON_Service = DFLT_MON_SERVICE;
	}

	tmpPtr = read_nvram_string((const char *)NVR_KEY_MON_SVR_IP);
	if (tmpPtr != NULL) {
		strcpy((char *)&MON_svrIp[0], (char *)tmpPtr);
	}
	else {
		strcpy((char *)&MON_svrIp[0], (char *)DFLT_MON_SERVER_IP);	/* TEMPORARY */
	}

#ifdef	USE_DOMAIN	/* del for TEST TEMPORARY */
	tmpPtr = read_nvram_string((const char *)NVR_KEY_MON_SVR_DOMAIN);
	if (tmpPtr != NULL) {
		strcpy((char *)MON_svrDomain, (char *)tmpPtr);
	}
	else {
		strcpy((CHAR *)&MON_svrDomain[0], (CHAR *)DFLT_MON_SVR_DOMAIN);
	}
#endif
}

#ifdef USE_DTLS
UINT MON_init_secure(MON_secure_config_t *config)
{
	int retval = 0;

	void *ssl_ctx = NULL;
	struct dtls_connection *dtls_conn = NULL;

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s] Init secure mode\n", __func__);
#endif

	memset(config, 0x00, sizeof(MON_secure_config_t));

	config->dtls_params.psk_id_blob = MON_PSK_ID;
	config->dtls_params.psk_id_blob_len = strlen((const char *)MON_PSK_ID);
	config->dtls_params.psk_key_blob = MON_PSK_KEY;
	config->dtls_params.psk_key_blob_len = strlen((const char *)MON_PSK_KEY);

	ssl_ctx = dtls_init(NULL);
	if (ssl_ctx == NULL) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] Failed to init dtls\n", __func__);
#endif
		return ER_NOT_CREATED;
	}

	dtls_conn = dtls_connection_init(ssl_ctx);
	if (dtls_conn == NULL) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] Failed to set dtls connection\n", __func__);
#endif
		dtls_deinit(ssl_ctx);
		return ER_NOT_CREATED;
	}

	retval = dtls_connection_set_params(ssl_ctx, dtls_conn,
					&config->dtls_params);
	if (retval) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] Failed to set secure config\n", __func__);
#endif
	}

	config->ssl_ctx = ssl_ctx;
	config->dtls_conn = dtls_conn;

	return ERR_OK;
}

void MON_deinit_secure(MON_secure_config_t *config)
{
#ifdef	DBG_PRINT_INFO
	PRINTF("[%s] Deinit secure mode\n", __func__);
#endif

	if (config->dtls_conn) {
		dtls_connection_deinit(config->ssl_ctx,
				config->dtls_conn);
	}

	if (config->ssl_ctx) {
		dtls_deinit(config->ssl_ctx);
	}

	memset(config, 0x00, sizeof(MON_secure_config_t));

	return ;
}

UINT MON_shutdown_secure(MON_secure_config_t *config)
{
	NX_UDP_SOCKET *sock = monClientSocketPtr;

	UINT status = ERR_OK;

	void *ssl_ctx = config->ssl_ctx;
	struct dtls_connection *dtls_conn = config->dtls_conn;

	struct wpabuf *alert_msg = NULL;

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s] Shutdown secure config\n", __func__);
#endif

	//send alert(Close Notify) whatever connection established or not
	alert_msg = dtls_connection_set_write_alerts(ssl_ctx, dtls_conn, 2, 0);
	if (alert_msg) {
#ifdef	DBG_PRINT_INFO
		PRINTF("[%s]Send alert message(Close Notify)\n", __func__);
#endif
		status = MON_SVC_sendPkt(sock,
				wpabuf_mhead(alert_msg),
				wpabuf_len(alert_msg));
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s] failed to send alert message"
				"(close notify)(0x%02x)\n",
				__func__, status);
#endif
		}

		wpabuf_free(alert_msg);
		alert_msg = NULL;
	}
	else {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] failed to generate alert message"
			"(close notify)(0x%02x)\n",
			__func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	if (dtls_connection_shutdown(ssl_ctx, dtls_conn) < 0) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] failed to shutdown secure config\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	return ERR_OK;
}

UINT MON_do_handshake(MON_secure_config_t *config)
{
	UINT status = ERR_OK;

	NX_UDP_SOCKET *sock = monClientSocketPtr;
	void *ssl_ctx = config->ssl_ctx;
	struct dtls_connection *dtls_conn = config->dtls_conn;

	struct dtlsv1_record_pool *pool = NULL;

	NX_PACKET *rcv_packet = NULL;
	unsigned char *rcv_buffer = NULL;
	ULONG rcv_bytes = 0;

	struct wpabuf *in_data = NULL;

	int retry_count = 0;
	ULONG wait_option = 500;

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s]Progress handshake\n", __func__);
#endif

	if (dtls_connection_established(ssl_ctx, dtls_conn)) {
#ifdef	DBG_PRINT_INFO
		PRINTF("[%s]Already establised\n", __func__);
#endif
		return ER_ALREADY_ENABLED;
	}

	if (chk_dpm_mode()) {
		set_dpm_init_done(REG_NAME_MON_SERVICE);

		set_dpm_rcv_ready(REG_NAME_MON_SERVICE);
	}

	//Send clienthello
	pool = dtls_connection_handshake(ssl_ctx, dtls_conn, NULL, NULL);
	if (pool) {
		status = MON_send_handshake_msg(ssl_ctx, dtls_conn, pool);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to send ClientHello message(0x%02x)\n",
				__func__, status);
#endif
			goto failed_handshake;
		}

		dtlsv1_record_pool_destroy(&pool);
	}
	else {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]Failed to generate ClientHello\n", __func__);
#endif
		status = ER_INVALID_PACKET;
		goto failed_handshake;
	}

	while (!dtls_connection_established(ssl_ctx, dtls_conn)) {
		status = nx_udp_socket_receive(sock, &rcv_packet, wait_option);
		if (status) {
			if (retry_count > MON_HANDSHAKE_MAX_RETRY) {
#ifdef	DBG_PRINT_ERR
				PRINTF("[%s]Failed to get packet(0x%02x)\n",
					__func__, status);
#endif
				goto failed_handshake;
			}

			if (dtls_connection_get_send_pool_len(ssl_ctx, dtls_conn) > 0) {
#ifdef	DBG_PRINT_INFO
				PRINTF("[%s]Retry #%d - send buffer...\n",
					__func__, retry_count);
#endif

				pool = dtls_connection_get_send_pool(ssl_ctx, dtls_conn);
				if (pool) {
					status = MON_send_handshake_msg(ssl_ctx, dtls_conn, pool);
					if (status) {
#ifdef	DBG_PRINT_ERR
						PRINTF("[%s]failed to send pakcet(0x%02x)\n",
							__func__, status);
#endif
						goto failed_handshake;
					}

					dtlsv1_record_pool_destroy(&pool);
				}
				else {
#ifdef	DBG_PRINT_INFO
					PRINTF("[%s]empty send buffer\n",
						__func__);
#endif
					status = ER_NOT_SUCCESSFUL;
					goto failed_handshake;
				}
			}
			else {
#ifdef	DBG_PRINT_INFO
				PRINTF("[%s]empty send buffer\n",
					__func__);
#endif
				status = ER_NOT_SUCCESSFUL;
				goto failed_handshake;
			}

			retry_count++;

			continue;
		}
		else {
			status = nx_packet_length_get(rcv_packet, &rcv_bytes);
			if (status != ERR_OK || rcv_bytes == 0) {
#ifdef	DBG_PRINT_ERR
				PRINTF("[%s]Failed to get "
					"received packet's length(0x%02x:%ld)\n",
					__func__,
					status, rcv_bytes);
#endif
				status = ER_NOT_SUCCESSFUL;
				goto failed_handshake;
			}

#ifdef	DBG_PRINT_INFO
			PRINTF("[%s]Received packet's length:%ld\n",
				__func__, rcv_bytes);
#endif
			rcv_buffer = (UCHAR *)pvPortMalloc(rcv_bytes);
			if (rcv_buffer == NULL) {
#ifdef	DBG_PRINT_ERR
				PRINTF("[%s]Failed to allocate memory\n",
					__func__);
#endif
				goto failed_handshake;
			}

			memset(rcv_buffer, 0x00, rcv_bytes);

			status = nx_packet_data_retrieve(rcv_packet,
							rcv_buffer, &rcv_bytes);
			if (status) {
#ifdef	DBG_PRINT_ERR
				PRINTF("[%s]Failed to retrieve packet(0x%02x)\n",
					__func__, status);
#endif
				goto failed_handshake;
			}

			in_data = wpabuf_alloc_copy(rcv_buffer, rcv_bytes);
			if (in_data == NULL) {
#ifdef	DBG_PRINT_ERR
				PRINTF("[%s]Failed to allocate memory\n",
					__func__);
#endif
				goto failed_handshake;
			}

			pool = dtls_connection_handshake(ssl_ctx, dtls_conn,
							in_data, NULL);
			if (pool) {
				status = MON_send_handshake_msg(ssl_ctx, dtls_conn, pool);
				if (status) {
#ifdef	DBG_PRINT_ERR
					PRINTF("[%s]failed to send packet(0x%02x)\n",
						__func__, status);
#endif
				}

				dtlsv1_record_pool_destroy(&pool);
			}

			if (rcv_buffer) {
				vPortFree(rcv_buffer);
				rcv_buffer = NULL;
			}

			if (rcv_packet) {
				nx_packet_release(rcv_packet);
				rcv_packet = NULL;
			}

			if (in_data != NULL) {
				wpabuf_free(in_data);
				in_data = NULL;
			}
		}

		if (dtls_connection_state(ssl_ctx, dtls_conn) == 19) {
#if 0
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to handshake with server\n",
				__func__);
#endif
#endif
			status = ER_NOT_SUCCESSFUL;
			goto failed_handshake;
		}

		while (!dtls_connection_established(ssl_ctx, dtls_conn)
			&& dtls_connection_get_recv_pool_len(ssl_ctx, dtls_conn) > 0) {

#ifdef	DBG_PRINT_INFO
			PRINTF("Number of buffered packet:%d - receive buffer\n",
				dtls_connection_get_recv_pool_len(ssl_ctx, dtls_conn));
#endif

			pool = dtls_connection_handshake_from_pool(ssl_ctx, dtls_conn);
			if (pool != NULL) {
				status = MON_send_handshake_msg(ssl_ctx, dtls_conn, pool);
				if (status) {
#ifdef	DBG_PRINT_ERR
					PRINTF("[%s]Failed to send packet(0x%02x)\n",
						__func__, status);
#endif
				}

				dtlsv1_record_pool_destroy(&pool);
			}
			else {
				break;
			}
		} // to check buffered data
	}

failed_handshake:

	dtls_connection_clear_recv_pool(ssl_ctx, dtls_conn);

	dtls_connection_clear_send_pool(ssl_ctx, dtls_conn);

	if (rcv_packet != NULL) {
		nx_packet_release(rcv_packet);
		rcv_packet = NULL;
	}

	if (pool != NULL) {
		dtlsv1_record_pool_destroy(&pool);
	}

	if (in_data != NULL) {
		wpabuf_free(in_data);
		in_data = NULL;
	}

	if (rcv_buffer != NULL) {
		vPortFree(rcv_buffer);
		rcv_buffer = NULL;
	}

	return status;
}

UINT MON_send_handshake_msg(void *ssl_ctx, struct dtls_connection *dtls_conn,
			struct dtlsv1_record_pool *pool)
{
	NX_UDP_SOCKET *sock = monClientSocketPtr;

	UINT status = ERR_OK;
	ULONG wait_options = 100;

	struct dtlsv1_record *record = NULL;
	struct dtlsv1_record *n = NULL;

	if (pool) {
		dl_list_for_each_safe(record, n, &pool->list, struct dtlsv1_record, list) {
			if (record->data) {
				status = MON_SVC_sendPkt(sock,
							record->data,
							record->len);
				if (status) {
#ifdef	DBG_PRINT_ERR
					PRINTF("[%s]failed to send handshake message\n",
						__func__);
#endif
					return status;
				}
				else {
					OAL_MSLEEP(wait_options);
				}
			}
		}
	}
	else {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]pool is empty\n", __func__);
#endif
		status = ER_NOT_SUCCESSFUL;
	}

	return status;
}

UINT MON_encrypt_packet(void *ssl_ctx, struct dtls_connection *dtls_conn,
		unsigned char *in_data, size_t in_len,
		NX_PACKET **out_data)
{
	UINT status = ERR_OK;
	struct wpabuf *raw_data = NULL;
	struct wpabuf *encrypted_data = NULL;

	*out_data = NULL;

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s]Encrpyt packet(len:%ld)\n",
		__func__, in_len);
#endif

	raw_data = wpabuf_alloc_copy(in_data, in_len);
	if (!raw_data) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to allocate memory\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	encrypted_data = dtls_connection_encrypt(ssl_ctx, dtls_conn, raw_data);
	if (!encrypted_data) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to encrypt packet\n", __func__);
#endif
		status = ER_NOT_SUCCESSFUL;

		goto failed_encryption;
	}

	status = nx_packet_allocate(MON_svcPool,
				out_data,
				NX_UDP_PACKET,
				portMAX_DELAY);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to allocate memory(0x%02x)\n",
			__func__, status);
#endif
		goto failed_encryption;
	}

	status = nx_packet_data_append(*out_data,
				(VOID *)encrypted_data->buf,
				(ULONG)encrypted_data->used,
				MON_svcPool,
				portMAX_DELAY);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to append packet(0x%02x)\n",
			__func__, status);
#endif
		goto failed_encryption;
	}

failed_encryption:
	if (raw_data) {
		wpabuf_free(raw_data);
	}

	if (encrypted_data) {
		wpabuf_free(encrypted_data);
	}

	return status;
}

UINT MON_decrypt_packet(void *ssl_ctx, struct dtls_connection *dtls_conn,
			NX_PACKET *in_data, NX_PACKET **out_data)
{
	UINT status = ER_NOT_IMPLEMENTED;

	struct wpabuf *encrypted_data = NULL;
	struct wpabuf *decrypted_data = NULL;

	UCHAR *buffer = NULL;
	ULONG in_len = 0;

	*out_data = NULL;

	status = nx_packet_length_get(in_data, &in_len);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to get received packet's length(0x%02x)\n",
			__func__, status);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	buffer = pvPortMalloc(in_len);
	if (!buffer) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to allocate memory\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	status = nx_packet_data_retrieve(in_data, buffer, &in_len);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to retrieve packet(0x%02x)\n",
			__func__, status);
#endif
		goto failed_decryption;
	}

	encrypted_data = wpabuf_alloc_copy(buffer, in_len);
	if (!encrypted_data) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to copy packet\n", __func__);
#endif
		status = ER_NOT_SUCCESSFUL;
		goto failed_decryption;
	}
	else {
		vPortFree(buffer);
		buffer = NULL;
	}

	decrypted_data = dtls_connection_decrypt(ssl_ctx,
						dtls_conn,
						encrypted_data);
	if (decrypted_data) {
		status = nx_packet_allocate(MON_svcPool,
					out_data,
					NX_UDP_PACKET,
					portMAX_DELAY);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to allocate memory(0x%02x)\n",
				__func__, status);
#endif
			goto failed_decryption;
		}

		status = nx_packet_data_append(*out_data,
					(VOID *)decrypted_data->buf,
					(ULONG)decrypted_data->used,
					MON_svcPool,
					portMAX_DELAY);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to append packet(0x%02x)\n",
				__func__, status);
#endif
			goto failed_decryption;
		}
	}
	else {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to decrypt packet\n", __func__);
#endif
		status = ER_NOT_SUCCESSFUL;
		goto failed_decryption;
	}

failed_decryption:

	if (buffer) {
		vPortFree(buffer);
		buffer = NULL;
	}

	wpabuf_free(encrypted_data);
	wpabuf_free(decrypted_data);

	return status;
}

UINT MON_save_secure(MON_secure_config_t *config)
{
	UINT status = ERR_OK;

	unsigned int len = 0;
	unsigned char *data = NULL;
	const ULONG wait_option = 100;

	void *ssl_ctx = config->ssl_ctx;
	struct dtls_connection *dtls_conn = config->dtls_conn;

	struct wpabuf *buffer = NULL;

	if (!chk_dpm_mode()) {
		return ERR_OK;
	}

	if ((ssl_ctx == NULL) || (dtls_conn == NULL)) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]secure is not initialized\n", __func__);
#endif
		return ER_NOT_CREATED;
	}

	if (!dtls_connection_established(ssl_ctx, dtls_conn)) {
#ifdef	DBG_PRINT_INFO
		PRINTF("[%s]secure connection is not established\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	//dtls conn
	buffer = dtls_connection_get_dtls_conn(ssl_ctx, dtls_conn);
	if (buffer == NULL) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to allocate rtm(0x%02x)\n",
			__func__, status);
#endif
		goto failed_save_secure;
	}

	len = dpm_user_rtm_get(MON_SVC_DTLS_CONN, &data);
	if (!len) {
		status = dpm_user_rtm_allocate(MON_SVC_DTLS_CONN,
					(VOID **)&data,
					wpabuf_len(buffer),
					wait_option);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to allocate rtm(0x%02x)\n",
				__func__, status);
#endif
			goto failed_save_secure;
		}

		len = wpabuf_len(buffer);
	}

	if (len != wpabuf_len(buffer)) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]length is not the same(rtm:%d, data:%ld)\n",
			__func__, len, wpabuf_len(buffer));
#endif
		status = ER_NOT_SUCCESSFUL;

		goto failed_save_secure;
	}

	memcpy(data, wpabuf_head(buffer), wpabuf_len(buffer));

	wpabuf_free(buffer);
	buffer = NULL;

	//record layer
	buffer = dtls_connection_get_record_layer(ssl_ctx, dtls_conn);
	if (buffer == NULL) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to get dtls record\n", __func__);
#endif
		status = ER_NOT_SUCCESSFUL;

		goto failed_save_secure;
	}

	len = dpm_user_rtm_get(MON_SVC_DTLS_RECORD, &data);
	if (!len) {
		status = dpm_user_rtm_allocate(MON_SVC_DTLS_RECORD,
						(VOID **)&data,
						wpabuf_len(buffer),
						wait_option);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s]failed to allocate rtm(0x%02x)\n",
				__func__, status);
#endif
			goto failed_save_secure;
		}

		len = wpabuf_len(buffer);
	}

	if (len != wpabuf_len(buffer)) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]length is not the same(rtm:%d, data:%ld)\n",
			__func__, len, wpabuf_len(buffer));
#endif
		status = ER_NOT_SUCCESSFUL;

		goto failed_save_secure;
	}

	memcpy(data, wpabuf_head(buffer), wpabuf_len(buffer));

	wpabuf_free(buffer);
	buffer = NULL;

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s]Succeeded to save dtls session\n", __func__);
#endif

failed_save_secure:

	wpabuf_free(buffer);

	return status;
}

UINT MON_restore_secure(MON_secure_config_t *config)
{
	unsigned int len = 0;
	unsigned char *data = NULL;

	void *ssl_ctx = config->ssl_ctx;
	struct dtls_connection *dtls_conn = config->dtls_conn;

	if (!chk_dpm_mode()) {
		return ERR_OK;
	}

	if ((ssl_ctx == NULL) || (dtls_conn == NULL)) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]secure is not initialized\n", __func__);
#endif
		return ER_NOT_CREATED;
	}

	//dtls conn
	len = dpm_user_rtm_get(MON_SVC_DTLS_CONN, &data);
	if (!len) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to get dtls session from rtm\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	if (dtls_connection_set_dtls_conn(ssl_ctx, dtls_conn,
					data, len) < 0) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to recover dtls session\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	//dtls record layer
	len = dpm_user_rtm_get(MON_SVC_DTLS_RECORD, &data);
	if (!len) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to get dtls session from rtm\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

	if (dtls_connection_set_record_layer(ssl_ctx, dtls_conn,
					data, len) < 0) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to recover dtls session\n", __func__);
#endif
		return ER_NOT_SUCCESSFUL;
	}

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s]Succeeded to restore dtls session\n", __func__);
#endif
	return ERR_OK;
}

UINT MON_clear_secure(MON_secure_config_t *config)
{
	UINT status = ERR_OK;

	if (!chk_dpm_mode()) {
		return ERR_OK;
	}

	status = dpm_user_rtm_release(MON_SVC_DTLS_CONN);
	if (status != ERR_OK && status != ER_NOT_FOUND) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to release dtls conn(0x%02x)\n",
			__func__, status);
#endif
	}

	status = dpm_user_rtm_release(MON_SVC_DTLS_RECORD);
	if (status != ERR_OK && status != ER_NOT_FOUND) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]failed to release dtls record layer(0x%02x)\n",
			__func__, status);
#endif
	}

	return status;
}
#endif	/* USE_DTLS */

void	da16x_mon_client(void)
{
	UINT	status;
	NX_IP	*curIpPtr;
	UINT	i = 0;
#ifdef	USE_DOMAIN	/* del for TEST TEMPORARY */
	CHAR	*ipStr;
#endif	/* 0 */
	UINT	sysmode;
	INT     tmpValue;
	UINT	ifaceSelect;
	UINT	terminate = pdFALSE;
	UINT	reNewCount = 0;
	UINT	myPortNo = 0;
	ULONG	dns_query_wait_option = 400;

	if (chk_dpm_mode()) {
		if (reg_dpm_sleep(REG_NAME_MON_SERVICE, MON_SERVICE_PORT) == DPM_REG_ERR) {
			PRINTF("[%s] Failed to register for DPM \n", __func__);
		}
	}

	/* Get System running mode ... */
	if (!chk_dpm_wakeup()) {
		sysmode = get_run_mode();
	}
	else {
		/* !!! Caution !!!
 		 * Do not read operation from NVRAM when dpm_wakeup.
 		 */
		sysmode = SYSMODE_STA_ONLY;
	}

	if (sysmode == SYSMODE_STA_ONLY) {

		ifaceSelect = WLAN0_IFACE;

		/* !!! Caution !!!
		 * Do not read operation from NVRAM when dpm_wakeup.
		 */
		if (!chk_dpm_wakeup()) {
			read_nvram_int((const char *)NVR_KEY_NETMODE_0, (int *)&tmpValue);
		}
		else {
			/* Read netmode from RTM */
			dpm_netinfo = (user_dpm_supp_net_info_t *)get_supp_net_info_ptr();

			tmpValue = dpm_netinfo->net_mode;
		}

		if (tmpValue == ENABLE_DHCP_CLIENT) {
			i = 0;
			while (1) {
				if (!check_net_ip_status(WLAN0_IFACE)) {
#ifdef DBG_PRINT_INFO
					if (i)
						PRINTF("\n[%s] delay %d Time\n", __func__, i);
#endif
					i = 0;
					break;
				}
				i++;
				OAL_MSLEEP(10);
			}
		}
	}
	else {
#ifdef DBG_PRINT_TEMP
		PRINTF("[%s] Not service - AP MODE. \n", __func__);
#endif

		return;
	}

	/* wait for network initialization */
	while (1) {
		if (check_net_init(ifaceSelect) == ERR_OK) {
#ifdef DBG_PRINT_INFO
			if (i)
				PRINTF("\n[%s] delay %d Sec", __func__, i);
#endif
			i=0;
			break;
		}
		i++;
		OAL_MSLEEP(10);
	}

	/* Waiting netif status */
	status = wifi_netif_status(ifaceSelect);
	while (status == 0xFF || status != pdTRUE) {
		OAL_MSLEEP(10);
		status = wifi_netif_status(ifaceSelect);
	}

#ifdef USE_MALLOC
	MON_json_data = pvPortMalloc(MAX_DATA_SIZE);
	MON_sendPacketStr = pvPortMalloc(MAX_DATA_SIZE);
	MON_recvPacketStr = pvPortMalloc(MAX_DATA_SIZE);
#else
	MON_json_data = (MON_jsonData_t *)&MON_json_data_buf[0];
	MON_sendPacketStr = &MON_sendPacketStr_buf[0];
	MON_recvPacketStr = &MON_recvPacketStr_buf[0];
#endif

	ifaceSelect = interface_select;

	get_thread_netx((void **)&MON_svcPool, (void **)&curIpPtr, ifaceSelect);

#ifdef USE_DTLS
	//init dtls
	status = MON_init_secure(&monSecureConf);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]Failed to init dtls(0x%02x)\n", __func__, status);
#endif
		goto udp_release;
	}
#endif	/* USE_DTLS */

	if (chk_dpm_wakeup()) {
#ifdef	DBG_PRINT_INFO
		PRINTF("[%s] DPM Wakeup!!!\n", __func__);
#endif
		mon_svc_get_all_rtm();

#ifdef USE_DTLS
		MON_restore_secure(&monSecureConf);
#endif	/* USE_DTLS */
	}
	else {
		MON_getConfigFromNvram();

		MON_totalRunCount = 0;
		MON_currRunTick = 0;
		MON_totalRunTick = 0;

		if (chk_dpm_mode()) {
			mon_svc_save_all_rtm();
		}
	}

	if (MON_Service == 0) {

		PRINTF(CYN_COL "[%s] STOP Monitor service !!!\n" CLR_COL, __func__);
		clr_dpm_sleep(REG_NAME_MON_SERVICE);
		unreg_dpm_sleep(REG_NAME_MON_SERVICE);
		return;
	}

#ifdef	USE_DOMAIN	/* TEMPORARY */
	i = 0;
retryQuery:
	ipStr = dns_A_Query((char *)MON_svrDomain, dns_query_wait_option);
#ifdef	DBG_PRINT_INFO
	PRINTF("[%s] dns_A_quary Svr domain=%s %s)\n",
		__func__, MON_svrDomain, ipStr);
#endif

	if ( (ipStr == NULL) && (sysmode == SYSMODE_STA_ONLY) ) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] Error: MON server address.(%s)\n",
			__func__, MON_svrDomain);
#endif
		if (i++ < 3) {
			OAL_MSLEEP(1000);
			goto retryQuery;
		}
		else {
			strcpy(&MON_svrIp[0], SERVER_IP);	/* TEMPORARY */
		}
	}
	else {
		strcpy(&MON_svrIp[0], ipStr);
	}
#endif	/* USE_DOMAIN */

	MON_svcServerIp = iptolong(MON_svrIp);

#ifdef	DBG_PRINT_TEMP
	PRINTF("[%s] Monitor service Start.. Server_IP:%d.%d.%d.%d \n"
		//"\t>>> Port: %d\n"
		//"\t>>> iface: %d\n"
		, __func__,
		(MON_svcServerIp >> 24)&0x0ff,
		(MON_svcServerIp >> 16)&0x0ff,
		(MON_svcServerIp >> 8)&0x0ff,
		(MON_svcServerIp >> 0)&0x0ff
		//MON_SERVICE_PORT,
		//ifaceSelect
		);
#endif

reNew:
	monClientSocketPtr = &monClientSocket;

	memset(monClientSocketPtr, 0, sizeof(NX_UDP_SOCKET));

	/* Create the socket. */
	status = nx_udp_socket_create(curIpPtr,
			monClientSocketPtr,
			MON_SVC_CLIENT_SOCKET,
			NX_IP_NORMAL,
			NX_FRAGMENT_OKAY,
			NX_IP_TIME_TO_LIVE,
			5);

	/* Check for error. */
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] SOCKET_CREATE ERROR(status=0x%02x)\n",
			__func__, status);
#endif

#ifdef USE_DTLS
		//deinit dtls session
		MON_deinit_secure(&monSecureConf);
#endif	/* USE_DTLS */

#ifdef USE_MALLOC
		vPortFree(MON_json_data);
		vPortFree(MON_sendPacketStr);
		vPortFree(MON_recvPacketStr);
#endif

		return;
	}

	myPortNo = MON_makePortNo();

	/* Bind the socket. */
	status = nx_udp_socket_bind(monClientSocketPtr,
				myPortNo,
				portMAX_DELAY);

	/* Check for error. */
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] SOCKET_BIND ERROR(status=0x%02x)\n",
			__func__, status);
#endif
		goto udp_tx_socket_delete;
	}

//dpm_wakeup:

	if (!chk_dpm_wakeup()) {
		if (MON_Service == 0) {	/* not service */
			unreg_dpm_sleep(REG_NAME_MON_SERVICE);
			terminate = pdTRUE;
			goto udp_release;
		}
	}

#ifdef USE_DTLS
	//handshake
	status = MON_do_handshake(&monSecureConf);
	if (status == ERR_OK) {
#ifdef	DBG_PRINT_INFO
		PRINTF("[%s]handshake completed(0x%02x)\n",
					__func__, status);
#endif
		if (chk_dpm_mode()) {
			MON_save_secure(&monSecureConf);
		}
#endif	/* USE_DTLS */

		if (!chk_dpm_wakeup()) {	/* Case of POR */
			mon_send();
		}
#ifdef	USE_DTLS
	}
	else if (status == ER_ALREADY_ENABLED) {
#ifdef	DBG_PRINT_INFO
		PRINTF("[%s]handshake already completed(0x%02x)\n",
					__func__, status);
#endif
	}
	else {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s]Failed to handshake with server(0x%02x)\n",
					__func__, status);
#endif

#if 1 /* maybe easy-setup working. it's temporary code. */
		if (status == ER_IP_ADDRESS_ERROR) {
			terminate = pdTRUE;
		}
#endif
		goto udp_release;
	}
#endif	/* USE_DTLS */

	if (chk_dpm_mode()) {
		set_dpm_init_done(REG_NAME_MON_SERVICE);
	}

	while(1) {

		if (ERR_OK == tx_queue_receive(&mon_ts_msg_tx_queue, mon_rx_msg_p, portNO_DELAY)) {

#ifdef	DBG_PRINT_INFO
			PRINTF("[%s] recv msg(status_code:%d, para0:0x%x, para1:%d, para2:%d, para3:%d, para4:%d, paraStr:%s)\n",
							__func__,
							mon_rx_msg_p->mon_tx_buf->status_code,
							mon_rx_msg_p->mon_tx_buf->para0,
							mon_rx_msg_p->mon_tx_buf->para1,
							mon_rx_msg_p->mon_tx_buf->para2,
							mon_rx_msg_p->mon_tx_buf->para3,
							mon_rx_msg_p->mon_tx_buf->para4,
							mon_rx_msg_p->mon_tx_buf->paraStr);
#endif

			MON_SVC_send(mon_rx_msg_p->mon_tx_buf->status_code,
					mon_rx_msg_p->mon_tx_buf->para0,
					mon_rx_msg_p->mon_tx_buf->para1,
					mon_rx_msg_p->mon_tx_buf->para2,
					mon_rx_msg_p->mon_tx_buf->para3,
					mon_rx_msg_p->mon_tx_buf->para4,
					mon_rx_msg_p->mon_tx_buf->paraStr);
		}

		status = mon_recv();
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s] ERROR: MON recv fail (status = 0x%0x)\n",
				__func__, status);
#endif
			break;
		}

		if (chk_dpm_mode()) {
			mon_dpm_action();
		}
	}

udp_release:

#ifdef USE_DTLS
	//shutdown dtls
	MON_shutdown_secure(&monSecureConf);

	if (chk_dpm_mode()) {
		status = MON_clear_secure(&monSecureConf);
		if (status) {
#ifdef	DBG_PRINT_ERR
			PRINTF("[%s] ERROR(0x%02x) to release rtm failed\n",
				__func__, status);
#endif
		}
	}
#endif /* USE_DTLS */

	/* Unbind the socket. */
	status = nx_udp_socket_unbind(monClientSocketPtr);

	/* Check for error. */
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] ERROR(0x%0x) socket unbind fail \n",
			__func__, status);
#endif
	}

udp_tx_socket_delete:

	/* Delete the socket. */
	status = nx_udp_socket_delete(monClientSocketPtr);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] ERROR(0x%02x) socket delete fail\n",
			__func__, status);
#endif
	}

	if (!terminate) {
		if (++reNewCount <= MON_HANDSHAKE_MAX_RETRY) {
#ifdef	DBG_PRINT_TEMP
			PRINTF(CYAN_COLOR "[%s] reNew recursive.(%d)\n" CLEAR_COLOR,
					__func__, reNewCount);
#endif
			OAL_MSLEEP(1000);
			goto reNew;	/* recursive */
		}
	}

	if (chk_dpm_mode()) {
		mon_svc_save_all_rtm();

		clr_dpm_sleep(REG_NAME_MON_SERVICE);
		unreg_dpm_sleep(REG_NAME_MON_SERVICE);
		OAL_MSLEEP(30);	/* TEMPORARY for print */
	}

#ifdef USE_DTLS
	//deinit dtls session
	MON_deinit_secure(&monSecureConf);
#endif /* USE_DTLS */

#ifdef	DBG_PRINT_INFO
	PRINTF("[%s] DONE.\n", __func__);
#endif

#ifdef USE_MALLOC
	vPortFree(MON_json_data);
	vPortFree(MON_sendPacketStr);
	vPortFree(MON_recvPacketStr);
#endif

	return;
}

void	initMonServiceClient(ULONG thread_input)
{
	da16x_mon_client();

	vTaskDelete(NULL);
}

void start_dpm_monitor_client(int dpm_wu_type)
{
	UINT	status;

	mon_queue_init();

	status = tx_thread_create(&dpm_mon_client,
			DPM_MON_CLIENT_NAME,
			initMonServiceClient,
			NULL,
			dpm_mon_client_stack,
			DPM_MON_CLIENT_STK_SIZE,
			DPM_MON_CLIENT_PRIORITY,
			DPM_MON_CLIENT_PRIORITY,
			TX_NO_TIME_SLICE,
			_AUTO_START);
	if (status) {
#ifdef	DBG_PRINT_ERR
		PRINTF("[%s] ERROR Thread Create(0x%0x)\n", __func__, status);
#endif
	}
}

#endif	/* __DA16X_DPM_MON_CLIENT__ */

/* EOF */
