/**
 ****************************************************************************************
 *
 * @file ieee80211.h
 *
 * @brief Header file for i3ed11 defines
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

#ifndef DIW_I3ED11_H
#define DIW_I3ED11_H

#include "if_ether.h"
#include "features.h" /* for station mac address */

#pragma GCC diagnostic ignored "-Wunused-function"

#define FCS_LEN 4

#define I3ED11_FCTL_VERS		0x0003
#define I3ED11_FCTL_FTYPE		0x000c
#define I3ED11_FCTL_STYPE		0x00f0
#define I3ED11_FCTL_TODS		0x0100
#define I3ED11_FCTL_FROMDS		0x0200
#define I3ED11_FCTL_MOREFRAGS	0x0400
#define I3ED11_FCTL_RETRY		0x0800
#define I3ED11_FCTL_PM		0x1000
#define I3ED11_FCTL_MOREDATA		0x2000
#define I3ED11_FCTL_PROTECTED	0x4000
#define I3ED11_FCTL_ORDER		0x8000
#define I3ED11_FCTL_CTL_EXT		0x0f00

#define I3ED11_SCTL_FRAG		0x000F
#define I3ED11_SCTL_SEQ		0xFFF0

#define I3ED11_FTYPE_MGMT		0x0000
#define I3ED11_FTYPE_CTL		0x0004
#define I3ED11_FTYPE_DATA		0x0008
#define I3ED11_FTYPE_EXT		0x000c

/* management */
#define I3ED11_STYPE_ASSOC_REQ	0x0000
#define I3ED11_STYPE_ASSOC_RESP	0x0010
#define I3ED11_STYPE_REASSOC_REQ	0x0020
#define I3ED11_STYPE_REASSOC_RESP	0x0030
#define I3ED11_STYPE_PROBE_REQ	0x0040
#define I3ED11_STYPE_PROBE_RESP	0x0050
#define I3ED11_STYPE_BEACON		0x0080
#define I3ED11_STYPE_ATIM		0x0090
#define I3ED11_STYPE_DISASSOC	0x00A0
#define I3ED11_STYPE_AUTH		0x00B0
#define I3ED11_STYPE_DEAUTH		0x00C0
#define I3ED11_STYPE_ACTION		0x00D0

/* control */
#define I3ED11_STYPE_CTL_EXT		0x0060
#define I3ED11_STYPE_BACK_REQ	0x0080
#define I3ED11_STYPE_BACK		0x0090
#define I3ED11_STYPE_PSPOLL		0x00A0
#define I3ED11_STYPE_RTS		0x00B0
#define I3ED11_STYPE_CTS		0x00C0
#define I3ED11_STYPE_ACK		0x00D0
#define I3ED11_STYPE_CFEND		0x00E0
#define I3ED11_STYPE_CFENDACK	0x00F0

/* data */
#define I3ED11_STYPE_DATA			0x0000
#define I3ED11_STYPE_DATA_CFACK		0x0010
#define I3ED11_STYPE_DATA_CFPOLL		0x0020
#define I3ED11_STYPE_DATA_CFACKPOLL		0x0030
#define I3ED11_STYPE_NULLFUNC		0x0040
#define I3ED11_STYPE_CFACK			0x0050
#define I3ED11_STYPE_CFPOLL			0x0060
#define I3ED11_STYPE_CFACKPOLL		0x0070
#define I3ED11_STYPE_QOS_DATA		0x0080
#define I3ED11_STYPE_QOS_DATA_CFACK		0x0090
#define I3ED11_STYPE_QOS_DATA_CFPOLL		0x00A0
#define I3ED11_STYPE_QOS_DATA_CFACKPOLL	0x00B0
#define I3ED11_STYPE_QOS_NULLFUNC		0x00C0
#define I3ED11_STYPE_QOS_CFACK		0x00D0
#define I3ED11_STYPE_QOS_CFPOLL		0x00E0
#define I3ED11_STYPE_QOS_CFACKPOLL		0x00F0

/* extension, added by 802.11ad */
#define I3ED11_STYPE_DMG_BEACON		0x0000

/* control extension - for I3ED11_FTYPE_CTL | I3ED11_STYPE_CTL_EXT */
#define I3ED11_CTL_EXT_POLL		0x2000
#define I3ED11_CTL_EXT_SPR		0x3000
#define I3ED11_CTL_EXT_GRANT	0x4000
#define I3ED11_CTL_EXT_DMG_CTS	0x5000
#define I3ED11_CTL_EXT_DMG_DTS	0x6000
#define I3ED11_CTL_EXT_SSW		0x8000
#define I3ED11_CTL_EXT_SSW_FBACK	0x9000
#define I3ED11_CTL_EXT_SSW_ACK	0xa000


#define I3ED11_SN_MASK		((I3ED11_SCTL_SEQ) >> 4)
#define I3ED11_MAX_SN		I3ED11_SN_MASK
#define I3ED11_SN_MODULO		(I3ED11_MAX_SN + 1)

static inline int i3ed11_sn_less(u16 sn1, u16 sn2)
{
	return ((sn1 - sn2) & I3ED11_SN_MASK) > (I3ED11_SN_MODULO >> 1);
}

static inline u16 i3ed11_sn_add(u16 sn1, u16 sn2)
{
	return (sn1 + sn2) & I3ED11_SN_MASK;
}

static inline u16 i3ed11_sn_inc(u16 sn)
{
	return i3ed11_sn_add(sn, 1);
}

static inline u16 i3ed11_sn_sub(u16 sn1, u16 sn2)
{
	return (sn1 - sn2) & I3ED11_SN_MASK;
}

#define I3ED11_SEQ_TO_SN(seq)	(((seq) & I3ED11_SCTL_SEQ) >> 4)
#define I3ED11_SN_TO_SEQ(ssn)	(((ssn) << 4) & I3ED11_SCTL_SEQ)

/* miscellaneous IEEE 802.11 constants */
#define I3ED11_MAX_FRAG_THRESHOLD	2352
#define I3ED11_MAX_RTS_THRESHOLD	2353
#define I3ED11_MAX_AID		2007
#define I3ED11_MAX_TIM_LEN		251
#define I3ED11_MAX_MESH_PEERINGS	63
#define I3ED11_MAX_DATA_LEN		2304
#define I3ED11_MAX_FRAME_LEN		2352
#define I3ED11_MAX_SSID_LEN		32
#define I3ED11_MAX_MESH_ID_LEN	32

/// For using memory optimization (rwnx_main.c also defined) Reduced TX ID Catagory 16 -> 8.
#define I3ED11_NUM_TIDS		8

#define I3ED11_QOS_CTL_LEN		2
#define I3ED11_QOS_CTL_TAG1D_MASK		0x0007
#define I3ED11_QOS_CTL_TID_MASK		0x000f
#define I3ED11_QOS_CTL_EOSP			0x0010
#define I3ED11_QOS_CTL_ACK_POLICY_NORMAL	0x0000
#define I3ED11_QOS_CTL_ACK_POLICY_NOACK	0x0020
#define I3ED11_QOS_CTL_ACK_POLICY_NO_EXPL	0x0040
#define I3ED11_QOS_CTL_ACK_POLICY_BLOCKACK	0x0060
#define I3ED11_QOS_CTL_ACK_POLICY_MASK	0x0060
#define I3ED11_QOS_CTL_A_MSDU_PRESENT	0x0080
#define I3ED11_QOS_CTL_MESH_CONTROL_PRESENT  0x0100
#define I3ED11_QOS_CTL_MESH_PS_LEVEL		0x0200
#define I3ED11_QOS_CTL_RSPI			0x0400

#define I3ED11_WMM_IE_AP_QOSINFO_UAPSD	(1<<7)
#define I3ED11_WMM_IE_AP_QOSINFO_PARAM_SET_CNT_MASK	0x0f

#define I3ED11_WMM_IE_STA_QOSINFO_AC_VO	(1<<0)
#define I3ED11_WMM_IE_STA_QOSINFO_AC_VI	(1<<1)
#define I3ED11_WMM_IE_STA_QOSINFO_AC_BK	(1<<2)
#define I3ED11_WMM_IE_STA_QOSINFO_AC_BE	(1<<3)
#define I3ED11_WMM_IE_STA_QOSINFO_AC_MASK	0x0f
#define I3ED11_WMM_IE_STA_QOSINFO_SP_ALL	0x00
#define I3ED11_WMM_IE_STA_QOSINFO_SP_2	0x01
#define I3ED11_WMM_IE_STA_QOSINFO_SP_4	0x02
#define I3ED11_WMM_IE_STA_QOSINFO_SP_6	0x03
#define I3ED11_WMM_IE_STA_QOSINFO_SP_MASK	0x03
#define I3ED11_WMM_IE_STA_QOSINFO_SP_SHIFT	5

#define I3ED11_HT_CTL_LEN		4

struct i3ed11_hdr
{
	__le16 frame_control;
	__le16 duration_id;
	uint8_t addr1[DIW_ETH_ALEN];
	uint8_t addr2[DIW_ETH_ALEN];
	uint8_t addr3[DIW_ETH_ALEN];
	__le16 seq_ctrl;
	uint8_t addr4[DIW_ETH_ALEN];
} __packed __aligned(2);

struct i3ed11_hdr_3addr
{
	__le16 frame_control;
	__le16 duration_id;
	uint8_t addr1[DIW_ETH_ALEN];
	uint8_t addr2[DIW_ETH_ALEN];
	uint8_t addr3[DIW_ETH_ALEN];
	__le16 seq_ctrl;
} __packed __aligned(2);

struct i3ed11_qos_hdr
{
	__le16 frame_control;
	__le16 duration_id;
	uint8_t addr1[DIW_ETH_ALEN];
	uint8_t addr2[DIW_ETH_ALEN];
	uint8_t addr3[DIW_ETH_ALEN];
	__le16 seq_ctrl;
	__le16 qos_ctrl;
} __packed __aligned(2);

static inline int i3ed11_has_tods(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_TODS)) != 0;
}

static inline int i3ed11_has_fromds(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FROMDS)) != 0;
}

static inline int i3ed11_has_a4(__le16 fc)
{
	__le16 tmp = cpu_to_le16(I3ED11_FCTL_TODS | I3ED11_FCTL_FROMDS);
	return (fc & tmp) == tmp;
}

static inline int i3ed11_has_morefrags(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_MOREFRAGS)) != 0;
}

static inline int i3ed11_has_retry(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_RETRY)) != 0;
}

static inline int i3ed11_has_pm(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_PM)) != 0;
}

static inline int i3ed11_has_moredata(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_MOREDATA)) != 0;
}

static inline int i3ed11_has_protected(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_PROTECTED)) != 0;
}

static inline int i3ed11_has_order(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_ORDER)) != 0;
}

static inline int i3ed11_is_mgmt(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT);
}

static inline int i3ed11_is_ctl(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_CTL);
}

static inline int i3ed11_is_data(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_DATA);
}

static inline int i3ed11_is_data_qos(__le16 fc)
{
	/* mask with QOS_DATA rather than I3ED11_FCTL_STYPE as we just need to check the one bit. */
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_STYPE_QOS_DATA)) ==
		   cpu_to_le16(I3ED11_FTYPE_DATA | I3ED11_STYPE_QOS_DATA);
}

static inline int i3ed11_is_data_present(__le16 fc)
{
	/* mask with 0x40 and test that that bit is clear to only return true for the data-containing substypes. */
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | 0x40)) ==
		   cpu_to_le16(I3ED11_FTYPE_DATA);
}

static inline int i3ed11_is_assoc_req(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_ASSOC_REQ);
}

static inline int i3ed11_is_assoc_resp(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_ASSOC_RESP);
}

static inline int i3ed11_is_reassoc_req(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_REASSOC_REQ);
}

static inline int i3ed11_is_reassoc_resp(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_REASSOC_RESP);
}

static inline int i3ed11_is_probe_req(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_PROBE_REQ);
}

static inline int i3ed11_is_probe_resp(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_PROBE_RESP);
}

static inline int i3ed11_is_beacon(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_BEACON);
}

static inline int i3ed11_is_atim(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_ATIM);
}

static inline int i3ed11_is_disassoc(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_DISASSOC);
}

static inline int i3ed11_is_auth(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_AUTH);
}

static inline int i3ed11_is_deauth(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_DEAUTH);
}

static inline int i3ed11_is_action(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_MGMT | I3ED11_STYPE_ACTION);
}

static inline int i3ed11_is_back_req(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_CTL | I3ED11_STYPE_BACK_REQ);
}

static inline int i3ed11_is_pspoll(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_CTL | I3ED11_STYPE_PSPOLL);
}

static inline int i3ed11_is_nullfunc(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_DATA | I3ED11_STYPE_NULLFUNC);
}

static inline int i3ed11_is_qos_nullfunc(__le16 fc)
{
	return (fc & cpu_to_le16(I3ED11_FCTL_FTYPE | I3ED11_FCTL_STYPE)) ==
		   cpu_to_le16(I3ED11_FTYPE_DATA | I3ED11_STYPE_QOS_NULLFUNC);
}

#ifdef FEATURE_UMAC_MESH 
struct ieee80211s_hdr {
	u8 flags;
	u8 ttl;
	__le32 seqnum;
	u8 eaddr1[DIW_ETH_ALEN];
	u8 eaddr2[DIW_ETH_ALEN];
} __packed __aligned(2);

/* Mesh flags */
#define MESH_FLAGS_AE_A4 	0x1
#define MESH_FLAGS_AE_A5_A6	0x2
#define MESH_FLAGS_AE		0x3
#define MESH_FLAGS_PS_DEEP	0x4

/// mesh PREQ element flags
enum i3ed11_preq_flags {
	/// proactive PREP subfield
	I3ED11_PREQ_PROACTIVE_PREP_FLAG	= 1<<2,
};

/// mesh PREQ element per target flags
enum i3ed11_preq_target_flags {
	/// target only subfield
	I3ED11_PREQ_TO_FLAG	= 1<<0,
	/// unknown target HWMP sequence number subfield
	I3ED11_PREQ_USN_FLAG	= 1<<2,
};
#endif /* FEATURE_UMAC_MESH */

/// This structure refers to "Measurement Request/Report information element"
struct i3ed11_msrment_ie
{
	uint8_t token;
	uint8_t mode;
	uint8_t type;
	uint8_t request[0];
}__packed;

/// This structure refers to "Channel Switch Announcement information element"
struct i3ed11_channel_sw_ie
{
	uint8_t mode;
	uint8_t new_ch_num;
	uint8_t count;
} __packed;

/// This structure represents the "Extended Channel Switch Announcement element"
struct i3ed11_ext_chansw_ie
{
	uint8_t mode;
	uint8_t new_operating_class;
	uint8_t new_ch_num;
	uint8_t count;
} __packed;

/// wide bandwidth channel switch IE
struct i3ed11_wide_bw_chansw_ie
{
	uint8_t new_channel_width;
	uint8_t new_center_freq_seg0, new_center_freq_seg1;
} __packed;

/// mesh channel switch parameters IE
struct i3ed11_mesh_chansw_params_ie
{
	uint8_t mesh_ttl;
	uint8_t mesh_flags;
	__le16 mesh_reason;
	__le16 mesh_pre_value;
}__packed;

/// secondary channel offset IE
struct i3ed11_sec_chan_offs_ie
{
	uint8_t sec_chan_offs;
}__packed;

/// This structure refers to "Traffic Indication Map information element"
 struct i3ed11_tim_ie
{
	uint8_t dtim_count;
	uint8_t dtim_period;
	uint8_t bitmap_ctrl;
	/* variable size: 1 - 251 bytes */
	uint8_t virtual_map[1];
}__packed;

#ifdef FEATURE_UMAC_MESH
/// This structure refers to "Mesh Configuration information element"
struct i3ed11_meshconf_ie
 {
	u8 meshconf_psel;
	u8 meshconf_pmetric;
	u8 meshconf_congest;
	u8 meshconf_synch;
	u8 meshconf_auth;
	u8 meshconf_form;
	u8 meshconf_cap;
} __packed;

/// Mesh Configuration IE capability field flags
enum mesh_config_capab_flags
{
	/// STA is willing to establish additional mesh peerings with other mesh STAs
	I3ED11_MESHCONF_CAPAB_ACCEPT_PLINKS		= 0x01,
	/// STA forwards MSDUs
	I3ED11_MESHCONF_CAPAB_FORWARDING		= 0x08,
	/// TBTT adjustment procedure  is ongoing
	I3ED11_MESHCONF_CAPAB_TBTT_ADJUSTING	= 0x20,
	/// STA is in deep sleep mode or has neighbors in deep sleep mode
	I3ED11_MESHCONF_CAPAB_POWER_SAVE_LEVEL	= 0x40,
};

/* mesh channel switch parameters element's flag indicator */
#define DIW_EID_CHAN_SWITCH_PARAM_TX_RESTRICT BIT(0)
#define DIW_EID_CHAN_SWITCH_PARAM_INITIATOR BIT(1)
#define DIW_EID_CHAN_SWITCH_PARAM_REASON BIT(2)
#endif

/// This structure refers to "Root Announcement information element"
struct i3ed11_rann_ie
{
	uint8_t rann_flags;
	uint8_t rann_hopcount;
	uint8_t rann_ttl;
	uint8_t rann_addr[DIW_ETH_ALEN];
	__le32 rann_seq;
	__le32 rann_interval;
	__le32 rann_metric;
}__packed;

#ifdef FEATURE_UMAC_MESH
enum i3ed11_rann_flags {
	RANN_FLAG_IS_GATE = 1 << 0,
};
#endif /* FEATURE_UMAC_MESH */

#define WLAN_SA_QUERY_TR_ID_LEN 2

struct i3ed11_mgmt
{
	__le16 frame_control;
	__le16 duration;
	uint8_t da[DIW_ETH_ALEN];
	uint8_t sa[DIW_ETH_ALEN];
	uint8_t bssid[DIW_ETH_ALEN];
	__le16 seq_ctrl;
	union
	{
		struct
		{
			__le16 auth_alg;
			__le16 auth_transaction;
			__le16 status_code;
			uint8_t variable[0];
		} __packed auth;
		struct
		{
			__le16 reason_code;
		} __packed deauth;
		struct
		{
			__le16 capab_info;
			__le16 listen_interval;
			uint8_t variable[0];
		}__packed assoc_req;
		struct
		{
			__le16 capab_info;
			__le16 status_code;
			__le16 aid;
			uint8_t variable[0];
		}__packed assoc_resp, reassoc_resp;
		struct
		{
			__le16 capab_info;
			__le16 listen_interval;
			uint8_t current_ap[DIW_ETH_ALEN];
			uint8_t variable[0];
		} __packed reassoc_req;
		struct
		{
			__le16 reason_code;
		} __packed disassoc;
		struct
		{
			__le64 timestamp;
			__le16 beacon_int;
			__le16 capab_info;
			uint8_t variable[0];
		} __packed beacon;
		struct
		{
			uint8_t variable[1];
		} __packed probe_req;
		struct
		{
			__le64 timestamp;
			__le16 beacon_int;
			__le16 capab_info;
			uint8_t variable[0];
		} __packed probe_resp;
		struct
		{
			uint8_t category;
			union
			{
				struct
				{
					uint8_t action_code;
					uint8_t dialog_token;
					uint8_t status_code;
					uint8_t variable[0];
				} __packed wme_action;
				struct
				{
					uint8_t action_code;
					uint8_t variable[0];
				} __packed chan_switch;
				struct
				{
					uint8_t action_code;
					struct i3ed11_ext_chansw_ie data;
					uint8_t variable[0];
				} __packed ext_chan_switch;
				struct
				{
					uint8_t action_code;
					uint8_t dialog_token;
					uint8_t element_id;
					uint8_t length;
					struct i3ed11_msrment_ie msr_elem;
				} __packed measurement;
				struct
				{
					uint8_t action_code;
					uint8_t dialog_token;
					__le16 capab;
					__le16 timeout;
					__le16 start_seq_num;
				} __packed addba_req;
				struct
				{
					uint8_t action_code;
					uint8_t dialog_token;
					__le16 status;
					__le16 capab;
					__le16 timeout;
				} __packed addba_resp;
				struct
				{
					uint8_t action_code;
					__le16 params;
					__le16 reason_code;
				} __packed delba;
				struct
				{
					uint8_t action_code;
					uint8_t variable[0];
				} __packed self_prot;
				struct
				{
					uint8_t action_code;
					uint8_t variable[0];
				} __packed mesh_action;
				struct
				{
					uint8_t action;
					uint8_t trans_id[WLAN_SA_QUERY_TR_ID_LEN];
				} __packed sa_query;
				struct
				{
					uint8_t action;
					uint8_t smps_control;
				} __packed ht_smps;
				struct
				{
					uint8_t action_code;
					uint8_t chanwidth;
				} __packed ht_notify_cw;
				struct
				{
					uint8_t action_code;
					uint8_t dialog_token;
					__le16 capability;
					uint8_t variable[0];
				} __packed tdls_discover_resp;
				struct
				{
					uint8_t action_code;
					uint8_t operating_mode;
				} __packed vht_opmode_notif;
			} u;
		} __packed action;
	} u;
} __packed __aligned(2);


/// Supported Rates value encodings in 802.11n-2009 7.3.2.2.
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY	127

/// mgmt header + 1 byte category code.
#define I3ED11_MIN_ACTION_SIZE offsetof(struct i3ed11_mgmt, u.action.u)


/// Management MIC information element (IEEE 802.11w).
struct i3ed11_mmie
{
	uint8_t element_id;
	uint8_t length;
	__le16 key_id;
	uint8_t sequence_number[6];
	uint8_t mic[8];
}__packed;

/// Management MIC information element (IEEE 802.11w) for GMAC and CMAC-256.
struct i3ed11_mmie_16
{
	uint8_t element_id;
	uint8_t length;
	__le16 key_id;
	uint8_t sequence_number[6];
	uint8_t mic[16];
}__packed;

struct i3ed11_vendor_ie
{
	uint8_t element_id;
	uint8_t len;
	uint8_t oui[3];
	uint8_t oui_type;
}__packed;

/// Control frames.
struct i3ed11_rts
{
	__le16 frame_control;
	__le16 duration;
	uint8_t ra[DIW_ETH_ALEN];
	uint8_t ta[DIW_ETH_ALEN];
} __packed __aligned(2);

struct i3ed11_cts
{
	__le16 frame_control;
	__le16 duration;
	uint8_t ra[DIW_ETH_ALEN];
} __packed __aligned(2);

struct i3ed11_pspoll
{
	__le16 frame_control;
	__le16 aid;
	uint8_t bssid[DIW_ETH_ALEN];
	uint8_t ta[DIW_ETH_ALEN];
} __packed __aligned(2);

/* Peer-to-Peer IE attribute related definitions. */

/// Identifies type of peer-to-peer attribute.
enum i3ed11_p2p_attr_id
{
	I3ED11_P2P_ATTR_STATUS = 0,
	I3ED11_P2P_ATTR_MINOR_REASON,
	I3ED11_P2P_ATTR_CAPABILITY,
	I3ED11_P2P_ATTR_DEVICE_ID,
	I3ED11_P2P_ATTR_GO_INTENT,
	I3ED11_P2P_ATTR_GO_CONFIG_TIMEOUT,
	I3ED11_P2P_ATTR_LISTEN_CHANNEL,
	I3ED11_P2P_ATTR_GROUP_BSSID,
	I3ED11_P2P_ATTR_EXT_LISTEN_TIMING,
	I3ED11_P2P_ATTR_INTENDED_IFACE_ADDR,
	I3ED11_P2P_ATTR_MANAGABILITY,
	I3ED11_P2P_ATTR_CHANNEL_LIST,
	I3ED11_P2P_ATTR_ABSENCE_NOTICE,
	I3ED11_P2P_ATTR_DEVICE_INFO,
	I3ED11_P2P_ATTR_GROUP_INFO,
	I3ED11_P2P_ATTR_GROUP_ID,
	I3ED11_P2P_ATTR_INTERFACE,
	I3ED11_P2P_ATTR_OPER_CHANNEL,
	I3ED11_P2P_ATTR_INVITE_FLAGS,
	/* 19 - 220: Reserved */
	I3ED11_P2P_ATTR_VENDOR_SPECIFIC = 221,

	I3ED11_P2P_ATTR_MAX
};

/* Notice of Absence attribute - described in P2P spec 4.1.14 */
/* Typical max value used here */
#define I3ED11_P2P_NOA_DESC_MAX	4

struct i3ed11_p2p_noa_desc
{
	uint8_t count;
	__le32 duration;
	__le32 interval;
	__le32 start_time;
}__packed ;

struct i3ed11_p2p_noa_attr
{
	uint8_t index;
	uint8_t oppps_ctw; /* oppps ct window */
	struct i3ed11_p2p_noa_desc desc[I3ED11_P2P_NOA_DESC_MAX];
}__packed;

#define I3ED11_P2P_OPPPS_ENABLE_BIT		BIT(7)
#define I3ED11_P2P_OPPPS_CTWINDOW_MASK	0x7F

/// HT Block Ack Request
struct i3ed11_bar
{
	__le16 frame_control;
	__le16 duration;
	__u8 ra[DIW_ETH_ALEN];
	__u8 ta[DIW_ETH_ALEN];
	__le16 control;
	__le16 start_seq_num;
}__packed;

/* Block Ack Request Control Masks */
#define I3ED11_BAR_CTRL_ACK_POLICY_NORMAL	0x0000
#define I3ED11_BAR_CTRL_MULTI_TID		0x0002
#define I3ED11_BAR_CTRL_CBMTID_COMPRESSED_BA	0x0004
#define I3ED11_BAR_CTRL_TID_INFO_MASK	0xf000
#define I3ED11_BAR_CTRL_TID_INFO_SHIFT	12

#define I3ED11_HT_MCS_MASK_LEN		10

/// MCS information
struct i3ed11_mcs_info
{
	uint8_t rx_mask[I3ED11_HT_MCS_MASK_LEN];
	__le16 rx_highest;
	uint8_t tx_params;
	uint8_t reserved[3];
}__packed;

/* 802.11n HT capability MSC set */
#define I3ED11_HT_MCS_RX_HIGHEST_MASK	0x3ff
#define I3ED11_HT_MCS_TX_DEFINED		0x01
#define I3ED11_HT_MCS_TX_RX_DIFF		0x02
/* value 0 == 1 stream etc */
#define I3ED11_HT_MCS_TX_MAX_STREAMS_MASK	0x0C
#define I3ED11_HT_MCS_TX_MAX_STREAMS_SHIFT	2
#define		I3ED11_HT_MCS_TX_MAX_STREAMS	4
#define I3ED11_HT_MCS_TX_UNEQUAL_MODULATION	0x10

#define I3ED11_HT_MCS_UNEQUAL_MODULATION_START 33
#define I3ED11_HT_MCS_UNEQUAL_MODULATION_START_BYTE \
	(I3ED11_HT_MCS_UNEQUAL_MODULATION_START / 8)

/// HT capabilities
struct i3ed11_ht_cap
{
	__le16 cap_info;
	uint8_t ampdu_params_info;

	/* 16 bytes MCS information */
	struct i3ed11_mcs_info mcs;

	__le16 extended_ht_cap_info;
	__le32 tx_BF_cap_info;
	uint8_t antenna_selection_info;
}__packed;

#define I3ED11_HT_CAP_LDPC_CODING		0x0001
#define I3ED11_HT_CAP_SUP_WIDTH_20_40	0x0002
#define I3ED11_HT_CAP_SM_PS			0x000C
#define I3ED11_HT_CAP_SM_PS_SHIFT	2
#define I3ED11_HT_CAP_GRN_FLD		0x0010
#define I3ED11_HT_CAP_SGI_20		0x0020
#define I3ED11_HT_CAP_SGI_40		0x0040
#define I3ED11_HT_CAP_TX_STBC		0x0080
#define I3ED11_HT_CAP_RX_STBC		0x0300
#define I3ED11_HT_CAP_RX_STBC_SHIFT	8
#define I3ED11_HT_CAP_DELAY_BA		0x0400
#define I3ED11_HT_CAP_MAX_AMSDU		0x0800
#define I3ED11_HT_CAP_DSSSCCK40		0x1000
#define I3ED11_HT_CAP_RESERVED		0x2000
#define I3ED11_HT_CAP_40MHZ_INTOLERANT	0x4000
#define I3ED11_HT_CAP_LSIG_TXOP_PROT	0x8000

#define I3ED11_HT_EXT_CAP_PCO		0x0001
#define I3ED11_HT_EXT_CAP_PCO_TIME	0x0006
#define I3ED11_HT_EXT_CAP_PCO_TIME_SHIFT	1
#define I3ED11_HT_EXT_CAP_MCS_FB		0x0300
#define I3ED11_HT_EXT_CAP_MCS_FB_SHIFT	8
#define I3ED11_HT_EXT_CAP_HTC_SUP		0x0400
#define I3ED11_HT_EXT_CAP_RD_RESPONDER	0x0800

#define I3ED11_HT_AMPDU_PARM_FACTOR		0x03
#define I3ED11_HT_AMPDU_PARM_DENSITY	0x1C
#define I3ED11_HT_AMPDU_PARM_DENSITY_SHIFT	2

/// Maximum length of AMPDU that the STA can receive.
/* Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets) */
enum i3ed11_max_ampdu_length_exp
{
	I3ED11_HT_MAX_AMPDU_8K = 0,
	I3ED11_HT_MAX_AMPDU_16K = 1,
	I3ED11_HT_MAX_AMPDU_32K = 2,
	I3ED11_HT_MAX_AMPDU_64K = 3
};

/// Minimum MPDU start spacing
enum i3ed11_min_mpdu_spacing
{
	/// No restriction
	I3ED11_HT_MPDU_DENSITY_NONE = 0,
	/// 1/4 usec
	I3ED11_HT_MPDU_DENSITY_0_25 = 1,
	/// 1/2 usec
	I3ED11_HT_MPDU_DENSITY_0_5 = 2,
	/// 1  usec
	I3ED11_HT_MPDU_DENSITY_1 = 3,
	/// 2 usec
	I3ED11_HT_MPDU_DENSITY_2 = 4,
	/// 4 usec
	I3ED11_HT_MPDU_DENSITY_4 = 5,
	/// 8 usec
	I3ED11_HT_MPDU_DENSITY_8 = 6,
	/// 16 usec
	I3ED11_HT_MPDU_DENSITY_16 = 7
};

/// HT operation IE
struct i3ed11_ht_operation
{
	uint8_t primary_chan;
	uint8_t ht_param;
	__le16 operation_mode;
	__le16 stbc_param;
	uint8_t basic_set[16];
} __packed;

/* for ht_param */
#define I3ED11_HT_PARAM_CHA_SEC_OFFSET		0x03
#define I3ED11_HT_PARAM_CHA_SEC_NONE		0x00
#define I3ED11_HT_PARAM_CHA_SEC_ABOVE		0x01
#define I3ED11_HT_PARAM_CHA_SEC_BELOW		0x03
#define I3ED11_HT_PARAM_CHAN_WIDTH_ANY		0x04
#define I3ED11_HT_PARAM_RIFS_MODE			0x08

/* for operation_mode */
#define I3ED11_HT_OP_MODE_PROTECTION			0x0003
#define I3ED11_HT_OP_MODE_PROTECTION_NONE		0
#define I3ED11_HT_OP_MODE_PROTECTION_NONMEMBER	1
#define I3ED11_HT_OP_MODE_PROTECTION_20MHZ		2
#define I3ED11_HT_OP_MODE_PROTECTION_NONHT_MIXED	3
#define I3ED11_HT_OP_MODE_NON_GF_STA_PRSNT		0x0004
#define I3ED11_HT_OP_MODE_NON_HT_STA_PRSNT		0x0010

/* for stbc_param */
#define I3ED11_HT_STBC_PARAM_DUAL_BEACON		0x0040
#define I3ED11_HT_STBC_PARAM_DUAL_CTS_PROT		0x0080
#define I3ED11_HT_STBC_PARAM_STBC_BEACON		0x0100
#define I3ED11_HT_STBC_PARAM_LSIG_TXOP_FULLPROT	0x0200
#define I3ED11_HT_STBC_PARAM_PCO_ACTIVE		0x0400
#define I3ED11_HT_STBC_PARAM_PCO_PHASE		0x0800


/* block-ack parameters */
#define I3ED11_ADDBA_PARAM_POLICY_MASK		0x0002
#define I3ED11_ADDBA_PARAM_TID_MASK			0x003C
#define I3ED11_ADDBA_PARAM_BUF_SIZE_MASK	0xFFC0
#define I3ED11_DELBA_PARAM_TID_MASK			0xF000
#define I3ED11_DELBA_PARAM_INITIATOR_MASK	0x0800

#define I3ED11_MIN_AMPDU_BUF 0x8
#define I3ED11_MAX_AMPDU_BUF 0x40

/* Authentication algorithms */
#define DIW_AUTH_OPEN 0
#define DIW_AUTH_SHARED_KEY 1
#define DIW_AUTH_FT 2
#define DIW_AUTH_SAE 3
#define DIW_AUTH_LEAP 128

#define DIW_AUTH_CHALLENGE_LEN 128

#define DIW_CAPAB_ESS		(1<<0)
#define DIW_CAPAB_IBSS		(1<<1)

#define DIW_CAPAB_IS_STA_BSS(cap)	\
	(!((cap) & (DIW_CAPAB_ESS | DIW_CAPAB_IBSS)))

#define DIW_CAPAB_CF_POLLABLE	(1<<2)
#define DIW_CAPAB_CF_POLL_REQUEST	(1<<3)
#define DIW_CAPAB_PRIVACY		(1<<4)
#define DIW_CAPAB_SHORT_PREAMBLE	(1<<5)
#define DIW_CAPAB_PBCC		(1<<6)
#define DIW_CAPAB_CHANNEL_AGILITY	(1<<7)
#define DIW_CAPAB_SPECTRUM_MGMT	(1<<8)
#define DIW_CAPAB_QOS		(1<<9)
#define DIW_CAPAB_SHORT_SLOT_TIME	(1<<10)
#define DIW_CAPAB_APSD		(1<<11)
#define DIW_CAPAB_RADIO_MEASURE	(1<<12)
#define DIW_CAPAB_DSSS_OFDM	(1<<13)
#define DIW_CAPAB_DEL_BACK	(1<<14)
#define DIW_CAPAB_IMM_BACK	(1<<15)

#define I3ED11_SPCT_MSR_RPRT_MODE_LATE	(1<<0)
#define I3ED11_SPCT_MSR_RPRT_MODE_INCAPABLE	(1<<1)
#define I3ED11_SPCT_MSR_RPRT_MODE_REFUSED	(1<<2)

#define I3ED11_SPCT_MSR_RPRT_TYPE_BASIC	0
#define I3ED11_SPCT_MSR_RPRT_TYPE_CCA	1
#define I3ED11_SPCT_MSR_RPRT_TYPE_RPI	2

#define DIW_ERP_NON_ERP_PRESENT (1<<0)
#define DIW_ERP_USE_PROTECTION (1<<1)
#define DIW_ERP_BARKER_PREAMBLE (1<<2)

enum
{
	DIW_ERP_PREAMBLE_SHORT = 0,
	DIW_ERP_PREAMBLE_LONG = 1,
};

/// Status codes.
enum i3ed11_statuscode
{
	DIW_STATUS_SUCCESS = 0,
	DIW_STATUS_UNSPECIFIED_FAILURE = 1,
	DIW_STATUS_CAPS_UNSUPPORTED = 10,
	DIW_STATUS_REASSOC_NO_ASSOC = 11,
	DIW_STATUS_ASSOC_DENIED_UNSPEC = 12,
	DIW_STATUS_NOT_SUPPORTED_AUTH_ALG = 13,
	DIW_STATUS_UNKNOWN_AUTH_TRANSACTION = 14,
	DIW_STATUS_CHALLENGE_FAIL = 15,
	DIW_STATUS_AUTH_TIMEOUT = 16,
	DIW_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA = 17,
	DIW_STATUS_ASSOC_DENIED_RATES = 18,
	/* 802.11b */
	DIW_STATUS_ASSOC_DENIED_NOSHORTPREAMBLE = 19,
	DIW_STATUS_ASSOC_DENIED_NOPBCC = 20,
	DIW_STATUS_ASSOC_DENIED_NOAGILITY = 21,
	/* 802.11h */
	DIW_STATUS_ASSOC_DENIED_NOSPECTRUM = 22,
	DIW_STATUS_ASSOC_REJECTED_BAD_POWER = 23,
	DIW_STATUS_ASSOC_REJECTED_BAD_SUPP_CHAN = 24,
	/* 802.11g */
	DIW_STATUS_ASSOC_DENIED_NOSHORTTIME = 25,
	DIW_STATUS_ASSOC_DENIED_NODSSSOFDM = 26,
	/* 802.11w */
	DIW_STATUS_ASSOC_REJECTED_TEMPORARILY = 30,
	DIW_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION = 31,
	/* 802.11i */
	DIW_STATUS_INVALID_IE = 40,
	DIW_STATUS_INVALID_GROUP_CIPHER = 41,
	DIW_STATUS_INVALID_PAIRWISE_CIPHER = 42,
	DIW_STATUS_INVALID_AKMP = 43,
	DIW_STATUS_UNSUPP_RSN_VERSION = 44,
	DIW_STATUS_INVALID_RSN_IE_CAP = 45,
	DIW_STATUS_CIPHER_SUITE_REJECTED = 46,
	/* 802.11e */
	DIW_STATUS_UNSPECIFIED_QOS = 32,
	DIW_STATUS_ASSOC_DENIED_NOBANDWIDTH = 33,
	DIW_STATUS_ASSOC_DENIED_LOWACK = 34,
	DIW_STATUS_ASSOC_DENIED_UNSUPP_QOS = 35,
	DIW_STATUS_REQUEST_DECLINED = 37,
	DIW_STATUS_INVALID_QOS_PARAM = 38,
	DIW_STATUS_CHANGE_TSPEC = 39,
	DIW_STATUS_WAIT_TS_DELAY = 47,
	DIW_STATUS_NO_DIRECT_LINK = 48,
	DIW_STATUS_STA_NOT_PRESENT = 49,
	DIW_STATUS_STA_NOT_QSTA = 50,
	/* 802.11s */
	DIW_STATUS_ANTI_CLOG_REQUIRED = 76,
	DIW_STATUS_FCG_NOT_SUPP = 78,
	DIW_STATUS_STA_NO_TBTT = 78,
	/* 802.11ad */
	DIW_STATUS_REJECTED_WITH_SUGGESTED_CHANGES = 39,
	DIW_STATUS_REJECTED_FOR_DELAY_PERIOD = 47,
	DIW_STATUS_REJECT_WITH_SCHEDULE = 83,
	DIW_STATUS_PENDING_ADMITTING_FST_SESSION = 86,
	DIW_STATUS_PERFORMING_FST_NOW = 87,
	DIW_STATUS_PENDING_GAP_IN_BA_WINDOW = 88,
	DIW_STATUS_REJECT_U_PID_SETTING = 89,
	DIW_STATUS_REJECT_DSE_BAND = 96,
	DIW_STATUS_DENIED_WITH_SUGGESTED_BAND_AND_CHANNEL = 99,
	DIW_STATUS_DENIED_DUE_TO_SPECTRUM_MANAGEMENT = 103,
};


/// Reason codes.
enum i3ed11_reasoncode
{
	DIW_REASON_UNSPECIFIED = 1,
	DIW_REASON_PREV_AUTH_NOT_VALID = 2,
	DIW_REASON_DEAUTH_LEAVING = 3,
	DIW_REASON_DISASSOC_DUE_TO_INACTIVITY = 4,
	DIW_REASON_DISASSOC_AP_BUSY = 5,
	DIW_REASON_CLASS2_FRAME_FROM_NONAUTH_STA = 6,
	DIW_REASON_CLASS3_FRAME_FROM_NONASSOC_STA = 7,
	DIW_REASON_DISASSOC_STA_HAS_LEFT = 8,
	DIW_REASON_STA_REQ_ASSOC_WITHOUT_AUTH = 9,
	/* 802.11h */
	DIW_REASON_DISASSOC_BAD_POWER = 10,
	DIW_REASON_DISASSOC_BAD_SUPP_CHAN = 11,
	/* 802.11i */
	DIW_REASON_INVALID_IE = 13,
	DIW_REASON_MIC_FAILURE = 14,
	DIW_REASON_4WAY_HANDSHAKE_TIMEOUT = 15,
	DIW_REASON_GROUP_KEY_HANDSHAKE_TIMEOUT = 16,
	DIW_REASON_IE_DIFFERENT = 17,
	DIW_REASON_INVALID_GROUP_CIPHER = 18,
	DIW_REASON_INVALID_PAIRWISE_CIPHER = 19,
	DIW_REASON_INVALID_AKMP = 20,
	DIW_REASON_UNSUPP_RSN_VERSION = 21,
	DIW_REASON_INVALID_RSN_IE_CAP = 22,
	DIW_REASON_IEEE8021X_FAILED = 23,
	DIW_REASON_CIPHER_SUITE_REJECTED = 24,
	/* 802.11e */
	DIW_REASON_DISASSOC_UNSPECIFIED_QOS = 32,
	DIW_REASON_DISASSOC_QAP_NO_BANDWIDTH = 33,
	DIW_REASON_DISASSOC_LOW_ACK = 34,
	DIW_REASON_DISASSOC_QAP_EXCEED_TXOP = 35,
	DIW_REASON_QSTA_LEAVE_QBSS = 36,
	DIW_REASON_QSTA_NOT_USE = 37,
	DIW_REASON_QSTA_REQUIRE_SETUP = 38,
	DIW_REASON_QSTA_TIMEOUT = 39,
	DIW_REASON_QSTA_CIPHER_NOT_SUPP = 45,
	/* 802.11s */
	DIW_REASON_MESH_PEER_CANCELED = 52,
	DIW_REASON_MESH_MAX_PEERS = 53,
	DIW_REASON_MESH_CONFIG = 54,
	DIW_REASON_MESH_CLOSE = 55,
	DIW_REASON_MESH_MAX_RETRIES = 56,
	DIW_REASON_MESH_CONFIRM_TIMEOUT = 57,
	DIW_REASON_MESH_INVALID_GTK = 58,
	DIW_REASON_MESH_INCONSISTENT_PARAM = 59,
	DIW_REASON_MESH_INVALID_SECURITY = 60,
	DIW_REASON_MESH_PATH_ERROR = 61,
	DIW_REASON_MESH_PATH_NOFORWARD = 62,
	DIW_REASON_MESH_PATH_DEST_UNREACHABLE = 63,
	DIW_REASON_MAC_EXISTS_IN_MBSS = 64,
	DIW_REASON_MESH_CHAN_REGULATORY = 65,
	DIW_REASON_MESH_CHAN = 66,
};


/// Information Element IDs.
enum i3ed11_eid
{
	DIW_EID_SSID = 0,
	DIW_EID_SUPP_RATES = 1,
	DIW_EID_FH_PARAMS = 2,
	DIW_EID_DS_PARAMS = 3,
	DIW_EID_CF_PARAMS = 4,
	DIW_EID_TIM = 5,
	DIW_EID_IBSS_PARAMS = 6,
	DIW_EID_COUNTRY = 7,
	DIW_EID_HP_PARAMS = 8,
	DIW_EID_HP_TABLE = 9,
	DIW_EID_REQUEST = 10,
	DIW_EID_QBSS_LOAD = 11,
	DIW_EID_EDCA_PARAM_SET = 12,
	DIW_EID_TSPEC = 13,
	DIW_EID_TCLAS = 14,
	DIW_EID_SCHEDULE = 15,
	DIW_EID_CHALLENGE = 16,
	/* 17-31 reserved for challenge text extension */
	DIW_EID_PWR_CONSTRAINT = 32,
	DIW_EID_PWR_CAPABILITY = 33,
	DIW_EID_TPC_REQUEST = 34,
	DIW_EID_TPC_REPORT = 35,
	DIW_EID_SUPPORTED_CHANNELS = 36,
	DIW_EID_CHANNEL_SWITCH = 37,
	DIW_EID_MEASURE_REQUEST = 38,
	DIW_EID_MEASURE_REPORT = 39,
	DIW_EID_QUIET = 40,
	DIW_EID_IBSS_DFS = 41,
	DIW_EID_ERP_INFO = 42,
	DIW_EID_TS_DELAY = 43,
	DIW_EID_TCLAS_PROCESSING = 44,
	DIW_EID_HT_CAPABILITY = 45,
	DIW_EID_QOS_CAPA = 46,
	/* 47 reserved for Broadcom */
	DIW_EID_RSN = 48,
	DIW_EID_802_15_COEX = 49,
	DIW_EID_EXT_SUPP_RATES = 50,
	DIW_EID_AP_CHAN_REPORT = 51,
	DIW_EID_NEIGHBOR_REPORT = 52,
	DIW_EID_RCPI = 53,
	DIW_EID_MOBILITY_DOMAIN = 54,
	DIW_EID_FAST_BSS_TRANSITION = 55,
	DIW_EID_TIMEOUT_INTERVAL = 56,
	DIW_EID_RIC_DATA = 57,
	DIW_EID_DSE_REGISTERED_LOCATION = 58,
	DIW_EID_SUPPORTED_REGULATORY_CLASSES = 59,
	DIW_EID_EXT_CHANSWITCH_ANN = 60,
	DIW_EID_HT_OPERATION = 61,
	DIW_EID_SECONDARY_CHANNEL_OFFSET = 62,
	DIW_EID_BSS_AVG_ACCESS_DELAY = 63,
	DIW_EID_ANTENNA_INFO = 64,
	DIW_EID_RSNI = 65,
	DIW_EID_MEASUREMENT_PILOT_TX_INFO = 66,
	DIW_EID_BSS_AVAILABLE_CAPACITY = 67,
	DIW_EID_BSS_AC_ACCESS_DELAY = 68,
	DIW_EID_TIME_ADVERTISEMENT = 69,
	DIW_EID_RRM_ENABLED_CAPABILITIES = 70,
	DIW_EID_MULTIPLE_BSSID = 71,
	DIW_EID_BSS_COEX_2040 = 72,
	DIW_EID_BSS_INTOLERANT_CHL_REPORT = 73,
	DIW_EID_OVERLAP_BSS_SCAN_PARAM = 74,
	DIW_EID_RIC_DESCRIPTOR = 75,
	DIW_EID_MMIE = 76,
	DIW_EID_ASSOC_COMEBACK_TIME = 77,
	DIW_EID_EVENT_REQUEST = 78,
	DIW_EID_EVENT_REPORT = 79,
	DIW_EID_DIAGNOSTIC_REQUEST = 80,
	DIW_EID_DIAGNOSTIC_REPORT = 81,
	DIW_EID_LOCATION_PARAMS = 82,
	DIW_EID_NON_TX_BSSID_CAP =  83,
	DIW_EID_SSID_LIST = 84,
	DIW_EID_MULTI_BSSID_IDX = 85,
	DIW_EID_FMS_DESCRIPTOR = 86,
	DIW_EID_FMS_REQUEST = 87,
	DIW_EID_FMS_RESPONSE = 88,
	DIW_EID_QOS_TRAFFIC_CAPA = 89,
	DIW_EID_BSS_MAX_IDLE_PERIOD = 90,
	DIW_EID_TSF_REQUEST = 91,
	DIW_EID_TSF_RESPOSNE = 92,
	DIW_EID_WNM_SLEEP_MODE = 93,
	DIW_EID_TIM_BCAST_REQ = 94,
	DIW_EID_TIM_BCAST_RESP = 95,
	DIW_EID_COLL_IF_REPORT = 96,
	DIW_EID_CHANNEL_USAGE = 97,
	DIW_EID_TIME_ZONE = 98,
	DIW_EID_DMS_REQUEST = 99,
	DIW_EID_DMS_RESPONSE = 100,
	DIW_EID_LINK_ID = 101,
	DIW_EID_WAKEUP_SCHEDUL = 102,
	/* 103 reserved */
	DIW_EID_CHAN_SWITCH_TIMING = 104,
	DIW_EID_PTI_CONTROL = 105,
	DIW_EID_PU_BUFFER_STATUS = 106,
	DIW_EID_INTERWORKING = 107,
	DIW_EID_ADVERTISEMENT_PROTOCOL = 108,
	DIW_EID_EXPEDITED_BW_REQ = 109,
	DIW_EID_QOS_MAP_SET = 110,
	DIW_EID_ROAMING_CONSORTIUM = 111,
	DIW_EID_EMERGENCY_ALERT = 112,
	DIW_EID_MESH_CONFIG = 113,
	DIW_EID_MESH_ID = 114,
	DIW_EID_LINK_METRIC_REPORT = 115,
	DIW_EID_CONGESTION_NOTIFICATION = 116,
	DIW_EID_PEER_MGMT = 117,
	DIW_EID_CHAN_SWITCH_PARAM = 118,
	DIW_EID_MESH_AWAKE_WINDOW = 119,
	DIW_EID_BEACON_TIMING = 120,
	DIW_EID_MCCAOP_SETUP_REQ = 121,
	DIW_EID_MCCAOP_SETUP_RESP = 122,
	DIW_EID_MCCAOP_ADVERT = 123,
	DIW_EID_MCCAOP_TEARDOWN = 124,
	DIW_EID_GANN = 125,
	DIW_EID_RANN = 126,
	DIW_EID_EXT_CAPABILITY = 127,
	/* 128, 129 reserved for Agere */
	DIW_EID_PREQ = 130,
	DIW_EID_PREP = 131,
	DIW_EID_PERR = 132,
	/* 133-136 reserved for Cisco */
	DIW_EID_PXU = 137,
	DIW_EID_PXUC = 138,
	DIW_EID_AUTH_MESH_PEER_EXCH = 139,
	DIW_EID_MIC = 140,
	DIW_EID_DESTINATION_URI = 141,
	DIW_EID_UAPSD_COEX = 142,
	DIW_EID_WAKEUP_SCHEDULE = 143,
	DIW_EID_EXT_SCHEDULE = 144,
	DIW_EID_STA_AVAILABILITY = 145,
	DIW_EID_DMG_TSPEC = 146,
	DIW_EID_DMG_AT = 147,
	DIW_EID_DMG_CAP = 148,
	/* 149 reserved for Cisco */
	DIW_EID_CISCO_VENDOR_SPECIFIC = 150,
	DIW_EID_DMG_OPERATION = 151,
	DIW_EID_DMG_BSS_PARAM_CHANGE = 152,
	DIW_EID_DMG_BEAM_REFINEMENT = 153,
	DIW_EID_CHANNEL_MEASURE_FEEDBACK = 154,
	/* 155-156 reserved for Cisco */
	DIW_EID_AWAKE_WINDOW = 157,
	DIW_EID_MULTI_BAND = 158,
	DIW_EID_ADDBA_EXT = 159,
	DIW_EID_NEXT_PCP_LIST = 160,
	DIW_EID_PCP_HANDOVER = 161,
	DIW_EID_DMG_LINK_MARGIN = 162,
	DIW_EID_SWITCHING_STREAM = 163,
	DIW_EID_SESSION_TRANSITION = 164,
	DIW_EID_DYN_TONE_PAIRING_REPORT = 165,
	DIW_EID_CLUSTER_REPORT = 166,
	DIW_EID_RELAY_CAP = 167,
	DIW_EID_RELAY_XFER_PARAM_SET = 168,
	DIW_EID_BEAM_LINK_MAINT = 169,
	DIW_EID_MULTIPLE_MAC_ADDR = 170,
	DIW_EID_U_PID = 171,
	DIW_EID_DMG_LINK_ADAPT_ACK = 172,
	/* 173 reserved for Symbol */
	DIW_EID_MCCAOP_ADV_OVERVIEW = 174,
	DIW_EID_QUIET_PERIOD_REQ = 175,
	/* 176 reserved for Symbol */
	DIW_EID_QUIET_PERIOD_RESP = 177,
	/* 178-179 reserved for Symbol */
	/* 180 reserved for ISO/IEC 20011 */
	DIW_EID_EPAC_POLICY = 182,
	DIW_EID_CLISTER_TIME_OFF = 183,
	DIW_EID_INTER_AC_PRIO = 184,
	DIW_EID_SCS_DESCRIPTOR = 185,
	DIW_EID_QLOAD_REPORT = 186,
	DIW_EID_HCCA_TXOP_UPDATE_COUNT = 187,
	DIW_EID_HL_STREAM_ID = 188,
	DIW_EID_GCR_GROUP_ADDR = 189,
	DIW_EID_ANTENNA_SECTOR_ID_PATTERN = 190,
	DIW_EID_VHT_CAPABILITY = 191,
	DIW_EID_VHT_OPERATION = 192,
	DIW_EID_EXTENDED_BSS_LOAD = 193,
	DIW_EID_WIDE_BW_CHANNEL_SWITCH = 194,
	DIW_EID_VHT_TX_POWER_ENVELOPE = 195,
	DIW_EID_CHANNEL_SWITCH_WRAPPER = 196,
	DIW_EID_AID = 197,
	DIW_EID_QUIET_CHANNEL = 198,
	DIW_EID_OPMODE_NOTIF = 199,

	DIW_EID_VENDOR_SPECIFIC = 221,
	DIW_EID_QOS_PARAMETER = 222,
	DIW_EID_CAG_NUMBER = 237,
	DIW_EID_AP_CSN = 239,
	DIW_EID_FILS_INDICATION = 240,
	DIW_EID_DILS = 241,
	DIW_EID_FRAGMENT = 242,
	DIW_EID_EXTENSION = 255
};

/* Element ID Extensions for Element ID 255 */
enum i3ed11_eid_ext {
	DIW_EID_EXT_ASSOC_DELAY_INFO = 1,
	DIW_EID_EXT_FILS_REQ_PARAMS = 2,
	DIW_EID_EXT_FILS_KEY_CONFIRM = 3,
	DIW_EID_EXT_FILS_SESSION = 4,
	DIW_EID_EXT_FILS_HLP_CONTAINER = 5,
	DIW_EID_EXT_FILS_IP_ADDR_ASSIGN = 6,
	DIW_EID_EXT_KEY_DELIVERY = 7,
	DIW_EID_EXT_FILS_WRAPPED_DATA = 8,
	DIW_EID_EXT_FILS_PUBLIC_KEY = 12,
	DIW_EID_EXT_FILS_NONCE = 13,
	DIW_EID_EXT_FUTURE_CHAN_GUIDANCE = 14,
	DIW_EID_EXT_HE_CAPABILITY = 35,
	DIW_EID_EXT_HE_OPERATION = 36,
	DIW_EID_EXT_UORA = 37,
	DIW_EID_EXT_HE_MU_EDCA = 38,
};

/// Action category code.
enum i3ed11_category
{
	DIW_CATEGORY_SPECTRUM_MGMT = 0,
	DIW_CATEGORY_QOS = 1,
	DIW_CATEGORY_DLS = 2,
	DIW_CATEGORY_BACK = 3,
	DIW_CATEGORY_PUBLIC = 4,
	DIW_CATEGORY_RADIO_MEASUREMENT = 5,
	DIW_CATEGORY_HT = 7,
	DIW_CATEGORY_SA_QUERY = 8,
	DIW_CATEGORY_PROTECTED_DUAL_OF_ACTION = 9,
	DIW_CATEGORY_WNM = 10,
	DIW_CATEGORY_WNM_UNPROTECTED = 11,
	DIW_CATEGORY_TDLS = 12,
	DIW_CATEGORY_MESH_ACTION = 13,
	DIW_CATEGORY_MULTIHOP_ACTION = 14,
	DIW_CATEGORY_SELF_PROTECTED = 15,
	DIW_CATEGORY_DMG = 16,
	DIW_CATEGORY_WMM = 17,
	DIW_CATEGORY_FST = 18,
	DIW_CATEGORY_UNPROT_DMG = 20,
	DIW_CATEGORY_VHT = 21,
	DIW_CATEGORY_VENDOR_SPECIFIC_PROTECTED = 126,
	DIW_CATEGORY_VENDOR_SPECIFIC = 127,
};

#if defined(SPECTRUM_MANAGEMENT_FOR_5GHZ) ||defined(FEATURE_UMAC_MESH)
/// SPECTRUM_MGMT action code.
enum i3ed11_spectrum_mgmt_actioncode
{
	DIW_ACTION_SPCT_MSR_REQ = 0,
	DIW_ACTION_SPCT_MSR_RPRT = 1,
	DIW_ACTION_SPCT_TPC_REQ = 2,
	DIW_ACTION_SPCT_TPC_RPRT = 3,
	DIW_ACTION_SPCT_CHL_SWITCH = 4,
};
#endif /* SPECTRUM_MANAGEMENT_FOR_5GHZ */

#ifdef FEATURE_UMAC_MESH
/// Self Protected Action codes.
enum i3ed11_self_protected_actioncode
{
	DIW_SP_RESERVED = 0,
	DIW_SP_MESH_PEERING_OPEN = 1,
	DIW_SP_MESH_PEERING_CONFIRM = 2,
	DIW_SP_MESH_PEERING_CLOSE = 3,
	DIW_SP_MGK_INFORM = 4,
	DIW_SP_MGK_ACK = 5,
};

/// Mesh action codes.
enum i3ed11_mesh_actioncode
{
	DIW_MESH_ACTION_LINK_METRIC_REPORT,	
	DIW_MESH_ACTION_HWMP_PATH_SELECTION,	
	DIW_MESH_ACTION_GATE_ANNOUNCEMENT,	
	DIW_MESH_ACTION_CONGESTION_CONTROL_NOTIFICATION,	
	DIW_MESH_ACTION_MCCA_SETUP_REQUEST,	
	DIW_MESH_ACTION_MCCA_SETUP_REPLY,	
	DIW_MESH_ACTION_MCCA_ADVERTISEMENT_REQUEST,	
	DIW_MESH_ACTION_MCCA_ADVERTISEMENT,	
	DIW_MESH_ACTION_MCCA_TEARDOWN,	
	DIW_MESH_ACTION_TBTT_ADJUSTMENT_REQUEST,	
	DIW_MESH_ACTION_TBTT_ADJUSTMENT_RESPONSE,
};
#endif /* FEATURE_UMAC_MESH */

/// Security key length.
enum i3ed11_key_len
{
	DIW_KEY_LEN_WEP40 = 5,
	DIW_KEY_LEN_WEP104 = 13,
	DIW_KEY_LEN_CCMP = 16,
	DIW_KEY_LEN_CCMP_256 = 32,
	DIW_KEY_LEN_TKIP = 32,
	DIW_KEY_LEN_AES_CMAC = 16,
	DIW_KEY_LEN_SMS4 = 32,
	DIW_KEY_LEN_GCMP = 16,
	DIW_KEY_LEN_GCMP_256 = 32,
	DIW_KEY_LEN_BIP_CMAC_256 = 32,
	DIW_KEY_LEN_BIP_GMAC_128 = 16,
	DIW_KEY_LEN_BIP_GMAC_256 = 32,
};

#define I3ED11_WEP_IV_LEN		4
#define I3ED11_WEP_ICV_LEN		4
#define I3ED11_CCMP_HDR_LEN		8
#define I3ED11_CCMP_MIC_LEN		8
#define I3ED11_CCMP_PN_LEN		6
#define I3ED11_TKIP_IV_LEN		8
#define I3ED11_TKIP_ICV_LEN		4
#define I3ED11_CMAC_PN_LEN		6

#define DIW_EXT_CAPA8_OPMODE_NOTIF	BIT(6)

#ifdef FEATURE_UMAC_MESH
/// mesh synchronization method identifier
enum
{
	/// Default synchronization method
	I3ED11_SYNC_METHOD_NEIGHBOR_OFFSET = 1,
	/// Vendor specific synchronization method that will be specified in a vendor specific information element
	I3ED11_SYNC_METHOD_VENDOR = 255,
};

/// mesh path selection protocol identifier
enum
{
	/// Default path selection protocol
	I3ED11_PATH_PROTOCOL_HWMP = 1,
	/// Vendor specific protocol that will be specified in a vendor specific information element
	I3ED11_PATH_PROTOCOL_VENDOR = 255,
};

/// mesh path selection metric identifier
enum
{
	/// the default path selection metric
	I3ED11_PATH_METRIC_AIRTIME = 1,
	/// a vendor specific metric that will be specified in a vendor specific information element
	I3ED11_PATH_METRIC_VENDOR = 255,
};

/// root mesh STA mode identifier
/* These attribute are used by d11_mesh_hwmp_root_mode to set root mesh STA mode */
enum i3ed11_root_mode_identifier
{
	/// the mesh STA is not a root mesh STA (default)
	I3ED11_ROOTMODE_NO_ROOT = 0,
	/// the mesh STA is a root mesh STA if greater than this value
	I3ED11_ROOTMODE_ROOT = 1,
	/// the mesh STA is a root mesh STA supports the proactive PREQ with proactive PREP subfield set to 0
	I3ED11_PROACTIVE_PREQ_NO_PREP = 2,
	/// the mesh STA is a root mesh STA supports the proactive PREQ with proactive PREP subfield set to 1
	I3ED11_PROACTIVE_PREQ_WITH_PREP = 3,
	/// the mesh STA is a root mesh STA supports the proactive RANN
	I3ED11_PROACTIVE_RANN = 4,
};
#endif /* FEATURE_UMAC_MESH */

/// Although the spec says 8 I'm seeing 6 in practice.
#define I3ED11_COUNTRY_IE_MIN_LEN	6

/// The Country String field of the element shall be 3 octets in length.
#define I3ED11_COUNTRY_STRING_LEN	3

#define I3ED11_COUNTRY_EXTENSION_ID 201

struct i3ed11_country_ie_triplet
{
	union
	{
		struct
		{
			uint8_t first_channel;
			uint8_t num_channels;
			s8 max_pwr;
		} __packed chans;
		struct
		{
			uint8_t reg_extension_id;
			uint8_t reg_class;
			uint8_t coverage_class;
		} __packed ext;
	};
} __packed;

enum i3ed11_timeout_interval_type
{
	/// 11r
	DIW_TIMEOUT_REASSOC_DEADLINE = 1,
	/// 11r
	DIW_TIMEOUT_KEY_LIFETIME = 2,
	/// 11w
	DIW_TIMEOUT_ASSOC_COMEBACK = 3,
};

struct i3ed11_timeout_interval_ie
{
	uint8_t type;
	__le32 value;
} __packed;

/// Block Ack Action Code.
enum i3ed11_back_actioncode
{
	DIW_ACTION_ADDBA_REQ = 0,
	DIW_ACTION_ADDBA_RESP = 1,
	DIW_ACTION_DELBA = 2,
};

/// Block Ack Parties.
enum i3ed11_back_parties
{
	DIW_BACK_RECIPIENT = 0,
	DIW_BACK_INITIATOR = 1,
};

/// SA Query action.
enum i3ed11_sa_query_action
{
	DIW_ACTION_SA_QUERY_REQUEST = 0,
	DIW_ACTION_SA_QUERY_RESPONSE = 1,
};


/* cipher suite selectors */
#define DIW_CIPHER_SUITE_USE_GROUP	0x000FAC00
#define DIW_CIPHER_SUITE_WEP40		0x000FAC01
#define DIW_CIPHER_SUITE_TKIP		0x000FAC02
/* reserved: 				0x000FAC03 */
#define DIW_CIPHER_SUITE_CCMP		0x000FAC04
#define DIW_CIPHER_SUITE_WEP104		0x000FAC05
#define DIW_CIPHER_SUITE_AES_CMAC	0x000FAC06
#define DIW_CIPHER_SUITE_GCMP		0x000FAC08
#define DIW_CIPHER_SUITE_GCMP_256	0x000FAC09
#define DIW_CIPHER_SUITE_CCMP_256	0x000FAC0A

#define DIW_CIPHER_SUITE_SMS4		0x00147201

/* AKM suite selectors */
#define DIW_AKM_SUITE_8021X		0x000FAC01
#define DIW_AKM_SUITE_PSK		0x000FAC02
#define DIW_AKM_SUITE_8021X_SHA256	0x000FAC05
#define DIW_AKM_SUITE_PSK_SHA256	0x000FAC06
#define DIW_AKM_SUITE_TDLS		0x000FAC07
#define DIW_AKM_SUITE_SAE		0x000FAC08
#define DIW_AKM_SUITE_FT_OVER_SAE	0x000FAC09

#define DIW_MAX_KEY_LEN		32

#define DIW_PMKID_LEN		16

#define DIW_OUI_WFA			0x506f9a
#define DIW_OUI_TYPE_WFA_P2P		9
#define DIW_OUI_MICROSOFT		0x0050f2
#define DIW_OUI_TYPE_MICROSOFT_WPA	1
#define DIW_OUI_TYPE_MICROSOFT_WMM	2
#define DIW_OUI_TYPE_MICROSOFT_WPS	4
#define DIW_OUI_TYPE_OWE			28

#ifdef FEATURE_MESH_STAR_LINK_CONTROL
#define DIW_OUI_DIALOG	0xd43d39 
#endif

static inline uint8_t *i3ed11_get_qos_ctl(struct i3ed11_hdr *hdr)
{
	if (i3ed11_has_a4(hdr->frame_control))
	{
		return (uint8_t *)hdr + 30;
	}
	else
	{
		return (uint8_t *)hdr + 24;
	}
}

static inline uint8_t *i3ed11_get_SA(struct i3ed11_hdr *hdr)
{
	if (i3ed11_has_a4(hdr->frame_control))
	{
		return hdr->addr4;
	}

	if (i3ed11_has_fromds(hdr->frame_control))
	{
		return hdr->addr3;
	}

	return hdr->addr2;
}

static inline uint8_t *i3ed11_get_DA(struct i3ed11_hdr *hdr)
{
	if (i3ed11_has_tods(hdr->frame_control))
	{
		return hdr->addr3;
	}
	else
	{
		return hdr->addr1;
	}
}

static inline bool i3ed11_is_robust_mgmt_frame(struct i3ed11_hdr *hdr)
{
	if (i3ed11_is_disassoc(hdr->frame_control) ||
			i3ed11_is_deauth(hdr->frame_control))
	{
		return true;
	}

	if (i3ed11_is_action(hdr->frame_control))
	{
		uint8_t *category;

		if (i3ed11_has_protected(hdr->frame_control))
		{
			return true;
		}

		category = ((uint8_t *) hdr) + 24;
		return *category != DIW_CATEGORY_PUBLIC &&
			   *category != DIW_CATEGORY_HT &&
			   *category != DIW_CATEGORY_SELF_PROTECTED &&
			   *category != DIW_CATEGORY_VENDOR_SPECIFIC;
	}

	return false;
}

static inline bool i3ed11_is_public_action(struct i3ed11_hdr *hdr,
		size_t len)
{
	struct i3ed11_mgmt *mgmt = (void *)hdr;

	if (len < I3ED11_MIN_ACTION_SIZE)
	{
		return false;
	}

	if (!i3ed11_is_action(hdr->frame_control))
	{
		return false;
	}

	return mgmt->u.action.category == DIW_CATEGORY_PUBLIC;
}

#ifdef FEATURE_UMAC_MESH
/**
 * _i3ed11_is_group_privacy_action - check if frame is a group addressed
 * privacy action frame
 * @hdr: the frame
 */
static inline bool _i3ed11_is_group_privacy_action(struct i3ed11_hdr *hdr)
{
	struct i3ed11_mgmt *mgmt = (void *)hdr;

	if (!i3ed11_is_action(hdr->frame_control) ||
	    !is_multicast_ether_addr(hdr->addr1))
		return false;

	return mgmt->u.action.category == DIW_CATEGORY_MESH_ACTION ||
	       mgmt->u.action.category == DIW_CATEGORY_MULTIHOP_ACTION;
}

/**
 * i3ed11_is_group_privacy_action - check if frame is a group addressed
 * privacy action frame
 * @wpkt: the wpkt containing the frame, length will be checked
 */
static inline bool i3ed11_is_group_privacy_action(struct wpktbuff *wpkt)
{
	if (wpkt->len < I3ED11_MIN_ACTION_SIZE)
		return false;
	return _i3ed11_is_group_privacy_action((void *)wpkt->data);
}
#endif /* FEATURE_UMAC_MESH */

static inline unsigned long i3ed11_tu_to_usec(unsigned long tu)
{
	return 1024 * tu;
}

static inline bool i3ed11_check_tim(const struct i3ed11_tim_ie *tim,
									uint8_t tim_len, u16 aid)
{
	uint8_t mask;
	uint8_t index, indexn1, indexn2;

	if (unlikely(!tim || tim_len < sizeof(*tim)))
	{
		return false;
	}

	aid &= 0x3fff;
	index = aid / 8;
	mask  = 1 << (aid & 7);

	indexn1 = tim->bitmap_ctrl & 0xfe;
	indexn2 = tim_len + indexn1 - 4;

	if (index < indexn1 || index > indexn2)
	{
		return false;
	}

	index -= indexn1;

	return !!(tim->virtual_map[index] & mask);
}

#define jiffies	xTaskGetTickCount()
#define TU_TO_JIFF(x)	(usecs_to_jiffies((x) * 1024))
#define TU_TO_EXP_TIME(x)	(jiffies + TU_TO_JIFF(x))

#endif /* DIW_I3ED11_H */

