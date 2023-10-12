/*
 * EAP-WSC common routines for Wi-Fi Protected Setup
 * Copyright (c) 2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_EAP_PEER

#include "supp_common.h"
#include "eap_defs.h"
#include "eap_common.h"
#include "wps/wps.h"
#include "eap_wsc_common.h"

#ifdef	EAP_WSC

struct wpabuf * eap_wsc_build_frag_ack(u8 id, u8 code)
{
	struct wpabuf *msg;

	msg = eap_msg_alloc(EAP_VENDOR_WFA, (EapType)EAP_VENDOR_TYPE_WSC, 2, code, id);
	if (msg == NULL) {
		da16x_wps_prt("     [%s] EAP-WSC: Failed to allocate memory for "
			   "FRAG_ACK\n", __func__);
		return NULL;
	}

	da16x_wps_prt("     [%s] EAP-WSC: Send WSC/FRAG_ACK\n", __func__);
	wpabuf_put_u8(msg, WSC_FRAG_ACK); /* Op-Code */
	wpabuf_put_u8(msg, 0); /* Flags */

	return msg;
}

#endif	/* EAP_WSC */
#endif	/* CONFIG_EAP_PEER */

/* EOF */
