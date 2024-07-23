/**
 ****************************************************************************************
 *
 * @file sample_defs.h
 *
 * @brief Definition of Sample apps.
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

#ifndef __SAMPLE_DEFS_H__
#define	__SAMPLE_DEFS_H__

/* For common */
#define	TCP_SERVER_KEEPALIVE_TIME			7200		// 2 Hours

/* For TCP samples */
#define SAMPLE_TCP_SVR						"TCPS"
#define SAMPLE_TCP_SVR_DPM					"TCPS_DPM"
#define SAMPLE_TCP_CLI						"TCPC"
#define SAMPLE_TCP_CLI_DPM					"TCPC_DPM"
#define SAMPLE_TCP_CLI_KA					"TCPC_KA"
#define SAMPLE_TCP_CLI_KA_DPM				"TCPC_KA_DPM"
#define SAMPLE_ALL_USED_DPM_MNG				"ALL_DPM_MNG"
#define SAMPLE_BC_CHK_L_DPM_MNG				"BC_L_DPM_MNG"
#define TCP_SVR_TEST_PORT					10190
#define TCP_CLI_TEST_PORT					10192
#define TCP_CLI_KA_TEST_PORT				10193

/* For UDP samples */
#define SAMPLE_UDP_SVR_DPM                  "UDPS_DPM"
#define SAMPLE_UDP_SOCK                     "UDP"
#define SAMPLE_UDP_CLI_DPM                  "UDPC_DPM"
#define UDP_SVR_TEST_PORT					10194
#define UDP_CLI_TEST_PORT					10195

/* For HTTP samples */
#define	SAMPLE_HTTP_CLIENT					"HTTP_CLT_APP"
#define	SAMPLE_HTTP_CLIENT_DPM				"HTTP_CLT_DPM_APP"
#define	SAMPLE_HTTP_SERVER					"HTTP_SVR_APP"

/* For Websocket samples */
#define	SAMPLE_WEBSOCKET_CLIENT				"WEBSOCKET_CLT_APP"

/* For OTA samples */
#define	SAMPLE_OTA_UPDATE					"OTA_UPDATE_APP"

/* For TLS samples */
#define SAMPLE_TLS_SVR						"TLS_SVR"
#define SAMPLE_TLS_SVR_DPM					"TLS_SVR_DPM"
#define SAMPLE_TLS_CLI						"TLS_CLI"
#define SAMPLE_TLS_CLI_DPM					"TLS_CLI_DPM"
#define TLS_CLI_TEST_PORT					10196
#define TLS_SVR_TEST_PORT					10197

/* For DTLS samples */
#define SAMPLE_DTLS_SVR						"DTLS_SVR"
#define SAMPLE_DTLS_SVR_DPM					"DTLS_SVR_DPM"
#define SAMPLE_DTLS_CLI						"DTLS_CLI"
#define SAMPLE_DTLS_CLI_DPM					"DTLS_CLI_DPM"
#define DTLS_CLI_TEST_PORT					10198
#define DTLS_SVR_TEST_PORT					10199

/* For CoAP samples */
#define SAMPLE_COAP_CLI_DPM					"COAP_CLI_DPM"
#define COAP_CLI_REQ_TEST_PORT				10200
#define COAP_CLI_OBS_TEST_PORT				10201

/* For MQTT sample */
#define	SAMPLE_MQTT_CLIENT					"MQTT_CLIENT"

/* For Cipher-Suite sample */
#define	SAMPLE_CIPHER_SUITE					"CIPHER_SUITE"

/* For SCAN result sample */
#define	SAMPLE_SCAN_RESULT					"SCAN_RES"

/* For current time sample */
#define	SAMPLE_CUR_TIME						"CUR_TIME"
#define	SAMPLE_CUR_TIME_DPM					"CUR_TIME_DPM"

/* For Cipher-Suite sample */
#define	SAMPLE_DNS_QUERY					"DNS_QUERY"

/* For ThreadX sample */
#define	SAMPLE_THREADX						"THREAD_X_SMPL"

/* For RTC Timer sample */
#define	SAMPLE_RTC_TIMER					"RTC_TIMER"

/* For DualTimer sample */
#define	SAMPLE_DTIMER						"DTIMER_SMPL"

/* For WatchDog sample */
#define	SAMPLE_WATCHDOG						"WATCHDOG_SMPL"

/* For UART1 sample */
#define SAMPLE_UART1						"UART1_SMPL"

/* For GPIO sample */
#define SAMPLE_GPIO							"GPIO_SMPL"

/* For GPIO Retention sample */
#define SAMPLE_GPIO_RETENTION				"GPIO_RETENTION_SMPL"

/* For I2C sample */
#define SAMPLE_I2C							"I2C_SMPL"

/* For SDIO sample */
#define SAMPLE_SDIO							"SDIO_SMPL"

/* For I2S sample */
#define SAMPLE_I2S							"I2S_SMPL"

/* For PWM sample */
#define SAMPLE_PWM							"PWM_SMPL"

/* For ADC sample */
#define SAMPLE_ADC							"ADC_SMPL"

/* For SD/eMMC sample */
#define SAMPLE_SD_EMMC						"SD_EMMC_SMPL"

/* For SPI sample */
#define SAMPLE_SPI							"SPI_SMPL"

#define SAMPLE_SPI_SLAVE					"spi_slave"

/* For RTC wakeup sample */
#define SAMPLE_WAKEUP                       "WAKEUP_SMPL"

/* For Serial-Flash sample */
#define SAMPLE_SFLASH						"SFLASH_SMPL"

/* For Customer_#7 tcp test sample */
#define	SAMPLE_TCP_TX_W_PING				"TCP_TX_W_PING"
#define TCP_TX_W_PING_TEST_PORT				10200

/* For checking current consumption sample */
#define	SAMPLE_CUR_CHK_TCP					"CUR_CHK_TCP"
#define	SAMPLE_CUR_CHK_UDP					"CUR_CHK_UDP"

/* For Crypto samples */
#define SAMPLE_CRYPTO_APIS					"CRYPTO_APIS"
#define SAMPLE_CRYPTO_AES					"CRYPTO_AES"
#define SAMPLE_CRYPTO_DES					"CRYPTO_DES"
#define SAMPLE_CRYPTO_HASH					"CRYPTO_HASH"
#define SAMPLE_CRYPTO_DRBG					"CRYPTO_DRBG"
#define SAMPLE_CRYPTO_RSA					"CRYPTO_RSA"
#define SAMPLE_CRYPTO_PK					"CRYPTO_PK"
#define SAMPLE_CRYPTO_DHM					"CRYPTO_DHM"
#define SAMPLE_CRYPTO_ECDSA					"CRYPTO_ECDSA"
#define SAMPLE_CRYPTO_ECDH					"CRYPTO_ECDH"
#define SAMPLE_CRYPTO_KDF					"CRYPTO_KDF"
#define SAMPLE_CRYPTO_CIPHER				"CRYPTO_CIPHER"

/* For PTIM Samples */
#define SAMPLE_UDP_TX_IN_PTIM				"UDPTX_PTIM"

/* For RTC timer DPM sample */
#define SAMPLE_DPM_TIMER_ERR				16

/* For ADC Sample */
#define ADC_SAMPLE						"ADC_SAMPLE"
#endif	/* __SAMPLE_DEFS_H__ */

/* EOF */
