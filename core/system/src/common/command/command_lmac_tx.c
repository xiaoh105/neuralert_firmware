/**
 ****************************************************************************************
 *
 * @file command_lmac_tx.c
 *
 * @brief Command Parser for LMAC Tx commands
 *
 * Copyright (C) RivieraWaves 2011-2015
 *
 * Copyright (c) 2019-2022 Modified by Renesas Electronics.
 ****************************************************************************************
 */

#include "sdk_type.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "monitor.h"
#include "command.h"
#include "command_lmac_tx.h"
#include "oal.h"
#include "da16x_system.h"
#include "app_common_util.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#undef	SUPPORT_SEND_NULL_FRAME
#undef	SUPPORT_ADC_CAL_TEST


typedef uint16_t u16_l;
/// MAC address length in bytes.
#define MAC_ADDR_LEN 6
#define MAC_SEQCTRL_NUM_OFT             4

/// MAC address structure.
struct mac_addr
{
	/// Array of bytes that make up the MAC address.
	u16_l array[MAC_ADDR_LEN/2];
};

/// Structure of a long control frame MAC header
struct mac_hdr
{
	/// Frame control
	uint16_t fctl;

	/// Duration/ID
	uint16_t durid;

	/// Address 1
	struct mac_addr addr1;

	/// Address 2
	struct mac_addr addr2;

	/// Address 3
	struct mac_addr addr3;

	/// Sequence control
	uint16_t seq;
} ;

enum
{
	/// MAC HW IDLE State.
	HW_IDLE = 0,

	/// MAC HW RESERVED State.
	HW_RESERVED,

	/// MAC HW DOZE State.
	HW_DOZE,

	/// MAC HW ACTIVE State.
	HW_ACTIVE
};

#define REG_PL_WR(addr, value)       ((*(volatile uint32_t *)(addr)) = (uint32_t)(value))
#define REG_PL_RD(addr)              (*(volatile uint32_t *)(addr))
#define BIT(x) (1 << (x))

struct machw_mib_tag
{
	/// MIB element to count the number of unencrypted frames that have been discarded
	uint32_t dot11_wep_excluded_count;

	/// MIB element to count the receive FCS errors
	uint32_t dot11_fcs_error_count;

	/**
	  * MIB element to count the number of PHY Errors reported during a receive
	  * transaction.
	  */
	uint32_t nx_rx_phy_error_count;

	/// MIB element to count the number of times the receive FIFO has overflowed
	uint32_t nx_rd_fifo_overflow_count;

	/**
	  * MIB element to count the number of times underrun has occured on the
	  * transmit side
	  */
	uint32_t nx_tx_underun_count;

	/// Counts the number of times an overflow occurrs at the MPIF FIFO when receiving a frame from the PHY
	uint32_t nx_rx_mpif_overflow_count;

	/// Reserved
	uint32_t reserved_1[6];

	/// MIB element to count unicast transmitted MPDU
	uint32_t nx_qos_utransmitted_mpdu_count[8];

	/// MIB element to count group addressed transmitted MPDU
	uint32_t nx_qos_gtransmitted_mpdu_count[8];

	/**
	  * MIB element to count the number of MSDUs or MMPDUs discarded because of
	  * retry-limit reached
	  */
	uint32_t dot11_qos_failed_count[8];

	/**
	  * MIB element to count number of unfragmented MSDU's or MMPDU's transmitted
	  * successfully after 1 or more transmission
	  */
	uint32_t dot11_qos_retry_count[8];

	/// MIB element to count number of successful RTS Frame transmission
	uint32_t dot11_qos_rts_success_count[8];

	/// MIB element to count number of unsuccessful RTS Frame transmission
	uint32_t dot11_qos_rts_failure_count[8];

	/// MIB element to count number of MPDU's not received ACK
	uint32_t nx_qos_ack_failure_count[8];

	/// MIB element to count number of unicast MPDU's received successfully
	uint32_t nx_qos_ureceived_mpdu_count[8];

	/// MIB element to count number of group addressed MPDU's received successfully
	uint32_t nx_qos_greceived_mpdu_count[8];

	/**
	  * MIB element to count the number of unicast MPDUs not destined to this device
	  * received successfully.
	  */
	uint32_t nx_qos_ureceived_other_mpdu[8];

	/// MIB element to count the number of MPDUs received with retry bit set
	uint32_t dot11_qos_retries_received_count[8];

	/**
	  * MIB element to count the number of unicast A-MSDUs that were transmitted
	  * successfully
	  */
	uint32_t nx_utransmitted_amsdu_count[8];

	/**
	  * MIB element to count the number of group-addressed A-MSDUs that were
	  * transmitted successfully
	  */
	uint32_t nx_gtransmitted_amsdu_count[8];

	/// MIB element to count number of AMSDU's discarded because of retry limit reached
	uint32_t dot11_failed_amsdu_count[8];

	/// MIB element to count number of A-MSDU's transmitted successfully with retry
	uint32_t dot11_retry_amsdu_count[8];

	/**
	  * MIB element to count number of bytes of an A-MSDU that was
	  * transmitted successfully
	  */
	uint32_t dot11_transmitted_octets_in_amsdu[8];

	/**
	  * MIB element to counts the number of A-MSDUs that did not receive an ACK frame
	  * successfully in response
	  */
	uint32_t dot11_amsdu_ack_failure_count[8];

	/// MIB element to count number of unicast A-MSDUs received successfully
	uint32_t nx_ureceived_amsdu_count[8];

	/// MIB element to count number of group addressed A-MSDUs received successfully
	uint32_t nx_greceived_amsdu_count[8];

	/**
	  * MIB element to count number of unicast A-MSDUs not destined to this device
	  * received successfully
	  */
	uint32_t nx_ureceived_other_amsdu[8];

	/// MIB element to count number of bytes in an A-MSDU is received
	uint32_t dot11_received_octets_in_amsdu_count[8];

	/// Reserved
	uint32_t reserved_2[24];

	/// MIB element to count number of A-MPDUs transmitted successfully
	uint32_t dot11_transmitted_ampdu_count;

	/// MIB element to count number of MPDUs transmitted in an A-MPDU
	uint32_t dot11_transmitted_mpdus_in_ampdu_count ;

	/// MIB element to count the number of bytes in a transmitted A-MPDU
	uint32_t dot11_transmitted_octets_in_ampdu_count ;

	/// MIB element to count number of unicast A-MPDU's received
	uint32_t wnlu_ampdu_received_count;

	/// MIB element to count number of group addressed A-MPDU's received
	uint32_t nx_gampdu_received_count;

	/**
	  * MIB element to count number of unicast A-MPDUs received not destined
	  * to this device
	  */
	uint32_t nx_other_ampdu_received_count ;

	/// MIB element to count number of MPDUs received in an A-MPDU
	uint32_t dot11_mpdu_in_received_ampdu_count;

	/// MIB element to count number of bytes received in an A-MPDU
	uint32_t dot11_received_octets_in_ampdu_count;

	/// MIB element to count number of CRC errors in MPDU delimeter of and A-MPDU
	uint32_t dot11_ampdu_delimiter_crc_error_count;

	/**
	  * MIB element to count number of implicit BAR frames that did not received
	  * BA frame successfully in response
	  */
	uint32_t dot11_implicit_bar_failure_count;

	/**
	  * MIB element to count number of explicit BAR frames that did not received
	  * BA frame successfully in response
	  */
	uint32_t dot11_explicit_bar_failure_count;

	/// Reserved
	uint32_t reserved_3[5];

	/// MIB element to count the number of frames transmitted at 20 MHz BW
	uint32_t dot11_20mhz_frame_transmitted_count;

	/// MIB element to count the number of frames transmitted at 40 MHz BW
	uint32_t dot11_40mhz_frame_transmitted_count;

	/// MIB element to count the number of frames transmitted at 80 MHz BW
	uint32_t dot11_80mhz_frame_transmitted_count;

	/// MIB element to count the number of frames transmitted at 160 MHz BW
	uint32_t dot11_160mhz_frame_transmitted_count;

	/// MIB element to count the number of frames received at 20 MHz BW
	uint32_t dot11_20mhz_frame_received_count;

	/// MIB element to count the number of frames received at 40 MHz BW
	uint32_t dot11_40mhz_frame_received_count;

	/// MIB element to count the number of frames received at 80 MHz BW
	uint32_t dot11_80mhz_frame_received_count;

	/// MIB element to count the number of frames received at 160 MHz BW
	uint32_t dot11_160mhz_frame_received_count;

	/// MIB element to count the number of attempts made to acquire a 20 MHz TXOP
	uint32_t nx_20mhz_failed_txop;

	/// MIB element to count the number of successful acquisitions of a 20 Mhz TXOP
	uint32_t nx_20mhz_successful_txops;

	/// MIB element to count the number of attempts made to acquire a 40 MHz TXOP
	uint32_t nx_40mhz_failed_txop;

	/// MIB element to count the number of successful acquisitions of a 40 Mhz TXOP
	uint32_t nx_40mhz_successful_txops;

	/// MIB element to count the number of attempts made to acquire a 80 MHz TXOP
	uint32_t nx_80mhz_failed_txop;

	/// MIB element to count the number of successful acquisitions of a 80 Mhz TXOP
	uint32_t nx_80mhz_successful_txops;

	/// MIB element to count the number of attempts made to acquire a 160 MHz TXOP
	uint32_t nx_160mhz_failed_txop;

	/// MIB element to count the number of successful acquisitions of a 160 Mhz TXOP
	uint32_t nx_160mhz_successful_txops;

	/// Count the number of BW drop using dynamic BW management
	uint32_t nx_dyn_bw_drop_count;

	/// Count the number of failure using static BW management
	uint32_t nx_static_bw_failed_count;

	/// Reserved
	uint32_t reserved_4[2];

	/// MIB element to count the number of times the dual CTS fails
	uint32_t dot11_dualcts_success_count;

	/**
	  * MIB element to count the number of times the AP does not detect a collision
	  * PIFS after transmitting a STBC CTS frame
	  */
	uint32_t dot11_stbc_cts_success_count;

	/**
	  * MIB element to count the number of times the AP detects a collision PIFS after
	  * transmitting a STBC CTS frame
	  */
	uint32_t dot11_stbc_cts_failure_count;

	/**
	  * MIB element to count the number of times the AP does not detect a collision PIFS
	  * after transmitting a non-STBC CTS frame
	  */
	uint32_t dot11_non_stbc_cts_success_count;

	/**
	  * MIB element to count the number of times the AP detects a collision PIFS after
	  * transmitting a non-STBC CTS frame
	  */
	uint32_t dot11_non_stbc_cts_failure_count;
};

struct phy_channel_info
{
	/// PHY channel information 1
	uint32_t info1;

	/// PHY channel information 2
	uint32_t info2;
};

#define PHY_CHNL_BW_20						0

#define STACK_SIZE							256
#define LMAC_TERM_EVT						(1<<31)
#define NXMAC_GEN_INT_ENABLE_ADDR           0x60B08074
#define NXMAC_TIMERS_INT_EVENT_CLEAR_ADDR   0x60B08088
#define NXMAC_TIMERS_INT_UN_MASK_ADDR       0x60B0808C
#define MAC_CONTROL_REG1					0x60B0004c
#define DMA_CONTROL_REG						0x60B08180

#define FCI_RX_TIME

#define MEMORY_REDUCE 1

#define PHY_BAND_2G4 0
#define PHY_PRIM 0


u32 prev_power;		/* save normal mode power for restore */

//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------
void send_test_vec(int vec,int trial, UINT rate, UINT power, UINT power2, UINT length, UINT cont_tx, UINT ch);

typedef void (*cfm_func_ptr)(void *, uint32_t);
static u32 str2rate(char *str, u8 GI, u8 greenField, u8 preambleType, u32 *data_size);


extern int ldpc_enabled;
extern unsigned int htoi(char *s);
extern void hal_machw_reset(void);
extern void hal_machw_init();
extern void rxl_reset();
extern void txl_reset();
extern void cmd_mem_read(int argc, char* argv[]);
extern void cmd_lmac_rx_ch(int argc, char* argv[]);
extern void rwnx_agc_load();
extern void intc_init();
extern void ipc_emb_init();
extern void phy_init();
extern void phy_set_channel(uint8_t band, uint8_t type, uint16_t prim20_freq,
				             uint16_t center1_freq, uint16_t center2_freq, uint8_t index);
extern void phy_get_channel(struct phy_channel_info *info, uint8_t index);
extern u32 set_power2(u32 power_level);
extern void cmd_lmac_tx_power2(int argc, char *argv[]);
extern unsigned int get_txp_power2();
extern int check_lmac_init();
extern void cmd_lmac_tx_init(int argc, char *argv[]);

/* mac register access */
extern void modem_agcswreset_setf(uint8_t value);
extern void lmac_next_state_setf(uint8_t value);
extern void lmac_mib_table_reset_setf(uint8_t value);
extern void lmac_wake_up_from_doze_setf(uint8_t value);
extern uint8_t modem_agcswreset_getf();
extern uint8_t lmac_next_state_getf();
extern uint8_t lmac_current_state_getf();
extern void intc_init(void);

#define DMA_STATUS_REG1		0x60B08188
#define DMA_STATUS_REG2		0x60B0818c
#define DMA_STATUS_REG3		0x60B08190
#define DMA_STATUS_REG4		0x60B08194

#define NX_TXQ_CNT			5
extern unsigned int lmac_tx_cfm_count[NX_TXQ_CNT];
extern unsigned int lmac_tx_count[NX_TXQ_CNT];
extern uint32_t timeout_count[6];

extern unsigned char cmd_lmac_tx_flag;

void tx_status()
{
	int i;

	for (i = 0; i < NX_TXQ_CNT-1; i++)
		PRINTF("TX AC%d COUNT [OP/CFM] %d/%d\n", i, lmac_tx_count[i], lmac_tx_cfm_count[i]);

	PRINTF("TX BCN COUNT [OP/CFM] %d/%d\n", lmac_tx_count[4], lmac_tx_cfm_count[4]);
}

static void dma_status()
{
	PRINTF("dma status reg1 : 0x%x\n", REG_PL_RD(DMA_STATUS_REG1) );
	PRINTF("dma status reg2 : 0x%x\n", REG_PL_RD(DMA_STATUS_REG2) );
	PRINTF("dma status reg3 : 0x%x\n", REG_PL_RD(DMA_STATUS_REG3) );
	PRINTF("dma status reg4 : 0x%x\n", REG_PL_RD(DMA_STATUS_REG4) );
}

static void timeout_status()
{
	int i;

	for (i = 0; i < NX_TXQ_CNT-1; i++)
		PRINTF("TX AC%d Timout COUNT %d\n", i, timeout_count[i]);

	PRINTF("TX BCN  Timout COUNT %d\n", timeout_count[4]);
	PRINTF("TX IDLE Timout COUNT %d\n", timeout_count[5]);
}

#define NXMAC_MAC_ERR_SET_STATUS_ADDR   0x60B00058
#define NXMAC_DEBUG_HWSM_1_ADDR			0x60B00500
#define NXMAC_DEBUG_HWSM_2_ADDR			0x60B00504

int test_flag = 7;
void cmd_lmac_status(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (test_flag & 1)
		PRINTF(" NXMAC_MAC_ERR_SET_STATUS_ADDR 0x%x \n", REG_PL_RD(NXMAC_MAC_ERR_SET_STATUS_ADDR) );
	if (test_flag & 2)
		PRINTF(" NXMAC_DEBUG_HWSM_1_ADDR 0x%x \n", REG_PL_RD(NXMAC_DEBUG_HWSM_1_ADDR) );
	if (test_flag & 4)
		PRINTF(" NXMAC_DEBUG_HWSM_2_ADDR 0x%x \n", REG_PL_RD(NXMAC_DEBUG_HWSM_2_ADDR) );

	dma_status();
	tx_status();
	timeout_status();
}

void fpga_maxim_rf_setting()
{
	/* For FPGA MAXIM RF Settings
				                MAXIM RF       FCI RF
		RFGAINMINDB :         0               -10
		VGAMINDB    :         0                 5
		VGAINDMAX  :         31                54       */

	REG_PL_WR(0x60C02000, 0x6400);
	REG_PL_WR(0x60C02004, 0x1E0F00);
	REG_PL_WR(0x60C02008, 0x1E1E0F);
}

struct sim_tx_desc {
	char str[64];
	UINT ac;
	UINT thd[17];
	UINT pea[13];
	UINT header[10];
	UINT tbd[5];
	UINT data[10];
};

enum {BCN = 0,AC0,AC1,AC2,AC3};
const int ac2cfm[5] = {4,0,1,2,3};

#define NUM_BCN_DATA_MODE 7
#define TX_DATA_NUM 10
const struct sim_tx_desc sim[TX_DATA_NUM] = {
	{ "testTxBeacon", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x77, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x80004, 0x6, 0x4, 0x4, 0x20, 0x20, 0x20, 0x20},
	{ 0x00000080, 0xffffffff, 0x4454FFFF, 0x80112233, 0x33445566, 0x8122,},
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0, 0, 0x00093333, 0x018f0100,0x03443302, 0x06040f01, 0xabcd0200, 0x04050000, 0x25040001, 0} },
	{ "testTxBeacon withoutAC", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x8F, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x80004, 0x6, 0x4, 0x4, 0x20, 0x20, 0x20, 0x20},
	{ 0x00800000, 0xffff0000, 0xffffffff, 0x22334454, 0x55668011, 0x81223344,},
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0, 0, 0x00093333, 0x018f0100,0x03443302, 0x06040f01, 0xabcd0200, 0x04050000, 0x25040001, 0} },
	{ "testTxBeacon, DSSS", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x8F, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x400, 0x400, 0x400, 0x400, 0x20, 0x20, 0x20, 0x20},
	{ 0x00800000, 0xffff0000, 0xffffffff, 0x22334454, 0x55668011, 0x81223344,},
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0, 0, 0x00093333, 0x018f0100,0x03443302, 0x06040f01, 0xabcd0200, 0x04050000,0x25040001, 0} },
	{ "testTxBeacon, AfterDefine", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x77, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x00080004, 0x6, 0x4, 0x4, 0x20, 0x20, 0x20, 0x20},
	{ 0, 0, 0xffff0000, 0xffffffff, 0x22334454, 0x55668011, 0x81223344,},
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0, 0, 0x00093333, 0x018f0100,0x03443302, 0x06040f01, 0xabcd0200, 0x04050000, 0x25040001, 0} },
	{ "tssTxBeacon DSSS, AfterDefine", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x77, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x400, 0x400, 0x400, 0x400, 0x20, 0x20, 0x20, 0x20},
	{ 0x00000080, 0xffffffff, 0x4454ffff, 0x80112233,0x33445566, 0x00008122, },
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0x00000000, 0x00000000, 0x00093333, 0x018f0100,0x03443302, 0x06040f01, 0xabcd0200, 0x04050000,0x25040001, 0x00000000} },

	{ "data all 0", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x8F, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x00080004, 0x6, 0x4, 0x4, 0x20, 0x20, 0x20, 0x20},
	{ 0,},
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0,} },
	{ "data all 1", BCN,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x8F, 0x40, 0,0, 0x200, 0,0,0, 0x642000, 0x140, 0,0},
	{ 0xBADCAB1E, 0, 0, 0,0xC80F0F, 0x00080004, 0x6, 0x4, 0x4, 0x20, 0x20, 0x20, 0x20},
	{ 0xffffffff,0xffffffff,0xffffffff, 0xffffffff,0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,0xffffffff, 0xffffffff},
	{ 0xCAFEFADE, 0, 0x120, 0x147, 0},
	{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,0xffffffff, 0xffffffff} },

	{ "Data", AC0,
	{ 0xCAFEBABE, 0, 0, 0x100, 0x60, 0x79, 0x32, 0,0, 0x200, 0,0,0, 0x642000, 0x144, 0,0},
	{ 0xBADCAB1E, 0, 0, 0, 0xC80F0F, 0x08081004, 0x08001006, 0x08001004, 0x08001002, 0x20, 0x20, 0x20, 0x20},
	{ 0x05798088, 0x55667788, 0x44548344, 0x80112233,0x33445566, 0x8122, 0x100,},
	{ 0xCAFEFADE, 0, 0x120, 0x133, 0},
	{ 0xE5E60820, 0x8C56009D, 0x02816DF6, 0xD272708B, 0, } },

	{ "RTS CTS Data", AC0,
	{ 0xCAFEBABE, 0, 0, 0x100,0x60, 0x7D, 0x32, 0,0, 0x200, 0, 0, 0,0x642200, 0x144, 0, 0},
	{ 0xBADCAB1E, 0, 0, 0x6000,0xC80F0F, 0x80088004, 0x80088004, 0x80088004,0x80088004, 0x20, 0x20, 0x20, 0x20},
	{ 0x05798088, 0x7C99E564, 0x0302C460, 0x07060504,0x05040302, 0x706, 0x100},
	{ 0xCAFEFADE, 0, 0x120, 0x133, 0 },
	{ 0xE5E60820, 0x8C56009D, 0x02816DF6, 0xD272708B, 0 } },

	{ "Retry Test", AC0,
	{ 0xCAFEBABE, 0, 0, 0x100,0x60, 0x7D, 0x32, 0,0, 0x200, 0, 0, 0,0x642200, 0x144, 0, 0},
	{ 0xBADCAB1E, 0, 0, 0x6000,0xC80F0F, 0x80000003, 0x80000002, 0x80000001,0x80000000, 0x20, 0x20, 0x20, 0x20},
	{ 0x05798088, 0x7C99E564, 0x0302C460, 0x07060504,0x05040302, 0x706, 0x100},
	{ 0xCAFEFADE, 0, 0x120, 0x133, 0 },
	{ 0xE5E60820, 0x8C56009D, 0x02816DF6, 0xD272708B, 0 } },
};

static int ap_mode = 0;
void set_ap_mode(int mode)
{
	volatile UINT mac_ctrl1 = REG_PL_RD(MAC_CONTROL_REG1);

	if (mode) {
		mac_ctrl1 |= 2; /* AP mode */
		ap_mode = 1;
	}
	else {
		mac_ctrl1 &= 0xfffffffd; /* AP mode */
		ap_mode = 0;
	}

	REG_PL_WR(MAC_CONTROL_REG1, mac_ctrl1);
	PRINTF("Set MAC Control to AP mode %d\n", ap_mode);
}

int set_rf_channel(unsigned int freq)
{
	if (freq < 2412 || freq > 2484) {
		PRINTF("RF Channel Frequency range wrong !!!freq = %d\n", freq);
		return 1;
	} else {
		lmac_next_state_setf(HW_IDLE);
		PRINTF("Set RF Channel Frequency %d\n", freq);
		phy_set_channel(PHY_BAND_2G4, PHY_CHNL_BW_20, (uint16_t)freq, (uint16_t)freq, 0, PHY_PRIM);
		lmac_next_state_setf(HW_ACTIVE);
		return 0;
	}
}

#define PHY_CTRL_INFO_CONT_TX                BIT(6)
extern void txl_machdr_format(uint32_t machdrptr);
#define NXMAC_MONOTONIC_COUNTER_2_LO_ADDR   0x60B00120
void send_test_vec(int vec, int trial, UINT rate, UINT power, UINT power2, UINT add_length, UINT cont_tx, UINT rfch)
{
	int i;
	int length;
	int pass = trial;
	UINT head_pointer;
	struct sim_tx_desc *desc;

	if (vec >= TX_DATA_NUM || vec < 0)  {
		for(i = 0;i< TX_DATA_NUM;i++) {
				PRINTF("%d. %s.\n", i, sim[i].str);
		}
		return;
	}

	desc = (struct sim_tx_desc*)pvPortMalloc(sizeof(struct sim_tx_desc));
	memcpy(desc, &sim[vec], sizeof(struct sim_tx_desc));

	if (rfch) {
		set_rf_channel(rfch);
	}

	head_pointer = 0x60B08198+(desc->ac)*4;
	REG_PL_WR(head_pointer, (u32)&(desc->thd[0]) );

	desc->thd[3] = (UINT)(desc->tbd);  /* Transmit Buffer Descriptor */

	length = (int)(desc->thd[5] - desc->thd[4]);
	desc->thd[4] = (UINT)(desc->header);  /* Data Start Pointer 1 */
	desc->thd[5] = desc->thd[4] + (UINT)length;

	desc->thd[9] = (UINT)(desc->pea);  /* Policy Entry Address  */

	if (ldpc_enabled)
				desc->pea[1] |=  1 << 6;   /* PHY Control Info 1 , Indicates whether LDPC encoding is used */

	desc->pea[5] = rate;
	prev_power = set_power2(power | (power2 << 0x08));
	length = (int)(desc->tbd[3] - desc->tbd[2]);
	if (add_length) {  /* add length MIN is 46 */
		if (add_length > 46)
			add_length = add_length - 46;
		else
			add_length = 0;
		length += (int)add_length;
		desc->thd[6] += add_length;
	}
	desc->tbd[2] = (UINT)(desc->data); /* Data Start Pointer 2 */
	desc->tbd[3] = desc->tbd[2] + (UINT)length;

	if ( (desc->ac == BCN) && (!ap_mode))
		set_ap_mode(1);

	// AGCFSMRESET  set for RX off
	modem_agcswreset_setf(1);
	PRINTF("TX Vector : %s.\n", desc->str);

	for(i = 0;i< trial;i++) {
		unsigned int timeout = 6000000;
		volatile int *status = (volatile int*)&desc->thd[15];
		unsigned int txp_power = get_txp_power2();

		desc->pea[9] = txp_power;
		desc->pea[10] = txp_power;
		desc->pea[11] = txp_power;
		desc->pea[12] = txp_power;

		txl_machdr_format((uint32_t)(desc->header)); /* set Sequence Number */
		*status = 0; /* clear status */

		REG_PL_WR(DMA_CONTROL_REG, 1<<(8+(desc->ac)));

		lmac_tx_count[ac2cfm[desc->ac]]++;

		if (cont_tx)
			break;

		while(!(*status)) {
			if (!(timeout--)) {
				PRINTF("Timeout Error!, TXHD Status = 0x%x\n", *status);
				dma_status();
				pass--;
				break;
			}
		}

		if (timeout)
			lmac_tx_cfm_count[ac2cfm[desc->ac]]++;
	}

	if (cont_tx)
		PRINTF(" TX Continuous Mode ON!!! \n");
	else
		PRINTF(" TX Vector %d Passed, %d/%d\n\n", vec, pass,trial);

	vPortFree(desc);

	// AGCFSMRESET  clear for RX on
	modem_agcswreset_setf(0);
	set_power2(prev_power);
}


/* Dumped Beacon Data */
#define BCN_SIZE (246)
unsigned char bcn_data[BCN_SIZE ] = {
/* struct ieee80211_hdr */
	0x80, 0x00,                             /* frame control  Subtype: 8 Beacon */
	0x00, 0x00,                             /* duration id */
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     /* Destination Address */
	0x64, 0xE5, 0x99, 0x7C, 0x5F, 0x8C,     /* Source Address */
	0x64, 0xE5, 0x99, 0x7C, 0x5F, 0x8C,     /* BSS ID */
	0xD0, 0x63,                             /* Sequence Number (variable) 0x63d it resetted at txl_machdr_format() */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    /* time stamp (variable) */

	0xF4, 0x01,   /* Beacon Interval  0x01f4 (500 msec) */
	0x01, 0x0C,   /* Capability Information 0x0c01 */
	0x00, 0x07, 0x53, 0x4F, 0x43, 0x43, 0x44, 0x54, 0x32,  /* SSID "SOCCDT1" */

	/* Supported rates  1Mbps 2Mbps 5.5Mbps 11Mbps 9Mbps 18Mbps 36Mbps 54Mbps */
	0x01, 0x08, 0x82, 0x84, 0x8B, 0x96, 0x12, 0x24, 0x48, 0x6C,

	/* Current Channel: 1 - 2412 */
	0x03, 0x01, 0x01,

	/* Extended Supported Rates */
	0x32, 0x04, 0x0C, 0x18, 0x30, 0x60,

	/* Country Info "KR ", 1st Ch 1, Num Ch 14, Max Trans Power 20 dBm */
	0x07, 0x06, 0x4B, 0x52, 0x20, 0x01, 0x0E, 0x14,

	/*AP CH Report */
	0x33, 0x08, 0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,

	/*AP CH Report */
	0x33, 0x08, 0x21, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* Traffic indication map (TIM) */
	0x05, 0x04, 0x00, 0x01, 0x00, 0x00,

	/* vendor specific */
	0xDD, 0x31, 0x00, 0x50, 0xF2, 0x04, 0x10, 0x4A,
	0x00, 0x01, 0x10, 0x10, 0x44, 0x00, 0x01, 0x02,
	0x10, 0x47, 0x00, 0x10, 0x28, 0x80, 0x28, 0x80,
	0x28, 0x80, 0x18, 0x80, 0xA8, 0x80, 0x64, 0xE5,
	0x99, 0x7C, 0x5F, 0x9C, 0x10, 0x3C, 0x00, 0x01,
	0x01, 0x10, 0x49, 0x00, 0x06, 0x00, 0x37, 0x2A,
	0x00, 0x01, 0x20,

	/* ERP Information */
	0x2A, 0x01, 0x04,

	/* HP Capabilities element */
	0x2D, 0x1A, 0x6C, 0x00, 0x16, 0xFF, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00,
	0x00, 0x00, 0x00, 0x00,

	/* HT Information element */
	0x3D, 0x16, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* Vendor specific */
	0xDD, 0x18, 0x00, 0x50, 0xF2, 0x02, 0x01, 0x01,
	0x80, 0x00, 0x03, 0xA4, 0x00, 0x00, 0x27, 0xA4,
	0x00, 0x00, 0x42, 0x43, 0x5E, 0x00, 0x62, 0x32,
	0x2F, 0x00,

	/* QBSS Load Element */
	0x0B, 0x05, 0x01, 0x00, 0x2A, 0x12, 0x7A,

	/* Vendor specific */
	0xDD, 0x07, 0x00,0x0C, 0x43, 0x04, 0x00, 0x00, 0x00,
};

void print_thd(UINT *thd)
{
	int data_length = 0;
	UINT *tbd;
	volatile UINT *next_mpdu = NULL;
	volatile UINT *next_atomic = NULL;
	int tbd_data_length = 0;
	int argc;
	char *argv2[3];
	char address[10];
	char count[10];
	int next_mpdu_flag = 0;
	int next_atomic_flag = 0;

	if (thd != 0) {
		PRINTF("\nTX Header Descriptor Pointer : 0x%08X\n", thd);
		if (thd[0] != 0xcafebabe) {
			PRINTF("Error TXHD PATTERN\n");
			return;
		}
		if (thd[1] != 0) {
			PRINTF("Next Atomic Frame Ex. SQ. Po.: 0x%08X\n", thd[1]);
			next_atomic = (volatile UINT*)(thd[1]);
			next_atomic_flag = 1;
		}
		if (thd[2] != 0) {
			PRINTF("Next MPDU Descriptor Pointer : 0x%08X\n", thd[2]);
			next_mpdu = (volatile UINT*)(thd[2]);
			next_mpdu_flag = 1;
		}

		if (thd[4] != 0) {
			PRINTF("THD Start & End Data Pointer : 0x%08X 0x%08X\n", thd[4], thd[5]);
			data_length = (int)(thd[5] - thd[4] + 1);
		}

		tbd = (UINT*)(thd[3]);  /* TBD */
		if (tbd) {
			PRINTF("TX Buffer Descriptor Pointer : 0x%08X\n", tbd);
			PRINTF("\tTBD Start & End Data Pointer : 0x%08X 0x%08X\n", tbd[2], tbd[3]);
			PRINTF("\tTBD Buffer Control Info.     : 0x%08X \n", tbd[4]);
			tbd_data_length = (int)(tbd[3] - tbd[2] + 1);
		}

		PRINTF("Frame Length                 : 0x%08X\n", thd[6]);
		if (thd[7] != 0) {
			PRINTF("Frame Lifetime               : 0x%08X\n", thd[7]);
		}
		PRINTF("PHY Control Information      : 0x%08X\n", thd[8]);
		PRINTF("Policy Entry Address         : 0x%08X\n", thd[9]);

		if (thd[9] != 0) {  /* policy Entry */
			UINT *pol = (UINT*)(thd[9]);
			PRINTF("\tP-Table PHY  CONT. INFO.1 2  : 0x%08X 0x%08X\n", pol[1],pol[2]);
			PRINTF("\tP-Table MAC  CONT. INFO.1 2  : 0x%08X 0x%08X\n", pol[3], pol[4]);
			PRINTF("\tP-Table Rate CONT. INFO.1234 : 0x%08X 0x%08X 0x%08X 0x%08X\n", pol[5], pol[6], pol[7], pol[8]);
			PRINTF("\tP-Table PowerCONT. INFO.1234 : 0x%08X 0x%08X 0x%08X 0x%08X\n", pol[9], pol[10], pol[11], pol[12]);
		}

		PRINTF("Optional Frame Length        : 0x%08X 0x%08X 0x%08X\n", thd[10], thd[11], thd[12]);
		PRINTF("MAC Control Information 1 2  : 0x%08X 0x%08X\n", thd[13], thd[14]);
		PRINTF("Status Information           : 0x%08X\r\n", thd[15]);
		PRINTF("Medium Time Used             : 0x%08X\r\n", thd[16]);

		if (data_length != 0) {  /* TXD data */
			argc = 3;
			sprintf(address, "%x", thd[4]);
			sprintf(count, "%x", 32/*data_length*/);
			argv2[1] = address;
			argv2[2] = count;
			cmd_mem_read(argc, argv2);
		}

		if (tbd_data_length != 0) {  /* TBD data */
			argc = 3;
			sprintf(address, "%x", tbd[2]);
			sprintf(count, "%x", 32/*tbd_data_length*/);
			argv2[1] = address;
			argv2[2] = count;
			cmd_mem_read(argc, argv2);
		}
	}

	if (next_mpdu_flag && next_atomic_flag) {
		print_thd((UINT*)next_mpdu);
	} else if (next_mpdu_flag) {
		print_thd((UINT*)next_mpdu);
	} else if (next_atomic_flag) {
		print_thd((UINT*)next_atomic);
	}
}

extern int p_table_rate_control[4];
static void set_pt_rate(int argc, char *argv[])
{
		if (!cmd_lmac_tx_flag) {
			return ;
		}

		if (argc == 3)
		{
			int num = (int)htoi(argv[1])-1;
			if (num < 4)
				p_table_rate_control[num] = (int)htoi(argv[2]);
		}
		PRINTF("P-Table Rate Control Information 1 : 0x%08x\n", p_table_rate_control[0]);
		PRINTF("P-Table Rate Control Information 2 : 0x%08x\n", p_table_rate_control[1]);
		PRINTF("P-Table Rate Control Information 3 : 0x%08x\n", p_table_rate_control[2]);
		PRINTF("P-Table Rate Control Information 4 : 0x%08x\n", p_table_rate_control[3]);
}

static void cmd_lmac_tx_ch(int argc, char *argv[])
{
	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (argc >= 2) {
		bcn_data[57] = (unsigned char)atoi(argv[1]);
	}

	cmd_lmac_rx_ch(argc, argv);
}

/* TX DATA
0 : HT Frame
1 : Normal Data Frame
2 : Null Data Frame
3 : QoS Data Frame
4 : OoS Null Data Frame
5 : HT Frame with RTS
6 : Encrytion Data Frame
7 : Encrytion Data Frame Test Vector WEP40, test_TxWep_seed_1.log
8 : Encrytion Data Frame Test Vector TKIP, testTxTkip_seed_1.log
9 : Encrytion Data Frame Test Vector CCMP, testTxCcmpLeg_seed_1.log

*/
#define NUM_TX_DATA_MODE 11
/* Transmit Header Descriptor*/
#define THD_LENGTH 17
UINT thd_data[NUM_TX_DATA_MODE][THD_LENGTH] = {
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x00000079, 0x00000032, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000144, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x00000077, 0x0000002C, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000104, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000000, 0x00000060, 0x00000077, 0x0000001C, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000124, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x00000079, 0x0000002E, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000144, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000000, 0x00000060, 0x00000079, 0x0000001E, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000164, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x0000007D, 0x00000032, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000144, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x00000079, 0x00000032, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642200, 0x00000144, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x0000007b, 0x00000263, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642000, 0x00000144, 0x00000000, 0 },
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x00000081, 0x0000008a, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642000, 0x00000144, 0x00000000, 0x0},
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x00000087, 0x00000094, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642000, 0x00000144, 0x00000000, 0x0},
	{ 0xCAFEBABE, 0x00000000, 0x00000000, 0x00000100, 0x00000060, 0x0000007b, 0x00000263, 0x00000000, 0x00000000, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00642000, 0x00000144, 0x00000000, 0 }
};

/* Policy Entry Address (0x200) */
UINT pea_data[NUM_TX_DATA_MODE][13] = { 
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00000000, 0x00C80F0F, 0x08081004, 0x08001006, 0x08001004, 0x08001002, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00000000, 0x00C80F0F, 0x00080004, 0x00000006, 0x00000004, 0x00000002, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00000000, 0x00C80F0F, 0x00080004, 0x00000006, 0x00000004, 0x00000002, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00000000, 0x00C80F0F, 0x00080004, 0x00000006, 0x00000004, 0x00000002, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00000000, 0x00C80F0F, 0x00080004, 0x00000006, 0x00000004, 0x00000002, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00006000, 0x00C80F0F, 0x80088004, 0x80088004, 0x80088004, 0x80088004, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00000001, 0x00C80F0F, 0x08081004, 0x08001006, 0x08001004, 0x08001002, 0x20, 0x20, 0x20, 0x20 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00006418, 0x00c80f0f, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x2020, 0x0201, 0x0201, 0x0201 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00006418, 0x00c80f0f, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x2020, 0x0201, 0x0201, 0x0201 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00006418, 0x00c80f0f, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x2020, 0x00000201, 0x00000201, 0x00000201 },
	{ 0xBADCAB1E, 0x00000000, 0x00000000, 0x00006418, 0x00c80f0f, 0x00000002 , 0x00000002 , 0x00000002 , 0x00000002 , 0x2020, 0x0201, 0x0201, 0x0201 }
};

/* Data Start Pointer 1 (0x60 to 0x8F) */
UINT dsp1_data[NUM_TX_DATA_MODE][10] = {
	{ 0x05798088, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00008122, 0x00000100 },
	{ 0x058C0008, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00008122, 0xFFFFFFFF },
	{ 0x050F0048, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00008122, 0xFFFFFFFF },
	{ 0x04790088, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00008122, 0x00000100 },
	{ 0x057900C8, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00008122, 0x00000100 },
	{ 0x05798088, 0x7C99E564, 0x0302C460, 0x07060504, 0x05040302, 0x00000706, 0x00000100 },
	{ 0x00004208, 0x7C99E564, 0x0302C460, 0x07060504, 0x05040302, 0x00000706, 0x00000100 },
	{ 0x05944008, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00218122, 0x00fafaee },
	{ 0x05944088, 0x55667788, 0x44548344, 0x80112233, 0x33445566, 0x00218122, 0xfaee0100, 0xcdef20fa, 0x000089ab },
	{ 0x05944388, 0x55667788, 0x44548344, 0x80112233, 0x55667788, 0x00218344, 0x22334454, 0x01008011, 0x20fafaee, 0x89abcdef },
	{ 0x00004208, 0xFFFFFFFF, 0x4454FFFF, 0x80112233, 0x33445566, 0x00218122, 0x00220000 }
};

/* Transmit Buffer Descriptor (0x100) */
UINT tbd_data[NUM_TX_DATA_MODE][5] = {
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000133, 0x00000000 },
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000133, 0x00000000 },
	{ 0x0,},
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000133, 0x00000000 },
	{ 0x0,},
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000133, 0x00000000 },
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000133, 0x00000000 },
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000183, 0x00000000 },
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000183, 0x00000000 },
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000183, 0x00000000 },
	{ 0xCAFEFADE, 0x00000000, 0x00000120, 0x00000183, 0x00000000 }
};

/* Data Start Pointer 2(0x120 to 0x147) */
UINT dsp2_data[NUM_TX_DATA_MODE][5] = {
	{ 0xE5E60820, 0x8C56009D, 0x02816DF6, 0xD272708B, 0x00000000 },
	{ 0x78D06B3D, 0xB8952F5C, 0x42C14ABA, 0xFEB91718, 0x00000000 },
	{ 0x0,},
	{ 0xBAA3A6B7, 0xEB67D051, 0xAA701CB3, 0x9D0C9C4C, 0x00000000 },
	{ 0x0,},
	{ 0xE5E60820, 0x8C56009D, 0x02816DF6, 0xD272708B, 0x00000000 },
	{ 0xE5E600E9, 0x8C56009D, 0x02816DF6, 0xD272708B, 0x00000000 },
	{ 0x870db6d3, 0x8b59f2c5, 0x241ca4ab, 0xef9b7181, 0xae3a6a7b },
	{ 0x870db6d3, 0x8b59f2c5, 0x241ca4ab, 0xef9b7181, 0xae3a6a7b },
	{ 0x870db6d3, 0x8b59f2c5, 0x241ca4ab, 0xef9b7181, 0xae3a6a7b },
	{ 0x870db6d3, 0x8b59f2c5, 0x241ca4ab, 0xef9b7181, 0xae3a6a7b }
};

UINT enc_data[25] = {
	0x78d06b3d, 0xb8952f5c, 0x42c14aba, 0xfeb91718, 0xeaa3a6b7, 0xeb67d051,
	0xaa701cb3, 0x9d0c9c4c, 0xe5e60820, 0x8c56009d, 0x02816df6, 0xd272708b,
	0x1d02209b, 0x88d27d89, 0xf6d769e0, 0xaf931679, 0x3d480121, 0xdf206da2,
	0x3e792d7d, 0x3187ef87, 0x6e1de388, 0x2b9a5157, 0x6fc34dfa, 0x23126d76,
	0x00000000
};

void set_txdesc_data(int mode, int ac, int rc)
{
	int length = 0;

	UINT *thd_address;
	REG_PL_WR(0x60B0819C + ac*4, (u32)&thd_data[mode][0]);
	length = (int)(thd_data[mode][5] - thd_data[mode][4]);

	thd_address = (UINT*)(*((volatile UINT *)(0x60B0819C + ac*4)));
	thd_address[15] = 0;

	if (thd_address[3] != 0)
		thd_address[3] = (UINT)&tbd_data[mode][0];  /* Transmit Buffer Descriptor */

	thd_address[4] = (UINT)&dsp1_data[mode][0];  /* Data Start Pointer 1 */
	thd_address[5] = thd_address[4] + (UINT)length;

	if (rc != -1) {
		pea_data[mode][5] = (UINT)rc;
		pea_data[mode][6] = (UINT)rc;
		pea_data[mode][7] = (UINT)rc;
		pea_data[mode][8] = (UINT)rc;
	}

	thd_address[9] = (UINT)&pea_data[mode][0];  /* Policy Entry Address  */

	if (thd_address[3] != 0) {
		tbd_data[mode][2] = (UINT)&dsp2_data[mode][0]; /* Data Start Pointer 2 */
		tbd_data[mode][3] = (UINT)&dsp2_data[mode][5] - 1;
	}

	if (mode > 6) {
		tbd_data[mode][2] = (UINT)&enc_data[0]; /* Data Start Pointer 2 */
		tbd_data[mode][3] = (UINT)&enc_data[25] - 1;
	}

	if (mode == 7 || mode == 10)
		tbd_data[mode][3] = tbd_data[mode][2] + thd_address[6] - (UINT)length;
}

char Mode[3] = {
	'b',
	'a',
	'g',
};
static void cmd_lmac_data_start(int argc, char *argv[])
{
	int abgnMode = 2;
	volatile UINT mac_ctrl1 = REG_PL_RD(MAC_CONTROL_REG1);

	if (argc == 2) {
		abgnMode = (int)htoi(argv[1]);
	}
	PRINTF("Set 802.11%c mode.\n", Mode[abgnMode]);

	mac_ctrl1 |= 2; /* AP mode */
	mac_ctrl1 &= 0xfffe3fff;
	mac_ctrl1 |= (UINT)(abgnMode<<14);

#ifdef BUILD_OPT_VSIM
	FPGA_DBG_TIGGER(MODE_LMAC_STEP(0x012) | MODE_DBG_RUN);   /* 0x8000 0120 */
#endif

	REG_PL_WR(MAC_CONTROL_REG1, mac_ctrl1);
	REG_PL_WR(DMA_CONTROL_REG, 0x200);
}

static void cmd_lmac_data_vector(int argc, char *argv[])
{
	int mode = -1;
	int start = -1;
	if (argc==2) {
		mode = atoi(argv[1]);
	} else if (argc==3) {
		mode = atoi(argv[1]);
		start = atoi(argv[2]);
	}

	if (mode >= 0 && mode < NUM_TX_DATA_MODE) {
		PRINTF("Use Simulation DATA TX Header Descriptor %d ! \n", mode);
		set_txdesc_data(mode, 1, -1);
	}
	if (start != -1) {
		int argc2 = 2;
		char *argv2[2];
		argv2[1] = argv[2];
		cmd_lmac_data_start(argc2, argv2);
	}
}

extern int data_test;
void cmd_lmac_data(int argc, char *argv[])
{
	int loop = 0;
	int i;
	data_test = 1;

	if (argc==2) {
		cmd_lmac_data_vector(argc,argv);
	} else if (argc==3) {
		cmd_lmac_data_vector(argc,argv);
	} else if (argc==4) {
		argc = 3;
		loop = atoi(argv[3]);
		for(i=0;i<loop;i++) {
			cmd_lmac_data_vector(argc,argv);
			OAL_MSLEEP(500);
		}
	}
	data_test = 0;
}


#define NXMAC_MAC_CNTRL_2_ADDR   0x60B08050
#define NXMAC_STATE_CNTRL_ADDR   0x60B00038
#define NXMAC_MAC_ERR_REC_CNTRL_ADDR   0x60B00054

#define MAC_ERR_REC_CNTRL_HW_FSM_RESET 0x4
#define MAC_ERR_REC_CNTRL_RX_FIFO_RESET 0x8
#define MAC_ERR_REC_CNTRL_TX_FIFO_RESET 0x10
#define MAC_ERR_REC_CNTRL_MAC_PHY_FIFO_RESET 0x20
#define MAC_ERR_REC_CNTRL_ENCR_RX_FIFO_RESET 0x40
#define MAC_ERR_REC_CNTRL_BA_PS_BITMAP_RESET 0x80

void cmd_lmac_errror_recovery(int argc, char *argv[])
{
		DA16X_UNUSED_ARG(argc);
		DA16X_UNUSED_ARG(argv);

		REG_PL_WR(NXMAC_STATE_CNTRL_ADDR, (uint32_t)HW_IDLE << 4);

		/* hw finite state mache reset (hwFSMReset) */
		REG_PL_WR(NXMAC_MAC_ERR_REC_CNTRL_ADDR, MAC_ERR_REC_CNTRL_HW_FSM_RESET);

		while (((REG_PL_RD(NXMAC_STATE_CNTRL_ADDR) & ((uint32_t)0x0000000F)) >> 0) != HW_IDLE);

		/* reset the Transmit and Receive FIFOs */
		REG_PL_WR(NXMAC_MAC_ERR_REC_CNTRL_ADDR, MAC_ERR_REC_CNTRL_TX_FIFO_RESET);
		REG_PL_WR(NXMAC_MAC_ERR_REC_CNTRL_ADDR, MAC_ERR_REC_CNTRL_RX_FIFO_RESET);

		/* reset MAC-PHY Interface FIFO */
		REG_PL_WR(NXMAC_MAC_ERR_REC_CNTRL_ADDR, MAC_ERR_REC_CNTRL_RX_FIFO_RESET);

		/* reset Encryption Receive FIFO */
		REG_PL_WR(NXMAC_MAC_ERR_REC_CNTRL_ADDR, MAC_ERR_REC_CNTRL_ENCR_RX_FIFO_RESET);

		/* reset Partial State Bitmap */
		REG_PL_WR(NXMAC_MAC_ERR_REC_CNTRL_ADDR, MAC_ERR_REC_CNTRL_BA_PS_BITMAP_RESET);

		REG_PL_WR(NXMAC_STATE_CNTRL_ADDR, (uint32_t)HW_ACTIVE << 4);
}


void cmd_lmac_bcn_test_ssid(int argc, char *argv[])
{
	char SSID[8] = {0,};

	char *ssid_p;
	volatile UINT *p;

	p = (volatile UINT*)(*((volatile UINT *)(0x60B08198)));
	if (p != 0) {
		p = (volatile UINT*)(p[3]);
		p = (volatile UINT*)(p[2]);
		ssid_p = (char*)p;
		if (argc == 2) {
				memcpy(SSID, argv[1], 7);
				memcpy(&ssid_p[38], SSID, 7); /* SSID */
				memcpy(&ssid_p[13], SSID, 3); /* source MAC addr */
				memcpy(&ssid_p[19], SSID, 3); /* BSS MAC addr */
		} else {
				memcpy(SSID, &ssid_p[38], 7);
		}
	}
	else {
		if (argc == 2) {
				memcpy(SSID, argv[1], 7);
				memcpy(&bcn_data[38], SSID, 7);
				memcpy(&bcn_data[13], SSID, 3); /* source MAC addr */
				memcpy(&bcn_data[19], SSID, 3); /* BSS MAC addr */

		} else
				memcpy(SSID, &bcn_data[38], 7);
	}

	PRINTF("Set Test Beacon SSID %s\n", SSID );
}

//-----------------------------------------------------------------------
//  Fragmented MSDU & MPDU Test
//-----------------------------------------------------------------------
#define MPDU_NUM 4
UINT mpdu_pt [MPDU_NUM][13] = {
	{ 0xbadcab1e,0x00000000,0x00000000,0x00006418,0x00c80f0f,0x00081004,0x00081007,0x00081007,0x00081007,0x00000030,0x00000030,0x00000030,0x00000030},
	{ 0xbadcab1e,0x00000000,0x00000000,0x00006418,0x00c80f0f,0x00081004,0x00081007,0x00081007,0x00081007,0x00000000,0x00000000,0x00000000,0x00000000},
	{ 0xbadcab1e,0x00000000,0x00000000,0x00006418,0x00c80f0f,0x00081004,0x00081007,0x00081007,0x00081007,0x00000000,0x00000000,0x00000000,0x00000000},
	{ 0xbadcab1e,0x00000000,0x00000000,0x00006418,0x00c80f0f,0x00080007,0x00080006,0x00080006,0x00080006,0x00000030,0x00000030,0x00000030,0x00000030}
};

UINT ampdu_txhd [1][17] = {
	{0xcafebabe,0x00000000,0x00000100,0x00000000,0x00000000,0x00000000,0x000002d0,0x00000000,0x00000000,0x00002000,0x0,0x0,0x0,0x00002600,0x00200100,0x0,0x0}
};

UINT mpdu_txhd [MPDU_NUM][17] = {
	{0xcafebabe,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000080,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x00002600,0x00680100,0x0,0x0},
	{0xcafebabe,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x000000f0,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x00002600,0x00700100,0x0,0x0},
	{0xcafebabe,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000154,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x00002600,0x00780100,0x0,0x0},
	{0xcafebabe,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000018,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x00000600,0x00000143,0x0,0x0} };

UINT mpdu_data1[MPDU_NUM][8] = {
	{0x05b00088, 0x7C99E564,0x0302C460,0x07060504,0x33445566,0x00008122, 0xfaee0000, 0xcd0f20fa},
	{0x05320088, 0x7C99E564,0x0302C460,0x07060504,0x33445566,0x00008122, 0xfaee0000, 0xcd0f20fa},
	{0x04b50088, 0x7C99E564,0x0302C460,0x07060504,0x33445566,0x00008122, 0xfaee0000, 0xcd0f20fa},
	{0x00000084, 0x7C99E564,0x0302C460,0x07060504,} };


#define AC0_HEAD_POINTER 0x60B0819c
#define TX_AC0_NEW_HEAD 0x200
#define AC1_HEAD_POINTER 0x60B081a0
#define TX_AC1_NEW_HEAD 0x400
#define MAX_MPDU_NUM 64

UINT *ampdu_status[MAX_MPDU_NUM];


void make_ampdu_txhd(UINT *thd_int[64], int ampdu_cnt, int data_size)
{
	int i;
	int length = 0;
	int index = 0;

	REG_PL_WR(AC0_HEAD_POINTER, (u32)&ampdu_txhd[0][0] );

	for(i=0;i<ampdu_cnt;i++)
	{
		thd_int[i] = (UINT*)pvPortMalloc(sizeof(UINT)*17);
		memcpy(thd_int[i], mpdu_txhd[1], sizeof(UINT)*17);
	}

	/* AMPUD EXTRA */
	ampdu_txhd[0][15] = 0;
	ampdu_txhd[0][2] = (UINT)&mpdu_txhd[0][0]; /* Next Link Address */
	ampdu_txhd[0][9] = (UINT)&mpdu_pt[0][0];  /* Policy Entry Address  */
	ampdu_status[index++] = &ampdu_txhd[0][15];

	/* First AMPDU */
	mpdu_txhd[0][15] = 0;
	ampdu_status[index++] = &mpdu_txhd[0][15];
	mpdu_txhd[0][2] =  (UINT)&thd_int[0][0];  /* Next Link Address */
	mpdu_txhd[0][4] = (UINT)&mpdu_data1[0][0]; /* Data Start Pointer 1 */
	mpdu_txhd[0][5] = mpdu_txhd[0][4] + (UINT)data_size-1;
	mpdu_txhd[0][6] = (UINT)data_size;
	length += data_size+4;
	/* Inter AMPDU */
	for(i=0;i<ampdu_cnt;i++)
	{
		thd_int[i][15] = 0;
		ampdu_status[index++] = &thd_int[i][15];
		thd_int[i][2] =  (UINT)&thd_int[i+1][0];  /* Next Link Address */
		thd_int[i][4] =  (UINT)&mpdu_data1[1][0]; /* Data Start Pointer 1 */
		thd_int[i][5] = thd_int[i][4] + (UINT)data_size-1;
		thd_int[i][6] = (UINT)data_size;
		length += data_size+4;
	}
	thd_int[i-1][2] = (UINT)&mpdu_txhd[2][0];
	/* Last AMPDU */
	mpdu_txhd[2][15] = 0;
	ampdu_status[index++] = &mpdu_txhd[2][15];
	mpdu_txhd[2][2] =  (UINT)&mpdu_txhd[3][0];  /* Next Link Address */
	mpdu_txhd[2][4] = (UINT)&mpdu_data1[2][0]; /* Data Start Pointer 1 */
	mpdu_txhd[2][5] = mpdu_txhd[2][4] + (UINT)data_size-1;
	mpdu_txhd[2][6] = (UINT)data_size;
	length += data_size+4;

	/* BAR Request */
	mpdu_txhd[3][15] = 0;
	ampdu_status[index++] = &mpdu_txhd[3][15];
	mpdu_txhd[3][4] = (UINT)&mpdu_data1[3][0]; /* Data Start Pointer 1 */
	mpdu_txhd[3][5] = mpdu_txhd[3][4] + mpdu_txhd[3][6] - 1;
	mpdu_txhd[3][9] = (UINT)&mpdu_pt[3][0];  /* Policy Entry Address  */

	/* update total length */
	ampdu_txhd[0][6] = (UINT)length;
}

void cmd_lmac_test_vec(int argc, char *argv[])
{
	int vec = -1;
	int trial = 1;
	UINT rate = 0;
	UINT power = 0;
	UINT power2 = 0;
	UINT len = 0;
	UINT cont_tx = 0;
	UINT ch = 0;

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (argc==1)
		PRINTF("USAGE: vec [vector] [trial] [rate] [power] [len] [cont_tx] [ch]\n");
	if (argc==2)
		vec = atoi(argv[1]);
	if (argc==3) {
		vec = atoi(argv[1]);
		trial = atoi(argv[2]);
	}
	if (argc==4) {
		vec = atoi(argv[1]);
		trial = atoi(argv[2]);
		rate = htoi(argv[3]);
	}
	if (argc==5) {
		vec = atoi(argv[1]);
		trial = atoi(argv[2]);
		rate = htoi(argv[3]);
		power = (UINT)atoi(argv[4]);
	power2 = power;
	}
	if (argc==6) {
		vec = atoi(argv[1]);
		trial = atoi(argv[2]);
		rate = htoi(argv[3]);
		power = (UINT)atoi(argv[4]);
		len = (UINT)atoi(argv[5]);
	power2 = power;
	}

	if (argc==7) {
		vec = atoi(argv[1]);
		trial = atoi(argv[2]);
		rate = htoi(argv[3]);
		power = (UINT)atoi(argv[4]);
		len = (UINT)atoi(argv[5]);
		cont_tx = (UINT)atoi(argv[6]);
	power2 = power;
		if (cont_tx == 0) {
			hal_machw_reset();
			rxl_reset();
			txl_reset();
			lmac_next_state_setf(HW_ACTIVE);
			PRINTF(" TX Continuous Mode OFF!!! \n");
			return;
		}
	}

	if (argc==8) {
		vec = atoi(argv[1]);
		trial = atoi(argv[2]);
		rate = htoi(argv[3]);
		power = (UINT)atoi(argv[4]);
		len = (UINT)atoi(argv[5]);
		cont_tx = (UINT)atoi(argv[6]);
		ch = (UINT)atoi(argv[7]);
	power2 = power;
		if (cont_tx == 0) {
			hal_machw_reset();
			rxl_reset();
			txl_reset();
			lmac_next_state_setf(HW_ACTIVE);
			PRINTF(" TX Continuous Mode OFF!!! \n");
			return;
		}
	}

	send_test_vec(vec, trial, rate, power, power2, len, cont_tx, ch);
}

#define SYS_TICK_SCALE 100

struct m_time{
	unsigned int tick;
	unsigned int ptime;
};

void measure_time(struct m_time *t, char *str)
{
	unsigned int time, ctime, curclock;
	unsigned int tick = xTaskGetTickCount();

	if (t->tick != 0) {
		_sys_tick_ioctl(TICK_CMD_GETUSEC,&ctime);
		_sys_clock_read( (unsigned int *)&curclock, sizeof(unsigned int));

		if ( t->ptime > ctime ){
			time = t->ptime - ctime ;
		} else {
			time = t->ptime + ((curclock/SYS_TICK_SCALE) - ctime) ;
			tick -= 1;
		}

		PRINTF("%s : tick count = %d, clock %d, %d us \n", str, tick - t->tick, time, time/(curclock/1000000));
	}

	t->tick = tick;
	_sys_tick_ioctl(TICK_CMD_GETUSEC,&ctime);
	t->ptime = ctime;
}

static void lmac_phy_reset()
{
	// phy reset
	REG_PL_WR(0x50001008, 0xf);
	REG_PL_WR(0x50001008, 0xffff);
}

/// Maximum number of words in the configuration buffer
#define PHY_CFG_BUF_SIZE     16
/// Structure containing the parameters of the PHY configuration
struct phy_cfg_tag
{
		/// Buffer containing the parameters specific for the PHY used
		uint32_t parameters[PHY_CFG_BUF_SIZE];
};

struct lmac_dpm_param {
	struct phy_cfg_tag phy_cfg;
	unsigned int channel;
};

int tx_dpm_test = 0;
static void cmd_lmac_dpm_mib(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	struct m_time ptime;
	struct lmac_dpm_param param = {
		{ { 2, 0x63, 0x63, 0, 2412, } },
		2412
	};

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	tx_dpm_test = 1;

	lmac_phy_reset();

	_sys_tick_ioctl(TICK_CMD_GETUSEC,&ptime);

	rwnx_agc_load();

	measure_time(&ptime, "agc");
	intc_init();
	measure_time(&ptime, "intc_init");
	ipc_emb_init();
	measure_time(&ptime, "ipc_emb_init");

	hal_machw_init();

	// initialize the header descriptors
	// rxl_hwdesc_init_dpm();

	// initialize the LMAC SW descriptors and the indication pool.
	//  rx_swdesc_init();

	// initialize the RX environment
	//  rxl_cntrl_init();
	measure_time(&ptime, "mm_init");

	// mm_start_req_handler
	phy_init(&param.phy_cfg);
	measure_time(&ptime, "phy_init");
	phy_set_channel(PHY_BAND_2G4, PHY_CHNL_BW_20, (uint16_t)(param.channel), (uint16_t)(param.channel), 0, PHY_PRIM);
	measure_time(&ptime, "phy_set_channel");
	lmac_next_state_setf(HW_ACTIVE);
	measure_time(&ptime, "lmac_next_state_setf");
 }

struct txp_s{
	u8 rfch;
	u32 rate;
	u32 power;
	u32 power2;
	u32 len;
	u32 bw;
};
int txp_on_off = 0;
TaskHandle_t test_thread_type = NULL;
extern SemaphoreHandle_t txp_lock;
static int txp_cont_mode = 0;
extern void	txp_cmd_trigger(void);
void txp_thread(void* thread_input)
{
	int length;
	UINT head_pointer;
	struct sim_tx_desc *desc;
	struct txp_s *txp_param = (struct txp_s*)thread_input;
	struct mac_hdr *mac_header;

	u8 vec = 7;
	u8 rfch = txp_param->rfch;
	u32 rate = txp_param->rate;
	u32 power = txp_param->power;
	u32 power2 = txp_param->power2;
	u32 add_length = txp_param->len;
	u32 bw = txp_param->bw;

	u16 seqnbr = 0;


	desc = pvPortMalloc(sizeof(struct sim_tx_desc));
	memcpy(desc, &sim[vec], sizeof(struct sim_tx_desc));
	PRINTF("desc = 0x%x\n", desc);

	head_pointer = 0x60B08198+(desc->ac)*4;
	REG_PL_WR(head_pointer, (u32)&(desc->thd[0]) );

	desc->thd[3] = (UINT)(desc->tbd);  /* Transmit Buffer Descriptor */

	length = (int)(desc->thd[5] - desc->thd[4]);
	desc->thd[4] = (UINT)(desc->header);  /* Data Start Pointer 1 */
	desc->thd[5] = desc->thd[4] + (UINT)length;

	mac_header = (struct mac_hdr *)desc->header;

	desc->thd[9] = (UINT)(desc->pea);  /* Policy Entry Address  */

	if (ldpc_enabled) {
		desc->pea[1] |=  1 << 6;   /* PHY Control Info 1 , Indicates whether LDPC encoding is used */
	}

	if (rate) {
		desc->pea[5] = rate;
	}
	if (bw) {
		desc->pea[5] = rate | ((bw & 0x3) << 7);
	}

	length = (int)(desc->tbd[3] - desc->tbd[2]);
	if (add_length) {
		length += (int)add_length;
		desc->thd[6] += add_length;
	}
	desc->tbd[2] = (UINT)(desc->data); /* Data Start Pointer 2 */
	desc->tbd[3] = desc->tbd[2] + (UINT)length;

	_sys_nvic_write(phy_intTxTrigger_n, (void *)txp_cmd_trigger, 0x7);

	// EDCA AC0 setting
	REG_PL_WR(0x60B00200, 0x101);

	// AGCFSMRESET  set for RX off
	modem_agcswreset_setf(1);

	if (rfch) {
		int freq;

		if (rfch == 14) {
			freq = 2484;
		} else {
			freq = 2412 + 5 * (rfch - 1 );
		}

		set_rf_channel((unsigned int)freq);
	}
	prev_power = set_power2(power | (power2 << 0x08));

	do {
		volatile int *status = (volatile int*)&desc->thd[15];
		unsigned int txp_power = get_txp_power2();

		desc->pea[9] = txp_power;
		desc->pea[10] = txp_power;
		desc->pea[11] = txp_power;
		desc->pea[12] = txp_power;

		*status = 0; /* clear status */

		mac_header->seq = (uint16_t)(seqnbr++ << MAC_SEQCTRL_NUM_OFT);
		lmac_tx_count[ac2cfm[desc->ac]]++;
		REG_PL_WR(DMA_CONTROL_REG, 1<<(8+(desc->ac)));

		if ( xSemaphoreTake(txp_lock, 100) != pdTRUE ) {
				  PRINTF("TX time out \n");
		}

	} while (txp_on_off);

	vSemaphoreDelete(txp_lock);
	vPortFree(desc);
	// AGCFSMRESET  clear for RX on
	modem_agcswreset_setf(0);
	set_power2(prev_power);
	intc_init(); /* restore RWNX interrupt */
	if (test_thread_type) {
		TaskHandle_t l_task = NULL;
		l_task = test_thread_type;
		test_thread_type = NULL;
		vTaskDelete(l_task);
	}
}


struct sim_tx_desc *desc = NULL;
extern void tx_cont_stop();
static int reg_agcfsmreset = 0;
void run_cont_mode(u32 cont_tx_mode, struct txp_s *txp_param)
{
	int length;
	UINT head_pointer;
	unsigned int txp_power;

	u8 vec = 7;
	u8 rfch = txp_param->rfch;
	u32 rate = txp_param->rate;
	u32 power = txp_param->power;
	u32 power2 = txp_param->power2;
	u32 add_length = txp_param->len;
	u32 bw = txp_param->bw;

	if (cont_tx_mode) {
		PRINTF("Start TX Continuous Mode !!!\n");
		desc = pvPortMalloc(sizeof(struct sim_tx_desc));
		memcpy(desc, &sim[vec], sizeof(struct sim_tx_desc));
	} else {
		if (desc) {
			vPortFree(desc);
			desc =  NULL;
		}

		// AGCFSMRESET  clear for RX on
		if (reg_agcfsmreset)
				modem_agcswreset_setf(0);
		tx_cont_stop();
		return;
	}

	head_pointer = 0x60B08198+(desc->ac)*4;
	REG_PL_WR(head_pointer, (u32)&(desc->thd[0]) );

	desc->thd[3] = (UINT)(desc->tbd);  /* Transmit Buffer Descriptor */

	length = (int)(desc->thd[5] - desc->thd[4]);
	desc->thd[4] = (UINT)(desc->header);  /* Data Start Pointer 1 */
	desc->thd[5] = desc->thd[4] + (UINT)length;

	desc->thd[9] = (UINT)(desc->pea);  /* Policy Entry Address  */

	if (ldpc_enabled)
	  desc->pea[1] |=  1 << 6;   /* PHY Control Info 1 , Indicates whether LDPC encoding is used */

	if (rate) {
		desc->pea[5] = rate;
	}

	if (bw) {
		desc->pea[5] = rate | ((bw & 0x3) << 7);
	}

	length = (int)(desc->tbd[3] - desc->tbd[2]);
	if (add_length) {
		length += (int)add_length;
		desc->thd[6] += add_length;
	}
	desc->tbd[2] = (UINT)(desc->data); /* Data Start Pointer 2 */
	desc->tbd[3] = desc->tbd[2] + (UINT)length;

	desc->thd[8] = PHY_CTRL_INFO_CONT_TX;

	// EDCA AC0 setting
	REG_PL_WR(0x60B00200, 0x101);

	// AGCFSMRESET  set for RX off
	reg_agcfsmreset = modem_agcswreset_getf();
	modem_agcswreset_setf(1);

	if (rfch) {
		int freq;
		if (rfch == 14)
			freq = 2484;
		else
			freq = 2412 + 5 * (rfch - 1 );

		set_rf_channel((unsigned int)freq);
	}

	prev_power = set_power2(power | (power2 << 0x08));
	txp_power = get_txp_power2();
	desc->pea[9] = txp_power;
	desc->pea[10] = txp_power;
	desc->pea[11] = txp_power;
	desc->pea[12] = txp_power;
	REG_PL_WR(DMA_CONTROL_REG, 1<<(8+(desc->ac)));

}

static int txp_command(int argc, char *argv[], struct txp_s *param)
{
	int mode = 0;

	if (argc==1) {
		PRINTF("USAGE: %s [cont_on(2)|on(1)|off(0)] [RF channel] [rate] [power] [data_len] [bw]\n", argv[0]);
		return -1;
	}
	if (argc==2) {
		mode = atoi(argv[1]);
	}
	if (argc==3) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
	}
	if (argc==4) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
		param->rate = htoi(argv[3]);
	}
	if (argc==5) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
		param->rate = htoi(argv[3]);
		param->power = htoi(argv[4]);
		param->power2 = param->power;
	}
	if (argc==6) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
		param->rate = htoi(argv[3]);
		param->power = htoi(argv[4]);
		param->len = htoi(argv[5]);
		param->power2 = param->power;
	}
	if (argc==7) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
		param->rate = htoi(argv[3]);
		param->power = htoi(argv[4]);
		param->len = htoi(argv[5]);
		param->bw = htoi(argv[6]);
		param->power2 = param->power;
	}

	return mode;
}

static int txpwr_command(int argc, char *argv[], struct txp_s *param)
{
	int mode = 0;

	if (argc==1) {
		PRINTF("USAGE: %s [on(1)|off(0)] [RF channel] [rate] [data_len]\n", argv[0]);
		return -1;
	}
	if (argc==2) {
		mode = atoi(argv[1]);
	}
	if (argc==3) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
	}
	if (argc==4) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
		param->rate = htoi(argv[3]);
	}
	if (argc==5) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
		param->rate = htoi(argv[3]);
		param->len = htoi(argv[4]);
	}
	param->power = 3; /* fixed power */
	param->power2 = 3; /* fixed power */

	return mode;
}

static int txp_fix_rate_command(int argc, char *argv[], struct txp_s *param)
{
	int mode = 0;
	if (argc==1) {
		PRINTF("USAGE: %s [on(1)|off(0)] [RF channel]\n", argv[0]);
		return -1;
	}
	if (argc==2) {
		mode = atoi(argv[1]);
	}
	if (argc==3) {
		mode = atoi(argv[1]);
		param->rfch = (u8)atoi(argv[2]);
	}
	param->power = 3; /* fixed power */
	param->power2 = 3; /* fixed power */
	param->len = 0x1f4;

	if (!strcmp(argv[0], "txp1m")) {
		param->rate = 0x400;      /* fixed 11b 1Mbps */
		param->len = 0x32;
	}  else if (!strcmp(argv[0], "txp6m")) {
		param->rate = 0x404;      /* fixed 11g 6Mbps */
	}  else if (!strcmp(argv[0], "txpm0")) {
		param->rate = 0x1000;      /* fixed 11n MCS0 */
	}
	return mode;
}

void cmd_lmac_txp(int argc, char *argv[])
{
	struct txp_s param = {1, 0x1007, 1, 1, 1024, 0};
	int mode = 0;

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (!check_lmac_init()) {
		PRINTF("Device not initialized. \n");
		return;
	}

	if (!strcmp(argv[0],"txp")) {
		mode = txp_command(argc, argv, &param);
	} else if (!strcmp(argv[0], "txpwr")) {
		mode = txpwr_command(argc, argv, &param);
	} else {
		mode = txp_fix_rate_command(argc, argv, &param);
	}

	if (mode == -1)
		return;

	if (mode == txp_on_off) {
		if (mode == 0) {
			PRINTF("Already TX off !!!\n");
		} else {
			PRINTF("Already TX on !!!\n");
		}

		return;
	} else {
		txp_on_off = mode;
	}

	if (txp_on_off == 2) {
		if (txp_cont_mode == 0) {
			txp_cont_mode = 1;

			REG_PL_WR(0x60c0c02c, 0x303);
			//REG_PL_WR(0x60c015ac, 0x0);
			//REG_PL_WR(0x60C0F07C,0x3);
			REG_PL_WR(0x50001600, 0x2); // FADC Suspend Mode
			run_cont_mode((u32)txp_cont_mode, &param);
		} else {
			PRINTF("Already Coninous TX mode !!!\n");
		}
		return;
	}

	if (txp_on_off == 1) {
		 PRINTF("TX on.\n");
		 REG_PL_WR(0x60c0c02c, 0x303);
		 //REG_PL_WR(0x60c015ac, 0x0);
		 //REG_PL_WR(0x60C0F07C,0x3);
		 REG_PL_WR(0x50001600, 0x2); // FADC Suspend Mode
		 txp_lock = xSemaphoreCreateBinary();
		 xTaskCreate(txp_thread, "TestThread", 2048, ( void * )(&param), OS_TASK_PRIORITY_SYSTEM+9, &test_thread_type );
		 return;
	} else if (txp_on_off == 0) {
		REG_PL_WR(0x60c0c02c, 0x0);
		REG_PL_WR(0x50001600, 0x0); // FADC Normal Mode
		if (txp_cont_mode) {
			txp_cont_mode = 0;
			run_cont_mode((u32)txp_cont_mode, &param);
			return;
		}
		PRINTF("TX off.\n");


	}

}

struct RFTX {
		u32 Ch;
		u32 numFrames;
		u32 frameLen;
		u32 txRate;
		u32 txPower;
		u32 txPower2;
		struct mac_addr destAddr;
		struct mac_addr bssid;
		u8 htEnable;
		u8 GI;
		u8 greenField;
		u8 preambleType;
		u8 qosEnable;
		u8 ackPolicy;
		u8 scrambler;
		u8 aifsnval;
		u8 ant;
		u8 BW;
};

static uint16_t co_bswap16(uint16_t val16)
{
		return (uint16_t)(((val16<<8)&0xFF00) | ((val16>>8)&0xFF));
}

static int str2macaddr(struct mac_addr *mac, char *macaddr)
{
		int status = 0;
		UINT len;
		char tmp_macstr[13];
		char *pTemp;
		memset(tmp_macstr, 0, 13);
		len = strlen(macaddr);

		if (len == 17 || len == 12) {
	if (len == 12) {
				    memcpy(tmp_macstr, macaddr, 12);
	} else {
				    sprintf(tmp_macstr, "%c%c%c%c%c%c%c%c%c%c%c%c",
				macaddr[0], macaddr[1],
				macaddr[3], macaddr[4],
				macaddr[6], macaddr[7],
				macaddr[9], macaddr[10],
				macaddr[12], macaddr[13],
				macaddr[15], macaddr[16]);
	}
				mac->array[2] = co_bswap16((uint16_t)strtol(&tmp_macstr[8], &pTemp, 16));
				tmp_macstr[8] = '\0';
				mac->array[1] = co_bswap16((uint16_t)strtol(&tmp_macstr[4], &pTemp, 16));
				tmp_macstr[4] = '\0';
				mac->array[0] = co_bswap16((uint16_t)strtol(&tmp_macstr[0], &pTemp, 16));
		} else {
				PRINTF("Wrong MAC Address Type !!!\n");
				status = 1;
		}

		return status;
}

static u32 edca_ac0(u8 aifsnval)
{
	u32 pre_value;
	u32 edcaAC0Reg = 0;
	u8 cwMin0 = 0;
	u8 cwMax0 = 1;
	edcaAC0Reg = (u32)(cwMax0<<8 | cwMin0 <<4 | aifsnval);
	pre_value = REG_PL_RD(0x60B00200);
	REG_PL_WR(0x60B00200, edcaAC0Reg);
	
	return pre_value;
}

extern int getMacAddrMswLsw(UINT iface, ULONG *macmsw, ULONG *maclsw);
extern void set_mac_header_fctl(struct mac_hdr *mac_header, u8 qosEnable);
void rftx_thread(void* thread_input)
{
	u32 length1, length2=0;
	UINT head_pointer;
	struct sim_tx_desc *tx_desc;
	struct RFTX *rftx_param = (struct RFTX*)thread_input;
	struct mac_hdr *mac_header;
	struct mac_addr dev_mac_addr;
	u8 vec = 7;
	u32 rfch = rftx_param->Ch;
	u32 num_frames = rftx_param->numFrames;
	u32 txRate = rftx_param->txRate;
	u32 power = rftx_param->txPower;
	u32 power2 = rftx_param->txPower2;
	u32 add_length = rftx_param->frameLen;
	u8 qosEnable = rftx_param->qosEnable;
	u8 ackPolicy = rftx_param->ackPolicy;
	u8 aifsnval = rftx_param->aifsnval;
	ULONG mac_msw;
	ULONG mac_lsw;

	u16 seqnbr = 0;

	u32 pre_edca;

	long state;

    getMacAddrMswLsw(0, &mac_msw, &mac_lsw);
    dev_mac_addr.array[0] = co_bswap16(mac_msw);
    dev_mac_addr.array[1] = co_bswap16((mac_lsw>>16) & 0xffff);
    dev_mac_addr.array[2] = co_bswap16(mac_lsw & 0xffff);

	tx_desc = pvPortMalloc(sizeof(struct sim_tx_desc));
	memcpy(tx_desc, &sim[vec], sizeof(struct sim_tx_desc));

	head_pointer = 0x60B08198+(tx_desc->ac)*4;
	REG_PL_WR(head_pointer, (u32)&(tx_desc->thd[0]) );

	tx_desc->thd[3] = (UINT)(tx_desc->tbd);  /* Transmit Buffer Descriptor */

	length1 = tx_desc->thd[5] - tx_desc->thd[4];
	tx_desc->thd[4] = (UINT)(tx_desc->header);  /* Data Start Pointer 1 */
	tx_desc->thd[5] = tx_desc->thd[4] + length1;

	mac_header = (struct mac_hdr *)tx_desc->header;
	set_mac_header_fctl(mac_header, qosEnable);
	
	mac_header->durid = 0;
	mac_header->addr1 = rftx_param->destAddr; // dest
	mac_header->addr2 = dev_mac_addr;  // src
	mac_header->addr3 = rftx_param->bssid;  // bssid


	tx_desc->thd[13] |=  (UINT)((ackPolicy&0x3)<<9);  /* MAC Control Information 1, 0x64200*/
				                                 /* MAC Control Information 2, 0x144 */

	tx_desc->thd[9] = (UINT)(tx_desc->pea);      /* Policy Entry Address  */
	if (ldpc_enabled)
				tx_desc->pea[1] |=  1 << 6;   /* PHY Control Info 1 , Indicates whether LDPC encoding is used */

	tx_desc->pea[5] = txRate;

	if (add_length>length1) {
		length2 = (add_length-length1);
		tx_desc->thd[6] = add_length+4;
	}
	tx_desc->tbd[2] = (UINT)(tx_desc->data); /* Data Start Pointer 2 */
	tx_desc->tbd[3] = tx_desc->tbd[2] + length2;

	_sys_nvic_write(phy_intTxTrigger_n, (void *)txp_cmd_trigger, 0x7);

	// EDCA AC0 setting
	pre_edca = edca_ac0(aifsnval);

	// AGCFSMRESET  set for RX off
	modem_agcswreset_setf(1);

	if (rfch) {
		set_rf_channel(rfch);
	}
	prev_power = set_power2(power | (power2 << 0x08));

        REG_PL_WR(0x50001600, 0x2); // FADC Suspend Mode
	do {
		volatile int *status = (volatile int*)&tx_desc->thd[15];
		unsigned int txp_power = get_txp_power2();

		if (((txRate & 0x7f) > 3) || (((txRate >> 11) & 0x07) > 0)) {
			txp_power = txp_power & 0xff;
		} else {
			txp_power = (txp_power >> 0x08) & 0xff;
		}

		tx_desc->pea[9] = txp_power;
		tx_desc->pea[10] = txp_power;
		tx_desc->pea[11] = txp_power;
		tx_desc->pea[12] = txp_power;

		*status = 0; /* clear status */

		mac_header->seq = (uint16_t)((seqnbr++ << MAC_SEQCTRL_NUM_OFT));
		lmac_tx_count[ac2cfm[tx_desc->ac]]++;
		REG_PL_WR(DMA_CONTROL_REG, 1<<(8+(tx_desc->ac)));
	 
		state = xSemaphoreTake(txp_lock, 100);
		if ( state != pdTRUE) {
				  PRINTF("TX time out, %d \n", state);
		}
	} while (txp_on_off && (--num_frames > 0));

	REG_PL_WR(0x50001600, 0x0); // FADC Normal Mode

	// AGCFSMRESET  clear for RX on
	modem_agcswreset_setf(0);
	edca_ac0((u8)pre_edca);
	intc_init(); /* restore RWNX interrupt */

	REG_PL_WR(head_pointer, 0);
	set_power2(prev_power);
	txp_on_off = 0;
	PRINTF(" End of RFTX cmd !!!\n\n");
	vSemaphoreDelete(txp_lock);
	vPortFree(tx_desc);
	if (test_thread_type) {
		TaskHandle_t l_task = NULL;
		l_task = test_thread_type;
		test_thread_type = NULL;
		vTaskDelete(l_task);
	}
}

static u32 str2rate(char *str, u8 GI, u8 greenField, u8 preambleType, u32 *data_size)
{
	u8 invalid = 0;
	u8 mcsIndexTxRCX = 0;
	u8 preTypeTxRCX = 0;  /* SHORT 0, LONG 1*/
	u8 formatModTxRCX = 0;/* NON-HT 0,NON-HT-DUP-OFDM 1,HT-MF 2, HT-GF 3, VHT 4*/
	u32 rate_control = 0;
	u32 size = 0;

	if (str[0] == 'b') { /* 11b */
		formatModTxRCX = 0; /* NON-HT */
		if (str[1] == '1') {
			mcsIndexTxRCX = 0, size = 50;
			if (str[2] == '1')
				mcsIndexTxRCX = 3, size = 600;
		} else if (str[1] == '2') {
			mcsIndexTxRCX = 1, size = 100;
		} else if (str[1] == '5' && str[3] == '5') {
			mcsIndexTxRCX = 2, size = 300;
		} else {
			invalid = 1;
		}
	} else if (str[0] == 'g') { /* 11g */
		formatModTxRCX = 0; /* NON-HT */

		if (str[1] == '6') {
			mcsIndexTxRCX = 4, size = 500;
		} else if (str[1] == '9') {
			mcsIndexTxRCX = 5, size = 700;
		} else if (str[1] == '1') {
			if (str[2] == '2') {
				mcsIndexTxRCX = 6, size = 1000;
			} else if (str[2] == '8') {
				mcsIndexTxRCX = 7, size = 1400;
			} else {
				invalid = 1;
			}
		} else if (str[1] == '2' && str[2] == '4') {
			mcsIndexTxRCX = 8, size = 1800;
		} else if (str[1] == '3' && str[2] == '6') {
			mcsIndexTxRCX = 9, size = 3000;
		} else if (str[1] == '4' && str[2] == '8') {
			mcsIndexTxRCX = 10, size = 4000;
		} else if (str[1] == '5' && str[2] == '4') {
			mcsIndexTxRCX = 11, size = 4000;
		} else {
			invalid = 1;
		}
	} else if (str[0] == 'n') { /* 11n */
		if (greenField) {
			formatModTxRCX = 3;    /* HT-GF  */
		} else {
			formatModTxRCX = 2;    /* HT-MF  */
		}

		if (str[1] == '6' && str[3] == '5') {
			mcsIndexTxRCX = 0, size = 500;
		} else if (str[1] == '1' && str[2] == '3') {
			mcsIndexTxRCX = 1, size = 1000;
		} else if (str[1] == '1' && str[2] == '9' && str[4] == '5') {
			mcsIndexTxRCX = 2, size = 1500;
		} else if (str[1] == '2' && str[2] == '6') {
			mcsIndexTxRCX = 3, size = 2000;
		} else if (str[1] == '3' && str[2] == '9') {
			mcsIndexTxRCX = 4, size = 3000;
		} else if (str[1] == '5' && str[2] == '2') {
			mcsIndexTxRCX = 5, size = 4000;
		} else if (str[1] == '5' && str[2] == '8' && str[4] == '5') {
			mcsIndexTxRCX = 6, size = 4500;
		} else if (str[1] == '6' && str[2] == '5') {
			mcsIndexTxRCX = 7, size = 5000;
		} else {
			invalid = 1;
		}
	} else {
		invalid = 1;
	}

	if (invalid) {
		rate_control = 0;
		size = 50;
		PRINTF("Wrong txRate. Force to DSSS 1MHz\n");
	} else {
		rate_control = (u32)(((formatModTxRCX&0x7)<<11) | ((preTypeTxRCX&0x1)<<10) | (mcsIndexTxRCX&0x7f));
		rate_control = (u32)(rate_control | (u32)(GI<<9) | (u32)(preambleType<<10));
	}

	if (data_size != 0)
		*data_size = size;

	PRINTF("rate_control %s, 0x%x\n", str, rate_control);

	return rate_control;
}

void cmd_lmac_rftx(int argc, char *argv[])
{
	struct RFTX param = {
		2412, 100, 100, 0, 25, 25, {{0x2010, 0x4030, 0x6050}}, {{0x8070, 0xa090, 0xc0b0}}, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0};

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (!check_lmac_init()) {
		PRINTF("Device not initialized. \n");
		return;
	}

	switch( argc ) {
	case 18:
		param.ant = (u8)atoi(argv[17]);
		/* fall through */
		//no break
	case 17:
		param.aifsnval = (u8)atoi(argv[16]);
		/* fall through */
		//no break
	case 16:
		param.scrambler = (u8)atoi(argv[15]);
		/* fall through */
		//no break
	case 15:
		/* MAC Control Information 1 */
		/*0: No-ACK, 1: Normal ACK, 2: BA, 3: Compressed BA */
		if (strcmp(argv[14],"NO") == 0)
			param.ackPolicy = 0;
		else if (strcmp(argv[14],"NORM") == 0)
			param.ackPolicy = 1;
		else if (strcmp(argv[14],"BA") == 0)
			param.ackPolicy = 2;
		else if (strcmp(argv[14],"CBA") == 0)
			param.ackPolicy = 3;
		else
			PRINTF("Wrong ackPolicy. [NO|NORM|BA|CBA]\n");
		/* fall through */
		//no break
	case 14:
		if (strcmp(argv[13], "on") == 0) /* 1(on), 0(off): qosEnable */
			param.qosEnable = 1;
		else if ( strcmp(argv[13], "off") == 0)
			param.qosEnable = 0;
		else
			PRINTF("Wrong qosEnable. [off|on]\n");
		/* fall through */
		//no break
	case 13:
		if (strcmp("long", argv[12]) == 0) /* 1(long), 0(short) */
			param.preambleType = 1;
		else if ( strcmp( "short", argv[12]) == 0)
			param.preambleType = 0;
		else
			PRINTF("Wrong preambleType. [short|long]\n");
		/* fall through */
		//no break
	case 12:
		if (strcmp(argv[11], "on") == 0) /* 1(on), 0(off): GreenField */
			param.greenField = 1;
		else if ( strcmp(argv[11], "off") == 0)
			param.greenField = 0;
		else
			PRINTF("Wrong greenField. [off|on]\n");
		/* fall through */
		//no break
	case 11:
		if (strcmp(argv[10], "short") == 0) /* 1(short),0(long): shortGI */
			param.GI = 1;
		else if ( strcmp(argv[10], "long") == 0)
			param.GI = 0;
		else
			PRINTF("Wrong GI. [long|short]\n");
		/* fall through */
		//no break
	case 10:
		param.htEnable = (u8)atoi(argv[9]);
		/* fall through */
		//no break
	case 9:
		if (str2macaddr(&param.bssid, argv[8]))
			return;
		/* fall through */
		//no break
	case 8:
		if (str2macaddr(&param.destAddr, argv[7]))
			return;
		/* fall through */
		//no break
	case 7:
		param.txPower2 = (u32)atoi(argv[6]);
		param.txPower = (u32)atoi(argv[6]);
		/* fall through */
		//no break
	case 6:
		param.txRate = str2rate(argv[5], param.GI, param.greenField, param.preambleType, 0);
		/* fall through */
		//no break
	case 5:
		param.frameLen = (u32)atoi(argv[4]);
		/* fall through */
		//no break
	case 4:
		param.numFrames = (u32)atoi(argv[3]);
		/* fall through */
		//no beak
	case 3:
		param.BW = (u8)atoi(argv[2]);
		/* fall through */
		//no break
	case 2:
		param.Ch = (u32)atoi(argv[1]);
		break;
	default:
		PRINTF("USAGE: rftx <Ch> <BW> <numFrames> <frameLen> \
<txRate> <txPower> <destAddr> <bssid> <htEnable> <GI> <greenField> \
<preambleType> <qosEnable> <ackPolicy> <scrambler> <aifsnVal> <ant>\n");
		return;
	}

	if (param.greenField && param.GI) {
		PRINTF(" Not Supported Mode !!!\n");
		return;
	}

	if (txp_on_off == 0) {
		 PRINTF("TX on.\n\n");
		 txp_on_off = 1;
		 txp_lock = xSemaphoreCreateBinary();

		 xTaskCreate( rftx_thread, "TestThread", 2048, ( void * )(&param), OS_TASK_PRIORITY_SYSTEM+9, &test_thread_type );
		 return;
	}
	else if (txp_on_off == 1) {
		PRINTF("TX ongoing now.\n");
	}
}

#define FORCE_ENABLE_ADDR 0x60c0c018
#define FORCE_VALUE_ADDR 0x60c0c01c
#define FCI_MULTITONE_PRIMARY 0x60C0F070
#define RF_REGISTER_BASE 0x60F00000   
#define RF_REG(reg) (RF_REGISTER_BASE+((reg)*4))
#define CW_POWER_GRADE_SIZE 13		  

uint16_t pre_freq = 2412;
static void set_cw_power(u16 power_grade, u16 freq)
{	 
	uint32_t rf_index = (uint32_t)((freq - 2412)/5);
	uint16_t rf_reg2 = (uint16_t)(0x800d | (rf_index+1)<<8);
	
	if (freq == 2484)
		rf_reg2 = 0x8e0d;

	/* save previous frequency */
	pre_freq = (uint16_t)(REG_PL_RD(RF_REG(0x02)));
	
	REG_PL_WR(RF_REG(0xe0), 9);
	REG_PL_WR(RF_REG(0x02), rf_reg2);
	REG_PL_WR(RF_REG(0x06), (0x505 + (power_grade<<8)));
}

static void restore_cw_power()
{
	REG_PL_WR(RF_REG(0xe0), 0x0000);
	REG_PL_WR(RF_REG(0x02), pre_freq);
	REG_PL_WR(RF_REG(0x06), 0x0505);
}

#define ONE_TONE_SCALE_ADDR	0x60c03714
#define RF_PACKAGE_TYPE_REG	0x00EF
uint16_t pre_one_tone_scale = 0xAA; /* Last value from RF CAL */
static void set_one_tone_scale()
{
	uint32_t package = REG_PL_RD(RF_REG(RF_PACKAGE_TYPE_REG));
	/* save pre value */
	pre_one_tone_scale = (uint16_t)(REG_PL_RD(ONE_TONE_SCALE_ADDR));
	
	if ( package == 0x110B) /* QFN */
		REG_PL_WR(ONE_TONE_SCALE_ADDR, 0x135);
	else if ( package == 0x210B) /* FcCSP NP */
		REG_PL_WR(ONE_TONE_SCALE_ADDR, 0x120);
	else if ( package == 0x2209) /* FcCSP LP */
		REG_PL_WR(ONE_TONE_SCALE_ADDR, 0xf0);
}   
			 
void cmd_lmac_rfcwtest(int argc, char *argv[])
{
	/* <Ch>,< BW >,<txPower>,<ant>,<CWCycle> */
	u16 freq = 2412;
	u8 BW = 0;
	u16 txPower = 0;
	u8 ant = 0;
	u8 CWCycle = 1;
	u8 temp = 0;

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	switch( argc ) {
	case 6:  /* <CWCycle> :[1, 2, 4, 5, 8, 10]. MHz  */
		temp = (u8)atoi(argv[5]);
		if (temp == 1) CWCycle = 0;
		else if (temp == 2) CWCycle = 1;
		else if (temp == 4) CWCycle = 2;
		else if (temp == 5) CWCycle = 3;
		else if (temp == 8) CWCycle = 4;
		else if (temp == 10) CWCycle = 5;
		/* fall through */
		//no break
	case 5:  /* <ant> : 0 */
		ant = (u8)atoi(argv[4]);
		/* fall through */
		//no break
	case 4:  /* <txPower> : dBm (-2~23, 0.1 Step) */
		txPower = (u16)atoi(argv[3]);
		/* fall through */
		//no break
	case 3:  /* <BW> : Carrier Bandwidth. 20MHz Fixed. */
		BW = (u8)atoi(argv[2]);
		/* fall through */
		//no break
	case 2:  /* <Frequency> : Carrier Frequency (2412 ~ 2484MHz). */
		freq = (u16)atoi(argv[1]);
		break;
	default:

		PRINTF("USAGE: rfcwtest <Frequency>,<BW>,<txPower>,<ant>,<CWCycle>\n");
		return;
	}

	if (freq < 2412 || freq > 2484) {
		PRINTF("RF Channel Frequency range wrong !!!\n");
		return;
	} else {
		PRINTF("Set RF Channel Frequency %d\n", freq);
	}
	
	set_cw_power(txPower, freq);

	set_one_tone_scale();
	
	REG_PL_WR(FORCE_ENABLE_ADDR, 0x3FF0F);
	REG_PL_WR(FORCE_VALUE_ADDR, 0xE | ((txPower&0x3ff)<<8) );
	REG_PL_WR(0x60c0f070, 0x100003c);
	REG_PL_WR(0x60c0f074, 0x003003c);
}


void cmd_lmac_rfcwstop(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	REG_PL_WR(FORCE_ENABLE_ADDR, 0x0);
	REG_PL_WR(FORCE_VALUE_ADDR, 0x73F3FF07);
	REG_PL_WR(0x60c0f070, 0x0);
	
	restore_cw_power();
	
	/* restore one tone scale */
	REG_PL_WR(ONE_TONE_SCALE_ADDR, pre_one_tone_scale); 
}

#define PLL_SET_SIZE	28
uint32_t backup_rf_pll[PLL_SET_SIZE];
void reset_rf_cw_pll()
{
  /* restore values */
  for(int i=0; i<PLL_SET_SIZE;i++)
  {
	  REG_PL_WR(RF_REG(0x110+i), backup_rf_pll[i]);
  }
}

void set_rf_cw_pll() 
{
  /* save values */
  for(int i=0; i<PLL_SET_SIZE;i++)
  {
	  backup_rf_pll[i] = REG_PL_RD(RF_REG(0x110+i));
  }

  /* set rf pll for cw */
  REG_PL_WR(RF_REG(0x110), 0x2802);
  REG_PL_WR(RF_REG(0x111), 0xEEEF);
  REG_PL_WR(RF_REG(0x112), 0x2804);
  REG_PL_WR(RF_REG(0x113), 0x4445);
  REG_PL_WR(RF_REG(0x114), 0x2805);
  REG_PL_WR(RF_REG(0x115), 0x9999);
  REG_PL_WR(RF_REG(0x116), 0x2806);
  REG_PL_WR(RF_REG(0x117), 0xEEEF);
  REG_PL_WR(RF_REG(0x118), 0x2808);
  REG_PL_WR(RF_REG(0x119), 0x4445);
  REG_PL_WR(RF_REG(0x11A), 0x2809);
  REG_PL_WR(RF_REG(0x11B), 0x9999);
  REG_PL_WR(RF_REG(0x11C), 0x280A);
  REG_PL_WR(RF_REG(0x11D), 0xEEEF);
  REG_PL_WR(RF_REG(0x11E), 0x280C);
  REG_PL_WR(RF_REG(0x11F), 0x4445);
  REG_PL_WR(RF_REG(0x120), 0x280D);
  REG_PL_WR(RF_REG(0x121), 0x9999);
  REG_PL_WR(RF_REG(0x122), 0x280E);
  REG_PL_WR(RF_REG(0x123), 0xEEEF);
  REG_PL_WR(RF_REG(0x124), 0x2900);
  REG_PL_WR(RF_REG(0x125), 0x4445);
  REG_PL_WR(RF_REG(0x126), 0x2901);
  REG_PL_WR(RF_REG(0x127), 0x9999);
  REG_PL_WR(RF_REG(0x128), 0x2902);
  REG_PL_WR(RF_REG(0x129), 0xEEEF);
  REG_PL_WR(RF_REG(0x12A), 0x2906);
  REG_PL_WR(RF_REG(0x12B), 0x2223);   
}

/* command for pll setting for rf cw test */
void cmd_lmac_rf_cw_pll(int argc, char *argv[])
{
  if(argc > 1) {
    if(strcasecmp(argv[1], "set") == 0) {	     
	  set_rf_cw_pll();
      PRINTF("Set PLL for CW Test Mode\n");
    } else if (strcasecmp(argv[1], "reset") == 0) {
	  reset_rf_cw_pll();
      PRINTF("Reset PLL for Normal Mode.\n");      
    }
  } else {
    PRINTF("USAGE: cw_pll [set/reset]\n");
  }
}

#ifdef SUPPORT_SEND_NULL_FRAME
#define NXMAC_MAC_CNTRL_1_ADDR   0x60B0004C

typedef void (*cfm_func_ptr)(void *, uint32_t);

static void nxmac_pwr_mgt_setf(uint8_t pwrmgt)
{
		REG_PL_WR(NXMAC_MAC_CNTRL_1_ADDR, (REG_PL_RD(NXMAC_MAC_CNTRL_1_ADDR) & ~((uint32_t)0x00000004)) | ((uint32_t)pwrmgt << 2));
}

static void send_null_cfm(void *env, uint32_t status)
{
	nxmac_pwr_mgt_setf(0);
	PRINTF("status = 0x%x\n", status);
}

void cmd_lmac_null(int argc, char *argv[])
{
	uint8_t sta_idx = 0;
	uint8_t pm = 0;
	cfm_func_ptr cfm = send_null_cfm;

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	switch (argc) {
	case 1:
		txl_frame_send_null_frame(sta_idx, cfm, NULL);
		//txl_frame_send_qossnull_frame(sta_idx, cfm, NULL);
		break;
	case 2:
		pm = atoi(argv[1]);
		nxmac_pwr_mgt_setf(pm);
		txl_frame_send_null_frame(sta_idx, cfm, NULL);
		//txl_frame_send_qosnull_frame(sta_idx, cfm, NULL);
	}
}
#endif

void tx_cont_start(int argc, char *argv[])
{
	/*
struct RFTX {
		u32 Ch;
		u32 numFrames;
		u32 frameLen;
		u32 txRate;
		u32 txPower;
		struct mac_addr destAddr;
		struct mac_addr bssid;
		u8 htEnable;
		u8 GI;
		u8 greenField;
		u8 preambleType;
		u8 qosEnable;
		u8 ackPolicy;
		u8 scrambler;
		u8 aifsnval;
		u8 ant;
		u8 BW;
};
	*/

	u32 frameLen;
	struct RFTX param =
	{2412, 0, 100, 0, 25, 25, {{0x2010, 0x4030, 0x6050}}, {{0x8070, 0xa090, 0xc0b0}}, 0,
	0, 0, 0, 0, 0, 0, 1, 0, 0};

	/*<txRate>,<txPower>,<Ch> */
	switch( argc ) {
	case 4:
		param.Ch = (u32)atoi(argv[3]);
		/* fall through */
	case 3:
		param.txPower = (u32)atoi(argv[2]);
		param.txPower2 = (u32)atoi(argv[2]);
		/* fall through */
	case 2:
		param.txRate = str2rate(argv[1], 0, 0, 0, &frameLen);
		/* fall through */
	default:
		break;
	}

	param.frameLen = frameLen;

	if (txp_on_off == 0) {
		 PRINTF("Continuos TX on.  ch = %d, txpower = %d\n\n", param.Ch, param.txPower);
		 vTaskDelay(1);
		 txp_on_off = 1;
		 txp_lock = xSemaphoreCreateBinary();
		 if (txp_lock == NULL)
		 {
				PRINTF("SEMAPHORE create err( Heap memory check).\n");
				return; 
		 }
		 xTaskCreate(rftx_thread, "TestThread", 2048, ( void * )(&param), OS_TASK_PRIORITY_SYSTEM+9, &test_thread_type );

		 return;
	} else if (txp_on_off == 1) {
		PRINTF("Continuos TX ongoing now.\n");
	}
}

void tx_cont_stop()
{
	hal_machw_reset();
	rxl_reset();
	txl_reset();
	lmac_next_state_setf(HW_ACTIVE);
	
	txp_on_off = 0;
	set_power2(prev_power);
	PRINTF(" TX Continuous Mode STOP !!! \n");
	if (test_thread_type) {
		TaskHandle_t l_task = NULL;
		l_task = test_thread_type;
		test_thread_type = NULL;
		vTaskDelete(l_task);
	}
}

void cmd_lmac_cont_tx(int argc, char *argv[])
{
	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (argc > 1) {
		if (strcasecmp(argv[1], "START") == 0) {
			PRINTF("CONT TX START.\n");
			tx_cont_start(argc-1, &argv[1]);

		} else if (strcasecmp(argv[1], "STOP") == 0) {
			PRINTF("CONT TX STOP.\n");
			if (txp_on_off == 1)
				  tx_cont_stop();
		}
	} else
		PRINTF("USAGE: cont_tx [start/stop] [rate] [power] [channel]\n");
}

/* Band (2.4GHz or 5GHz) */
static uint8_t band = PHY_BAND_2G4;
/* Channel type: 20,40,80,160 or 80+80 MHz */
static uint8_t type = PHY_CHNL_BW_20;
/* Frequency for Primary 20MHz channel (in MHz) */
static uint16_t prim20_freq = 0;
/* Frequency for Center of the contiguous channel or center of Primary 80+80 */
static uint16_t center1_freq = 2412;
/* Frequency for Center of the non-contiguous secondary 80+80 */
static uint16_t center2_freq = 255;

#define RWNX_LMAC_MIB_ADDRESS               0x60B00800

extern void modem_reset();
static uint32_t set_ch()
{
	/* Once in IDLE, handle the packets already in the RX queue to
		 ensure that the channel information indicated to
		 the upper MAC is correct.  This has to be done with
		 interrupts disabled, as the normal handling of the packets
		 is done under interrupt */
	/*GLOBAL_INT_DISABLE();
	rxl_timer_int_handler();
	rxl_cntrl_evt(0);
	GLOBAL_INT_RESTORE();*/

				uint8_t pre_state = lmac_current_state_getf();
				lmac_next_state_setf(HW_IDLE);

	/* Now we can move the PHY to the requested channel */
	phy_set_channel(band, type, prim20_freq, center1_freq,
			center2_freq, 0 /* PHY_PRIM */);

				lmac_next_state_setf(pre_state);

				modem_reset();
				
				return 0;
}

static const char *get_state(uint8_t state)
{
	switch (state) {
	case HW_IDLE:          /* MAC HW IDLE State. */
		return "IDLE";
	case HW_RESERVED:      /* MAC HW RESERVED State. */
		return "RESERVED";
	case HW_DOZE:          /* MAC HW DOZE State. */
		return "DOZE";
	case HW_ACTIVE:        /* MAC HW ACTIVE State */
		return "ACTIVE";
	}

	return "UNKNOWN";
}

static uint32_t get_ch2freq(uint32_t ch)
{
	switch (ch) {
	case 1:
		return 2412;
	case 2:
		return 2417;
	case 3:
		return 2422;
	case 4:
		return 2427;
	case 5:
		return 2432;
	case 6:
		return 2437;
	case 7:
		return 2442;
	case 8:
		return 2447;
	case 9:
		return 2452;
	case 10:
		return 2457;
	case 11:
		return 2462;
	case 12:
		return 2467;
	case 13:
		return 2472;
	case 14:
		return 2484;
	default:
		return 0;
	}
}

static uint32_t get_freq2ch(uint32_t freq)
{
	switch (freq) {
	case 2412:
		return 1;
	case 2417:
		return 2;
	case 2422:
		return 3;
	case 2427:
		return 4;
	case 2432:
		return 5;
	case 2437:
		return 6;
	case 2442:
		return 7;
	case 2447:
		return 8;
	case 2452:
		return 9;
	case 2457:
		return 10;
	case 2462:
		return 11;
	case 2467:
		return 12;
	case 2472:
		return 13;
	case 2484:
		return 14;
	default:
		return 0;
	}
}

extern unsigned int is_mac_doze;
void cmd_lmac_rx_state(int argc, char *argv[])
{
	u8 usage_en = 0;
	u8 status_en = 0;
	u8 set_en = 0;
	uint8_t state;

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	switch (argc) {
	case 2:
		if (strcmp(argv[1], "i") == 0) {
				                if (is_mac_doze) {
				                  lmac_wake_up_from_doze_setf(1);
				                  is_mac_doze = 0;
				                }
			state = HW_IDLE;
			set_en = 1;
			status_en = 1;
		} else if (strcmp(argv[1], "d") == 0) {
				                if (lmac_current_state_getf() == HW_ACTIVE) {
				                  PRINTF("ACTIVE to DOZE not allow.\n");
				                  PRINTF("ACTIVE to IDLE and set to DOZE.\n");
				                  return;
				                }
			state = HW_DOZE;
			set_en = 1;
			status_en = 1;
		} else if (strcmp(argv[1], "a") == 0) {
				                if (is_mac_doze) {
				                  lmac_wake_up_from_doze_setf(1);
				                  is_mac_doze = 0;
				                }
			state = HW_ACTIVE;
			set_en = 1;
			status_en = 1;
		} else {
			usage_en = 1;
		}
		break;
	default:
		usage_en = 1;
		status_en = 1;
	}

	if (set_en) {
		lmac_next_state_setf(state);
				}

	if (status_en) {

				  if (is_mac_doze) {
				            PRINTF("%s::current %s\n", argv[0],
			get_state(2));
				            PRINTF("%s::next %s\n", argv[0],
			get_state(2));
				        }
				        else {
				            PRINTF("%s::current %s\n", argv[0],
			get_state(lmac_current_state_getf()));
				            PRINTF("%s::next %s\n", argv[0],
			get_state(lmac_next_state_getf()));
				        }
	}

	if (usage_en)
		PRINTF("Usage: %s [i: IDLE|d: DOZE|a: ACTIVE]\n", argv[0]);

}

void cmd_lmac_rx_ch(int argc, char *argv[])
{
	u8 usage_en = 0;
	u8 status_en = 0;
	u8 set_en = 0;
	uint32_t ch;

	switch (argc) {
	case 4:
		ch = (uint32_t)atoi(argv[1]);
		prim20_freq = (uint16_t)get_ch2freq(ch);
		ch = (uint32_t)atoi(argv[2]);
		center1_freq = (uint16_t)get_ch2freq(ch);
		ch = (uint32_t)atoi(argv[3]);
		center2_freq = (uint16_t)get_ch2freq(ch);
		set_en = 1;
		status_en = 1;
		break;
	case 3:
		ch = (uint32_t)atoi(argv[1]);
		prim20_freq = (uint16_t)get_ch2freq(ch);
		ch = (uint32_t)atoi(argv[2]);
		center1_freq = (uint16_t)get_ch2freq(ch);
		center2_freq = 255;
		set_en = 1;
		status_en = 1;
		break;
	case 2:
		ch = (uint32_t)atoi(argv[1]);
		center1_freq = prim20_freq = (uint16_t)get_ch2freq(ch);
		center2_freq = 255;
		set_en = 1;
		status_en = 1;
		break;
	default:
		usage_en = 1;
		status_en = 1;
	}

	if (set_en) {
		set_ch();
	} else {
		struct phy_channel_info phy_info;
			phy_get_channel(&phy_info, PHY_PRIM);
		
		band = phy_info.info1 & 0xFF;
		type = (phy_info.info1>>8) & 0xFF; 
		prim20_freq = (uint16_t)((phy_info.info1>>16) & 0xFFFF);
		center1_freq = phy_info.info2 & 0xFFFF;
		/*center2_freq = (phy_info.info2>>16) & 0xFFFF;*/
	}

	if (status_en) {
		PRINTF("ch::band         %s\n", band ? "5G" : "2.4G");
		PRINTF("ch::type         %d (BW 20M)\n", type);
		PRINTF("ch::prim20_freq  %d Mhz (%d)\n", prim20_freq,
					get_freq2ch(prim20_freq));
		PRINTF("ch::center1_freq %d Mhz (%d)\n", center1_freq,
					get_freq2ch(center1_freq));
		/*PRINTF("ch::center2_freq %d Mhz (%d)\n", center2_freq,
					get_freq2ch(center2_freq));*/
	}

	if (usage_en) {
		PRINTF("Usage: %s [ch: 1 ~ 14]\n", argv[0]);
		PRINTF("       %s [prim20_ch] [center1_ch] [center2_ch]\n",
								argv[0]);
	}
}


void cmd_lmac_rx_mib(int argc, char *argv[])
{
	u8 usage_en = 0;
	u8 reset_en = 1;

	struct machw_mib_tag *mib = (struct machw_mib_tag *)
		(RWNX_LMAC_MIB_ADDRESS);

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	switch (argc) {
	case 2:
		if (strcmp("h", argv[1]) == 0) // Hold
			reset_en = 0;
		else if (strcmp("r", argv[1]) == 0) // Reset
			reset_en = 1;
		else
			usage_en = 1;
		break;
	case 1:
		usage_en = 1;
	}

	/*printf("MIB element to count the number of unencrypted frames that "
			"have been discarded\n");*/
	PRINTF("dot11_wep_excluded_count:               %u\n",
			mib->dot11_wep_excluded_count);

	/*printf("MIB element to count the receive FCS errors\n");*/
	PRINTF("dot11_fcs_error_count:                  %u\n",
			mib->dot11_fcs_error_count);

	/*printf("MIB element to count the number of PHY Errors reported during "
			"a receive transaction.\n");*/
	PRINTF("nx_rx_phy_error_count:                  %u\n",
			mib->nx_rx_phy_error_count);

	/*printf("MIB element to count the number of times the receive FIFO has "
			"overflowed\n");*/
	PRINTF("nx_rd_fifo_overflow_count:              %u\n",
			mib->nx_rd_fifo_overflow_count);

	/*printf("MIB element to count the number of times underrun has occured "
			"on the transmit side\n");*/
	PRINTF("nx_tx_underun_count:                    %u\n",
			mib->nx_tx_underun_count);

	/*printf("Counts the number of times an overflow occurrs at the MPIF "
				"FIFO when receiving a frame from the PHY\n");*/
	PRINTF("nx_tx_underun_count:                    %u\n",
			mib->nx_rx_mpif_overflow_count);

	/*printf("MIB element to count unicast transmitted MPDU\n");*/
	PRINTF("nx_qos_utransmitted_mpdu_count:         "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_qos_utransmitted_mpdu_count[0],
			mib->nx_qos_utransmitted_mpdu_count[1],
			mib->nx_qos_utransmitted_mpdu_count[2],
			mib->nx_qos_utransmitted_mpdu_count[3],
			mib->nx_qos_utransmitted_mpdu_count[4],
			mib->nx_qos_utransmitted_mpdu_count[5],
			mib->nx_qos_utransmitted_mpdu_count[6],
			mib->nx_qos_utransmitted_mpdu_count[7]);

	/*printf("MIB element to count group addressed transmitted MPDU\n");*/
	PRINTF("nx_qos_gtransmitted_mpdu_count:         "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_qos_gtransmitted_mpdu_count[0],
			mib->nx_qos_gtransmitted_mpdu_count[1],
			mib->nx_qos_gtransmitted_mpdu_count[2],
			mib->nx_qos_gtransmitted_mpdu_count[3],
			mib->nx_qos_gtransmitted_mpdu_count[4],
			mib->nx_qos_gtransmitted_mpdu_count[5],
			mib->nx_qos_gtransmitted_mpdu_count[6],
			mib->nx_qos_gtransmitted_mpdu_count[7]);

	/*printf("MIB element to count the number of MSDUs or MMPDUs discarded "
			"because of retry-limit reached\n");*/
	PRINTF("dot11_qos_failed_count:                 "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_qos_failed_count[0],
			mib->dot11_qos_failed_count[1],
			mib->dot11_qos_failed_count[2],
			mib->dot11_qos_failed_count[3],
			mib->dot11_qos_failed_count[4],
			mib->dot11_qos_failed_count[5],
			mib->dot11_qos_failed_count[6],
			mib->dot11_qos_failed_count[7]);

	/*printf("MIB element to count number of unfragmented MSDU\'s or MMPDU's "
			"transmitted successfully after 1 or more "
			"transmission\n");*/
	PRINTF("dot11_qos_retry_count:                  "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_qos_retry_count[0],
			mib->dot11_qos_retry_count[1],
			mib->dot11_qos_retry_count[2],
			mib->dot11_qos_retry_count[3],
			mib->dot11_qos_retry_count[4],
			mib->dot11_qos_retry_count[5],
			mib->dot11_qos_retry_count[6],
			mib->dot11_qos_retry_count[7]);

	/*printf("MIB element to count number of successful RTS Frame "
			"transmission\n");*/
	PRINTF("dot11_qos_rts_success_count:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_qos_rts_success_count[0],
			mib->dot11_qos_rts_success_count[1],
			mib->dot11_qos_rts_success_count[2],
			mib->dot11_qos_rts_success_count[3],
			mib->dot11_qos_rts_success_count[4],
			mib->dot11_qos_rts_success_count[5],
			mib->dot11_qos_rts_success_count[6],
			mib->dot11_qos_rts_success_count[7]);

	/*printf("MIB element to count number of unsuccessful RTS Frame "
			"transmission\n");*/
	PRINTF("dot11_qos_rts_failure_count:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_qos_rts_failure_count[0],
			mib->dot11_qos_rts_failure_count[1],
			mib->dot11_qos_rts_failure_count[2],
			mib->dot11_qos_rts_failure_count[3],
			mib->dot11_qos_rts_failure_count[4],
			mib->dot11_qos_rts_failure_count[5],
			mib->dot11_qos_rts_failure_count[6],
			mib->dot11_qos_rts_failure_count[7]);

	/*printf("MIB element to count number of MPDU's not received ACK\n");*/
	PRINTF("nx_qos_ack_failure_count:               "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_qos_ack_failure_count[0],
			mib->nx_qos_ack_failure_count[1],
			mib->nx_qos_ack_failure_count[2],
			mib->nx_qos_ack_failure_count[3],
			mib->nx_qos_ack_failure_count[4],
			mib->nx_qos_ack_failure_count[5],
			mib->nx_qos_ack_failure_count[6],
			mib->nx_qos_ack_failure_count[7]);

	/*printf("MIB element to count number of unicast MPDU's received "
			"successfully\n");*/
	PRINTF("nx_qos_ureceived_mpdu_count:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_qos_ureceived_mpdu_count[0],
			mib->nx_qos_ureceived_mpdu_count[1],
			mib->nx_qos_ureceived_mpdu_count[2],
			mib->nx_qos_ureceived_mpdu_count[3],
			mib->nx_qos_ureceived_mpdu_count[4],
			mib->nx_qos_ureceived_mpdu_count[5],
			mib->nx_qos_ureceived_mpdu_count[6],
			mib->nx_qos_ureceived_mpdu_count[7]);

	/*printf("MIB element to count number of group addressed MPDU's received "
			"successfully\n");*/
	PRINTF("nx_qos_greceived_mpdu_count:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_qos_greceived_mpdu_count[0],
			mib->nx_qos_greceived_mpdu_count[1],
			mib->nx_qos_greceived_mpdu_count[2],
			mib->nx_qos_greceived_mpdu_count[3],
			mib->nx_qos_greceived_mpdu_count[4],
			mib->nx_qos_greceived_mpdu_count[5],
			mib->nx_qos_greceived_mpdu_count[6],
			mib->nx_qos_greceived_mpdu_count[7]);

	/*printf("MIB element to count the number of unicast MPDUs not destined "
			"to this device received successfully.\n");*/
	PRINTF("nx_qos_ureceived_other_mpdu:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_qos_ureceived_other_mpdu[0],
			mib->nx_qos_ureceived_other_mpdu[1],
			mib->nx_qos_ureceived_other_mpdu[2],
			mib->nx_qos_ureceived_other_mpdu[3],
			mib->nx_qos_ureceived_other_mpdu[4],
			mib->nx_qos_ureceived_other_mpdu[5],
			mib->nx_qos_ureceived_other_mpdu[6],
			mib->nx_qos_ureceived_other_mpdu[7]);

	/*printf("MIB element to count the number of MPDUs received with retry "
			"bit set\n");*/
	PRINTF("dot11_qos_retries_received_count:       "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_qos_retries_received_count[0],
			mib->dot11_qos_retries_received_count[1],
			mib->dot11_qos_retries_received_count[2],
			mib->dot11_qos_retries_received_count[3],
			mib->dot11_qos_retries_received_count[4],
			mib->dot11_qos_retries_received_count[5],
			mib->dot11_qos_retries_received_count[6],
			mib->dot11_qos_retries_received_count[7]);

	/*printf("MIB element to count the number of unicast A-MSDUs that were "
			"transmitted successfully\n");*/
	PRINTF("nx_utransmitted_amsdu_count:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_utransmitted_amsdu_count[0],
			mib->nx_utransmitted_amsdu_count[1],
			mib->nx_utransmitted_amsdu_count[2],
			mib->nx_utransmitted_amsdu_count[3],
			mib->nx_utransmitted_amsdu_count[4],
			mib->nx_utransmitted_amsdu_count[5],
			mib->nx_utransmitted_amsdu_count[6],
			mib->nx_utransmitted_amsdu_count[7]);

	/*printf("MIB element to count the number of group-addressed A-MSDUs "
			"that were transmitted successfully\n");*/
	PRINTF("nx_gtransmitted_amsdu_count:            "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_gtransmitted_amsdu_count[0],
			mib->nx_gtransmitted_amsdu_count[1],
			mib->nx_gtransmitted_amsdu_count[2],
			mib->nx_gtransmitted_amsdu_count[3],
			mib->nx_gtransmitted_amsdu_count[4],
			mib->nx_gtransmitted_amsdu_count[5],
			mib->nx_gtransmitted_amsdu_count[6],
			mib->nx_gtransmitted_amsdu_count[7]);

	/*printf("MIB element to count number of AMSDU's discarded because of "
			"retry limit reached\n");*/
	PRINTF("dot11_failed_amsdu_count:               "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_failed_amsdu_count[0],
			mib->dot11_failed_amsdu_count[1],
			mib->dot11_failed_amsdu_count[2],
			mib->dot11_failed_amsdu_count[3],
			mib->dot11_failed_amsdu_count[4],
			mib->dot11_failed_amsdu_count[5],
			mib->dot11_failed_amsdu_count[6],
			mib->dot11_failed_amsdu_count[7]);

	/*printf("MIB element to count number of A-MSDU's transmitted "
			"successfully with retry\n");*/
	PRINTF("dot11_retry_amsdu_count:                "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_retry_amsdu_count[0],
			mib->dot11_retry_amsdu_count[1],
			mib->dot11_retry_amsdu_count[2],
			mib->dot11_retry_amsdu_count[3],
			mib->dot11_retry_amsdu_count[4],
			mib->dot11_retry_amsdu_count[5],
			mib->dot11_retry_amsdu_count[6],
			mib->dot11_retry_amsdu_count[7]);

	/*printf("MIB element to count number of bytes of an A-MSDU that was "
			"transmitted successfully\n");*/
	PRINTF("dot11_transmitted_octets_in_amsdu:      "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_transmitted_octets_in_amsdu[0],
			mib->dot11_transmitted_octets_in_amsdu[1],
			mib->dot11_transmitted_octets_in_amsdu[2],
			mib->dot11_transmitted_octets_in_amsdu[3],
			mib->dot11_transmitted_octets_in_amsdu[4],
			mib->dot11_transmitted_octets_in_amsdu[5],
			mib->dot11_transmitted_octets_in_amsdu[6],
			mib->dot11_transmitted_octets_in_amsdu[7]);

	/*printf("MIB element to counts the number of A-MSDUs that did not "
			"receive an ACK frame successfully in response\n");*/
	PRINTF("dot11_amsdu_ack_failure_count:          "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_amsdu_ack_failure_count[0],
			mib->dot11_amsdu_ack_failure_count[1],
			mib->dot11_amsdu_ack_failure_count[2],
			mib->dot11_amsdu_ack_failure_count[3],
			mib->dot11_amsdu_ack_failure_count[4],
			mib->dot11_amsdu_ack_failure_count[5],
			mib->dot11_amsdu_ack_failure_count[6],
			mib->dot11_amsdu_ack_failure_count[7]);

	/*printf("MIB element to count number of unicast A-MSDUs received "
			"successfully\n");*/
	PRINTF("nx_ureceived_amsdu_count:               "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_ureceived_amsdu_count[0],
			mib->nx_ureceived_amsdu_count[1],
			mib->nx_ureceived_amsdu_count[2],
			mib->nx_ureceived_amsdu_count[3],
			mib->nx_ureceived_amsdu_count[4],
			mib->nx_ureceived_amsdu_count[5],
			mib->nx_ureceived_amsdu_count[6],
			mib->nx_ureceived_amsdu_count[7]);

	/*printf("MIB element to count number of group addressed A-MSDUs "
			"received successfully\n");*/
	PRINTF("nx_greceived_amsdu_count:               "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_greceived_amsdu_count[0],
			mib->nx_greceived_amsdu_count[1],
			mib->nx_greceived_amsdu_count[2],
			mib->nx_greceived_amsdu_count[3],
			mib->nx_greceived_amsdu_count[4],
			mib->nx_greceived_amsdu_count[5],
			mib->nx_greceived_amsdu_count[6],
			mib->nx_greceived_amsdu_count[7]);

	/*printf("MIB element to count number of unicast A-MSDUs not destined to "
			"this device received successfully\n");*/
	PRINTF("nx_ureceived_other_amsdu:               "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->nx_ureceived_other_amsdu[0],
			mib->nx_ureceived_other_amsdu[1],
			mib->nx_ureceived_other_amsdu[2],
			mib->nx_ureceived_other_amsdu[3],
			mib->nx_ureceived_other_amsdu[4],
			mib->nx_ureceived_other_amsdu[5],
			mib->nx_ureceived_other_amsdu[6],
			mib->nx_ureceived_other_amsdu[7]);

	/*printf("MIB element to count number of bytes in an A-MSDU is "
PRINTF"received\n");*/
	PRINTF("dot11_received_octets_in_amsdu_count:   "
			"%u %u %u %u "
			"%u %u %u %u\n",
			mib->dot11_received_octets_in_amsdu_count[0],
			mib->dot11_received_octets_in_amsdu_count[1],
			mib->dot11_received_octets_in_amsdu_count[2],
			mib->dot11_received_octets_in_amsdu_count[3],
			mib->dot11_received_octets_in_amsdu_count[4],
			mib->dot11_received_octets_in_amsdu_count[5],
			mib->dot11_received_octets_in_amsdu_count[6],
			mib->dot11_received_octets_in_amsdu_count[7]);

	/*printf("MIB element to count number of A-MPDUs transmitted "
			"successfully\n");*/
	PRINTF("dot11_transmitted_ampdu_count:          %u\n",
			mib->dot11_transmitted_ampdu_count);

	/*printf("MIB element to count number of MPDUs transmitted in an "
			"A-MPDU\n");*/
	PRINTF("dot11_transmitted_mpdus_in_ampdu_count: %u\n",
			mib->dot11_transmitted_mpdus_in_ampdu_count);

	/*printf("MIB element to count the number of bytes in a transmitted "
			"A-MPDU\n");*/
	PRINTF("dot11_transmitted_octets_in_ampdu_count:%u\n",
			mib->dot11_transmitted_octets_in_ampdu_count);

	/*printf("MIB element to count number of unicast A-MPDU's received\n");*/
	PRINTF("wnlu_ampdu_received_count:              %u\n",
			mib->wnlu_ampdu_received_count);

	/*printf("MIB element to count number of group addressed A-MPDU's "
			"received\n");*/
	PRINTF("nx_gampdu_received_count:               %u\n",
			mib->nx_gampdu_received_count);

	/*printf("MIB element to count number of unicast A-MPDUs received not "
			"destined to this device\n");*/
	PRINTF("nx_other_ampdu_received_count:          %u\n",
			mib->nx_other_ampdu_received_count);

	/*printf("MIB element to count number of MPDUs received in an A-MPDU\n");*/
	PRINTF("dot11_mpdu_in_received_ampdu_count:     %u\n",
			mib->dot11_mpdu_in_received_ampdu_count);

	/*printf("MIB element to count number of bytes received in an A-MPDU\n");*/
	PRINTF("dot11_received_octets_in_ampdu_count:   %u\n",
			mib->dot11_received_octets_in_ampdu_count);

	/*printf("MIB element to count number of CRC errors in MPDU delimeter of "
			"and A-MPDU\n");*/
	PRINTF("dot11_ampdu_delimiter_crc_error_count:  %u\n",
			mib->dot11_ampdu_delimiter_crc_error_count);

	/*printf("MIB element to count number of implicit BAR frames that did "
			"not received BA frame successfully in response\n");*/
	PRINTF("dot11_implicit_bar_failure_count:       %u\n",
			mib->dot11_implicit_bar_failure_count);

	/*printf("MIB element to count number of explicit BAR frames that did "
			"not received BA frame successfully in response\n");*/
	PRINTF("dot11_explicit_bar_failure_count:       %u\n",
			mib->dot11_explicit_bar_failure_count);

	/*printf("MIB element to count the number of frames transmitted at 20 "
			"MHz BW\n");*/
	PRINTF("dot11_20mhz_frame_transmitted_count:    %u\n",
			mib->dot11_20mhz_frame_transmitted_count);

	/*printf("MIB element to count the number of frames transmitted at 40 "
			"MHz BW\n");*/
	PRINTF("dot11_40mhz_frame_transmitted_count:    %u\n",
			mib->dot11_40mhz_frame_transmitted_count);

	/*printf("MIB element to count the number of frames transmitted at 80 "
			"MHz BW\n");*/
	/*printf("dot11_80mhz_frame_transmitted_count:    %u\n",
			mib->dot11_80mhz_frame_transmitted_count);*/

	/*printf("MIB element to count the number of frames transmitted at 160 "
			"MHz BW\n");*/
	/*printf("dot11_160mhz_frame_transmitted_count:   %u\n",
			mib->dot11_160mhz_frame_transmitted_count);*/

	/*printf("MIB element to count the number of frames received at 20 MHz "
			"BW\n");*/
	PRINTF("dot11_20mhz_frame_received_count:       %u\n",
			mib->dot11_20mhz_frame_received_count);

	/*printf("MIB element to count the number of frames received at 40 MHz "
			"BW\n");*/
	PRINTF("dot11_40mhz_frame_received_count:       %u\n",
			mib->dot11_40mhz_frame_received_count);

	/*printf("MIB element to count the number of frames received at 80 MHz "
			"BW\n");*/
	/*printf("dot11_80mhz_frame_received_count:       %u\n",
			mib->dot11_80mhz_frame_received_count);*/

	/*printf("MIB element to count the number of frames received at 160 MHz "
			"BW\n");*/
	/*printf("dot11_160mhz_frame_received_count:      %u\n",
			mib->dot11_160mhz_frame_received_count);*/

	/*printf("MIB element to count the number of attempts made to acquire a "
			"20 MHz TXOP\n");*/
	PRINTF("nx_20mhz_failed_txop:                   %u\n",
			mib->nx_20mhz_failed_txop);

	/*printf("MIB element to count the number of successful acquisitions of "
				"a 20 Mhz TXOP\n");*/
	PRINTF("nx_20mhz_successful_txops:              %u\n",
			mib->nx_20mhz_successful_txops);

	/*printf("MIB element to count the number of attempts made to acquire a "
			"40 MHz TXOP\n");*/
	PRINTF("nx_40mhz_failed_txop:                   %u\n",
			mib->nx_40mhz_failed_txop);

	/*printf("MIB element to count the number of successful acquisitions of "
				"a 40 Mhz TXOP\n");*/
	PRINTF("nx_40mhz_successful_txops:              %u\n",
			mib->nx_40mhz_successful_txops);

	/*printf("MIB element to count the number of attempts made to acquire a "
			"80 MHz TXOP\n");*/
	/*printf("nx_80mhz_failed_txop:                   %u\n",
			mib->nx_80mhz_failed_txop);*/

	/*printf("MIB element to count the number of successful acquisitions of "
				"a 80 Mhz TXOP\n");*/
	/*printf("nx_80mhz_successful_txops:              %u\n",
			mib->nx_80mhz_successful_txops);*/

	/*printf("MIB element to count the number of attempts made to acquire a "
			"160 MHz TXOP\n");*/
	/*printf("nx_160mhz_failed_txop:                  %u\n",
			mib->nx_160mhz_failed_txop);*/

	/*printf("MIB element to count the number of successful acquisitions of "
				"a 160 Mhz TXOP\n");*/
	/*printf("nx_160mhz_successful_txops:             %u\n",
			mib->nx_160mhz_successful_txops);*/

	/*printf("Count the number of BW drop using dynamic BW management\n");*/
	PRINTF("nx_dyn_bw_drop_count:                   %u\n",
			mib->nx_dyn_bw_drop_count);

	/*printf("Count the number of failure using static BW management\n");*/
	PRINTF("nx_static_bw_failed_count:              %u\n",
			mib->nx_static_bw_failed_count);

	/*printf("MIB element to count the number of times the dual CTS fails\n");*/
	PRINTF("dot11_dualcts_success_count:            %u\n",
			mib->dot11_dualcts_success_count);

	/*printf("MIB element to count the number of times the AP does not "
			"detect a collision PIFS after transmitting a STBC CTS "
			"frame\n");*/
	PRINTF("dot11_stbc_cts_success_count:           %u\n",
			mib->dot11_stbc_cts_success_count);

	/*printf("MIB element to count the number of times the AP detects a "
			"collision PIFS after transmitting a STBC CTS frame\n");*/
	PRINTF("dot11_stbc_cts_failure_count:           %u\n",
			mib->dot11_stbc_cts_failure_count);

	/*printf("MIB element to count the number of times the AP does not "
			"detect a collision PIFS after transmitting a non-STBC "
			"CTS frame\n");*/
	PRINTF("dot11_non_stbc_cts_success_count:       %u\n",
			mib->dot11_non_stbc_cts_success_count);

	/*printf("MIB element to count the number of times the AP detects a "
			"collision PIFS after transmitting a non-STBC CTS "
			"frame\n");*/
	PRINTF("dot11_non_stbc_cts_failure_count:       %u\n",
			mib->dot11_non_stbc_cts_failure_count);

	if (usage_en) {
		PRINTF("Usage: %s\n", argv[0]);
		PRINTF("       %s [h: hold]\n", argv[0]);
	}

	if (reset_en)
		lmac_mib_table_reset_setf(1);
}

void cmd_lmac_ldpc(int argc, char *argv[])
{
	if (!cmd_lmac_tx_flag) {
		return ;
	}

	if (argc == 2) {
		ldpc_enabled = atoi(argv[1]);
	}

	PRINTF("ldpc_enabled = %d\n", ldpc_enabled);
}

#ifdef SUPPORT_ADC_CAL_TEST
unsigned long adc_dout_delta_boot[2000];
unsigned long *adc_dout_delta = adc_dout_delta_boot;
long adc_dout_delta_cnt = 0;
void cmd_lmac_adc(int argc, char *argv[])
{
	//static unsigned long adc_dout_delta[ADC_CAL_TEST_CNT];
	unsigned long fadc_cal1, cal_done;
	unsigned char boot_en = 0;
	int i, n = 2000, c = 100;

	if (!cmd_lmac_tx_flag) {
		return ;
	}

	switch (argc) {
	case 1:
		break;
	case 3:
		n = atoi(argv[2]);
	case 2:
		if (strcmp("boot", argv[1]) == 0) {
			boot_en = 1;
		} else {
			c = atoi(argv[1]);
		}
	default:
	}

	/* APP'memory is not enouth. */
		//adc_dout_delta = pvPortMalloc(sizeof(unsigned long) * n);
	if (!boot_en) {
		adc_dout_delta = pvPortMalloc(sizeof(unsigned long) * n);
	}

	if (adc_dout_delta == NULL) {
		PRINTF("Not enought memory!");
		return;
	}

	if (!boot_en) {
		/* MPW BUG!!! MPW's CAL need to restart. */
		/* CAL start */
		fadc_cal1 = DA16200_FDAC14->FADC_CAL1;
		/*fadc_cal1 |= 0x20000000;
		DA16200_FDAC14->FADC_CAL1 = fadc_cal1;*/

		/* CAL clear */
		/*fadc_cal1 &= 0xdfffffff;
		DA16200_FDAC14->FADC_CAL1 = fadc_cal1;*/

		/* CAL restart */
		fadc_cal1 |= 0x20000000;
		DA16200_FDAC14->FADC_CAL1 = fadc_cal1;

		/* CAL done */
		do {
			for (volatile int k = 0; k < c; k++);

			adc_dout_delta[adc_dout_delta_cnt++] =
				DA16200_FDAC14->dout_delta;
			cal_done = DA16200_FDAC14->cal_done_i;
		} while (((cal_done & 0x00030000) != 0x00030000) && (adc_dout_delta_cnt < n));
	}

	for (i = 0; i < adc_dout_delta_cnt && i < n; i++) {
		PRINTF("%s::%4d %08x\n", argv[0], i, adc_dout_delta[i]);
	}

	if (!boot_en)
		vPortFree(adc_dout_delta);
}
#endif

extern void cmd_lmac_ops_start(int argc, char *argv[]);
extern void cmd_lmac_ops_stop(int argc, char *argv[]);
extern void cmd_lmac_rx_state(int argc, char *argv[]);
extern void cmd_lmac_rx_mib(int argc, char *argv[]);
extern void cmd_lmac_dbg_mac(int argc, char *argv[]);
extern void cmd_phy_rc_isr_test(int argc, char *argv[]);
extern void set_mac_clock(int argc, char *argv[]);
extern void cmd_lmac_rf_cal(int argc, char *argv[]);
extern void cmd_lmac_rf_dpd(int argc, char *argv[]);
extern void cmd_lmac_mac_recover(int argc, char *argv[]);
//-----------------------------------------------------------------------
// Command NET-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE cmd_lmac_tx_list[] = {
 	/* Head */
	{ "LMAC_TX",	CMD_MY_NODE,	&cmd_lmac_tx_list[0], NULL, "lmac_tx"},

	{ "-------",	CMD_FUNC_NODE,	NULL,	NULL,	"----------------------------"},

	{ "init",		CMD_FUNC_NODE,	NULL,	cmd_lmac_tx_init,		"init (type addr) - lmac tx init"		},
	{ "start",		CMD_FUNC_NODE,	NULL,	cmd_lmac_ops_start,		"lmac ops start"						},
	{ "stop",		CMD_FUNC_NODE,	NULL,	cmd_lmac_ops_stop,		"lmac ops stop"							},
	{ "ch",			CMD_FUNC_NODE,	NULL,	cmd_lmac_tx_ch,			"channel"								},
	{ "state",		CMD_FUNC_NODE,	NULL,	cmd_lmac_rx_state,		"state"									},
	{ "dpm_mib",	CMD_FUNC_NODE,	NULL,	cmd_lmac_dpm_mib,		"MIB RCV for DPM Test "					},
	{ "power2",		CMD_FUNC_NODE,	NULL,	cmd_lmac_tx_power2,		"set power2 level"						},
	{ "txp",		CMD_FUNC_NODE,  NULL,   cmd_lmac_txp,			"tx power measurement"					},
	{ "txpwr",		CMD_FUNC_NODE,  NULL,   cmd_lmac_txp,			"tx power measurement for fixed power"	},
	{ "txp1m",		CMD_FUNC_NODE,  NULL,   cmd_lmac_txp,			"tx power measurement for 11b 1Mbps"	},
	{ "txp6m",		CMD_FUNC_NODE,  NULL,   cmd_lmac_txp,			"tx power measurement for 11g 6Mbps"	},
	{ "txpm0",		CMD_FUNC_NODE,  NULL,   cmd_lmac_txp,			"tx power measurement for 11n MCS0"		},
#ifdef SUPPORT_SEND_NULL_FRAME
	{"null",		CMD_FUNC_NODE,	NULL,	cmd_lmac_null, 			"send a null frame to AP"				},
#endif
	{ "vec",		CMD_FUNC_NODE,	NULL,	cmd_lmac_test_vec,		"lmac test vector test [vec][num][rate][power][length][cont][ch]"	},
	{ "status",		CMD_FUNC_NODE,	NULL,	cmd_lmac_status,		"dma status"							},
	{ "pt_rate",	CMD_FUNC_NODE,	NULL,	set_pt_rate,			"set policy table rate [0:4] value"		},
	{ "mac_clock",	CMD_FUNC_NODE,	NULL,	set_mac_clock,			"set mac clock 20MHz(20) or 40MHz(40)"	},
	{ "rftx",		CMD_FUNC_NODE,  NULL,   cmd_lmac_rftx,			"rftx"									},
	{ "rfcwtest",	CMD_FUNC_NODE,  NULL,   cmd_lmac_rfcwtest,		"rfcwtest"								},
	{ "rfcwstop",	CMD_FUNC_NODE,  NULL,   cmd_lmac_rfcwstop,		"rfcwtest stop"							},
	{ "rfcwpll",	CMD_FUNC_NODE,  NULL,   cmd_lmac_rf_cw_pll,		"rfcwpll set/reset"  					},
	{ "mib",		CMD_FUNC_NODE,	NULL,	cmd_lmac_rx_mib,		"mib"									} ,
	{ "cal_req",	CMD_FUNC_NODE,	NULL,	cmd_lmac_rf_cal,		"RF CAL"								},
	{ "dpd",		CMD_FUNC_NODE,	NULL,	cmd_lmac_rf_dpd,		"RF DPD"								},
	{ "mac_recover",CMD_FUNC_NODE,	NULL,	cmd_lmac_mac_recover,	"MAC reset"								},
#if NX_AP_PS
	{ "ap_ps",		CMD_FUNC_NODE,	NULL,	cmd_lmac_ap_ps,			"AP PS mode"							},
#endif
	{ "cont_tx",	CMD_FUNC_NODE,	NULL,	cmd_lmac_cont_tx,		"CONT. TX"								},
	{ "dbg",		CMD_FUNC_NODE,	NULL,	cmd_lmac_dbg_mac,		"debug options"							},
	{ "radar",		CMD_FUNC_NODE,	NULL,	cmd_phy_rc_isr_test,	"radar isr"								},
	{ "ldpc",		CMD_FUNC_NODE,	NULL,	cmd_lmac_ldpc,			"ldpc mode"								},
#ifdef SUPPORT_ADC_CAL_TEST
	{ "adc",		CMD_FUNC_NODE,	NULL,	cmd_lmac_adc,			"ADC Test"								},
#endif

/* Tail */
	{ NULL, CMD_NULL_NODE, NULL, NULL, NULL	}
};

/* EOF */
