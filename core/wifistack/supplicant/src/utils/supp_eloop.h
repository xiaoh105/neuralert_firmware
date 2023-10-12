/**
 *****************************************************************************************
 * @file    supp_eloop.h
 * @brief   Event loop from wpa_supplicant-2.4
 *****************************************************************************************
 */

/*
 * Event loop
 * Copyright (c) 2002-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file defines an event loop interface that supports processing events
 * from registered timeouts (i.e., do something after N seconds), sockets
 * (e.g., a new packet available for reading), and signals. eloop.c is an
 * implementation of this interface using select() and sockets. This is
 * suitable for most UNIX/POSIX systems. When porting to other operating
 * systems, it may be necessary to replace that implementation with OS specific
 * mechanisms.
 *
 * Copyright (c) 2021-2022 Modified by Renesas Electronics.
 */


#ifndef SUPP_ELOOP_H
#define SUPP_ELOOP_H


#include "FreeRTOS.h"

#include "project_config.h"
#include "timers.h"

/* Assign 1 flag for fc80211_driver */
#define DA16X_SP_DRV_FLAG		0x80000000
#define DA16X_SP_PROBE_FLAG		0x40000000

/* Assign 2 flags for fc9000 supplicant cli */
#define	DA16X_SP_CLI_TX_FLAG		0x00000001
#define	DA16X_SP_CLI_RX_FLAG		0x00000002

/* Assign 1 flag for l2_packet_rx */
#define	DA16X_L2_PKT_RX_EV		0x00000010
#ifdef CONFIG_RECEIVE_DHCP_EVENT
#define	DA16X_DHCP_EV			0x00000020
#endif /* CONFIG_RECEIVE_DHCP_EVENT */

#ifdef CONFIG_IMMEDIATE_SCAN
/* Scan result event from supp to cli */
#define DA16X_SCAN_RESULTS_RX_EV		0x00000020
#define DA16X_SCAN_RESULTS_TX_EV		0x00000040
#define DA16X_SCAN_RESULTS_FAIL_EV       0x00000080
#endif /* CONFIG_IMMEDIATE_SCAN */

/* FC9000 Message Queue total size */
#define	DA16X_QUEUE_BUF_SIZE		128
#define TO_SUPP_QUEUE_SIZE		20	//5
#define TO_CLI_QUEUE_SIZE		1

#define DA16X_MAX_CLI_CMD		4
#define	DA16X_TX_QSIZE			16	// TX_1_ULONG * 4
#define	DA16X_RX_QSIZE			8	// TX_1_ULONG * 2

#ifdef	__WPA3_WIFI_CERTFIED__
#define	DA16X_DRV_EV_DATA_SIZE		2300
#else
#define	DA16X_DRV_EV_DATA_SIZE		512     //4096 -> 512, 20150729_trinity
#endif	/* __WPA3_WIFI_CERTFIED__ */

#define DA16X_MAX_DRV_EVENTS		8       //4->8, 20150729_trinity
#define DA16X_DRV_EV_DATA_BLK_POOL_ERR	1

typedef struct da16x_drv_event_attr {
	u16 softmac_idx;
	u16 ifindex;
	u16 wdev_id;
	u16 timed_out;
	int freq;
	int sig_dbm;
	ULONG cookie;
	int ack;
	u8 addr[ETH_ALEN];
	u16 status;
	u16 reason;
	u16 disconnected_by_ap;
	u16 req_ie_len;
	u16 resp_ie_len;
	u16 attr_ie_len;
	u8 n_ssids;
	u8 n_channels;
	u32 flags;
	int duration;
} da16x_drv_event_attr_t;

/* Drv event data from UMAC */
typedef struct da16x_drv_event_data {
	da16x_drv_event_attr_t	attr;
	char	data[DA16X_DRV_EV_DATA_SIZE];
	int	data_len;
} da16x_drv_ev_data_t;

/* FC80211 Link Layer -> Supplicant RX Buffer */
typedef struct da16x_drv_msg_buffer {
	UINT	cmd;
	struct da16x_drv_event_data *data;
} da16x_drv_msg_buf_t;

/****************************************************************/

#define	CLI_SCAN_RSP_BUF_SIZE		3584

#define DA16X_MAX_CLI_RX_EVENTS			1
#define DA16X_CLI_RX_EV_DATA_BLK_POOL_ERR	1
#define DA16X_CLI_CMD_TX_BLK_POOL_ERR		2

typedef struct da16x_cli_cmd_buffer {
	char	cmd_str[DA16X_QUEUE_BUF_SIZE];
} da16x_cli_cmd_buf_t;

typedef struct da16x_cli_tx_msg {
	da16x_cli_cmd_buf_t *cli_tx_buf;
} da16x_cli_tx_msg_t;

/* CLI event data from SP */
typedef struct da16x_cli_rx_event_data {
	/* da16x_cli_event_attr_t attr; */
	char	data[CLI_SCAN_RSP_BUF_SIZE];
	int	data_len;
} da16x_cli_rx_ev_data_t;

/* Supplicant -> CLI RSP Buffer */
typedef struct da16x_cli_rsp_buffer {
	struct da16x_cli_rx_event_data *event_data;
} da16x_cli_rsp_buf_t;


/////////////////////////////////////////////////////////////////////////////

/* !!! NOTICE !!!
 * These values are common code between supplicant and command_net.c
 */

/*
 * #ifdef CONFIG_WIFI_MONITOR || #ifdef	__SUPPORT_WIFI__
 */

#define WIFI_MONITOR_EV_DATA_SIZE	52 //64->52, 20170223_trinity

/* Supplicant -> App Event Buffer */

typedef struct wifi_monitor_msg_buffer {
    UINT    cmd;
    UINT    data_len;
    ULONG  value;  //__SUPPORT_ASD__, 20170223_trinity
    CHAR    data[WIFI_MONITOR_EV_DATA_SIZE];
} wifi_monitor_msg_buf_t;

#define WIFI_EVENT_SEND_MSG_TO_APP	0x01
#define WIFI_EVENT_SEND_MSG_TO_UART	0x02

#define WIFI_CMD_P2P_READ_AP_STR	0
#define WIFI_CMD_P2P_READ_PIN		1
#define WIFI_CMD_P2P_READ_MAIN_STR	2
#define WIFI_CMD_P2P_READ_GID_STR	3
#define WIFI_CMD_SEND_DHCP_RECONFIG	4
#define WIFI_CMD_ASD_BCN_NOTI		5		/* Notification */

#if 0
/* __SUPPORT_VIRTUAL_ONELINK__ */
#define WIFI_CMD_FACTORY_PROMISC	6		/* Promiscuous mode */
struct smart_config_send_event{
  int freq;
  int size;
  u8 data[3];
};
typedef enum promisc_cmd {
	PROMISC_CMD_IDLE = 0,
	PROMISC_CMD_ENABLE = 1,
	PROMISC_CMD_DISABLE = 2,
	PROMISC_CMD_FREQ,
	PROMISC_CMD_SSID,
	PROMISC_CMD_PWD,
	PROMISC_CMD_PAIRWISE,
	PROMISC_CMD_APP_ADDRESS,
	PROMISC_CMD_VOL_DONE,
#if 1 /* FEATURE_VOL_ONE_EVENT */
	PROMISC_CMD_ALL_DATA,
#endif
#if 1//def VOL_EVENT_THREAD
	PROMISC_SCONFIG_COMPLETE,
	PROMISC_CHANNEL_STARTING = 11,
	PROMISC_CHANNEL_EVENT = 12,
	PROMISC_CHANNEL_EVENT_START = 13,
	PROMISC_CONNNECTION_DATA_EVENT = 14,
	PROMISC_CONNNECTION_DATA_EVENT_S = 15,
	PROMISC_CMD_CANCEL = 16,
#endif
	PROMISC_CMD_DONE,
}PROMISC_CMD;

typedef enum vol_pairwise_set {
	VOL_PAIRWISE_OPEN = 0,
	VOL_PAIRWISE_WEP40 = 1,
	VOL_PAIRWISE_TKIP = 2,
	VOL_PAIRWISE_CCMP = 4,
	VOL_PAIRWISE_WEP104=5,
	VOL_PAIRWISE_UNKNOWN,
	VOL_PAIRWISE_END,
}VOL_PAIRWISE_SET;

#if 1//def VOL_EVENT_THREAD	
struct vol_event_send{
	int event_type;
	int channel;
	u8 ssid[32];
	int ssid_len;
	u8 passwd[128];
	int passwd_len;
	int pairwise;
	u8 address[4];
};

#define VOL_EVENT_Q_COUNT 5
extern OAL_QUEUE	vol_queue;
extern ULONG		vol_queuebuf[VOL_EVENT_Q_COUNT];
#endif
#endif
/////////////////////////////////////////////////////////////////////////////

/****************************************************************/

typedef void (*da16x_eloop_timeout_handler)(void *da16x_eloop_data,
						void *user_ctx);

int da16x_eloop_init(void);

#define ELOOP_ALL_CTX (void *) -1

/* SCAN Timer */
#define	ONE_SEC_TICK			100
#define	USEC_100_TICK			10

#define	SCAN_TIMER_INIT_SEC		1
#define	SCAN_TIMER_INTERVAL_SEC		5

extern	TimerHandle_t	supp_scan_timer;
extern	UCHAR		*supp_timer_name;

extern int da16x_eloop_register_timeout(unsigned int secs,
					unsigned int usecs,
			   		da16x_eloop_timeout_handler handler,
			   		void *eloop_data,
					void *user_data);

extern int da16x_eloop_cancel_timeout(da16x_eloop_timeout_handler handler,
			 		void *da16x_eloop_data,
					void *user_data);

extern int da16x_eloop_cancel_timeout_one(da16x_eloop_timeout_handler handler,
			     void *eloop_data, void *user_data,
			     struct os_reltime *remaining);

extern int da16x_eloop_is_timeout_registered(da16x_eloop_timeout_handler handler,
					void *da16x_eloop_data,
					void *user_data);

extern int da16x_eloop_deplete_timeout(unsigned int req_secs,
					unsigned int req_usecs,
			  		da16x_eloop_timeout_handler handler,
					void *da16x_eloop_data,
			  		void *user_data);

/* EOF */
#endif /* SUPP_ELOOP_H */
