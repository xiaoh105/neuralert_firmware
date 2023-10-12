/**
 ****************************************************************************************
 *
 * @file wifi_common_features.h
 *
 * @brief Define for common system features
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

#if !defined ( __WIFI_COMMON_FEATURES_H__ )
#define __WIFI_COMMON_FEATURES_H__

/// Wi-Fi features for Supplicant 2.7 ///

// 
#if defined (__SUPPORT_MESH_PORTAL__) || defined (__SUPPORT_MESH_POINT_ONLY__)
    // STA, Soft-AP, Wi-Fi Direct, Concurrent, MESH
    #undef  __TINY_MEM_MODEL__
    #undef  __LIGHT_SUPPLICANT__

#else

  //
  // Enable Wi-Fi concurrent mode in Wi-Fi core
  //
  #define __SUPPORT_WIFI_CONCURRENT_CORE__

  #if defined ( __SUPPORT_WIFI_CONCURRENT_CORE__ )
    // STA, Soft-AP, Wi-Fi Direct (Option), Concurrent-mode
    #undef  __TINY_MEM_MODEL__
    #undef  __LIGHT_SUPPLICANT__

    #undef  __SUPPORT_P2P__                  // It must be sync with CONFIG_P2P in supp_def.
    #define __SUPPORT_CONCURRENT__           // It must be sync with CONFIG_CONCURRENT in supp_def.h

  #else
      // Only STA, Soft-AP
      #define __TINY_MEM_MODEL__
      #define __LIGHT_SUPPLICANT__
  #endif //  __SUPPORT_WIFI_CONCURRENT_CORE__

#endif // !__SUPPORT_MESH_PORTAL__ && !__SUPPORT_MESH_POINT_ONLY__


// Features for Wi-Fi configuration
#define __SCAN_ON_AP_MODE__                  // Support SCAN on AP mode
#define __SUPPORT_WPA3_PERSONAL_CORE__       // WPA3-Personal SAE, OWE
#define __SUPPORT_WPA_ENTERPRISE_CORE__      // WPA Enterprise : Only 4MB feature
#define __SUPPORT_FAST_CONN_SLEEP_12__       // Sleep mode 1/2 Fast connection Feature

#undef  __SUPPORT_AP_PS_MODE                 // Support AP Power-Save mode
#undef  __SUPPORT_AUTO_CAL__                 // Auto RF calibratoin
#undef  __SUPPORT_LEGACY_WIFI_PS__           // Legacy Wi-Fi power-save mode
#undef  __SUPPORT_WIFI_CONN_NOTIFY_CB__      // Wi-Fi Connection User Callback
#undef  __SUPPORT_BSSID_SCAN__               // Scan with BSSID 
#undef  __SUPPORT_DIR_SSID_SCAN__            // Scan with User Dir SSID

// For NAT feature : Not supported on lwIP of the DA16200
#undef  __SUPPORT_MULTI_IP_IF__

#if defined ( __SUPPORT_WPA_ENTERPRISE_CORE__ )
    #define __SUPPORT_WPA3_ENTERPRISE_CORE__          // WPA3-Enaterpise
    #if defined ( __SUPPORT_WPA3_ENTERPRISE_CORE__ )
        #undef __SUPPORT_WPA3_ENTERPRISE_192B_CORE__  // UMAC, LMAC not supported yet
    #endif // __SUPPORT_WPA3_ENTERPRISE_CORE__
#else
    #undef  __SUPPORT_WPA3_ENTERPRISE_CORE__
#endif /* __SUPPORT_WPA_ENTERPRISE_CORE__ */

#if defined ( __SUPPORT_FAST_CONN_SLEEP_12__ )
    #define __SUPPORT_ASSOC_CHANNEL__
    #define __SUPPORT_DHCPC_IP_TO_STATIC_IP__

    #define __OPTIMIZE_DPM_SLP_TIME__                 // Reduce DPM Wakeup -> Sleep
    #if !defined ( __SUPPORT_WIFI_CONN_CB__ )
        #define __SUPPORT_WIFI_CONN_CB__              // Connection check after provisioning    
    #endif // __SUPPORT_WIFI_CONN_CB__

    #undef  __BOOT_CONN_TIME_PRINT__
#endif // __SUPPORT_FAST_CONN_SLEEP_12__

// For MESH configuration
#if defined ( __SUPPORT_MESH_PORTAL__ ) || defined ( __SUPPORT_MESH_POINT_ONLY__ )
    #define __SUPPORT_WPA3_PERSONAL_CORE__

    #if defined ( __SUPPORT_WPA3_PERSONAL_CORE__ )
        #define __SUPPORT_IEEE80211W__                // PMF for WPA3-Personal
        #if !defined ( __SUPPORT_WPA3_SAE__ )         // for MESH feature
            #define __SUPPORT_WPA3_SAE__
        #endif // __SUPPORT_WPA3_SAE__

        #undef  __SUPPORT_WPA3_OWE__

        #if defined ( __SUPPORT_WPA3_OWE__ )
            #undef __SUPPORT_OWE_TRANS__
        #endif // __SUPPORT_WPA3_OWE__
    #endif // __SUPPORT_WPA3_PERSONAL_CORE__

    #undef  __SUPPORT_SOFTAP_DFLT_GW__
#else
    #undef  __SUPPORT_IEEE80211W__                // PMF for WPA2-Personal

    #if defined ( __SUPPORT_WPA3_PERSONAL_CORE__ )
        #define __SUPPORT_WPA3_SAE__
        #define __SUPPORT_WPA3_OWE__

        #if defined ( __SUPPORT_WPA3_SAE__ ) || defined ( __SUPPORT_WPA3_OWE__ )
            #if !defined __SUPPORT_IEEE80211W__
                #define __SUPPORT_IEEE80211W__    // PMF for WPA3-Personal
            #endif // __SUPPORT_IEEE80211W__
        #endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_WPA3_OWE__

        #if defined ( __SUPPORT_WPA3_OWE__ )
            #undef  __SUPPORT_OWE_TRANS__
        #endif // __SUPPORT_WPA3_OWE__
    #endif // __SUPPORT_WPA3_PERSONAL_CORE__    

    #if defined ( __SUPPORT_WPA3_ENTERPRISE_CORE__ )
        #if !defined __SUPPORT_IEEE80211W__
            #define __SUPPORT_IEEE80211W__        // PMF for WPA3-Enterprise
        #endif // __SUPPORT_IEEE80211W__
    #endif // __SUPPORT_WPA3_ENTERPRISE_CORE__

    #define __APPLE_DEVICE_PATCH__
    #define __SUPPORT_SOFTAP_DFLT_GW__
#endif // __SUPPORT_MESH_PORTAL__ || __SUPPORT_MESH_POINT_ONLY__

#endif // __WIFI_COMMON_FEATURES_H__

/* EOF */
