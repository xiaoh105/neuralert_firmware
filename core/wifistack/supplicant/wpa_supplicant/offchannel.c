/*
 * wpa_supplicant - Off-channel Action frame TX/RX
 * Copyright (c) 2009-2010, Atheros Communications
 * Copyright (c) 2011, Qualcomm Atheros
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

//#ifdef	CONFIG_P2P

#include "supp_eloop.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "offchannel.h"



static struct wpa_supplicant *
wpas_get_tx_interface(struct wpa_supplicant *wpa_s, const u8 *src)
{
	struct wpa_supplicant *iface;

	if (os_memcmp(src, wpa_s->own_addr, ETH_ALEN) == 0)
		return wpa_s;

	/*
	 * Try to find a group interface that matches with the source address.
	 */
	iface = wpa_s->global->ifaces;
	while (iface) {
		if (os_memcmp(wpa_s->pending_action_src,
			      iface->own_addr, ETH_ALEN) == 0)
			break;
		iface = iface->next;
	}
	if (iface) {
		da16x_p2p_prt("[%s] P2P: Use group interface %s "
			   "instead of interface %s for Action TX\n",
			   __func__, iface->ifname, wpa_s->ifname);

		return iface;
	}

	return wpa_s;
}


static void wpas_send_action_cb(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct wpa_supplicant *iface;
	int res;
	int without_roc;

	without_roc = wpa_s->pending_action_without_roc;
	wpa_s->pending_action_without_roc = 0;

	da16x_p2p_prt("[%s] Off-channel: Send Action callback "
		"(without_roc=%d pending_action_tx=%p "
		"pending_action_tx_done=%d)\n",
		__func__,
		without_roc, wpa_s->pending_action_tx,
		!!wpa_s->pending_action_tx_done);

	if (wpa_s->pending_action_tx == NULL || wpa_s->pending_action_tx_done)
		return;

	/*
	 * This call is likely going to be on the P2P device instance if the
	 * driver uses a separate interface for that purpose. However, some
	 * Action frames are actually sent within a P2P Group and when that is
	 * the case, we need to follow power saving (e.g., GO buffering the
	 * frame for a client in PS mode or a client following the advertised
	 * NoA from its GO). To make that easier for the driver, select the
	 * correct group interface here.
	 */
	iface = wpas_get_tx_interface(wpa_s, wpa_s->pending_action_src);

	if (wpa_s->off_channel_freq != wpa_s->pending_action_freq &&
	    wpa_s->pending_action_freq != 0 &&
	    wpa_s->pending_action_freq != iface->assoc_freq) {
		da16x_p2p_prt("[%s] Off-channel: Pending Action frame TX "
			   "waiting for another freq=%u (off_channel_freq=%u "
			   "assoc_freq=%u)\n",
			   __func__,
			   wpa_s->pending_action_freq,
			   wpa_s->off_channel_freq,
			   iface->assoc_freq);

		if (without_roc && wpa_s->off_channel_freq == 0) {
			/*
			 * We may get here if wpas_send_action() found us to be
			 * on the correct channel, but remain-on-channel cancel
			 * event was received before getting here.
			 */
			da16x_p2p_prt("[%s] Off-channel: Schedule "
				"remain-on-channel to send Action frame\n",
				__func__);

			if (wpa_drv_remain_on_channel(
				wpa_s, wpa_s->pending_action_freq, 200) < 0) {
				da16x_p2p_prt("[%s] Off-channel: Failed to "
					"request driver to remain on "
					"channel (%u MHz) for Action Frame "
					"TX\n",
					__func__, wpa_s->pending_action_freq);
			} else {
				wpa_s->off_channel_freq = 0;
				wpa_s->roc_waiting_drv_freq =
					wpa_s->pending_action_freq;
			}
		}
		return;
	}

	da16x_p2p_prt("[%s] Off-channel: Sending pending Action frame to "
		MACSTR " using interface %s\n",
		__func__,
		MAC2STR(wpa_s->pending_action_dst), iface->ifname);

	res = wpa_drv_send_action(iface, wpa_s->pending_action_freq, 0,
				  wpa_s->pending_action_dst,
				  wpa_s->pending_action_src,
				  wpa_s->pending_action_bssid,
				  wpabuf_head(wpa_s->pending_action_tx),
				  wpabuf_len(wpa_s->pending_action_tx),
				  wpa_s->pending_action_no_cck);
	if (res) {
		da16x_p2p_prt("[%s] Off-channel: Failed to send the "
			"pending Action frame\n",
			__func__);
		/*
		 * Use fake TX status event to allow state machines to
		 * continue.
		 */
		offchannel_send_action_tx_status(
			wpa_s, wpa_s->pending_action_dst,
			wpabuf_head(wpa_s->pending_action_tx),
			wpabuf_len(wpa_s->pending_action_tx),
			OFFCHANNEL_SEND_ACTION_FAILED);
	}
}


/**
 * offchannel_send_action_tx_status - TX status callback
 * @wpa_s: Pointer to wpa_supplicant data
 * @dst: Destination MAC address of the transmitted Action frame
 * @data: Transmitted frame payload
 * @data_len: Length of @data in bytes
 * @result: TX status
 *
 * This function is called whenever the driver indicates a TX status event for
 * a frame sent by offchannel_send_action() using wpa_drv_send_action().
 */
void offchannel_send_action_tx_status(
	struct wpa_supplicant *wpa_s, const u8 *dst, const u8 *data,
	size_t data_len, enum offchannel_send_action_result result)
{
	if (wpa_s->pending_action_tx == NULL) {
		da16x_p2p_prt("[%s] Off-channel: Ignore Action TX status - "
			   "no pending operation\n", __func__);
		return;
	}

	if (os_memcmp(dst, wpa_s->pending_action_dst, ETH_ALEN) != 0) {
		da16x_p2p_prt("[%s] Off-channel: Ignore Action TX status - "
			"unknown destination address\n",
			__func__);
		return;
	}

	/* Accept report only if the contents of the frame matches */
	if (data_len - wpabuf_len(wpa_s->pending_action_tx) != 24 ||
	    os_memcmp(data + 24, wpabuf_head(wpa_s->pending_action_tx),
		      wpabuf_len(wpa_s->pending_action_tx)) != 0) {
		da16x_p2p_prt("[%s] Off-channel: Ignore Action TX status - "
			"mismatching contents with pending frame\n",
			__func__);

		da16x_dump("TX status frame data", data, data_len);

		return;
	}

	da16x_p2p_prt("[%s] Off-channel: Delete matching pending action frame\n",
			__func__);

	wpabuf_free(wpa_s->pending_action_tx);
	wpa_s->pending_action_tx = NULL;

	da16x_p2p_prt("[%s] Off-channel: TX status result=%d cb=%p\n",
		   __func__, result, wpa_s->pending_action_tx_status_cb);

	if (wpa_s->pending_action_tx_status_cb) {
		wpa_s->pending_action_tx_status_cb(
			wpa_s, wpa_s->pending_action_freq,
			wpa_s->pending_action_dst, wpa_s->pending_action_src,
			wpa_s->pending_action_bssid,
			data, data_len, result);
	}
}


/**
 * offchannel_send_action - Request off-channel Action frame TX
 * @wpa_s: Pointer to wpa_supplicant data
 * @freq: The frequency in MHz indicating the channel on which the frame is to
 *	transmitted or 0 for the current channel (only if associated)
 * @dst: Action frame destination MAC address
 * @src: Action frame source MAC address
 * @bssid: Action frame BSSID
 * @buf: Frame to transmit starting from the Category field
 * @len: Length of @buf in bytes
 * @wait_time: Wait time for response in milliseconds
 * @tx_cb: Callback function for indicating TX status or %NULL for now callback
 * @no_cck: Whether CCK rates are to be disallowed for TX rate selection
 * Returns: 0 on success or -1 on failure
 *
 * This function is used to request an Action frame to be transmitted on the
 * current operating channel or on another channel (off-channel). The actual
 * frame transmission will be delayed until the driver is ready on the specified
 * channel. The @wait_time parameter can be used to request the driver to remain
 * awake on the channel to wait for a response.
 */
int offchannel_send_action(struct wpa_supplicant *wpa_s, unsigned int freq,
			   const u8 *dst, const u8 *src, const u8 *bssid,
			   const u8 *buf, size_t len, unsigned int wait_time,
			   void (*tx_cb)(struct wpa_supplicant *wpa_s,
					 unsigned int freq, const u8 *dst,
					 const u8 *src, const u8 *bssid,
					 const u8 *data, size_t data_len,
					 enum offchannel_send_action_result
					 result),
			   int no_cck)
{
	da16x_p2p_prt("[%s] Off-channel: Send action frame: freq=%d dst="
		MACSTR " src=" MACSTR " bssid=" MACSTR " len=%d\n",
		__func__,
		freq, MAC2STR(dst), MAC2STR(src), MAC2STR(bssid),
		(int) len);

	wpa_s->pending_action_tx_status_cb = tx_cb;

	if (wpa_s->pending_action_tx) {
		da16x_p2p_prt("[%s] Off-channel: Dropped pending Action "
			"frame TX to " MACSTR "\n",
			__func__, MAC2STR(wpa_s->pending_action_dst));

		wpabuf_free(wpa_s->pending_action_tx);
	}
	wpa_s->pending_action_tx_done = 0;
	wpa_s->pending_action_tx = wpabuf_alloc(len);
	if (wpa_s->pending_action_tx == NULL) {
		da16x_p2p_prt("[%s] Off-channel: Failed to allocate Action "
			"frame TX buffer (len=%llu)\n",
			__func__,
			(unsigned long long) len);

		return -1;
	}
	wpabuf_put_data(wpa_s->pending_action_tx, buf, len);
	os_memcpy(wpa_s->pending_action_src, src, ETH_ALEN);
	os_memcpy(wpa_s->pending_action_dst, dst, ETH_ALEN);
	os_memcpy(wpa_s->pending_action_bssid, bssid, ETH_ALEN);
	wpa_s->pending_action_freq = freq;
	wpa_s->pending_action_no_cck = no_cck;

	if (freq != 0 && wpa_s->drv_flags & WPA_DRIVER_FLAGS_OFFCHANNEL_TX) {
		struct wpa_supplicant *iface;
		int ret;

		iface = wpas_get_tx_interface(wpa_s,
					      wpa_s->pending_action_src);
		wpa_s->action_tx_wait_time = wait_time;

		ret = wpa_drv_send_action(
			iface, wpa_s->pending_action_freq,
			wait_time, wpa_s->pending_action_dst,
			wpa_s->pending_action_src, wpa_s->pending_action_bssid,
			wpabuf_head(wpa_s->pending_action_tx),
			wpabuf_len(wpa_s->pending_action_tx),
			wpa_s->pending_action_no_cck);
		if (ret == 0)
			wpa_s->pending_action_tx_done = 1;
		return ret;
	}

	if (freq) {
		struct wpa_supplicant *tx_iface;
		tx_iface = wpas_get_tx_interface(wpa_s, src);
		if (tx_iface->assoc_freq == freq) {
			da16x_p2p_prt("Off-channel: Already on "
				"requested channel (TX interface operating "
				"channel)\n", __func__);
			freq = 0;
		}
	}

	if (wpa_s->off_channel_freq == freq || freq == 0) {
		da16x_p2p_prt("[%s] Off-channel: Already on requested "
			"channel; send Action frame immediately\n",
			__func__);

		/* TODO: Would there ever be need to extend the current
		 * duration on the channel? */
		wpa_s->pending_action_without_roc = 1;

#if 0	/* by Bright */
		da16x_eloop_cancel_timeout(wpas_send_action_cb, wpa_s, NULL);
		da16x_eloop_register_timeout(0, 0, wpas_send_action_cb, wpa_s, NULL);
#else
da16x_p2p_prt("[%s] What do I do for eloop timer ???\n", __func__);
#endif	/* 0 */
		return 0;
	}
	wpa_s->pending_action_without_roc = 0;

	if (wpa_s->roc_waiting_drv_freq == freq) {
		da16x_p2p_prt("[%s] Off-channel: Already waiting for "
			"driver to get to frequency %u MHz; continue "
			"waiting to send the Action frame\n",
			__func__, freq);
		return 0;
	}

	da16x_p2p_prt("[%s] Off-channel: Schedule Action frame to be "
		   "transmitted once the driver gets to the requested "
		   "channel\n", __func__);

	if (wait_time > wpa_s->max_remain_on_chan)
		wait_time = wpa_s->max_remain_on_chan;
	else if (wait_time == 0)
		wait_time = 20;

	if (wpa_drv_remain_on_channel(wpa_s, freq, wait_time) < 0) {
		da16x_p2p_prt("[%s] Off-channel: Failed to request driver "
			"to remain on channel (%u MHz) for Action "
			"Frame TX\n", __func__, freq);

		return -1;
	}
	wpa_s->off_channel_freq = 0;
	wpa_s->roc_waiting_drv_freq = freq;

	return 0;
}


/**
 * offchannel_send_send_action_done - Notify completion of Action frame sequence
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function can be used to cancel a wait for additional response frames on
 * the channel that was used with offchannel_send_action().
 */
void offchannel_send_action_done(struct wpa_supplicant *wpa_s)
{
	da16x_p2p_prt("[%s] Off-channel: Action frame sequence done "
			"notification\n", __func__);

	wpabuf_free(wpa_s->pending_action_tx);
	wpa_s->pending_action_tx = NULL;
	if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_OFFCHANNEL_TX &&
	    wpa_s->action_tx_wait_time)
		wpa_drv_send_action_cancel_wait(wpa_s);

	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}
}


/**
 * offchannel_remain_on_channel_cb - Remain-on-channel callback function
 * @wpa_s: Pointer to wpa_supplicant data
 * @freq: Frequency (in MHz) of the selected channel
 * @duration: Duration of the remain-on-channel operation in milliseconds
 *
 * This function is called whenever the driver notifies beginning of a
 * remain-on-channel operation.
 */
void offchannel_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
				     unsigned int freq, unsigned int duration)
{
	wpa_s->roc_waiting_drv_freq = 0;
	wpa_s->off_channel_freq = freq;
	wpas_send_action_cb(wpa_s, NULL);
}


/**
 * offchannel_cancel_remain_on_channel_cb - Remain-on-channel stopped callback
 * @wpa_s: Pointer to wpa_supplicant data
 * @freq: Frequency (in MHz) of the selected channel
 *
 * This function is called whenever the driver notifies termination of a
 * remain-on-channel operation.
 */
void offchannel_cancel_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
					    unsigned int freq)
{
	wpa_s->off_channel_freq = 0;
}


/**
 * offchannel_pending_action_tx - Check whether there is a pending Action TX
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: Pointer to pending frame or %NULL if no pending operation
 *
 * This function can be used to check whether there is a pending Action frame TX
 * operation. The returned pointer should be used only for checking whether it
 * is %NULL (no pending frame) or to print the pointer value in debug
 * information (i.e., the pointer should not be dereferenced).
 */
const void * offchannel_pending_action_tx(struct wpa_supplicant *wpa_s)
{
	return wpa_s->pending_action_tx;
}


/**
 * offchannel_clear_pending_action_tx - Clear pending Action frame TX
 * @wpa_s: Pointer to wpa_supplicant data
 */
void offchannel_clear_pending_action_tx(struct wpa_supplicant *wpa_s)
{
	wpabuf_free(wpa_s->pending_action_tx);
	wpa_s->pending_action_tx = NULL;
}


/**
 * offchannel_deinit - Deinit off-channel operations
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to free up any allocated resources for off-channel
 * operations.
 */
void offchannel_deinit(struct wpa_supplicant *wpa_s)
{
	offchannel_clear_pending_action_tx(wpa_s);

#if 0	/* by Bright */
	da16x_eloop_cancel_timeout(wpas_send_action_cb, wpa_s, NULL);
#else
da16x_p2p_prt("[%s] What do I do for eloop timer ???\n", __func__);
#endif	/* 0 */
}

//#endif	/* CONFIG_P2P */

/* EOF */
