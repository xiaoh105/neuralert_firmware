/**
 ****************************************************************************************
 *
 * @file all_used_dpm_manager_sample.c
 *
 * @brief Sample code to support all features on DPM manager
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

#include "sample_defs.h"

#if defined ( __ALL_USED_DPM_MANAGER_SAMPLE__ )

#include "sdk_type.h"
#include "da16x_types.h"

#include "user_dpm.h"
#include "user_dpm_api.h"
#include "driver.h"
#include "common_def.h"
#include "iface_defs.h"
#include "da16x_network_common.h"
#include "user_dpm_manager.h"
#include "da16x_system.h"

#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"

// test config
#undef	TCP_CLIENT_TEST
#if defined ( TCP_CLIENT_TEST )
#define	TCP_SERVER_IP_1		"192.168.0.33"
#define	TCP_SERVER_PORT_1	11111
#define	TCP_CLIENT_RECEIVE_ECHO
#undef	TCP_CLIENT_ECHO_RECOVERY
#endif	// TCP_CLIENT_TEST

#undef	UDP_CLIENT_TEST
#if defined ( UDP_CLIENT_TEST )
#define	UDP_SERVER_IP_2		"192.168.0.33"
#define	UDP_SERVER_PORT_2	22222
#undef	UDP_CLIENT_RECEIVE_ECHO
#endif	// UDP_CLIENT_TEST

#if !defined ( __LIGHT_DPM_MANAGER__ )
#undef	TCP_SERVER_TEST
#if defined ( TCP_SERVER_TEST )
#define	TCP_SERVER_PORT_3	33333
#endif	// TCP_SERVER_TEST

#undef	UDP_SERVER_TEST
#if defined ( UDP_SERVER_TEST )
#define	UDP_SERVER_PORT_4	44444
#endif	// UDP_SERVER_TEST
#endif  // !__LIGHT_DPM_MANAGER__

#define TIMER_TYPE_NONE			0
#define TIMER_TYPE_PERIODIC		1
#define TIMER_TYPE_ONETIME		2

#define REG_TYPE_NONE			0
#define REG_TYPE_TCP_SERVER		1
#define REG_TYPE_TCP_CLIENT		2
#define REG_TYPE_UDP_SERVER		3
#define REG_TYPE_UDP_CLIENT		4

#define DISABLE					0
#define ENABLE					1

#define PEER_NONE				0
#define PEER_ONLY_ONE			1
#define PEER_ANOTHER			2

#define DEBUG_LEVEL				4
#define ACK_TIMEOUT				1	// sec
#define HANDSHAKE_MIN_TIMEOUT	1	// sec
#define HANDSHAKE_MAX_TIMEOUT	4	// sec		// 8->4 20210511

#define NVR_KEY_TCP_CLI_TX_PERIOD	"tcp_tx_period"
#define DEFAULT_TCP_CLI_TX_PERIOD	9			// sec

#define NVR_KEY_UDP_CLI_TX_PERIOD	"udp_tx_period"
#define DEFAULT_UDP_CLI_TX_PERIOD	10			// sec

#define __SECURE_AP_USE_DEF_CERTIFICATE__
#define SECURE_AP_CA_CERT_SIZE		2048
#define SECURE_AP_PRIVATE_KEY_SIZE	2048
static char SECURE_SERVER_CA_CERT[SECURE_AP_CA_CERT_SIZE];
static char SECURE_SERVER_PRIVATE_KEY[SECURE_AP_PRIVATE_KEY_SIZE];

const char sample_ap_def_cert[] =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDbjCCAlYCCQCMZs/t76SmITANBgkqhkiG9w0BAQUFADB5MQswCQYDVQQGEwJL\r\n"
    "UjEOMAwGA1UECAwFU0VPVUwxDjAMBgNVBAcMBVNFT1VMMQwwCgYDVQQKDANGQ0kx\r\n"
    "DDAKBgNVBAsMA05EVDEMMAoGA1UEAwwDTEVFMSAwHgYJKoZIhvcNAQkBFhFqaW5y\r\n"
    "bzkyQG5hdmVyLmNvbTAeFw0xODAxMTUwMDAwMThaFw0yODAxMTMwMDAwMThaMHkx\r\n"
    "CzAJBgNVBAYTAktSMQ4wDAYDVQQIDAVTRU9VTDEOMAwGA1UEBwwFU0VPVUwxDDAK\r\n"
    "BgNVBAoMA0ZDSTEMMAoGA1UECwwDTkRUMQwwCgYDVQQDDANMRUUxIDAeBgkqhkiG\r\n"
    "9w0BCQEWEWppbnJvOTJAbmF2ZXIuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\r\n"
    "MIIBCgKCAQEAtIYQLQgjZIlqXf+/li2XSl1TCqmo+sirO31BNUcVBUUdFxMZEAc9\r\n"
    "RRNopnGNOCmoP6ohVR/koyBTdJbsLjD/VBnYO6fGR3oeToOamppO64pOFLu8V7mA\r\n"
    "sNTBUjiBLsr9Hu6bNN+tr8DTyAHXvKZtkrJzcfdtXYnT6B5T1L72plygbpNwR+JT\r\n"
    "rCbT9d1bcJdr+tJmwDM8tmomeCwEgRPorfS8ghgQemhCCUkpA6R+7FUu7UZc+gBY\r\n"
    "TB7PZR8Zfa8GwkKjOyKp6M+mohdfFz49oyow836sa0X3++kZ0kgp/abZELbNyO1c\r\n"
    "jg4Grx1BTyexGoUtWOMa2G+Ie9Ws9hkvDQIDAQABMA0GCSqGSIb3DQEBBQUAA4IB\r\n"
    "AQBdMEsQllTeKdCKrJk2QyNtwDpEErE1AS9N8fGdQKwAfOutU/UZJf2+8EkzQhv1\r\n"
    "EII+6NbbiCX3EV7DbYyQlwBsK8wgZx/b+HHytyc2dcPZ7FXP1RAIDdQjMdZDxTrV\r\n"
    "UH42tRgeHN+1WVYePZMfoYLAxpNXt9Y7iaqhunRVaJZTCHH1hSzdwaytAGMW/EdA\r\n"
    "dR87CHgATzhxsvIi3SNobbvPrOyEyycvXke9+d3bzHUMyk36yVv9UHSXld1jJNMB\r\n"
    "f6cD1votU/c3inx6tdWrFg7wryAzvi6dy0ypUP6qhYGNP/rf3ixVMDxd/0URG090\r\n"
    "NjVFSXS0Fsf3pY56Z13XoYtM\r\n"
    "-----END CERTIFICATE-----\r\n";
const size_t sample_ap_def_cert_len = sizeof(sample_ap_def_cert);

const char sample_ap_def_private_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIIEogIBAAKCAQEAtIYQLQgjZIlqXf+/li2XSl1TCqmo+sirO31BNUcVBUUdFxMZ\r\n"
    "EAc9RRNopnGNOCmoP6ohVR/koyBTdJbsLjD/VBnYO6fGR3oeToOamppO64pOFLu8\r\n"
    "V7mAsNTBUjiBLsr9Hu6bNN+tr8DTyAHXvKZtkrJzcfdtXYnT6B5T1L72plygbpNw\r\n"
    "R+JTrCbT9d1bcJdr+tJmwDM8tmomeCwEgRPorfS8ghgQemhCCUkpA6R+7FUu7UZc\r\n"
    "+gBYTB7PZR8Zfa8GwkKjOyKp6M+mohdfFz49oyow836sa0X3++kZ0kgp/abZELbN\r\n"
    "yO1cjg4Grx1BTyexGoUtWOMa2G+Ie9Ws9hkvDQIDAQABAoIBAHfBh/GXuIL1dg6t\r\n"
    "npct33OBvZkRO/xCKvXn/d4uLY+1bbrk/HdZlhfA7GeeZKShXe/+GOazpaVhyHtL\r\n"
    "s5pg/sD39C5++lZoeLp1K5bsTvaKZYnzkNpRQUINocG9olU8a/adK3FEAaDaNqQ0\r\n"
    "NL39Wis7HsK39WFcEaumks5LcKyjILEUJkanXNsLTaBpvk6KEIbgR+dhTcEX2AjD\r\n"
    "eMfj7baCKtFr+trGT3cap3JRG/5l+xTWVBDHHCRnuX9/02tiVfRMDWGnRkdK/AA5\r\n"
    "nBDGBj3u/5jVQVm7LjJUAh/alIjItu+R70Nf95N4qfJkJLbeleohRVAanG4Cexyt\r\n"
    "Kwr29DECgYEA24zVK/t1xYKZrfS6UyFlXETD+Vb05gu2HgrCLKi5XoMZCNMDMhYc\r\n"
    "b/FwPJWmfi5LPwPPAYdEYe0mz6KYUealoxW/xlAyJww+PrBzeOITSa7pREUC++K+\r\n"
    "Z5BS98/DurMSDRzC9EdGaqmipVeIAlCmXSMhCM5zQhNpqGBaANs6tAMCgYEA0n6O\r\n"
    "Pt1ibimxFjOvjvwrElHtRYmZOUglq6vVGG6MZHfIo7edZ7bj9hkDNcrP60/SV8Pd\r\n"
    "8yLevr25YiVxNhUzcpFLnupk10jnx6UzVjBx4KQGJnVon0ZXhBfRdfRPgiM9cH58\r\n"
    "y4IUo3CVUfepdcLfXxRLuKiuACWfAwqwIIEUC68CgYAlbgQlxHAWpSZYHuHpLEdg\r\n"
    "jKSwjJ+h3JtG0eS6wmUf9M++zmK8FkLw3sOYwJaq6m+PpbGT/CCvZUP5oCnBxMW2\r\n"
    "YQ3Z3HBAcfjmrSRylnBdXoGMTwusL8LwWw2aDAS9fqv0KlQeW0xpANSmxZ59box+\r\n"
    "Um/eVXwW5sJbn1mYzcdbSwKBgEVHhgXG05iyfCh0hnmnIX38HP3gFeA2kL955L8P\r\n"
    "04FVs2G/Ez2JgmoDPX087UXjlbVxL+HQPkPFKfjVnfdQ7wfsLll1iA8bXb3l74mU\r\n"
    "lEZ9ddC+n9qcSj2doUETUf/hHV2jI+vPOn+4lEzQGKQ7qU0f3vQ+AFboCvfzUpjA\r\n"
    "IxrdAoGAXOQgDrf02ALXZ6lxcaB4oTFOB25cuX64EFw+FBLUH9KMi4U8JxBDFmTF\r\n"
    "Bd22179Ixz6j4F6wdRgpuPHSNi0ktqrOncqL3QY0VVkHrC4Fdw1aUkrcXcn0ApXl\r\n"
    "X/eX062KvHU3oj1uoHDDdK9ssyBnMamlenv9c2ST6a9TQ0yZQao=\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";
const size_t sample_ap_def_private_key_len = sizeof(sample_ap_def_private_key);

typedef struct {
    short			wakeupCount;
    short			timer1WakeupCount;
    short			timer2WakeupCount;
    short			timer3WakeupCount;
    short			timer4WakeupCount;
    short			tcpServerWakeupCount;
    short			tcpClientWakeupCount;
    short			udpServerWakeupCount;
    short			udpClientWakeupCount;
    unsigned int	session1ConnectFlag;
    unsigned int	tcp_peer_count;
    unsigned int	tcp_peer_sent_toggle;
    unsigned int	tcp_peer1_IP;
    unsigned int	tcp_peer1_PORT;
    unsigned int	tcp_peer2_IP;		// another one
    unsigned int	tcp_peer2_PORT;		// another one
    unsigned int	udp_peer_IP;
    unsigned int	udp_peer_PORT;
    unsigned int	udpClientSendPeriod;
    unsigned int	tcpClientSendPeriod;
    char 			state[20];
} sampleParamForRtm;

sampleParamForRtm sampleParams = { 0, };

#define	BOOT_INIT_FUNC					initConfigSampleByBoot		// Boot initial callback function
#define	WAKEUP_INIT_FUNC				initConfigSampleByWakeup	// Wakeup initial callback function

#if defined ( TCP_CLIENT_TEST )
#define	TIMER1_TYPE					TIMER_TYPE_PERIODIC			// timer1 type
#else
#define	TIMER1_TYPE					TIMER_TYPE_NONE				// timer1 type
#endif	// TCP_CLIENT_TEST
#define	TIMER1_INTERVAL					7							// timer1 interval
#define	TIMER1_FUNC						timer1_callback				// timer1 callback function

#if defined ( UDP_CLIENT_TEST )
#define TIMER2_TYPE					TIMER_TYPE_PERIODIC			// timer2 type
#else
#define TIMER2_TYPE					TIMER_TYPE_NONE				// timer2 type
#endif	// UDP_CLIENT_TEST
#define	TIMER2_INTERVAL					8							// timer2 interval
#define	TIMER2_FUNC						timer2_callback				// timer2 callback function

#if defined ( TCP_SERVER_TEST )
#define	TIMER3_TYPE					TIMER_TYPE_PERIODIC			// timer3 type
#else
#define	TIMER3_TYPE					TIMER_TYPE_NONE				// timer3 type
#endif	// TCP_SERVER_TEST
#define	TIMER3_INTERVAL					13							// timer3 interval
#define	TIMER3_FUNC						timer3_callback				// timer3 callback function

#if defined ( UDP_SERVER_TEST )
#define TIMER4_TYPE					TIMER_TYPE_PERIODIC			// timer4 type
#else
#define TIMER4_TYPE					TIMER_TYPE_NONE				// timer4 type
#endif	// UDP_SERVER_TEST
#define	TIMER4_INTERVAL					14							// timer4 interval
#define	TIMER4_FUNC						timer4_callback				// timer4 callback function

#if defined ( TCP_CLIENT_TEST )
#define REGIST_SESSION_TYPE1			REG_TYPE_TCP_CLIENT			// Session Type
#define REGIST_MY_PORT_1				0							// My port no
#define REGIST_SERVER_IP_1			TCP_SERVER_IP_1				// Server ip : Client only
#define REGIST_SERVER_PORT_1			TCP_SERVER_PORT_1			// Server port : Client only
#define SESSION1_KA_INTERVAL			0							// Keep alive interval:TCP only, Sec
#define SESSION1_CONN_FUNC			connect_callback_1			// Connect callback function
#define SESSION1_RECV_FUNC			recvPacket_callback_1		// Receive callback function
#define SESSION1_CONNECT_WAIT_TIME	2							// connect wait time(SEC): TCP Cli Only
#define SESSION1_CONNECT_RETRY_COUNT	1							// connect retry count : TCP Client Only
#define SESSION1_AUTO_RECONNECT		ENABLE						// auto reconnect : TCP Client Only
#define SESSION1_SECURE_SETUP			DISABLE						// TLS enable/disable
#define SESSION1_SECURE_SETUP_FUNC	setup_secure_callback_1 	// setup tls function
#else
#define	REGIST_SESSION_TYPE1		REG_TYPE_NONE				// Session Type
#endif	// TCP_CLIENT_TEST

#if defined ( UDP_CLIENT_TEST )
#define REGIST_SESSION_TYPE2			REG_TYPE_UDP_CLIENT			// Session Type
#define REGIST_MY_PORT_2				0							// My port no
#define REGIST_SERVER_IP_2			UDP_SERVER_IP_2				// Server ip : Client only
#define REGIST_SERVER_PORT_2			UDP_SERVER_PORT_2			// Server port : Client only
#define SESSION2_KA_INTERVAL			0							// Keep alive interval:TCP only, Sec
#define SESSION2_CONN_FUNC			connect_callback_2			// Connect callback function
#define SESSION2_RECV_FUNC			recvPacket_callback_2		// Receive callback function
#define SESSION2_SECURE_SETUP			DISABLE						// DTLS enable/disable
#define SESSION2_SECURE_SETUP_FUNC	setup_secure_callback_2 	// setup tls function
#else
#define REGIST_SESSION_TYPE2			REG_TYPE_NONE				// Session Type
#endif	// UDP_CLIENT_TEST

#if defined ( TCP_SERVER_TEST )
#define REGIST_SESSION_TYPE3			REG_TYPE_TCP_SERVER			// Session Type
#define REGIST_MY_PORT_3				TCP_SERVER_PORT_3			// My port no
#define REGIST_SERVER_IP_3			"0.0.0.0"					// Server ip : Client only
#define REGIST_SERVER_PORT_3			0							// Server port : Client only
#define SESSION3_KA_INTERVAL			0							// Keep alive interval:TCP only, Sec
#define SESSION3_CONN_FUNC			connect_callback_3			// Connect callback function
#define SESSION3_RECV_FUNC			recvPacket_callback_3		// Receive callback function
#define SESSION3_SECURE_SETUP			DISABLE						// TLS enable/disable
#define SESSION3_SECURE_SETUP_FUNC	setup_secure_callback_3		// setup tls function
#else
#define	REGIST_SESSION_TYPE3		REG_TYPE_NONE				// Session Type
#endif	// TCP_SERVER_TEST

#if defined ( UDP_SERVER_TEST )
#define REGIST_SESSION_TYPE4			REG_TYPE_UDP_SERVER			// Session Type
#define REGIST_MY_PORT_4				UDP_SERVER_PORT_4			// My port no
#define REGIST_SERVER_IP_4			"0.0.0.0"					// Server ip : Client only
#define REGIST_SERVER_PORT_4			0							// Server port : Client only
#define SESSION4_KA_INTERVAL			0							// Keep alive interval : TCP only
#define SESSION4_CONN_FUNC			connect_callback_4			// Connect callback function
#define SESSION4_RECV_FUNC			recvPacket_callback_4		// Receive callback function
#define SESSION4_SECURE_SETUP			DISABLE						// DTLS enable/disable
#define SESSION4_SECURE_SETUP_FUNC	setup_secure_callback_4 	// setup tls function
#else
#define	REGIST_SESSION_TYPE4		REG_TYPE_NONE				// Session Type
#endif	// UDP_SERVER_TEST

#define	NON_VOLITALE_MEM_ADDR			(UCHAR *)&sampleParams		// non-volitale memory address
#define	NON_VOLITALE_MEM_SIZE			sizeof(sampleParamForRtm)	// non-volitale memory size

#define	EXTERN_WU_FUNCTION				external_wu_callback		// external wakeup callback function
#define ERROR_FUNCTION					error_callback				// error callback function

#define	TX_BUF1_SIZE	100
#define	TX_BUF2_SIZE	100
#define	TX_BUF3_SIZE	100
#define	TX_BUF4_SIZE	100

#define	USE_MALLOC

#if defined ( USE_MALLOC )
char	*txBuffForSession1;
char	*txBuffForSession2;
char	*txBuffForSession3;
char	*txBuffForSession3ByTimer;
char	*txBuffForSession4;
char	*txBuffForSession4ByTimer;
#else
char	txBuffForSession1[TX_BUF1_SIZE];
char	txBuffForSession2[TX_BUF2_SIZE];
char	txBuffForSession3[TX_BUF3_SIZE];
char	txBuffForSession3ByTimer[TX_BUF3_SIZE];
char	txBuffForSession4[TX_BUF4_SIZE];
char	txBuffForSession4ByTimer[TX_BUF4_SIZE];
#endif	// USE_MALLOC

#if defined ( TCP_CLIENT_TEST ) || defined ( UDP_CLIENT_TEST )
extern	long	iptolong(char *ip);
#endif	//( TCP_CLIENT_TEST ) || ( UDP_CLIENT_TEST )

#ifdef TCP_CLIENT_RECEIVE_ECHO
int		tcpReceivedEchoFlag = pdFALSE;
char	tcpRecvEchoData[TX_BUF1_SIZE];
#endif	// TCP_CLIENT_RECEIVE_ECHO

#ifdef UDP_CLIENT_RECEIVE_ECHO
int		udpReceivedEchoFlag = pdFALSE;
char	udpRecvEchoData[TX_BUF2_SIZE];
#endif	// UDP_CLIENT_RECEIVE_ECHO

static void secure_debug(void *ctx, int level, const char *file, int line,
                         const char *str)
{
    DA16X_UNUSED_ARG(ctx);

    const char *p, *basename;

    for (p = basename = file ; *p != '\0' ; p++) {
        if (*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }

    PRINTF("%s:%04d: |%d| %s\n", basename, line, level, str );
}

static int sampleApTlsRSADecrypt( void *ctx, int mode, size_t *olen,
                                  const unsigned char *input, unsigned char *output,
                                  size_t output_max_len )
{
    return ( mbedtls_rsa_pkcs1_decrypt( (mbedtls_rsa_context *) ctx, NULL, NULL, mode, olen,
                                        input, output, output_max_len ) );
}

static int sampleApTlsRSASign( void *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
                               int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
                               const unsigned char *hash, unsigned char *sig )
{
    return ( mbedtls_rsa_pkcs1_sign( (mbedtls_rsa_context *) ctx, f_rng, p_rng, mode,
                                     md_alg, hashlen, hash, sig ) );
}

static size_t sampleApTlsRSAKeyLen( void *ctx )
{
    return ( ((const mbedtls_rsa_context *) ctx)->len );
}

UINT sampleApTlsLoadCert(void)
{
#if defined ( CERT_FROM_NVRAM )
    extern int cert_flash_read(int flash_addr, char *result, size_t len);
    extern int cert_flash_write(int flash_addr, char *start_addr, size_t len);

    int ret = 0;
#endif	// CERT_FROM_NVRAM

    memset(SECURE_SERVER_CA_CERT, 0x00, SECURE_AP_CA_CERT_SIZE);

#if defined ( CERT_FROM_NVRAM )
    ret = cert_flash_read(SFLASH_CERTIFICATE_ADDR2, SECURE_SERVER_CA_CERT,
                          SECURE_AP_CA_CERT_SIZE);

    if (ret) {
        PRINTF(" [%s] Failed to read Certificate(%d)\n", __func__, ret);
        return ER_NOT_SUCCESSFUL;
    } else {
        if ( SECURE_SERVER_CA_CERT[0] != '-') {
            PRINTF(" [%s] Certificate is not registed \n", __func__);
#endif	// CERT_FROM_NVRAM

#if defined ( __SECURE_AP_USE_DEF_CERTIFICATE__ )
            //PRINTF(" [%s] Use default certificate for test\n", __func__);

            memcpy(SECURE_SERVER_CA_CERT, sample_ap_def_cert, sample_ap_def_cert_len);
#if defined ( CERT_FROM_NVRAM )
            cert_flash_write(SFLASH_CERTIFICATE_ADDR2, SECURE_SERVER_CA_CERT, SECURE_AP_CA_CERT_SIZE);
#endif	// CERT_FROM_NVRAM

#else
            return ER_NOT_FOUND;
#endif	// __SECURE_AP_USE_DEF_CERTIFICATE__

#if defined ( CERT_FROM_NVRAM )
        } else {
            PRINTF(" [%s] Certificate is registed \n", __func__);
        }
    }

#endif	// CERT_FROM_NVRAM

    return ERR_OK;
}

UINT sampleApTlsLoadPrivateKey()
{
#if defined ( CERT_FROM_NVRAM )
    extern int cert_flash_read(int flash_addr, char *result, size_t len);
    extern int cert_flash_write(int flash_addr, char *start_addr, size_t len);

    int ret = 0;
#endif	// CERT_FROM_NVRAM

    memset(SECURE_SERVER_PRIVATE_KEY, 0x00, SECURE_AP_PRIVATE_KEY_SIZE);

#if defined ( CERT_FROM_NVRAM )
    ret = cert_flash_read(SFLASH_PRIVATE_KEY_ADDR2, SECURE_SERVER_PRIVATE_KEY,
                          SECURE_AP_PRIVATE_KEY_SIZE);

    if (ret) {
        PRINTF(" [%s] Failed to read Private Key(%d)\n", __func__, ret);
        return ER_NOT_SUCCESSFUL;
    } else {
        if ( SECURE_SERVER_PRIVATE_KEY[0] != '-' ) {
            PRINTF(" [%s] Private key is not registed \n", __func__);

#endif	// CERT_FROM_NVRAM

#if defined ( __SECURE_AP_USE_DEF_CERTIFICATE__ )
            //PRINTF(" [%s] Use default certificate for test\n", __func__);

            memcpy(SECURE_SERVER_PRIVATE_KEY, sample_ap_def_private_key, sample_ap_def_private_key_len);
#if defined ( CERT_FROM_NVRAM )
            cert_flash_write(SFLASH_PRIVATE_KEY_ADDR2, SECURE_SERVER_PRIVATE_KEY,
                             SECURE_AP_PRIVATE_KEY_SIZE);
#endif	// CERT_FROM_NVRAM

#else
            return ER_NOT_FOUND;
#endif	// __SECURE_AP_USE_DEF_CERTIFICATE__

#if defined ( CERT_FROM_NVRAM )
        } else {
            PRINTF(" [%s] Private key is registed \n", __func__);
        }
    }

#endif	// CERT_FROM_NVRAM

    return ERR_OK;
}

void sample_setup_tls_server(SECURE_INFO_T *config)
{
    int ret = 0;
    UINT status = ERR_OK;

    PRINTF(" [%s] Start setup TLS Manager (config:0x%x) \n", __func__, config);

    status = sampleApTlsLoadCert();

    if (status) {
        PRINTF(RED_COLOR " [%s] Certificate is not registed(0x%02x) \n" CLEAR_COLOR, __func__, status);
        return;
    }

    status = sampleApTlsLoadPrivateKey();

    if (status) {
        PRINTF(RED_COLOR " [%s] Private key is not registed(0x%02x) \n" CLEAR_COLOR, __func__, status);
        return;
    }

    ret = mbedtls_ssl_config_defaults(config->ssl_conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);

    if (ret) {
        PRINTF("[%s:%d] Failed to set default ssl config(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    ret = mbedtls_x509_crt_parse(config->cert_crt,
                                 (UCHAR *)SECURE_SERVER_CA_CERT,
                                 strlen((char *)SECURE_SERVER_CA_CERT) + 1);

    if (ret) {
        PRINTF("[%s:%d] Failed to parse cert(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    ret = mbedtls_pk_parse_key(config->pkey_ctx,
                               (UCHAR *)SECURE_SERVER_PRIVATE_KEY,
                               strlen((char *)SECURE_SERVER_PRIVATE_KEY) + 1,
                               NULL, 0);

    if (ret) {
        PRINTF("[%s:%d] Failed to parse private key(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    if (mbedtls_pk_get_type(config->pkey_ctx) == MBEDTLS_PK_RSA) {
        ret = mbedtls_pk_setup_rsa_alt(config->pkey_alt_ctx,
                                       (void *)mbedtls_pk_rsa(*config->pkey_ctx),
                                       sampleApTlsRSADecrypt,
                                       sampleApTlsRSASign,
                                       sampleApTlsRSAKeyLen);

        if (ret) {
            PRINTF("[%s:%d] Failed to set rsa alt(0x%x)\n", __func__, __LINE__, -ret);
            return;
        }

        ret = mbedtls_ssl_conf_own_cert(config->ssl_conf, config->cert_crt, config->pkey_alt_ctx);

        if (ret) {
            PRINTF("[%s:%d] Failed to set cert & private key(0x%x)\n", __func__, __LINE__, -ret);
            return;
        }
    } else {
        ret = mbedtls_ssl_conf_own_cert(config->ssl_conf, config->cert_crt, config->pkey_ctx);

        if (ret) {
            PRINTF("[%s:%d] Failed to set cert & private key(0x%x)\n", __func__, __LINE__, -ret);
            return;
        }
    }

    ret = dpm_mng_setup_rng(config->ssl_conf);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set random generator(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    //Don't care verification in this progress.
    mbedtls_ssl_conf_authmode(config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);

    ret = mbedtls_ssl_setup(config->ssl_ctx, config->ssl_conf);
    if (ret) {
        PRINTF("[%s:%d] Failed to set ssl config(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    PRINTF(" [%s] End setup TLS Manager \n", __func__);

    return;
}

UINT sample_setup_tls_client(SECURE_INFO_T *config)	// for test
{
    int ret = 0;

    PRINTF(" [%s] Start setup TLS Manager (session type:%d) \n", __func__, config->sessionType);

    ret = mbedtls_ssl_config_defaults(config->ssl_conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        PRINTF("[%s:%d] Failed to set default ssl config(0x%x)\n", __func__, __LINE__, -ret);
        return ER_NOT_SUCCESSFUL;
    }

    ret = dpm_mng_setup_rng(config->ssl_conf);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set random generator(0x%x\n", __func__, __LINE__, -ret);
        return ER_NOT_SUCCESSFUL;
    }

    //don't care verification in this sample.
    mbedtls_ssl_conf_authmode(config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);

    ret = mbedtls_ssl_setup(config->ssl_ctx, config->ssl_conf);
    if (ret) {
        PRINTF("[%s:%d] Failed to set ssl config(0x%x)\n", __func__, __LINE__, -ret);
        return ER_NOT_SUCCESSFUL;
    }

#if 0
    mbedtls_ssl_set_bio(config->ssl_ctx,
                        (void *)config,
                        tls_client_sample_send_func,
                        tls_client_sample_recv_func,
                        NULL);
#endif

    PRINTF(" [%s] End setup TLS Manager \n", __func__);

    return ERR_OK;
}

void sample_setup_dtls_server(SECURE_INFO_T *config)	// for test
{
    int ret = 0;
    UINT status = ERR_OK;

    PRINTF(" [%s] Start setup DTLS Manager (config:0x%x) \n", __func__, config);

#if 0	// FOR DEBUG
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif
    ret = mbedtls_ssl_config_defaults(config->ssl_conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);

    if (ret != 0) {
        PRINTF(" [%s:%d] Failed to set default ssl config(%d)\n", __func__, __LINE__, ret);
        return;
    }

    status = sampleApTlsLoadCert();

    if (status) {
        PRINTF(RED_COLOR " [%s] Certificate is not registed(0x%02x) \n" CLEAR_COLOR, __func__, status);
        return;
    }

    status = sampleApTlsLoadPrivateKey();

    if (status) {
        PRINTF(RED_COLOR " [%s] Private key is not registed(0x%02x) \n" CLEAR_COLOR, __func__, status);
        return;
    }

    ret = mbedtls_x509_crt_parse(config->cert_crt,
                                 (UCHAR *)SECURE_SERVER_CA_CERT,
                                 strlen((char *)SECURE_SERVER_CA_CERT) + 1);

    if (ret != 0) {
        PRINTF(" [%s:%d] Failed to parse ca cert(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    ret = mbedtls_pk_parse_key(config->pkey_ctx,
                               (UCHAR *)SECURE_SERVER_PRIVATE_KEY,
                               strlen((char *)SECURE_SERVER_PRIVATE_KEY) + 1,
                               NULL, 0);

    if (ret) {
        PRINTF(" [%s:%d] Failed to parse private key(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    if (mbedtls_pk_get_type(config->pkey_ctx) == MBEDTLS_PK_RSA) {
        ret = mbedtls_pk_setup_rsa_alt(config->pkey_alt_ctx,
                                       (void *)mbedtls_pk_rsa(*config->pkey_ctx),
                                       sampleApTlsRSADecrypt,
                                       sampleApTlsRSASign,
                                       sampleApTlsRSAKeyLen);

        if (ret) {
            PRINTF(" [%s:%d] Failed to set rsa alt(0x%x)\n", __func__, __LINE__, -ret);
            return;
        }

        ret = mbedtls_ssl_conf_own_cert(config->ssl_conf, config->cert_crt, config->pkey_alt_ctx);

        if (ret) {
            PRINTF(" [%s:%d] Failed to set cert & private key(0x%x)\n", __func__, __LINE__, -ret);
            return;
        }
    } else {
        ret = mbedtls_ssl_conf_own_cert(config->ssl_conf, config->cert_crt, config->pkey_ctx);

        if (ret) {
            PRINTF(" [%s:%d] Failed to set cert & private key(0x%x)\n", __func__, __LINE__, -ret);
            return;
        }
    }

    ret = dpm_mng_setup_rng(config->ssl_conf);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set random generator(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    ret = dpm_mng_cookie_setup_rng(config->cookie_ctx);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set cookie(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    mbedtls_ssl_conf_dtls_cookies(config->ssl_conf,
                                  mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check,
                                  config->cookie_ctx);
    mbedtls_ssl_conf_dbg(config->ssl_conf, secure_debug, NULL);
    mbedtls_ssl_conf_dtls_anti_replay(config->ssl_conf, MBEDTLS_SSL_ANTI_REPLAY_ENABLED);
    mbedtls_ssl_conf_authmode(config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_max_frag_len(config->ssl_conf, MBEDTLS_SSL_MAX_FRAG_LEN_NONE);
    mbedtls_ssl_conf_read_timeout(config->ssl_conf, ACK_TIMEOUT * 1000);
    mbedtls_ssl_conf_handshake_timeout(config->ssl_conf,
                                       HANDSHAKE_MIN_TIMEOUT * 1000,
                                       HANDSHAKE_MAX_TIMEOUT * 1000);

    //TODO: set cipher suites for coaps
    //mbedtls_ssl_conf_ciphersuites(secure_conf->ssl_conf, cipher_suites);

    ret = mbedtls_ssl_setup(config->ssl_ctx, config->ssl_conf);
    if (ret != 0) {
        PRINTF(" [%s:%d] Failed to set ssl config(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    PRINTF(" [%s] End setup DTLS Manager \n", __func__);
}

void sample_setup_dtls_client(SECURE_INFO_T *config)	// for test
{
    int ret = 0;

#if 0	// FOR DEBUG
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

    PRINTF(" [%s] Start setup DTLS Manager (config:0x%x) \n", __func__, config);

    ret = mbedtls_ssl_config_defaults(config->ssl_conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        PRINTF(" [%s:%d] Failed to set default ssl config(%d)\n", __func__, __LINE__, ret);
        return;
    }

    ret = dpm_mng_setup_rng(config->ssl_conf);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set random generator(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    mbedtls_ssl_conf_dbg(config->ssl_conf, secure_debug, NULL);
    mbedtls_ssl_conf_dtls_anti_replay(config->ssl_conf, MBEDTLS_SSL_ANTI_REPLAY_ENABLED);
    mbedtls_ssl_conf_authmode(config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_max_frag_len(config->ssl_conf, MBEDTLS_SSL_MAX_FRAG_LEN_NONE);
    mbedtls_ssl_conf_read_timeout(config->ssl_conf, ACK_TIMEOUT * 1000);
    mbedtls_ssl_conf_handshake_timeout(config->ssl_conf,
                                       HANDSHAKE_MIN_TIMEOUT * 1000,
                                       HANDSHAKE_MAX_TIMEOUT * 1000);

    //TODO: set cipher suites for coaps
    //mbedtls_ssl_conf_ciphersuites(secure_conf->ssl_conf, cipher_suites);

    ret = mbedtls_ssl_setup(config->ssl_ctx, config->ssl_conf);

    if (ret != 0) {
        PRINTF(" [%s:%d] Failed to set ssl config(0x%x)\n", __func__, __LINE__, -ret);
        return;
    }

    PRINTF(" [%s] End setup DTLS Manager \n", __func__);
}

void initConfigSampleByBoot()
{
    PRINTF(GREEN_COLOR " [%s] Called by boot...\n" CLEAR_COLOR, __func__);

    memset(&sampleParams, 0, sizeof(sampleParamForRtm));

    strcpy(sampleParams.state, "Start Boot"); ;

    PRINTF(GREEN_COLOR " [%s] %s \n" CLEAR_COLOR, __func__, sampleParams.state);

    // Read tcp_tx_period from nvram
    if (read_nvram_int((const char *)NVR_KEY_TCP_CLI_TX_PERIOD, (int *)&sampleParams.tcpClientSendPeriod)) {
        sampleParams.tcpClientSendPeriod = DEFAULT_TCP_CLI_TX_PERIOD;
    }
    PRINTF(GREEN_COLOR " [%s] tcp_tx_period:%d \n" CLEAR_COLOR, __func__, sampleParams.tcpClientSendPeriod);

    // Read udp_tx_period from nvram
    if (read_nvram_int((const char *)NVR_KEY_UDP_CLI_TX_PERIOD, (int *)&sampleParams.udpClientSendPeriod)) {
        sampleParams.udpClientSendPeriod = DEFAULT_UDP_CLI_TX_PERIOD;
    }
    PRINTF(GREEN_COLOR " [%s] udp_tx_period:%d \n" CLEAR_COLOR, __func__, sampleParams.udpClientSendPeriod);

#if defined ( USE_MALLOC )

    if (REGIST_SESSION_TYPE1) {
        txBuffForSession1 = pvPortMalloc(TX_BUF1_SIZE);
    }

    if (REGIST_SESSION_TYPE2) {
        txBuffForSession2 = pvPortMalloc(TX_BUF2_SIZE);
    }

    if (REGIST_SESSION_TYPE3) {
        txBuffForSession3 = pvPortMalloc(TX_BUF3_SIZE);
        txBuffForSession3ByTimer = pvPortMalloc(TX_BUF3_SIZE);
    }

    if (REGIST_SESSION_TYPE4) {
        txBuffForSession4 = pvPortMalloc(TX_BUF4_SIZE);
        txBuffForSession4ByTimer = pvPortMalloc(TX_BUF4_SIZE);
    }

#endif	// USE_MALLOC

    // Customer/Developer need to set if to control DPM operation ...
    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void initConfigSampleByWakeup()
{
    PRINTF(GREEN_COLOR " [%s] Called by wake-up...\n" CLEAR_COLOR, __func__);

    strcpy(sampleParams.state, "Start Wake-up"); ;

    sampleParams.wakeupCount++;

    PRINTF(" [Statistics] \n");
    PRINTF("  wakeup count(%d) \n", sampleParams.wakeupCount);

    PRINTF("  timer1  count(%d) \t", sampleParams.timer1WakeupCount);
    PRINTF("  timer2  count(%d) \t", sampleParams.timer2WakeupCount);
    PRINTF("  timer3  count(%d) \t", sampleParams.timer3WakeupCount);
    PRINTF("  timer4  count(%d) \n", sampleParams.timer4WakeupCount);

    PRINTF("  TCP Svr count(%d) \t", sampleParams.tcpServerWakeupCount);
    PRINTF("  TCP Cli count(%d) \t", sampleParams.tcpClientWakeupCount);
    PRINTF("  UDP Svr count(%d) \t", sampleParams.udpServerWakeupCount);
    PRINTF("  UDP Cli count(%d) \n", sampleParams.udpClientWakeupCount);

#if defined ( USE_MALLOC )

    if (REGIST_SESSION_TYPE1) {
        txBuffForSession1 = pvPortMalloc(TX_BUF1_SIZE);
    }

    if (REGIST_SESSION_TYPE2) {
        txBuffForSession2 = pvPortMalloc(TX_BUF2_SIZE);
    }

    if (REGIST_SESSION_TYPE3) {
        txBuffForSession3 = pvPortMalloc(TX_BUF3_SIZE);
        txBuffForSession3ByTimer = pvPortMalloc(TX_BUF3_SIZE);
    }

    if (REGIST_SESSION_TYPE4) {
        txBuffForSession4 = pvPortMalloc(TX_BUF4_SIZE);
        txBuffForSession4ByTimer = pvPortMalloc(TX_BUF4_SIZE);
    }

#endif	// USE_MALLOC

    // Customer/Developer need to set if to control DPM operation ...
    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void timer1_callback()	// TCP_CLIENT timer
{
#if defined ( TCP_CLIENT_TEST )
    ULONG	ip;
    int		status;
    int		tx_len = 0;
#endif	// TCP_CLIENT_TEST

#ifdef TCP_CLIENT_RECEIVE_ECHO
    int	waitTime = 0;

    tcpReceivedEchoFlag = pdFALSE;
#endif	// TCP_CLIENT_RECEIVE_ECHO

    PRINTF(GREEN_COLOR " [%s] Called by timer1...\n" CLEAR_COLOR, __func__);

    sampleParams.timer1WakeupCount++;

    if (sampleParams.session1ConnectFlag != pdTRUE) {
        PRINTF(RED_COLOR " [%s] Not connected session_1 ... goto end \n" CLEAR_COLOR, __func__);
        goto timer1_callback_end;
    }

#if defined ( TCP_CLIENT_TEST )
    // Send TCP client
    memset(txBuffForSession1, 0, TX_BUF1_SIZE);

    snprintf(txBuffForSession1, TX_BUF1_SIZE - 1, "%s_%d", "Hello1", sampleParams.timer1WakeupCount);
    tx_len = strlen(txBuffForSession1);

    ip = iptolong(REGIST_SERVER_IP_1);
    status = dpm_mng_send_to_session(SESSION1, ip, REGIST_SERVER_PORT_1, txBuffForSession1, tx_len);
    if (status) {
        PRINTF(RED_COLOR " Send fail %s (session%d)\n" CLEAR_COLOR, txBuffForSession1, SESSION1);
        goto timer1_callback_end;
    } else {
        PRINTF(MAGENTA_COLOR " <====== %s sended TCP Client(size:%d) to %s:%d \n" CLEAR_COLOR,
               txBuffForSession1, tx_len, REGIST_SERVER_IP_1, REGIST_SERVER_PORT_1);
    }
#endif	// TCP_CLIENT_TEST

#ifdef TCP_CLIENT_RECEIVE_ECHO
    while (1) {
        vTaskDelay(10);					// 100 ms delay

        if (tcpReceivedEchoFlag == pdTRUE) {
            tcpReceivedEchoFlag = pdFALSE;
            if (strncmp(txBuffForSession1, tcpRecvEchoData, tx_len) == 0) {
                PRINTF(GREEN_COLOR " Recv same echo packet %s \n" CLEAR_COLOR, tcpRecvEchoData);
                break;
            } else {
                if (!(waitTime % 20)) {	// 2 Sec
                    PRINTF(RED_COLOR " Diff receive_echo_data %s and send_data %s \n" CLEAR_COLOR,
                           tcpRecvEchoData, txBuffForSession1);
                }
            }
        } else {
            if (++waitTime < 100) {		// 10 Sec
                continue;
            } else {
                PRINTF(RED_COLOR " Receive timeout %s (session%d)\n" CLEAR_COLOR, txBuffForSession1, SESSION1);

#ifdef TCP_CLIENT_ECHO_RECOVERY
                status = dpm_mng_stop_session(SESSION1);
                if (status) {
                    PRINTF(RED_COLOR " Error Stop (session%d) \n" CLEAR_COLOR, SESSION1);
                } else {
                    status = dpm_mng_start_session(SESSION1);
                    if (status) {
                        PRINTF(RED_COLOR " Error Start (session%d) \n" CLEAR_COLOR, SESSION1);
                    } else {
                        PRINTF(CYAN_COLOR " Done. Stop/Start (session%d) \n" CLEAR_COLOR, SESSION1);
                    }
                }
#endif
                break;
            }
        }
    }
#endif	// TCP_CLIENT_RECEIVE_ECHO

timer1_callback_end:
    vTaskDelay(7);		// FOR_PRINT
    // Customer/Developer need to set if to control DPM operation ...
    dpm_mng_job_done();
}

void timer2_callback()	// UDP_CLIENT timer
{
#if defined ( UDP_CLIENT_TEST )
    ULONG	ip;
    int		status;
    int		tx_len = 0;
#endif	// UDP_CLIENT_TEST

#ifdef UDP_CLIENT_RECEIVE_ECHO
    int waitTime = 0;
    int reSendCount = 0;

    udpReceivedEchoFlag = pdFALSE;
#endif	// UDP_CLIENT_RECEIVE_ECHO

    PRINTF(GREEN_COLOR " [%s] Called by timer2...\n" CLEAR_COLOR, __func__);

    sampleParams.timer2WakeupCount++;

#if defined ( UDP_CLIENT_TEST )
    // Send UDP client
    memset(txBuffForSession2, 0, TX_BUF2_SIZE);

    snprintf(txBuffForSession2, TX_BUF2_SIZE - 1, "%s_%d", "Hello2", sampleParams.timer2WakeupCount);
    tx_len = strlen(txBuffForSession2);

    ip = iptolong(REGIST_SERVER_IP_2);

reSend:

    status = dpm_mng_send_to_session(SESSION2, ip, REGIST_SERVER_PORT_2, txBuffForSession2, tx_len);
    if (status) {
        PRINTF(RED_COLOR " Send fail %s (session%d) \n" CLEAR_COLOR, txBuffForSession2, SESSION2);
        goto timer2_callback_end;
    } else {
        PRINTF(MAGENTA_COLOR " <====== %s sended UDP Client(size:%d) to %s:%d \n" CLEAR_COLOR,
               txBuffForSession2, tx_len, REGIST_SERVER_IP_2, REGIST_SERVER_PORT_2);
    }

#endif	// UDP_CLIENT_TEST

#ifdef UDP_CLIENT_RECEIVE_ECHO
    while (1) {
        vTaskDelay(10);					// 100 ms delay

        if (udpReceivedEchoFlag == pdTRUE) {
            udpReceivedEchoFlag = pdFALSE;
            if (strncmp(txBuffForSession2, udpRecvEchoData, tx_len) == 0) {
                PRINTF(GREEN_COLOR " Recv same echo packet %s \n" CLEAR_COLOR, udpRecvEchoData);
                break;
            } else {
                if (!(waitTime % 20)) {	// 2 Sec
                    PRINTF(RED_COLOR " Diff receive_echo_data %s and send_data %s \n" CLEAR_COLOR,
                           udpRecvEchoData, txBuffForSession2);
                }
            }
        } else {
            if (++waitTime < 30) {	// 3 Sec
                continue;
            } else {
                PRINTF(RED_COLOR " Receive timeout \n" CLEAR_COLOR);

                if (++reSendCount <= 3) {
                    waitTime = 0;
                    PRINTF(CYAN_COLOR " Retry send (cnt:%d) \n" CLEAR_COLOR, reSendCount);
                    goto reSend;
                }

                dpm_mng_stop_session(SESSION2);
                dpm_mng_start_session(SESSION2);
                PRINTF(CYAN_COLOR " Done. Stop/Start (session%d) \n" CLEAR_COLOR, SESSION2);

                break;
            }
        }
    }
#endif	// UDP_CLIENT_RECEIVE_ECHO

#if defined ( UDP_CLIENT_TEST )
timer2_callback_end:
#endif	//( UDP_CLIENT_TEST )
    vTaskDelay(7);		// FOR_PRINT
    // Customer/Developer need to set if to control DPM operation ...
    dpm_mng_job_done();
}

void timer3_callback()	// TCP_SERVER timer
{
#define	SEND_BY_TIMER
#if defined ( TCP_SERVER_TEST )
    int	status;
#endif	//( TCP_SERVER_TEST )
    PRINTF(GREEN_COLOR " [%s] Called by timer3...(peer_count:%d) \n" CLEAR_COLOR,
           __func__, sampleParams.tcp_peer_count);

    sampleParams.timer3WakeupCount++;

#if defined ( TCP_SERVER_TEST )
    // Send TCP client to 1st & another_one
    if (sampleParams.tcp_peer_count == PEER_NONE) {
        PRINTF(RED_COLOR " Not yet receive from peer\n" CLEAR_COLOR);
    } else if (sampleParams.tcp_peer_count == PEER_ONLY_ONE) {
send_1st:
        if ((sampleParams.tcp_peer1_IP != 0) && (sampleParams.tcp_peer1_PORT != 0)) {
            memset(txBuffForSession3ByTimer, 0, TX_BUF3_SIZE);
            strcpy(txBuffForSession3ByTimer, "Hello to 1st ");
#ifdef	SEND_BY_TIMER
            status = dpm_mng_send_to_session(SESSION3,
                                             sampleParams.tcp_peer1_IP,
                                             sampleParams.tcp_peer1_PORT,
                                             txBuffForSession3ByTimer,
                                             strlen(txBuffForSession3ByTimer));

            if (status) {
                sampleParams.tcp_peer_count--;
                sampleParams.tcp_peer1_IP = 0;
                sampleParams.tcp_peer1_PORT = 0;
                PRINTF(RED_COLOR " Send fail %s (session%d) to 0x%x:%d status:%d \n" CLEAR_COLOR,
                       txBuffForSession3ByTimer, SESSION3, sampleParams.tcp_peer1_IP, sampleParams.tcp_peer1_PORT, status);
            } else {
                PRINTF(MAGENTA_COLOR " <====== %s sended TCP Server(size:%d) to 0x%x:%d \n" CLEAR_COLOR,
                       txBuffForSession3ByTimer, strlen(txBuffForSession3ByTimer), sampleParams.tcp_peer1_IP, sampleParams.tcp_peer1_PORT);
            }
#else	// SEND_BY_TIMER
            PRINTF(" ****** %s (session%d)\n", txBuffForSession3ByTimer, SESSION3);
#endif	// SEND_BY_TIMER
        } else {
            PRINTF(RED_COLOR " Not yet receive from peer 1st (tcp_peer1_IP:0x%x tcp_peer1_PORT:%d) \n"
                   CLEAR_COLOR, sampleParams.tcp_peer1_IP, sampleParams.tcp_peer1_PORT);
        }
    } else if (sampleParams.tcp_peer_count == PEER_ANOTHER) {
        // there is another one
        PRINTF(CYAN_COLOR " Send to peer(another) \n" CLEAR_COLOR);
        sampleParams.tcp_peer_sent_toggle ^= 1;
        if (!sampleParams.tcp_peer_sent_toggle) {
            if ((sampleParams.tcp_peer2_IP != 0) && (sampleParams.tcp_peer2_PORT != 0)) {
                memset(txBuffForSession3ByTimer, 0, TX_BUF3_SIZE);
                strcpy(txBuffForSession3ByTimer, "Hello to another_one ");

                status = dpm_mng_send_to_session(SESSION3,
                                                 sampleParams.tcp_peer2_IP,
                                                 sampleParams.tcp_peer2_PORT,
                                                 txBuffForSession3ByTimer,
                                                 strlen(txBuffForSession3ByTimer));
                if (status) {
                    sampleParams.tcp_peer_count--;
                    sampleParams.tcp_peer2_IP = 0;
                    sampleParams.tcp_peer2_PORT = 0;
                    PRINTF(RED_COLOR " Send fail %s (session%d) to 0x%x:%d status:%d \n" CLEAR_COLOR,
                           txBuffForSession3ByTimer, SESSION3, sampleParams.tcp_peer2_IP,
                           sampleParams.tcp_peer2_PORT, status);
                }
                PRINTF(MAGENTA_COLOR " <====== %s sended TCP Server(size:%d) to 0x%x:%d \n" CLEAR_COLOR,
                       txBuffForSession3ByTimer, strlen(txBuffForSession3ByTimer),
                       sampleParams.tcp_peer2_IP, sampleParams.tcp_peer2_PORT);
            } else {
                PRINTF(RED_COLOR " Not yet receive from peer another (tcp_peer2_IP:0x%x tcp_peer2_PORT:%d) \n"
                       CLEAR_COLOR, sampleParams.tcp_peer2_IP, sampleParams.tcp_peer2_PORT);
            }
        } else {
            goto send_1st;
        }
    }
#endif	// TCP_SERVER_TEST

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void timer4_callback()	// UDP_SERVER timer
{
#if defined ( UDP_SERVER_TEST )
    int	status;
#endif	//( UDP_SERVER_TEST )
    PRINTF(GREEN_COLOR " [%s] Called by timer4...\n" CLEAR_COLOR, __func__);

    sampleParams.timer4WakeupCount++;

#if defined ( UDP_SERVER_TEST )
    // Send UDP server
    if ((sampleParams.udp_peer_IP != 0) && (sampleParams.udp_peer_PORT != 0)) {
        memset(txBuffForSession4ByTimer, 0, TX_BUF4_SIZE);
        sprintf(txBuffForSession4ByTimer,
                "Hello client! (Send from timer, interval:%d sec, count:%d)",
                TIMER4_INTERVAL,
                sampleParams.timer4WakeupCount);

        status = dpm_mng_send_to_session(SESSION4,
                                         sampleParams.udp_peer_IP,
                                         sampleParams.udp_peer_PORT,
                                         txBuffForSession4ByTimer,
                                         strlen(txBuffForSession4ByTimer));
        if (status) {
            PRINTF(RED_COLOR " Send fail %s (session%d)\n" CLEAR_COLOR, txBuffForSession4ByTimer, SESSION4);
        } else {
            PRINTF(MAGENTA_COLOR " <====== %s sended UDP Server(size:%d) to 0x%x:%d by timer. \n" CLEAR_COLOR,
                   txBuffForSession4ByTimer, strlen(txBuffForSession4ByTimer),
                   sampleParams.udp_peer_IP, sampleParams.udp_peer_PORT);
        }
    } else {
        PRINTF(RED_COLOR " Not yet receive from peer (udp_peer_IP:0x%x udp_peer_PORT:%d) \n" CLEAR_COLOR, sampleParams.udp_peer_IP,
               sampleParams.udp_peer_PORT);
    }
#endif	// UDP_SERVER_TEST

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void external_wu_callback()
{
    PRINTF(GREEN_COLOR " [%s]Called by external wakeup...\n" CLEAR_COLOR, __func__);

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

// TCP_CLIENT ///////////////////////////////////////
void connect_callback_1(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] Called by connect result (status:%s) \n" CLEAR_COLOR, __func__,
           (conn_status == 0) ? "CONNECTED" : "NOT_CONNECTED");

    if (conn_status == 0) {
        sampleParams.session1ConnectFlag = pdTRUE;
    } else {
        sampleParams.session1ConnectFlag = pdFALSE;
    }

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void recvPacket_callback_1(void *sock, UCHAR *rx_buf, UINT rx_len, ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);
    DA16X_UNUSED_ARG(rx_ip);
    DA16X_UNUSED_ARG(rx_port);

    PRINTF(GREEN_COLOR " [%s] Called by recv_packet_1...\n" CLEAR_COLOR, __func__);

    sampleParams.tcpClientWakeupCount++;

    PRINTF(MAGENTA_COLOR " ======> %s received TCP Client(size:%d) \n" CLEAR_COLOR, rx_buf, rx_len);

#ifdef TCP_CLIENT_RECEIVE_ECHO
    if (rx_len >= TX_BUF1_SIZE) {
        rx_len = TX_BUF1_SIZE - 1;
    }
    memcpy(tcpRecvEchoData, rx_buf, rx_len);
    tcpRecvEchoData[rx_len] = 0;

    tcpReceivedEchoFlag = pdTRUE;
#endif	// TCP_CLIENT_RECEIVE_ECHO

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void setup_secure_callback_1(void *config)
{
    PRINTF(GREEN_COLOR " [%s] Called by setup_secure_callback1. \n" CLEAR_COLOR, __func__);
    sample_setup_tls_client(config);
    PRINTF(GREEN_COLOR " [%s] Done setup tls. \n" CLEAR_COLOR, __func__);
    vTaskDelay(5);

    //vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

// UDP_CLIENT ///////////////////////////////////////
void connect_callback_2(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] Called by connect result (status:%s) \n" CLEAR_COLOR, __func__,
           (conn_status == 0) ? "CONNECTED" : "NOT_CONNECTED");

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void recvPacket_callback_2(void *sock, UCHAR *rx_buf, UINT rx_len, ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);
    DA16X_UNUSED_ARG(rx_ip);
    DA16X_UNUSED_ARG(rx_port);

    PRINTF(GREEN_COLOR " [%s] Called by recv_packet_2...\n" CLEAR_COLOR, __func__);

    sampleParams.udpClientWakeupCount++;

    PRINTF(MAGENTA_COLOR " ======> %s received UDP Client(size:%d) \n" CLEAR_COLOR, rx_buf, rx_len);

#ifdef UDP_CLIENT_RECEIVE_ECHO
    if (rx_len >= TX_BUF2_SIZE) {
        rx_len = TX_BUF2_SIZE - 1;
    }
    memcpy(udpRecvEchoData, rx_buf, rx_len);
    udpRecvEchoData[rx_len] = 0;

    udpReceivedEchoFlag = pdTRUE;
#endif	// UDP_CLIENT_RECEIVE_ECHO

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void setup_secure_callback_2(void *config)
{
    PRINTF(GREEN_COLOR " [%s] Called by setup_secure_callback2. \n" CLEAR_COLOR, __func__);

    sample_setup_dtls_client(config);
    PRINTF(GREEN_COLOR " [%s] Done setup dtls. \n" CLEAR_COLOR, __func__);

    //vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

#if !defined ( __LIGHT_DPM_MANAGER__ )
// TCP_SERVER ///////////////////////////////////////
void connect_callback_3(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] Called by connect result (status:%s) \n" CLEAR_COLOR, __func__,
           (conn_status == 0) ? "CONNECTED" : "NOT_CONNECTED");
    vTaskDelay(7);	// FOR_PRINT
    dpm_mng_job_done();
}

void recvPacket_callback_3(void *sock, UCHAR *rx_buf, UINT rx_len, ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);

    int	status;

    PRINTF(GREEN_COLOR " [%s] Called by recv_packet_3...(len:%d, rx_ip:0x%x, rx_port:%d) \n"
           CLEAR_COLOR, __func__, rx_len, rx_ip, rx_port);

    if (sampleParams.tcp_peer1_IP == 0) {
        sampleParams.tcp_peer1_IP = rx_ip;
        sampleParams.tcp_peer1_PORT = rx_port;
        sampleParams.tcp_peer_count = 1;
    } else if ((sampleParams.tcp_peer1_IP != rx_ip) || (sampleParams.tcp_peer1_PORT != rx_port)) {	// another one
        sampleParams.tcp_peer2_IP = rx_ip;
        sampleParams.tcp_peer2_PORT = rx_port;
        sampleParams.tcp_peer_count = 2;	// more one
    }

    sampleParams.tcpServerWakeupCount++;

    PRINTF(MAGENTA_COLOR " ======> %s received TCP Server(size:%d) \n" CLEAR_COLOR, rx_buf, rx_len);

    // echo TCP Server
    memset(txBuffForSession3, 0, TX_BUF3_SIZE);

    if (rx_len > TX_BUF3_SIZE) {
        rx_len = TX_BUF3_SIZE;
    }

    memcpy(txBuffForSession3, rx_buf, rx_len);
    status = dpm_mng_send_to_session(SESSION3, rx_ip, rx_port, txBuffForSession3, rx_len);
    if (status) {
        PRINTF(RED_COLOR " Send fail %s sended TCP Server(size:%d) to 0x%x:%d \n" CLEAR_COLOR,
               txBuffForSession3, rx_len, rx_ip, rx_port);
    } else {
        PRINTF(MAGENTA_COLOR " <====== %s sended TCP Server(size:%d) to 0x%x:%d \n" CLEAR_COLOR,
               txBuffForSession3, rx_len, rx_ip, rx_port);
    }

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void setup_secure_callback_3(void *config)
{
    PRINTF(GREEN_COLOR " [%s] Called by setup_secure_callback3. \n" CLEAR_COLOR, __func__);

    sample_setup_tls_server(config);
    PRINTF(GREEN_COLOR " [%s] Done setup tls. \n" CLEAR_COLOR, __func__);
    vTaskDelay(5);

    //vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

// UDP_SERVER ///////////////////////////////////////
void connect_callback_4(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] Called by connect result (status:%s) \n" CLEAR_COLOR, __func__,
           (conn_status == 0) ? "CONNECTED" : "NOT_CONNECTED");

    vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}

void recvPacket_callback_4(void *sock, UCHAR *rx_buf, UINT rx_len, ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);

    int	status;

    PRINTF(GREEN_COLOR " [%s] Called by recv_packet_4... (peer_ip:0x%x peer_port:%d len:%d) \n" CLEAR_COLOR,
           __func__, rx_ip, rx_port, rx_len);

    sampleParams.udp_peer_IP = rx_ip;
    sampleParams.udp_peer_PORT = rx_port;

    sampleParams.udpServerWakeupCount++;

    PRINTF(MAGENTA_COLOR " ======> %s received UDP Server(size:%d) \n" CLEAR_COLOR, rx_buf, rx_len);

    // echo UDP Server
    memset(txBuffForSession4, 0, TX_BUF4_SIZE);

    if (rx_len > TX_BUF4_SIZE) {
        rx_len = TX_BUF4_SIZE;
    }

    memcpy(txBuffForSession4, rx_buf, rx_len);
    status = dpm_mng_send_to_session(SESSION4, rx_ip, rx_port, txBuffForSession4, rx_len);
    if (status) {
        PRINTF(RED_COLOR " Send fail %s sended packet (size:%d) \n" CLEAR_COLOR, txBuffForSession4, rx_len);
    } else {
        PRINTF(MAGENTA_COLOR " <====== %s sended UDP Server(size:%d) to 0x%x:%d \n" CLEAR_COLOR,
               txBuffForSession4, rx_len, rx_ip, rx_port);
    }

    vTaskDelay(10);
    dpm_mng_job_done();
}

void setup_secure_callback_4(void *config)
{
    PRINTF(GREEN_COLOR " [%s] Called by setup_secure_callback4. \n" CLEAR_COLOR, __func__);

    sample_setup_dtls_server(config);
    PRINTF(GREEN_COLOR " [%s] Done setup dtls. \n" CLEAR_COLOR, __func__);

    //vTaskDelay(7);		// FOR_PRINT
    dpm_mng_job_done();
}
#endif	// !__LIGHT_DPM_MANAGER__


void error_callback(UINT error_code, char *comment)
{
    PRINTF(RED_COLOR " [%s] err:0x%x (%s) \n" CLEAR_COLOR, __func__, error_code, comment);
}

static void init_DPM_sample_config(dpm_user_config_t *dpmUserConf)
{
    dpmUserConf->bootInitCallback = BOOT_INIT_FUNC;
    dpmUserConf->wakeupInitCallback = WAKEUP_INIT_FUNC;

    dpmUserConf->timerConfig[0].timerType = TIMER1_TYPE;
    if (sampleParams.tcpClientSendPeriod) {
        dpmUserConf->timerConfig[0].timerInterval = sampleParams.tcpClientSendPeriod;
    } else {
        dpmUserConf->timerConfig[0].timerInterval = TIMER1_INTERVAL;
    }
    dpmUserConf->timerConfig[0].timerCallback = TIMER1_FUNC;

    dpmUserConf->timerConfig[1].timerType = TIMER2_TYPE;
    if (sampleParams.udpClientSendPeriod) {
        dpmUserConf->timerConfig[1].timerInterval = sampleParams.udpClientSendPeriod;
    } else {
        dpmUserConf->timerConfig[1].timerInterval = TIMER2_INTERVAL;
    }
    dpmUserConf->timerConfig[1].timerCallback = TIMER2_FUNC;

    dpmUserConf->timerConfig[2].timerType = TIMER3_TYPE;
    dpmUserConf->timerConfig[2].timerInterval = TIMER3_INTERVAL;
    dpmUserConf->timerConfig[2].timerCallback = TIMER3_FUNC;

    dpmUserConf->timerConfig[3].timerType = TIMER4_TYPE;
    dpmUserConf->timerConfig[3].timerInterval = TIMER4_INTERVAL;
    dpmUserConf->timerConfig[3].timerCallback = TIMER4_FUNC;

#if defined ( TCP_CLIENT_TEST )
    dpmUserConf->sessionConfig[0].sessionType = REGIST_SESSION_TYPE1;
    dpmUserConf->sessionConfig[0].sessionMyPort = REGIST_MY_PORT_1;
    memcpy(dpmUserConf->sessionConfig[0].sessionServerIp, REGIST_SERVER_IP_1, sizeof(REGIST_SERVER_IP_1));
    dpmUserConf->sessionConfig[0].sessionServerPort = REGIST_SERVER_PORT_1;
    dpmUserConf->sessionConfig[0].sessionKaInterval = SESSION1_KA_INTERVAL;
    dpmUserConf->sessionConfig[0].sessionConnectCallback = SESSION1_CONN_FUNC;
    dpmUserConf->sessionConfig[0].sessionRecvCallback = SESSION1_RECV_FUNC;
    dpmUserConf->sessionConfig[0].sessionConnRetryCnt = SESSION1_CONNECT_RETRY_COUNT;	// Only TCP Client
    dpmUserConf->sessionConfig[0].sessionConnWaitTime = SESSION1_CONNECT_WAIT_TIME;		// Only TCP Client
    dpmUserConf->sessionConfig[0].sessionAutoReconn = SESSION1_AUTO_RECONNECT;			// Only TCP Client
    dpmUserConf->sessionConfig[0].supportSecure = SESSION1_SECURE_SETUP;
    dpmUserConf->sessionConfig[0].sessionSetupSecureCallback = SESSION1_SECURE_SETUP_FUNC;
#endif	// TCP_CLIENT_TEST

#if defined ( UDP_CLIENT_TEST )
    dpmUserConf->sessionConfig[1].sessionType = REGIST_SESSION_TYPE2;
    dpmUserConf->sessionConfig[1].sessionMyPort = REGIST_MY_PORT_2;
    memcpy(dpmUserConf->sessionConfig[1].sessionServerIp, REGIST_SERVER_IP_2, sizeof(REGIST_SERVER_IP_2));
    dpmUserConf->sessionConfig[1].sessionServerPort = REGIST_SERVER_PORT_2;
    dpmUserConf->sessionConfig[1].sessionKaInterval = SESSION2_KA_INTERVAL;
    dpmUserConf->sessionConfig[1].sessionConnectCallback  = SESSION2_CONN_FUNC;
    dpmUserConf->sessionConfig[1].sessionRecvCallback = SESSION2_RECV_FUNC;
    dpmUserConf->sessionConfig[1].supportSecure = SESSION2_SECURE_SETUP;
    dpmUserConf->sessionConfig[1].sessionSetupSecureCallback = SESSION2_SECURE_SETUP_FUNC;
#endif	// UDP_CLIENT_TEST


#if !defined ( __LIGHT_DPM_MANAGER__ )
#if defined ( TCP_SERVER_TEST )
    dpmUserConf->sessionConfig[2].sessionType = REGIST_SESSION_TYPE3;
    dpmUserConf->sessionConfig[2].sessionMyPort = REGIST_MY_PORT_3;
    memcpy(dpmUserConf->sessionConfig[2].sessionServerIp, REGIST_SERVER_IP_3, sizeof(REGIST_SERVER_IP_3));
    dpmUserConf->sessionConfig[2].sessionServerPort = REGIST_SERVER_PORT_3;
    dpmUserConf->sessionConfig[2].sessionKaInterval = SESSION3_KA_INTERVAL;
    dpmUserConf->sessionConfig[2].sessionConnectCallback  = SESSION3_CONN_FUNC;
    dpmUserConf->sessionConfig[2].sessionRecvCallback = SESSION3_RECV_FUNC;
    dpmUserConf->sessionConfig[2].supportSecure = SESSION3_SECURE_SETUP;
    dpmUserConf->sessionConfig[2].sessionSetupSecureCallback = SESSION3_SECURE_SETUP_FUNC;
#endif	// TCP_SERVER_TEST

#if defined ( UDP_SERVER_TEST )
    dpmUserConf->sessionConfig[3].sessionType = REGIST_SESSION_TYPE4;
    dpmUserConf->sessionConfig[3].sessionMyPort = REGIST_MY_PORT_4;
    memcpy(dpmUserConf->sessionConfig[3].sessionServerIp, REGIST_SERVER_IP_4, sizeof(REGIST_SERVER_IP_4));
    dpmUserConf->sessionConfig[3].sessionServerPort = REGIST_SERVER_PORT_4;
    dpmUserConf->sessionConfig[3].sessionKaInterval = SESSION4_KA_INTERVAL;
    dpmUserConf->sessionConfig[3].sessionConnectCallback  = SESSION4_CONN_FUNC;
    dpmUserConf->sessionConfig[3].sessionRecvCallback = SESSION4_RECV_FUNC;
    dpmUserConf->sessionConfig[3].supportSecure = SESSION4_SECURE_SETUP;
    dpmUserConf->sessionConfig[3].sessionSetupSecureCallback = SESSION4_SECURE_SETUP_FUNC;
#endif	// UDP_SERVER_TEST
#endif	// !__LIGHT_DPM_MANAGER__

    dpmUserConf->ptrDataFromRetentionMemory = NON_VOLITALE_MEM_ADDR;
    dpmUserConf->sizeOfRetentionMemory = NON_VOLITALE_MEM_SIZE;

    dpmUserConf->externWakeupCallback = EXTERN_WU_FUNCTION;
    dpmUserConf->errorCallback = ERROR_FUNCTION;

    PRINTF(GREEN_COLOR " [%s] >>>>>> Done ... Register a config callback function \n" CLEAR_COLOR, __func__);
}

void all_used_dpm_manager_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    PRINTF(GREEN_COLOR " [%s] >>>>>> Reigster a config callback function \n" CLEAR_COLOR, __func__);

    dpm_mng_regist_config_cb(init_DPM_sample_config);
    dpm_mng_start();

    //dpm_abnormal_chk_hold();	// FOR_TEST : skip abnormal check
    vTaskDelete(NULL);
}

#endif	// __ALL_USED_DPM_MANAGER_SAMPLE__

// EOF
