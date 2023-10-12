/**
 ****************************************************************************************
 *
 * @file act_agent.h
 *
 * @brief DPM Auto Configuration tool
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


#ifndef ACT_AGENT_H
#define ACT_AGENT_H

#define ACT_TST_TYPE_NOT_YET_CONF        0
#define ACT_TST_TYPE_TIM_WAKEUP_T        1
#define ACT_TST_TYPE_BCN_TIME_OUT        2
#define ACT_TST_TYPE_DPM_SETTING         3

/*
  The range 49152-65535 (2^15+2^14 to 2^16-1) contains dynamic or private ports that cannot be registered with IANA.[311]
  This range is used for private or customized services, for temporary purposes, and for automatic allocation of ephemeral ports.
  [311]Internet Assigned Numbers Authority (IANA) Procedures for the Management of the Service Name and Transport Protocol Port Number Registry.
  IETF. August 2011. RFC 6335.
*/
#define ACT_AGENT_TCP_PORT     50000


//Flags to sync between UDP and TCP task. Please keep 1,2,4,8 order....
#define ACT_NOT_CONNECTED                       0x00000001
#define ACT_CONNECTED                           0x00000002
#define ACT_WAITING_TCP_CMD                     0x00000004
#define ACT_NEED_START_TST                      0x00000008
#define ACT_STARTED_TST                         0x00000010
#define ACT_NEED_TO_TST_RESULT                  0x00000020
//Don't udp rx due to no power down at lease. Start UDP RX after power down since test start.
#define ACT_NOT_UDP_RX_DUETO_NO_PW              0x00000040
#define ACT_PREVIOUS_TEST_REPORT_SENT           0x00000080
#define ACT_GOT_DPM_RESUME_DURING_TEST          0x00000100

//When get ACT_CTYPE_TST_START_RDY cmd, set.
#define ACT_FULL_TST_STARTED                    0x00000200
#define ACT_TCP_RX_TIMER_ON                     0x00000400
#define ACT_TCP_TX_TIMER_ON                     0x00000800

//AOL+DAC : Factory-> AOL-> Reboot->Assoc AP -> Assoc Phone
//Configuration DPM after AOL.
#define ACT_CONF_DPM_AF_AOL                     0x00001000


//To store value of AMPDU RX configuration. During test, need to disable this.
#define ACT_AMPDU_RX_DECLINE                    0x00100000

//Whether launch act agt or not during booting/dpm wake up.
#define ACT_LAUNCH_AGT_RIGHT                    0x38000000


#define ACT_CALLED_BOOTING 0x1
#define ACT_CALLED_WORKING 0x2

// 2 means 2sec
// ACT APP will send UDP data after 2 sec since sending dpm_resume cmd.
// This is to avoid a symptom that da16x can't complete power down promptly
//                                beacause DA16X didn't get TCP ack's ieee802.11 ACK.

#define ACT_UWT_OFFSET       3
#define ACT_UWT_OFFSET2      1  //for beacon timeout test case
#define ACT_UWT_OFFSET_RC    10 //To wakeup from abnormal long DPM power down

//10 seconds
#define ACT_TCP_WAIT_CMD_TIMEOUT    10
#define ACT_TCP_TX_TIMEOUT          10

/***
  *
- When testing with DA16X App using VOL & DAC , define ACT_USE_EXTENDED_SUBTYPE
- When testing with DPM Auto Configuration Tool, undef ACT_USE_EXTENDED_SUBTYPE
  *
***/
#define  ACT_USE_EXTENDED_SUBTYPE
#define  ACT_USE_WPA_CLI           //Refer to command_net.c



#define ACT_NVRAM_STR_USER_IP	    "VOL_IP"
#define ACT_NVRAM_STR_PROV_SVR_IP   "PROV_SVR_IP"
#define ACT_NVRAM_STR_PROV_SVR_PORT "PROV_SVR_PORT"
#define NVRAM_STR_DAC_ON_FLAG       "DAC_ON_FLAG"

#define DAC_ON_CLEARED               0
#define DAC_ON_AS_TCP_CLIENT         1
#define DAC_ON_AS_TCP_SERVER         2

#ifdef ACT_USE_EXTENDED_SUBTYPE

  #define ACT_TYPE				0x03
  #define ACT_SUBTYPE			0xfc

  #define ACT_TYPE_MGMT			(0x04 >> 2)
  #define ACT_TYPE_DATA			(0x08 >> 2)

  #define ACT_SUBTYPE_TST_RESULT_C1       (0x40 >> 2)
  #define ACT_SUBTYPE_TST_START_RDY_OK    (0x70 >> 2)
  #define ACT_SUBTYPE_TST_FINISH_OK       (0x90 >> 2)
  #define ACT_SUBTYPE_CUR_BCN_TIMEOUT     (0xc0 >> 2)
  #define ACT_SUBTYPE_TST_RESULT_C2       (0xd0 >> 2)

#else

  #define ACT_RESERVED   		0x0
  #define ACT_TYPE    			0x0c
  #define ACT_SUBTYPE			0xf0

  #define ACT_TYPE_MGMT			0x04
  #define ACT_TYPE_DATA			0x08

  #define ACT_SUBTYPE_TST_RESULT_C1       0x40
  #define ACT_SUBTYPE_TST_START_RDY_OK    0x70
  #define ACT_SUBTYPE_TST_FINISH_OK       0x90
  #define ACT_SUBTYPE_CUR_BCN_TIMEOUT     0xc0
  #define ACT_SUBTYPE_TST_RESULT_C2       0xd0

#endif //ACT_USE_EXTENDED_SUBTYPE

/*
TIM PS-POLL : 0x180548

DPM_FEATURE_KEEP               = 0x00000001,
DPM_FEATURE_ARP_REPLY          = 0x00000002,
DPM_FEATURE_GARP               = 0x00000004,
DPM_FEATURE_PS_POLL            = 0x00000008                 1:ON, 0:OFF
*/

//Define To control PS-POLL of PTIM
#define DPM_FEATURE_KEEP                 0x00000001
#define DPM_FEATURE_ARP_REPLY            0x00000002
#define DPM_FEATURE_GARP                 0x00000004
#define DPM_FEATURE_PS_POLL              0x00000008

//Refer to retmem.h
#define DPM_FEATURE_FORBIDDEN_UC         0x00001000
#define DPM_FEATURE_FORBIDDEN_BC_MC      0x00002000
#define DPM_FEATURE_FORBIDDEN_NO_ACK     0x10000000
#define DPM_FEATURE_FORCE_SET_DTIM_CNT   0x00080000
#define DPM_FEATURE_FORBIDDEN_NO_BCN     0x00008000



#define DPM_PTIM_FT_CTL_REG				0x180548
#define DPM_FEATURE_CTL_REG				0x180310

#define DPM_PTIM_REG_BCN_TIMEOUT		0x1804C8
#define DPM_PTIM_REG_NO_BCN_CNT			0x186d24  // this mean beacon loss count.


#define ACT_APP_TCP_SERVER_PORT			50004
#define ACT_DA16X_TCP_CLIENT_PORT		50006

#define ACT_VOL_DAT_PEER_TCP_SVR_PORT	50000;
#define ACT_VOL_DAT_LOCAL_TCP_PORT		50000;

#define BE_2_LE16(n)  (( ((USHORT)(n) & 0x00FF) << 8 ) | (((USHORT)(n) & 0xFF00) >> 8 ) )
#define LE_2_BE16(n)  (( ((USHORT)(n) & 0x00FF) << 8 ) | (((USHORT)(n) & 0xFF00) >> 8 ) )

#define NTOH32(x)	  ( ((x & 0xff000000) >> 24) |  \
                        ((x & 0x00ff0000) >> 8)  |  \
                        ((x & 0x0000ff00) << 8)  |  \
                        ((x & 0x000000ff) << 24) )

#define  ACT_ERROR_PRINT
#define  ACT_INFO_PRINT
#define  ACT_DEBUG_CMD_PRINT
#undef   ACT_DEBUG_PRINT

#ifdef  ACT_ERROR_PRINT
    #define ACT_PRTF_E(...)		PRINTF(__VA_ARGS__ )
#else
    #define ACT_PRTF_E(...)
#endif

#ifdef  ACT_INFO_PRINT
    #define ACT_PRTF_I(...)		PRINTF(__VA_ARGS__ )
#else
    #define ACT_PRTF_I(...)
#endif

#ifdef  ACT_DEBUG_CMD_PRINT
    #define ACT_PRTF_D_C(...)	PRINTF(__VA_ARGS__ )
#else
    #define ACT_PRTF_D_C(...)
#endif

#ifdef  ACT_DEBUG_PRINT
    #define ACT_PRTF_D(...)		PRINTF(__VA_ARGS__ )
#else
    #define ACT_PRTF_D(...)
#endif


/* define ACT management commands */
enum act_cmd_type{

    ACT_CTYPE_UP_USR_WT = 1,
    ACT_CTYPE_CTL_TIM_PSPOLL,
    ACT_CTYPE_RESUME_DPM,
    ACT_CTYPE_TST_RESULT_C1,			// 4 : Result for wake up time
    ACT_CTYPE_UP_TIM_WT,				// 5
    ACT_CTYPE_TST_START_RDY,
    ACT_CTYPE_TST_START_RDY_OK,
    ACT_CTYPE_TST_FINISH,				// 8
    ACT_CTYPE_TST_FINISH_OK,			// 9
    ACT_CTYPE_GET_BCN_TOUT,				// 10(0xa)
    ACT_CTYPE_SET_BCN_TOUT,				// 11(0xb)
    ACT_CTYPE_CUR_BCN_TOUT,				// 12(0xc)
    ACT_CTYPE_TST_RESULT_C2,			// 13(0xd)	: Result for beacon timeout
    ACT_CTYPE_NEXT_TCP_PORT,			// 14(0xe)

    ACT_CYYPE_DPM_SET_START,			// 15(0Xf)
    ACT_CYYPE_DPM_SET_START_OK,			// 16(0X10)
    ACT_CTYPE_DPM_ON,					// 17(0x11)
    ACT_CTYPE_DPM_DAC_ON,				// 18(0X12)
    ACT_CTYPE_DPM_TIM_WT_100MS,			// 19(0x13)	: TIM WAKEUPTIME on ms unit.
    ACT_CTYPE_DPM_KA,					// 20(0x14)
    ACT_CTYPE_DPM_SET_FINISH,			// 21(0x15)
    ACT_CTYPE_DPM_SET_FINISH_OK,		// 22(0x16)
    ACT_CTYPE_PROV_SVR_IP,				// 23(0x17)
    ACT_CTYPE_PROV_SVR_PORT,			// 24(0x18)
    ACT_CTYPE_DAC_OFF_AF_DISC,			// 25(0x19)

    ACT_CTYPE_MAX						// 26(0x1a)
};

/* TCP server type to connect */
enum act_tcp_svr_type{
    ACT_TCP_SVR_TYPE_AFTER_AOL = 1,
    ACT_TCP_SVR_TYPE_ON_TESTING,
};


/* define data type */
enum act_data_type{

    ACT_DTYPE_CH_PS_BUF = 1,
    ACT_DTYPE_MAX
};

/* Start of packet format define */

#pragma pack(1)

struct act_data_next_tcp_port{
        USHORT port_num;
};

struct act_msg_hdr{
        UCHAR  type;
        USHORT length; //doesn't include length of the type and Length. only has length of data.
};

//data field of ACT_CTYPE_UP_USR_WT
struct act_data_usr_wu_time{
        USHORT usr_wu_t;
        UCHAR  total_try_cnt;
};

//data field of ACT_CTYPE_TST_RESULT_C1
struct act_data_tst_rsl_c1{
        USHORT usr_wu_time;
        UCHAR  success_cnt;
        UCHAR  failure_cnt;
};

//data field of ACT_CTYPE_TST_RESULT_C2
struct act_data_tst_rsl_c2{
        USHORT usr_wu_time;
        UCHAR  bcn_timeout;
        USHORT no_bcn_cnt;
};

#pragma pack()

/* End of packet format define */


struct act_tst_rsl_n1_info{
        USHORT usr_wu_time;
        UCHAR  success_cnt;
        UCHAR  failure_cnt;
};


/*refer to RTM_DPM_AUTO_CONFIG_BASE, da16x_dpm_regs.h */
typedef struct	act_rtm_map
{
    volatile UINT16	act_total_cnt;
    volatile UINT16	act_succ_cnt;
    volatile UINT16	act_fail_cnt;
    volatile UINT16 act_test_type;
    volatile UINT32 act_tim_wakeup_t;
    volatile UINT32	act_ctl_flag;
    volatile UINT32 act_usr_wakeup_t;
    volatile ULONG  act_tcp_svr_ip;
    volatile UINT32 act_next_tcp_port;
    volatile UINT16 act_rs_tmr_n;     //tmr # to wake up from abnormal continuous  dpm power down.
} ATC_RTM_MAP;

/* To free when delete task */
typedef struct _act_task_mem_info
{
    void        * tcp_stack_ptr;
    TX_THREAD   * tcp_thread_ptr;
    char        * tcp_rx_buf;
    void        * udp_stack_ptr;
    TX_THREAD   * udp_thread_ptr;
    char        * udp_rx_buf;

} ACT_TASK_MEM_INFO_T;


struct act_env{
    ACT_TASK_MEM_INFO_T task_mem_info;
    USHORT       nobcn_cnt;

    // To send TCP packet by using other function.
    //NX_TCP_SOCKET   * tcp_trx_sock_p;
    NX_UDP_SOCKET   * agt_udp_sock_p;
    NX_IP           * agt_ctlp_rx_ip_p;
    int               tcp_port_no;

    //Need to make pointer and alloc
    TX_MUTEX rtm_val_lock;

    char  *tcp_sock_name_str_p;
    char  *tcp_rx_name_p;
    char  *udp_sock_name_str_p;

    int    tcp_rx_timer_id;
    int    tcp_tx_timer_id;

    //When tx timeout, try to connect w/ next tcp port.
    unsigned char act_tout_cnt;
    //When act_set_2nd_tx_timer is set, set tx_timer.
    unsigned char set_2nd_tx_timer;

    //To go DAC's TCP server listen state.
    unsigned char dpm_dac_on;
    //To turn off DAC after disconnection
    unsigned char dac_off_after_disc;
    //To disconnect due to timeout after VOL
    unsigned char needto_disc_dueto_tout;
};



typedef	struct	_act_thd_info_	{
    char	*name;				        // Thread Name
    VOID 	(*entry_function)(ULONG);	// Funtion Entry_point
    UINT	stksize;			        // Thread Stack Size
    UINT	priority;			        // Thread Priority
    UINT	preempt;			        // Thread Preemption Priority
    ULONG	tslice;				        // Thread Time Slice
    UINT	autorun;			        // Thread Start type

    char	*dpm_reg_name;		        // DPM Unique registration name
    UINT	port_no;			        // Port number for Data comm.
    UINT	dpm_usage;			        // Usage flag for DPM running
} ACT_THD_INFO_T;


#endif /* ACT_AGENT_H */

/* EOF */
