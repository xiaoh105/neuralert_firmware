/*
 * EAP method registration
 * Copyright (c) 2004-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_EAP_PEER
#ifdef	CONFIG_EAP_METHOD

#include "eap_peer/eap_methods.h"
#include "eap_server/eap_methods.h"
#include "driver.h"
#include "wpa_supplicant_i.h"


/**
 * eap_register_methods - Register statically linked EAP methods
 * Returns: 0 on success, -1 or -2 on failure
 *
 * This function is called at program initialization to register all EAP
 * methods that were linked in statically.
 */
int eap_register_methods(void)
{
	int ret = 0;

#ifdef EAP_MD5
	if (ret == 0)
		ret = eap_peer_md5_register();
#endif /* EAP_MD5 */

#ifdef EAP_TLS
	if (ret == 0)
		ret = eap_peer_tls_register();
#endif /* EAP_TLS */

#ifdef EAP_MSCHAPv2
	if (ret == 0)
		ret = eap_peer_mschapv2_register();
#endif /* EAP_MSCHAPv2 */

#ifdef EAP_PEAP
	if (ret == 0)
		ret = eap_peer_peap_register();
#endif /* EAP_PEAP */

#ifdef EAP_TTLS
	if (ret == 0)
		ret = eap_peer_ttls_register();
#endif /* EAP_TTLS */

#ifdef EAP_GTC
	if (ret == 0)
		ret = eap_peer_gtc_register();
#endif /* EAP_GTC */

#ifdef EAP_FAST
	if (ret == 0)
		ret = eap_peer_fast_register();
#endif /* EAP_FAST */

#ifdef EAP_WSC
	if (ret == 0)
		ret = eap_peer_wsc_register();
#endif /* EAP_WSC */

#ifdef EAP_SERVER_IDENTITY
	if (ret == 0)
		ret = eap_server_identity_register();
#endif /* EAP_SERVER_IDENTITY */

#ifdef EAP_SERVER_WSC
	if (ret == 0)
		ret = eap_server_wsc_register();
#endif /* EAP_SERVER_WSC */

	return ret;
}

#endif	/* CONFIG_EAP_METHOD */
#endif	/* CONFIG_EAP_PEER */

/* EOF */
