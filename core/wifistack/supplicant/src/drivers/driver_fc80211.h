/**
 *****************************************************************************************
 * @file    driver_fc80211.h
 * @brief   Driver interaction with DA16X fc80211 from wpa_supplicant-2.4
 *****************************************************************************************
 */

/*
 * Driver interaction with Linux nld11/cfgd11 - definitions
 * Copyright (c) 2002-2014, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2003-2004, Instant802 Networks, Inc.
 * Copyright (c) 2005-2006, Devicescape Software, Inc.
 * Copyright (c) 2007, Johannes Berg <johannes@sipsolutions.net>
 * Copyright (c) 2009-2010, Atheros Communications
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * Copyright (c) 2020-2022 Modified by Renesas Electronics.
 */


#ifndef	__DRIVER_FC80211_H__
#define	__DRIVER_FC80211_H__


#include <stdbool.h>
#include "fc80211_copy.h"

#include "da16x_types.h"
#include "nl80211.h"
#include "cfg80211.h"
#include "supp_eloop.h"
#include "supp_driver.h"

extern int fc80211_get_interface_softmac_index(ULONG wdev_id, int ifidx);
extern int fc80211_get_interface_mode(ULONG wdev_id, int ifidx);
extern int fc80211_get_interface_channel_width(int ifidx,
			struct wpa_signal_info *sig);
extern UCHAR *fc80211_get_interface_macaddr(ULONG wdev_id, int ifidx);
extern int fc80211_set_interface(ULONG wdev_id, int ifidx,
			enum fc80211_iftype mode);
extern int fc80211_new_interface(ULONG wdev_id, int ifidx, const char *ifname,
			enum fc80211_iftype mode);
extern int fc80211_del_interface(ULONG wdev_id, int ifidx);
extern int fc80211_authenticate(int ifidx,
			struct wpa_driver_auth_params *params,
			enum fc80211_auth_type auth_type);
extern int fc80211_deauthenticate(int ifidx, const UCHAR *bssid,
			USHORT reason_code,
			int local_state_change);
extern int fc80211_associate(int ifidx,
			struct wpa_driver_associate_params *params);
/* P2P GO Inactivity */
extern int fc80211_chk_deauth_send_done(int ifidx);
/* For P2P_PS */
extern int fc80211_p2p_go_ps_on_off(int ifindex, int p2p_ps_status);
#if 1	/* by Shingu 20170524 (P2P_PS) */
extern int fc80211_p2p_go_ps_on_off(int ifindex, int p2p_ps_status);
#endif	/* 1 */
#if 1	/* by Shingu 20170524 (P2P_PS) */
extern int fc80211_p2p_go_ps_on_off(int ifindex, int p2p_ps_status);
#endif	/* 1 */
extern int fc80211_connect(int ifidx,
			struct wpa_driver_associate_params *params,
			enum fc80211_auth_type auth_type, int privacy);
extern int fc80211_disconnect(int ifidx, USHORT reason_code);
extern int fc80211_tx_mgmt(ULONG wdev_id, int ifidx,
			unsigned int freq, unsigned int wait,
			const UCHAR *buf, size_t buf_len,
			int offchanok_tx_ok,
			int no_cck, int no_ack, ULONG *cookie);
#if 0	/* by Shingu 20161012 (Not used yet) */
extern int fc80211_set_noa(int ifindex, int count, int duration, int interval,
			   int start);
extern int fc80211_set_opp_ps(int ifindex, int ctwindow);
#endif	/* 0 */
extern int fc80211_enable_rssi_report(int ifindex,
				      int rssi_min_thold, int rssi_max_thold);
extern int fc80211_get_rtctimegap(int ifindex, u64 *rtctimegap);
extern int fc80211_get_empty_rid(void);
extern int fc80211_set_app_keepalivetime(unsigned char tid, unsigned int msec , void (* callback_func)(unsigned int tid));

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* !!!     NOT DEFINED YET     !!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
extern int fc80211_get_reg(ULONG wdev_id, int ifidx,
			int (*handler)(void *), void *arg);
extern int fc80211_start_p2p_dev(ULONG wdev_id, int ifidx);
extern int fc80211_stop_p2p_dev(ULONG wdev_id, int ifidx);
extern int fc80211_set_beacon(int ifidx, struct wpa_driver_ap_params *params);
#ifdef CONFIG_OWE_TRANS
extern int fc80211_set_beacon_dual(int ifidx, struct wpa_driver_ap_params *params);
#endif // CONFIG_OWE_TRANS
extern int fc80211_new_beacon(int ifidx, struct wpa_driver_ap_params *params);
extern int fc80211_del_beacon(int ifidx);
extern int fc80211_join_ibss(int ifidx,
			struct wpa_driver_associate_params *params,
			int privacy);
extern int fc80211_leave_ibss(int ifidx);
extern int fc80211_frame_wait_cancel(int ifidx);
extern int fc80211_remain_on_channel(int ifindex, UINT freq, UINT duration);
extern int fc80211_cancel_remain_on_channel(int ifidx, ULONG cookie);
extern int fc80211_set_tx_bitrate_mask(int ifidx, int disabled);
extern int fc80211_get_scan(ULONG wdev_id, int ifidx, void *arg);
extern int fc80211_get_station(int ifindex, const u8 *sta_mac, int cmd_type, void *arg);
extern int fc80211_req_set_reg(int ifidx, char *alpha2);
extern int fc80211_get_protocol_features(int ifidx, UINT *feat);
extern int fc80211_get_softmac(ULONG wdev_id, int ifidx, void *arg);
extern int fc80211_register_action(ULONG wdev_id, int ifindex, USHORT type,
			    const UCHAR *match, size_t match_len);
extern int fc80211_unexpected_frame(int ifidx);
extern int fc80211_trigger_scan(ULONG wdev_id, int ifidx,
			struct wpa_driver_scan_params *params);
extern int fc80211_abort_scan(ULONG wdev_id, int ifindex);
extern int fc80211_free_umac_scan_results(ULONG wdev_id, int ifindex);
extern int fc80211_sched_scan(int ifidx,
			struct wpa_driver_scan_params *params,
			UINT interval);
extern int fc80211_stop_sched_scan(int ifidx);
extern int fc80211_get_key(int ifindex, u8 *seq,u8 key_idx,const u8 *mac_addr);
extern int fc80211_del_key(int ifidx, const UCHAR *addr,
			const UCHAR *seq, size_t seq_len,
			int key_idx, int set_tx);
extern int fc80211_new_key(int ifidx, const UCHAR *addr,
			const UCHAR *key, size_t key_len,
			const UCHAR *seq, size_t seq_len,
			int key_idx, int set_tx, int alg);
extern int fc80211_set_key(int ifidx, const UCHAR *addr,
			int alg, int key_idx);
extern int fc80211_set_channel(int ifidx,
			struct hostapd_freq_params *freq,
			int set_chan);
//extern int fc80211_channel_switch(int ifidx);
#ifdef CONFIG_WNM_SLEEP_MODE
extern int fc80211_set_wnm_sleep_mode(int oper, int ifindex, int intval);
#endif /* CONFIG_WNM_SLEEP_MODE */
extern int fc80211_set_bss(int ifindex, int cts, int preamble, int slot,
		    int ht_opmode, int ap_isolate, u8 *rates,int rates_len);


extern int fc80211_set_mac_acl(int ifidx, struct hostapd_acl_params *params);
#ifdef CONFIG_MESH
extern int fc80211_set_station(int ifidx, 
			struct hostapd_sta_add_params *param, void *arg);
#else
extern int fc80211_set_station(int ifidx, const UCHAR *mac_addr, void *arg);
#endif
extern int fc80211_new_station(int ifidx,
			struct hostapd_sta_add_params *params, void *arg);
extern int fc80211_del_station(int ifidx, const UCHAR *addr, int idx);
extern int fc80211_set_softmac(int ifidx, int queue, int aifs, int cwmin,
			int cwmax, int burst_time);
extern int fc80211_set_softmac_rts(int ifidx, UINT rts);
extern int fc80211_get_softmac_rts(int ifidx);
#ifdef __FRAG_ENABLE__
extern int fc80211_set_softmac_frag(int ifidx, UINT frag);
#endif
extern int fc80211_set_softmac_retry(int ifindex, u8 retry, u8 retry_long);
extern int fc80211_get_softmac_retry(int ifindex, u8 retry_long);
extern int fc80211_set_cqm(int ifidx, int threshold, int hysteresis);
#if 0
extern int fc80211_set_rekey_offload(int ifidx,
				const UCHAR *kek, const UCHAR *kck,
				const UCHAR *replay_ctr);
extern int fc80211_probe_client(int ifidx, const UCHAR *addr);
#endif

extern int fc80211_ctrl_bridge(bool bridge_control);
extern int fc80211_set_power_save(int ifidx, int ps_state, int timeout);
extern void fc80211_ctrl_ampdu_rx(int ampdu_rx_control);
extern void fc80211_set_sta_power_save(int ifindex, int pwrsave_mode);
extern void fc80211_set_ampdu_flag(int mode, int val);
extern int fc80211_get_ampdu_flag(int mode);

#ifdef CONFIG_SIMPLE_ROAMING
extern void fc80211_set_roaming_flag(int mode);
#endif /* CONFIG_SIMPLE_ROAMING */

#ifdef	CONFIG_AP
#ifdef CONFIG_AP_NONE_STA
extern int fc80211_none_station(int ifindex);
#endif /* CONFIG_AP_NONE_STA */
extern int fc80211_radar_detect(int ifidx, struct hostapd_freq_params *freq);
#endif	/* CONFIG_AP */

#ifdef	CONFIG_AP_POWER
extern int fc80211_set_ap_power(int ifindex, int type, int power, int *get_power);
#endif /* CONFIG_AP_POWER */

#ifdef	CONFIG_AP /* by Shingu 20161010 (Keep-alive) */
extern int fc80211_sta_null_send(int ifindex, u8 *mac_addr);
#endif	/* CONFIG_AP */

void driver_fc80211_process_global_ev(da16x_drv_msg_buf_t *drv_msg_buf);

/*
 * This should be replaced with user space header once one is available with C
 * library, etc..
 */

#define IFF_UP		0x1		/* interface is up	*/
#define IFF_RUNNING     0x40            /* driver signals L1 up         */
#define IFF_LOWER_UP	0x10000         /* driver signals L1 up         */
#define IFF_DORMANT	0x20000         /* driver signals dormant       */

#define IFLA_IFNAME	3
#define IFLA_MASTER	10
#define IFLA_WIRELESS	11
#define IFLA_OPERSTATE	16
#define IFLA_LINKMODE	17
#define IF_OPER_DORMANT 5
#define IF_OPER_UP	6

#define NLM_F_REQUEST	1

#define NETLINK_ROUTE	0
#define RTMGRP_LINK	1
#define RTM_BASE	0x10
#define RTM_NEWLINK	(RTM_BASE + 0)
#define RTM_DELLINK	(RTM_BASE + 1)
#define RTM_SETLINK	(RTM_BASE + 3)

#define NLMSG_ALIGNTO	4
#define NLMSG_ALIGN(len) (((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1))
#define NLMSG_HDRLEN ((int) NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(len) ((len) + NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_SPACE(len) NLMSG_ALIGN(NLMSG_LENGTH(len))
#define NLMSG_DATA(nlh) ((void*) (((char*) nlh) + NLMSG_LENGTH(0)))
#define NLMSG_NEXT(nlh,len) ((len) -= NLMSG_ALIGN((nlh)->nlmsg_len), \
			     (struct nlmsghdr *) \
			     (((char *)(nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))
#define NLMSG_OK(nlh,len) ((len) >= (int) sizeof(struct nlmsghdr) && \
			   (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) && \
			   (int) (nlh)->nlmsg_len <= (len))
#define NLMSG_PAYLOAD(nlh,len) ((nlh)->nlmsg_len - NLMSG_SPACE((len)))

#define RTA_ALIGNTO	4
#define RTA_ALIGN(len)	(((len) + RTA_ALIGNTO - 1) & ~(RTA_ALIGNTO - 1))
#define RTA_OK(rta,len)	\
		((len) > 0 && (rta)->rta_len >= sizeof(struct rtattr) && \
		(rta)->rta_len <= (len))
#define RTA_NEXT(rta,attrlen) \
		((attrlen) -= RTA_ALIGN((rta)->rta_len), \
		(struct rtattr *) (((char *)(rta)) + RTA_ALIGN((rta)->rta_len)))

#define RTA_LENGTH(len) (RTA_ALIGN(sizeof(struct rtattr)) + (len))
#define RTA_DATA(rta) ((void *) (((char *) (rta)) + RTA_LENGTH(0)))
#define RTA_PAYLOAD(rta) ((int) ((rta)->rta_len) - RTA_LENGTH(0))


/* NETLINK_ERRNO_H */

#define NLE_SUCCESS		0
#define NLE_FAILURE		1
#define NLE_INTR		2
#define NLE_BAD_SOCK		3
#define NLE_AGAIN		4
#define NLE_NOMEM		5
#define NLE_EXIST		6
#define NLE_INVAL		7
#define NLE_RANGE		8
#define NLE_MSGSIZE		9
#define NLE_OPNOTSUPP		10
#define NLE_AF_NOSUPPORT	11
#define NLE_OBJ_NOTFOUND	12
#define NLE_NOATTR		13
#define NLE_MISSING_ATTR	14
#define NLE_AF_MISMATCH		15
#define NLE_SEQ_MISMATCH	16
#define NLE_MSG_OVERFLOW	17
#define NLE_MSG_TRUNC		18
#define NLE_NOADDR		19
#define NLE_SRCRT_NOSUPPORT	20
#define NLE_MSG_TOOSHORT	21
#define NLE_MSGTYPE_NOSUPPORT	22
#define NLE_OBJ_MISMATCH	23
#define NLE_NOCACHE		24
#define NLE_BUSY		25
#define NLE_PROTO_MISMATCH	26
#define NLE_NOACCESS		27
#define NLE_PERM		28
#define NLE_PKTLOC_FILE		29
#define NLE_PARSE_ERR		30
#define NLE_NODEV		31
#define NLE_IMMUTABLE		32
#define NLE_DUMP_INTR		33

#define NLE_MAX			NLE_DUMP_INTR

/**
 * @ingroup attr
 * Iterate over a stream of attributes
 * @arg pos     loop counter, set to current attribute
 * @arg head    head of attribute stream
 * @arg len     length of attribute stream
 * @arg rem     initialized to len, holds bytes currently remaining in stream
 */
#define nla_for_each_attr(pos, head, len, rem) \
	for (pos = head, rem = len; \
		nla_ok(pos, rem); \
		pos = nla_next(pos, &(rem)))


/*
 * nla_type (16 bits)
 * +---+---+-------------------------------+
 * | N | O | Attribute Type                |
 * +---+---+-------------------------------+
 * N := Carries nested attributes
 * O := Payload stored in network byte order
 *
 * Note: The N and O flag are mutually exclusive.
 */
#define	NLA_F_NESTED		(1 << 15)
#define	NLA_F_NET_BYTEORDER	(1 << 14)
#define	NLA_TYPE_MASK		~(NLA_F_NESTED | NLA_F_NET_BYTEORDER)

#define	NLA_ALIGNTO		4
#define	NLA_ALIGN(len)		(((len) + NLA_ALIGNTO - 1) & ~(NLA_ALIGNTO - 1))
#define	NLA_HDRLEN		((int) NLA_ALIGN(sizeof(struct nlattr)))

#ifdef CONFIG_ACS
struct supp_ieee80211_channel {
	enum i3ed11_band band;
	u16 cent_freq;
	u16 hw_value;
	u32 flags;
	int max_ant_gain;
	int max_pwr;
	int max_reg_pwr;
	bool beacon_found;
	u32 orig_flags;
	int orig_mag, orig_mpwr;
};

struct supp_survey_info {
	struct supp_ieee80211_channel *channel;
	u64 channel_time;
	u64 channel_time_busy;
	u64 channel_time_ext_busy;
	u64 channel_time_rx;
	u64 channel_time_tx;
	u32 filled;
	char noise;
};
#endif /* CONFIG_ACS */
/**
 * @ingroup attr
 * Basic attribute data types
 *
 * See section @core_doc{core_attr_parse,Attribute Parsing} for more details.
 */
enum {
	NLA_UNSPEC,		/**< Unspecified type, binary data chunk */
	NLA_U8,			/**< 8 bit integer */
	NLA_U16,		/**< 16 bit integer */
	NLA_U32,		/**< 32 bit integer */
	NLA_U64,		/**< 64 bit integer */
	NLA_STRING,		/**< NUL terminated character string */
	NLA_FLAG,		/**< Flag */
	NLA_MSECS,		/**< Micro seconds (64bit) */
	NLA_NESTED,		/**< Nested attributes */
	__NLA_TYPE_MAX,
};

#define NLA_TYPE_MAX (__NLA_TYPE_MAX - 1)


/**
 * @ingroup msg
 * @defgroup attr Attributes
 * Netlink Attributes Construction/Parsing Interface
 *
 * Related sections in the development guide:
 * - @core_doc{core_attr,Netlink Attributes}
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/attr.h>
 * ~~~~
 */

/**
 * @name Attribute Size Calculation
 * @{
 */

struct nlmsghdr
{
	u32 nlmsg_len;
	u16 nlmsg_type;
	u16 nlmsg_flags;
	u32 nlmsg_seq;
	u32 nlmsg_pid;
};

struct ifinfomsg
{
	unsigned char ifi_family;
	unsigned char __ifi_pad;
	unsigned short ifi_type;
	int ifi_index;
	unsigned ifi_flags;
	unsigned ifi_change;
};

struct rtattr
{
	unsigned short rta_len;
	unsigned short rta_type;
};

/*
 *  <------- NLA_HDRLEN ------> <-- NLA_ALIGN(payload)-->
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 * |        Header       | Pad |     Payload       | Pad |
 * |   (struct nlattr)   | ing |                   | ing |
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 *  <-------------- nlattr->nla_len -------------->
 */

struct nlattr {
	u16	nla_len;
	u16	nla_type;
};

/**
 * @ingroup attr
 * Attribute validation policy.
 *
 * See section @core_doc{core_attr_parse,Attribute Parsing} for more details.
 */
struct nla_policy {
	/** Type of attribute or NLA_UNSPEC */
	u16	type;

	/** Minimal length of payload required */
	u16	minlen;

	/** Maximal length of payload allowed */
	u16	maxlen;
};

struct nlmsgerr {
	int	error;
	struct nlmsghdr msg;
};

enum {
        NETLINK_UNCONNECTED = 0,
        NETLINK_CONNECTED,
};

/**
 * @ingroup attr
 * Iterate over a stream of attributes
 * @arg pos     loop counter, set to current attribute
 * @arg head    head of attribute stream
 * @arg len     length of attribute stream
 * @arg rem     initialized to len, holds bytes currently remaining in stream
 */
#define nla_for_each_attr(pos, head, len, rem) \
	for (pos = head, rem = len; \
		nla_ok(pos, rem); \
		pos = nla_next(pos, &(rem)))

/**
 * @ingroup attr
 * Iterate over a stream of nested attributes
 * @arg pos     loop counter, set to current attribute
 * @arg nla     attribute containing the nested attributes
 * @arg rem     initialized to len, holds bytes currently remaining in stream
 */
#define nla_for_each_nested(pos, nla, rem) \
	for (pos = nLa_data(nla), rem = nla_len(nla); \
		nla_ok(pos, rem); \
		pos = nla_next(pos, &(rem)))

extern u8	nla_get_u8(struct nlattr *nla);
extern u16	nla_get_u16(struct nlattr *nla);
extern u32	nla_get_u32(struct nlattr *nla);
extern u64	nla_get_u64(struct nlattr *nla);
extern int	nla_len(const struct nlattr *nla);
extern void	*nla_data(const struct nlattr *nla);
extern int	nla_len(const struct nlattr *nla);
extern int	nla_ok(const struct nlattr *nla, int remaining);
extern void	*nla_data(const struct nlattr *nla);
extern struct	nlattr *nla_next(const struct nlattr *nla, int *remaining);

#if 0
#ifdef	NEED_AP_MLME
extern int i3ed11_freq_to_ch(int freq);
extern int i3ed11_ch_to_freq(int chan, int band);
#endif	/* NEED_AP_MLME */
#endif	/*0*/

#ifdef CONFIG_ACS
extern int fc80211_dump_survey(int ifindex, int idx, void *get_survey);
#endif /* CONFIG_ACS */

#endif	/* __DRIVER_FC80211_H__ */

/* EOF */
