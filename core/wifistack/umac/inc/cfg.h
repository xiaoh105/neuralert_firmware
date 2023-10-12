/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief Header file for macd11 Configuration Hooks for cfgd11
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
#ifndef __CFG_H
#define __CFG_H

//extern struct cfgd11_ops macd11_config_ops;

extern int queue_work(struct work_struct *work);

extern int getMacAddrMswLsw(UINT iface, ULONG *macmsw, ULONG *maclsw);
//extern void wifi_rx_packet_proc(struct wpktbuff *wpkt , unsigned int offset);
//extern void wpkt_nx_packet_sync(struct wpktbuff *wpkt);
extern int i3ed11_set_power_mgmt(struct softmac *softmac, struct net_device *dev,
								 bool enabled, int timeout);
#if 0
extern int i3ed11_set_cqm_rssi_config(struct softmac *softmac,
									  struct net_device *dev,
									  s32 rssi_thold, u32 rssi_hyst);
#endif
extern int i3ed11_cancel_roc(struct i3ed11_macctrl *macctrl,
							 ULONG cookie, bool mgmt_tx);
#if 0
extern int i3ed11_probe_client(struct softmac *softmac, struct net_device *dev,
							   const u8 *peer, ULONG *cookie);
#endif
extern int i3ed11_auth(struct softmac *softmac, struct net_device *dev,
					   struct cfgd11_auth_req *req);
extern int i3ed11_assoc(struct softmac *softmac, struct net_device *dev,
						struct cfgd11_assoc_req *req);
extern int i3ed11_deauth(struct softmac *softmac, struct net_device *dev,
						 struct cfgd11_deauth_req *req);
extern int i3ed11_add_key(struct softmac *softmac, struct net_device *dev,
						  u8 key_idx, bool pairwise, const u8 *mac_addr,
						  struct key_params *params);
extern int i3ed11_config_default_key(struct softmac *softmac,
									 struct net_device *dev, u8 key_idx, bool uni, bool multi);
extern int i3ed11_config_default_mgmt_key(struct softmac *softmac,
		struct net_device *dev, u8 key_idx);
extern int i3ed11_del_key(struct softmac *softmac, struct net_device *dev,
						  u8 key_idx, bool pairwise, const u8 *mac_addr);
extern void i3ed11_stop_p2p_device(struct softmac *softmac,
								   struct wifidev *wdev);
extern int i3ed11_cfg_get_channel(struct softmac *softmac,
								  struct wifidev *wdev,
								  struct cfgd11_chan_def *chandef);

extern int i3ed11_set_tx_power(struct softmac *softmac,
							   struct wifidev *wdev,
							   enum nld11_tx_power_setting type, int mbm);

extern int i3ed11_get_tx_power(struct softmac *softmac,
							   struct wifidev *wdev,
							   int *dbm);

extern int i3ed11_change_iface(struct softmac *softmac,
							   struct net_device *dev,
							   enum nld11_iftype type, u32 *flags,
							   struct vif_params *params);
extern int i3ed11_change_station(struct softmac *softmac,
								 struct net_device *dev, u8 *mac,
								 struct station_parameters *params);
extern int i3ed11_scan(struct softmac *softmac,
					   struct cfgd11_scan_request *req);
extern void i3ed11_mgmt_frame_register(struct softmac *softmac,
									   struct wifidev *wdev,
									   u16 frame_type, bool reg);
extern int i3ed11_set_txq_params(struct softmac *softmac,
								 struct net_device *dev,
								 struct i3ed11_txq_params *params);
extern int i3ed11_del_iface(struct softmac *softmac, struct wifidev *wdev);
extern int i3ed11_set_bitrate_mask(struct softmac *softmac,
								   struct net_device *dev,
								   const u8 *addr,
								   const struct cfgd11_bitrate_mask *mask);
extern int i3ed11_start_p2p_device(struct softmac *softmac,
								   struct wifidev *wdev);
extern int i3ed11_set_softmac_params(struct softmac *softmac, u32 changed);
extern int i3ed11_change_beacon(struct softmac *softmac, struct net_device *dev,
								struct cfgd11_beacon_data *params);
extern int i3ed11_start_ap(struct softmac *softmac, struct net_device *dev,
						   struct cfgd11_ap_settings *params);
extern int i3ed11_get_station(struct softmac *softmac, struct net_device *dev,
							  u8 *mac, struct station_info *sinfo);
extern int i3ed11_stop_ap(struct softmac *softmac, struct net_device *dev);

extern int i3ed11_get_station(struct softmac *softmac, struct net_device *dev,
							  u8 *mac, struct station_info *sinfo);

extern int i3ed11_add_station(struct softmac *softmac, struct net_device *dev,
							  u8 *mac, struct station_parameters *params);

extern int i3ed11_del_station(struct softmac *softmac, struct net_device *dev,
							  u8 *mac);

extern int i3ed11_mgmt_tx(struct softmac *softmac, struct wifidev *wdev,
						  struct cfgd11_mgmt_tx_params *params,
						  ULONG *cookie);
extern int i3ed11_cancel_remain_on_channel(struct softmac *softmac,
		struct wifidev *wdev,
		ULONG cookie);

extern int i3ed11_remain_on_channel(struct softmac *softmac,
									struct wifidev *wdev,
									struct i3ed11_channel *chan,
									unsigned int duration,
									ULONG *cookie);
#endif /* __CFG_H */
