/**
 ****************************************************************************************
 *
 * @file user_dpm_manager.h
 *
 * @brief Defines and macros for DPM manager operation
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


#ifndef    __DA16X_DPM_MANAGER_H__
#define __DA16X_DPM_MANAGER_H__

#include "sdk_type.h"
#include "user_dpm_api.h"

#include "common_def.h"
#include "da16x_network_common.h"
#include "common_config.h"
#include "da16x_dpm.h"
#include "user_dpm.h"

#include "da16x_system.h"
#include "application.h"
#include "iface_defs.h"
#include "cc3120_hw_eng_initialize.h"

#include "mbedtls/config.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"
#include "mbedtls/rsa.h"

#undef  KEEPALIVE_UDP_CLI

#undef  DM_DEBUG_INFO
#define DM_DEBUG_ERROR
#define DM_DEBUG_TEMP

#ifdef DM_DEBUG_INFO
    #define DM_DBG_INFO             PRINTF
#else
    #define DM_DBG_INFO(...)        do { } while (0);
#endif

#ifdef DM_DEBUG_ERROR
    #define DM_DBG_ERR              PRINTF
#else
    #define DM_DBG_ERR(...)         do { } while (0);
#endif

#ifdef DM_DEBUG_TEMP
    #define DM_DBG_TEMP             PRINTF
#else
    #define DM_DBG_TEMP(...)        do { } while (0);
#endif

#define __ALLOWED_MULTI_CONNECTION__
#define SESSION_MUTEX
#define DM_MUTEX
#define CHANGE_TCP_LOCAL_PORT_BY_RESTART
#define CHANGE_UDP_LOCAL_PORT_BY_RESTART
#define SEPERATE_HANDSHAKE

//
// Macro defines
//
#define DPM_MAGIC_CODE                   0xDEADBEAF

#define DPM_CONFIG_NAME                  "DPM_CONFIG"
#define DPM_USER_PARAMS_NAME             "DPM_USER_PARAMS"

#define DM_NEED_INIT                     ((unsigned char)0x01)
#define DM_NEED_CONNECT                  ((unsigned char)0x02)
#define DM_RECV_WAKEUP                   ((unsigned char)0x04)
#define DM_SENSOR_WAKEUP                 ((unsigned char)0x08)
#define DM_RTC_WAKEUP                    ((unsigned char)0x10)
#define DM_BOOT_WAKEUP                   ((unsigned char)0x20)

#define DPM_EVENT_TIMER1                 0x1
#define DPM_EVENT_TIMER2                 0x2
#define DPM_EVENT_TIMER3                 0x4
#define DPM_EVENT_TIMER4                 0x8
#define DPM_EVENT_TCP_S_CB_TIMER1        0x10
#define DPM_EVENT_TCP_S_CB_TIMER2        0x20
#define DPM_EVENT_TCP_S_CB_TIMER3        0x40
#define DPM_EVENT_TCP_S_CB_TIMER4        0x80
#define DPM_EVENT_TCP_C_CB_TIMER1        0x100
#define DPM_EVENT_TCP_C_CB_TIMER2        0x200
#define DPM_EVENT_TCP_C_CB_TIMER3        0x400
#define DPM_EVENT_TCP_C_CB_TIMER4        0x800
#define DPM_EVENT__ALL                   0xFFF

#define DPM_MANAGER_REG_NAME             "DPM_CRTL_MANAGER"
#define DPM_EVENT_REG_NAME               "DPM_EVENT_MANAGER"

#define CMD_NONE                         0x0
#define CMD_REQUEST_STOP                 0x1
#define CMD_REQUEST_RESTART              0x2

#define TIMER_TYPE_NONE                  0
#define TIMER_TYPE_PERIODIC              1
#define TIMER_TYPE_ONETIME               2

#define REG_TYPE_NONE                    0
#define REG_TYPE_TCP_SERVER              1
#define REG_TYPE_TCP_CLIENT              2
#define REG_TYPE_UDP_SERVER              3
#define REG_TYPE_UDP_CLIENT              4

#define SESSION1                         1
#define SESSION2                         2
#define SESSION3                         3
#define SESSION4                         4

#define DM_IP_LEN                        32
#define TASK_NAME_LEN                    20

#define TCP_LISTEN_BAGLOG                4

#define MAX_TIMER_CNT                    4
#define MAX_SESSION_CNT                  4

#ifndef    __LIGHT_DPM_MANAGER__
#define DM_TCP_SVR_MAX_SESS              2
#define DM_UDP_SVR_MAX_SESS              2

#define TCP_SVR_DPM_BACKLOG              5

#define TCP_SVR_RX_TIMEOUT               100
#define TCP_SVR_RX_SECURE_TIMEOUT        2000
#endif // !__LIGHT_DPM_MANAGER__

#define TCP_CLI_RX_TIMEOUT               100            // 100 ms
#define TCP_CLI_RX_SECURE_TIMEOUT        1000        // 1 sec

#ifndef    __LIGHT_DPM_MANAGER__
#define UDP_SVR_RX_TIMEOUT               300
#define UDP_SVR_RX_SECURE_TIMEOUT        2000
#define UDP_SVR_ACCEPT_TIMEOUT           100            // 100 ms
#define UDP_SVR_SECURE_ACCEPT_TIMEOUT    2000
#endif // !__LIGHT_DPM_MANAGER__

#define SSL_READ_TIMEOUT                 100

#define UDP_CLI_RX_TIMEOUT               300            // 300 ms
#define UDP_CLI_RX_SECURE_TIMEOUT        1000        // 1 sec

#define SESSION_STATUS_STOP              0
#define SESSION_STATUS_INIT              1
#define SESSION_STATUS_WAIT_ACCEPT       2
#define SESSION_STATUS_CONNECTED         3
#define SESSION_STATUS_RUN               4
#define SESSION_STATUS_GOING_STOP        5
#define SESSION_STATUS_READY_TO_CREATE   6

#define SECURE_MAX_NAME                  32

#define HANDSHAKE_RETRY_COUNT            2

#define WAIT_ACCEPT_NONE                 0xFFFF

typedef enum {
    CANCELLED = -1,
    NO_EXPIRY = 0,
    INT_EXPIRY = 1,
    FIN_EXPIRY = 2
} SECRE_TIMER_STATE;

typedef struct secure_timer {
    UINT int_ms;
    UINT fin_ms;
    TickType_t snapshot;
} SECURE_TIMER;

typedef    struct SECURE_INFO {
    mbedtls_net_context     *server_ctx;
    mbedtls_net_context     *listen_ctx;
    mbedtls_net_context     *client_ctx;
    mbedtls_ssl_context     *ssl_ctx;
    mbedtls_ssl_config      *ssl_conf;
    mbedtls_ssl_cookie_ctx  *cookie_ctx;
    mbedtls_x509_crt        *cacert_crt;      // for TLS
    mbedtls_x509_crt        *cert_crt;        // for TLS
    mbedtls_pk_context      *pkey_ctx;        // for TLS
    mbedtls_pk_context      *pkey_alt_ctx;    // for TLS

    UINT          localPort;                  // for client
    char          localIp[DM_IP_LEN];
    UINT          subIndex;                   // for TCP_SERVER

    ULONG         timeout;                    // common
    SECURE_TIMER  timer;
    UINT          sessionType;
    void          *sessionConfPtr;
} SECURE_INFO_T;

typedef struct _timer_config {
    /// timer #1 type
    UINT            timerType;

    /// timer #1 interval
    UINT            timerInterval;

    /// timer #1 callback function
    void            (*timerCallback)();
} timer_config_t;

#ifndef __LIGHT_DPM_MANAGER__
typedef struct tcpRxSubTaskConf {               // for MULTI_TCP_SERVER
    int                 allocFlag;
    UINT                requestCmd;
    UINT                runStatus;
    UINT                connStatus;
    UINT                subIndex;
    char                tcpRxSubTaskName[TASK_NAME_LEN];
    ULONG               peerPort;
    char                peerIpAddr[DM_IP_LEN];
    UINT                subSessionKaTid;        // for keepalive tid
    int                 clientSock;
    UCHAR               *tcpServerSessDataBuffer;
    TaskHandle_t        tcpRxSubTaskHandler;
    SECURE_INFO_T       secureConfig;
    UINT                sendingFlag;
    UINT                receivingFlag;
    unsigned long       lastAccessRTC;
    void                *sessConfPtr;
} tcpRxSubTaskConf_t;

typedef struct udpRxSubSessConf {               // for MULTI_UDP_SERVER
    int                 allocFlag;
    UINT                requestCmd;
    UINT                runStatus;
    UINT                subIndex;
    char                udpSubSessName[TASK_NAME_LEN];
    ULONG               peerPort;
    char                peerIpAddr[DM_IP_LEN];
    UINT                subSessionKaTid;        // for keepalive tid
    int                 clientSock;
    UCHAR               *udpServerSessDataBuffer;
    SECURE_INFO_T       secureConfig;
    UINT                sendingFlag;
    UINT                receivingFlag;
    unsigned long       lastAccessRTC;
    void                *sessConfPtr;
} udpSubSessConf_t;
#endif // !__LIGHT_DPM_MANAGER__

typedef struct _session_config {
    /// Session index number
    UINT                sessionNo;
    /// Session type
    UINT                sessionType;

    /// for Server : My local port number
    UINT                sessionMyPort;
    /// for Server : Peer port number
    ULONG               peerPort;
    /// for Server : peer IP address
    char                peerIpAddr[DM_IP_LEN];

    /// for Client : My local port
    ULONG                localPort;
    /// for Client : Server domain or IP address
    char                sessionServerIp[DM_IP_LEN];
    /// for Client : Server port number
    UINT                sessionServerPort;

#ifndef __LIGHT_DPM_MANAGER__
    /// for TCP server (MULTI_TCP_SERVER)    : next TCP information for multi-connection
    tcpRxSubTaskConf_t  *tcpSubInfo[DM_TCP_SVR_MAX_SESS];
    /// for TCP server (MULTI_TCP_SERVER)    : next TCP session ID for multi-connection
    UINT                connSessionId;

    /// for UDP server (MULTI_UDP_SERVER)    : next UDP information for multi-connection
    udpSubSessConf_t    *udpSubInfo[DM_UDP_SVR_MAX_SESS];
#endif // !__LIGHT_DPM_MANAGER__

    /// for TCP    : Keepalive interval
    UINT                sessionKaInterval;
    /// for TCP    : Keepalive TID
    int                    sessionKaTid;

    /// for TCP    Client : connect status
    UINT                sessionConnStatus;
    /// for TCP    Client : TCP window size ( SESSION_OPTION )
    UINT                sessionWindowSize;
    /// for TCP    Client : Connect retry count ( SESSION_OPTION )
    UINT                sessionConnRetryCnt;
    /// for TCP client    : Connect wait time ( SESSION_OPTION )
    UINT                sessionConnWaitTime;
    /// for TCP client    : Auto reconnect ( SESSION_OPTION )
    UINT                sessionAutoReconn;

    /// Config for Secure mode
    SECURE_INFO_T       secureConfig;

    /// Connect status callback function
    void                (*sessionConnectCallback)();
    /// Packet receive callback function
    void                (*sessionRecvCallback)();

    /// Register name for DPM operation
    char                dpmRegName[TASK_NAME_LEN];

    /// Flag for TLS/TCP mode
    UINT                supportSecure;
    /// TLS/TCP setup callback function
    void                (*sessionSetupSecureCallback)();

    char                sessTaskName[TASK_NAME_LEN];
    size_t              sessionStackSize;

    int                 socket;
    char                *rxDataBuffer;
    UINT                requestCmd;
    UINT                runStatus;
    UINT                sendingFlag;
    UINT                receivingFlag;
    UINT                is_force_stopped;

    unsigned long       lastAccessRTC;

#ifdef SESSION_MUTEX
    SemaphoreHandle_t   socketMutex;
#endif

} session_config_t;

typedef    struct {
    UINT                magicCode;

    /// POR booting function
    void                (*bootInitCallback)();
    /// DPM Wakeup booting function
    void                (*wakeupInitCallback)();

    timer_config_t      timerConfig[MAX_TIMER_CNT];
    session_config_t    sessionConfig[MAX_SESSION_CNT];

    UCHAR               *ptrDataFromRetentionMemory;
    UINT                sizeOfRetentionMemory;

    void                (*externWakeupCallback)();
    /// error callback function
    void                (*errorCallback)();
    /// udp holepunch data area
    char                udphData[128];

#ifndef __LIGHT_DPM_MANAGER__
    tcpRxSubTaskConf_t  tcpRxTaskConfig[DM_TCP_SVR_MAX_SESS];
    udpSubSessConf_t    udpRxTaskConfig[DM_UDP_SVR_MAX_SESS];
#endif // !__LIGHT_DPM_MANAGER__

} dpm_user_config_t;

#define DM_ERROR_NO_BOOT_INIT_CALLBACK          0x01
#define DM_ERROR_NO_WAKEUP_INIT_CALLBACK        0x02
#define DM_ERROR_ASSIGN_RTM                     0x03
#define DM_ERROR_ALLOCATE_RTM                   0x04
#define DM_ERROR_INVALID_SIZE_RTM               0x05
#define DM_ERROR_TIMER_TYPE                     0x06
#define DM_ERROR_TCP_SEND_NOT_ESTABLISHED       0x07
#define DM_ERROR_SEND_ALLOC_FAIL                0x08
#define DM_ERROR_SEND_APPEND_FAIL               0x09
#define DM_ERROR_SEND_FAIL                      0x0A
#define DM_ERROR_DPM_SOCKET_RESTORE_FAIL        0x0B
#define DM_ERROR_DPM_SOCKET_LISTEN_FAIL         0x0C
#define DM_ERROR_SOCKET_ALLOC_FAIL              0x0D
#define DM_ERROR_SOCKET_CREATE_FAIL             0x0E
#define DM_ERROR_SOCKET_BIND_FAIL               0x0F
#define DM_ERROR_SOCKET_CONNECT_FAIL            0x10
#define DM_ERROR_RX_BUFFER_ALLOC_FAIL           0x11
#define DM_ERROR_SESSION_START_FAIL             0x12
#define DM_ERROR_RX_DATA_RETRIEVE_FAIL          0x13
#define DM_ERROR_NOT_FOUND_SUB_SESSION          0x14
#define DM_ERROR_TASK_CREATE_FAIL               0x15
#define DM_ERROR_TASK_STACK_ALLOC_FAIL          0x16
#define DM_ERROR_RTM_READ_FAIL                  0x17
#define DM_ERROR_SETSOCKOPT_FAIL                0x18
#define DM_ERROR_NET_DOWN                       0x19
#define DM_ERROR_SECURE_INIT_FAIL               0x1A
#define DM_ERROR_RESTORE_SSL_FAIL               0x1B
#define DM_ERROR_HANDSHAKE_FAIL                 0x1C
#define DM_ERROR_SOCKET_MUTEX_CREATE_FAIL       0x1D
#define DM_ERROR_STOP_REQUEST                   0x1E
#define DM_ERROR_RECV_SIZE_FAIL                 0x1F
#define DM_ERROR_SOCKET_NOT_CONNECT             0x20
#define DM_ERROR_SECURE_UNKNOWN_FAIL            0x21
#define DM_ERROR_RESTART_REQUEST                0x22
#define DM_ERROR_PEER_CLOSED_CONNECTION         0x23
#define DM_ERROR_CLOSED_CONNECTION              0x24
#define DM_ERROR_RECEIVE_TIMEOUT                0x25
#define DM_ERROR_TIMER_CREATE                   0x26
#define DM_ERROR_OUT_OF_RANGE                   0x27
#define DM_ERROR_SAVE_SSL_FAIL                  0x28
#define DM_ERROR_INVALID_TYPE                   0x29
#define DM_ERROR_FORCE_STOPPED                  0x30

///////////////////////////////////////////////////////////////////

#define TCP_SERVER_WINDOW_SIZE                  (1024*4)
#define TCP_SERVER_RX_BUF_SIZE                  1500

#define TCP_CLIENT_WINDOW_SIZE                  (1024*4)
#define TCP_CLIENT_RX_BUF_SIZE                  TCP_SERVER_RX_BUF_SIZE
#define TIMER_CNT_SECOND                        5
#define RECONN_WAIT_TIME                        500        // 500 ticks : 5 seconds
#define DEFAULT_BIND_WAIT_TIME                  100        // 100 ticks : 1 seconds
#define DEFAULT_CONN_WAIT_TIME                  100        // 100 ticks : 1 seconds

#define TCP_SERVER_DEF_WAIT_TIME                700        // tick
#define TCP_CLIENT_DEF_WAIT_TIME                700        // tick
#define UDP_SERVER_DEF_WAIT_TIME                500        // tick
#define UDP_CLIENT_DEF_WAIT_TIME                700        // tick

#define UDP_SERVER_RX_BUF_SIZE                  1500
#define UDP_SERVER_MAX_QUEUE                    5
#define UDP_PERIOD_SEC                          5

#define UDP_CLIENT_RX_BUF_SIZE                  UDP_SERVER_RX_BUF_SIZE
#define UDP_CLIENT_MAX_QUEUE                    5

#define DPM_MANAGER_NAME                        "dpm_manager"
#define DPM_CTRL_MNG_STK                        (128*4)
#define DPM_CONTROL_MANAGER_PRI                 (OS_TASK_PRIORITY_SYSTEM+12)

#define DPM_EVENT_NAME                          "dpm_event"
#define DPM_EVENT_MNG_STK                       (128*4)
#define DPM_EVENT_MANAGER_PRI                   (OS_TASK_PRIORITY_SYSTEM+1)

#define TCP_SERVER_SESSION_TASK_PRIORITY        OS_TASK_PRIORITY_SYSTEM
#define UDP_SERVER_SESSION_TASK_PRIORITY        OS_TASK_PRIORITY_SYSTEM-1
#define TCP_CLIENT_SESSION_TASK_PRIORITY        OS_TASK_PRIORITY_SYSTEM-2
#define UDP_CLIENT_SESSION_TASK_PRIORITY        OS_TASK_PRIORITY_SYSTEM-3

// Local Defines
#define SESSION_TASK_STACK_SIZE                 128*5
#define SESSION_TASK_UDP_SVR_STACK_SIZE         128*5
#define SESSION_TASK_TCP_SVR_STACK_SIZE         128*2
#define SESSION_TASK_TCP_SUB_TASK_STACK_SIZE    128*5

#define TCP_SERVER_DPM_SECURE_NAME              "tcp_srv_tls"
#define TCP_CLIENT_DPM_SECURE_NAME              "tcp_cnt_tls"
#define UDP_SERVER_DPM_SECURE_NAME              "udp_svr_tls"
#define UDP_CLIENT_DPM_SECURE_NAME              "udp_cnt_tls"

#define DTLS_SERVER_DEF_TIMEOUT                 500        // msec
#define DTLS_SERVER_ACCEPT_TIMEOUT              1000    // msec
#define DTLS_SERVER_RECEIVE_TIMEOUT             100        // msec
#define DTLS_SERVER_HANDSAHKE_MIN_TIMEOUT       100        // msec
#define DTLS_SERVER_HANDSAHKE_MAX_TIMEOUT       400        // msec

#define DM_SECURE_RECEIVE_EVENT                 ((ULONG) 0x00000001)
#define DM_SECURE_ALL_EVENTS                    ((ULONG) 0xFFFFFFFF)

#endif // __DA16X_DPM_MANAGER_H__

#define DM_CA_CERT_SIZE                         2048
#define DM_PRIVATE_KEY_SIZE                     2048


//
// External functions
//
extern UINT get_ip_info(int iface_flag, int info_flag, char* result_str);
extern int  get_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx);
extern int  set_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx);
extern int  clr_tls_session(const char *name);
extern int  check_net_init(int iface);
extern long iptolong(char *ip);
extern void longtoip(long ip, char *ipbuf);
extern USHORT get_random_value_ushort(void);

//
// Local API functions
//
/*
 ****************************************************************************************
 * @brief  Get User configuration of DPM manager.
 * @return returns the pointer of user configuration.
 ****************************************************************************************
 */
dpm_user_config_t *dpm_get_user_config(void);

/*
 ****************************************************************************************
 * @brief     Get User configuration in RTM of DPM manager.
 * @return    returns the pointer of user configuration in RTM.
 ****************************************************************************************
 */
dpm_user_config_t *dpm_get_rtm_user_config(void);

//
// Global API functions
//

/**
 *********************************************************************************
 * @brief     Display session information to handle DPM manager
 * @return    None
 *********************************************************************************
 */
void dpm_mng_print_session_info(void);

/**
 *********************************************************************************
 * @brief     Display session configuration to handle DPM manager
 * @return    None
 *********************************************************************************
 */
void dpm_mng_print_session_config(UINT sessionNo);

/**
 *********************************************************************************
 * @brief        Start DPM manager Task
 * @return       ERR_OK on success
 *********************************************************************************
 */
int dpm_mng_start(void);

/**
 *********************************************************************************
 * @brief        Start user application session on DPM manager
 * @param[in]    sess_idx    DPM Manager session index to stop operation
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_start_session(UINT sess_idx);

/**
 *********************************************************************************
 * @brief        Stop user application session on DPM manager
 * @param[in]    sess_idx    DPM Manager session index to stop operation
 * @return       ERR_OK on success
 *********************************************************************************
 */
int dpm_mng_stop_session(UINT sessionNo);


/**
 *********************************************************************************
 * @brief        Packet transmission through session
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    ip:          IP address
 * @param[in]    port:        port number
 * @param[in]    size:        Size of packet
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_send_to_session(UINT sessionNo, ULONG ip, ULONG port, char *buf, UINT size);

/**
 *********************************************************************************
 * @brief        Request configuration of session information
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    type:        Type of session (0:NONE, 1:TCP Server, 2:TCP Client, 3:UDP Server, 4:UDP Client
 * @param[in]    myPort:      Local port number(
 * @param[in]    peerIp:      Peer IP address
 * @param[in]    peerPort:    Peer port number
 * @param[in]    kaInterval:  Keepalive interval
 * @param[in]    connCb():    Callback function to get connection notification
 * @param[in]    recvCb():    Callback Function to Receive Packet
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info(UINT sessionNo,
                            ULONG type,
                            ULONG myPort,
                            char *peerIp,
                            ULONG peerPort,
                            ULONG kaInterval,
                            void (*connCb)(),
                            void (*recvCb)());

/**
 *********************************************************************************
 * @brief        Register your own port number for the session
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    port:        Port number
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_my_port_no(UINT sessionNo, ULONG port);

/**
 *********************************************************************************
 * @brief        Register peer port number for the session (server only)
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    port:        Port number
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_peer_port_no(UINT sessionNo, ULONG port);

/**
 *********************************************************************************
 * @brief        Register peer port number (server only)
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    ip:          IP address
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_peer_ip_addr(UINT sessionNo, char *ip);

/**
 *********************************************************************************
 * @brief        Register server ip address (client only)
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    ip:          IP address
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_server_ip_addr(UINT sessionNo, char *ip);

/**
 *********************************************************************************
 * @brief        Register server port number (client only)
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    port:        Port number
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_server_port_no(UINT sessionNo, ULONG port);

/**
 *********************************************************************************
 * @brief        Register local port number (client only)
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    port:        Port number
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_local_port(UINT sessionNo, ULONG port);

/**
 *********************************************************************************
 * @brief        Register window size in TCP connection
 * @param[in]    sessionNo:   Session number (1~4)
 * @param[in]    windowSize:  TCP Window size
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_window_size(UINT sessionNo, UINT windowSize);

/**
 *********************************************************************************
 * @brief        Register count for reconnection on TCP connection
 * @param[in]    sessionNo:      Session number (1~4)
 * @param[in]    connRetryCount: Count attempts to reconnect
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_conn_retry_count(UINT sessionNo, UINT connRetryCount);

/**
 *********************************************************************************
 * @brief        Register timeout value in TCP connection
 * @param[in]    sessionNo:    Session number (1~4)
 * @param[in]    connWaitTime: Waiting time for connection
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_conn_wait_time(UINT sessionNo, UINT connWaitTime);

/**
 *********************************************************************************
 * @brief
 * @param[in]    sessionNo
 * @param[in]    autoReconnect: 1 means auto connect mode, 0 means no auto connect
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_session_info_auto_reconnect(UINT sessionNo, UINT autoReconnect);

/**
 *********************************************************************************
 * @brief        Register timer with corresponding id and parameter information
 * @param[in]    id:             Timer id(1~4)
 * @param[in]    type:           Type of timer (0:NONE, 1:PERIODIC, 2:ONETIME)
 * @param[in]    interval:       Timer period (uint:second)
 * @param[in]    timerCallback:  Callback function by timer
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_set_DPM_timer(UINT id, UINT type, UINT interval, void (*timerCallback)());

/**
 *********************************************************************************
 * @brief        Remove timer registered with corresponding id
 * @param[in]    id :            Timer id(1~4)
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_unset_DPM_timer(UINT id);

/**
 *********************************************************************************
 * @brief        Request data save to retention memory
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_save_to_RTM(void);

/**
 *********************************************************************************
 * @brief        Indicates that the job has started
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_job_start(void);

/**
 *********************************************************************************
 * @brief        Indicate that the job is completed
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_job_done(void);

/**
 *********************************************************************************
 * @brief        Register a function to initialize in application
 * @param[in]    regConfigFunction: Callback function for configuration
 * @return       0 on success
 *********************************************************************************
 */
int dpm_mng_regist_config_cb(void (*regConfigFunction)());

/**
 *********************************************************************************
 * @brief        Read initialization status of DPM Manager
 * @return       0 means initialization; 1 means completion
 *********************************************************************************
 */
int dpm_mng_init_done(void);

#ifdef __DPM_MNG_SAVE_RTM__
/**
 *********************************************************************************
 * @brief        Save data to retention memory
 * @return
 *********************************************************************************
 */
void dpm_mng_save_rtm(void);
#endif /*__DPM_MNG_SAVE_RTM__*/

int dpm_mng_setup_rng(mbedtls_ssl_config *ssl_conf);
int dpm_mng_cookie_setup_rng(mbedtls_ssl_cookie_ctx *cookie_ctx);

/* EOF */
