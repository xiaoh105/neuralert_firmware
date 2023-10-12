/**
 ****************************************************************************************
 *
 * @file command_lmac.c
 *
 * @brief Wi-Fi Lower-MAC verify and test console commands
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


#include "sdk_type.h"

#include <stdio.h>
#include <stdlib.h>

#include "command.h"
#include "command_lmac.h"
#include "command_lmac_tx.h"
#include "da16x_system.h"

#include "romac_primitives_patch.h"
#include "timp_patch.h"
#include "common_def.h"

#undef SUPPORT_BCN_RX_CANCELLATION
#undef TIMP_CMD_EN


int	duration = 1000;
int	total_received_packets = 0;
int	total_errored_packet = 0;
int	errored_packet = 0;
int	received_packet = 0;
int	phy_error_acc = 0;
int	rx_ovfl_acc = 0;
int	phy_error = 0;
int	rx_ovfl = 0;
unsigned int	pktperduration = 10;
unsigned int	timer_count = 0;
unsigned int	bytesperpkt = 1000;
unsigned int	timer_reset_flag = 0;
unsigned int	per_loop = 0;

TimerHandle_t	timer;


#define	RWNX_LMAC_MIB_ADDRESS			0x60B00800
#define	RWNX_LMAC_MIB_FCSERROR			0x60B00804
#define	RWNX_LMAC_MIB_RX_PHYERROR		0x60B00808
#define	RWNX_LMAC_MIB_RX_FIFO_OVFL		0x60B0080C
#define	RWNX_LMAC_MIB_RX_FRAME_CNT		0x60B00B80
#define	RWNX_MODEM_AGCSTAT				0x60c00060

#define	MAC_CONTROL_REG1				0x60B0004c


//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------

const char terminal_color_red[6]	= { 27, '[', '9', '1', 'm', 0 };
const char terminal_color_normal[4] = { 27, '[', 'm', 0 };
const char terminal_color_blue[6]	= { 27, '[', '9', '4', 'm', 0 };
const char terminal_color_yellow[6]	= { 27, '[', '9', '3', 'm', 0 };

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
#ifdef SUPPORT_BCN_RX_CANCELLATION
extern unsigned int htoi(char *s);
#endif	//SUPPORT_BCN_RX_CANCELLATION

extern unsigned char cmd_lmac_flag;

char terminal_clear[5]			= { 27, '[', '1', 'J', 0 };
char terminal_top_left[5]		= { 27, '[', '1', 'H', 0 };
char terminal_save_cursor[4]	= { 27, '[', 's', 0};
char terminal_restore_cursor[4] = { 27, '[', 'u', 0};

TaskHandle_t	per_thread_type;
unsigned int    *per_thread_stack;
unsigned int    per_thread_stacksize = 128*sizeof(unsigned int);
EventGroupHandle_t  per_mutex;

extern void lmac_mib_table_reset_setf(uint8_t value);

void get_per(UINT32 reset, UINT32 *pass, UINT32 *fcs, UINT32 *phy, UINT32 *overflow)
{
	if (reset) {
		lmac_mib_table_reset_setf(1);
		return;
	}

	*pass = *((unsigned int *)(RWNX_LMAC_MIB_RX_FRAME_CNT));
	*fcs = *((unsigned int *)(RWNX_LMAC_MIB_FCSERROR));
	*phy = *((unsigned int *)(RWNX_LMAC_MIB_RX_PHYERROR));
	*overflow = *((unsigned int *)(RWNX_LMAC_MIB_RX_FIFO_OVFL));
}

void command_per_thread(void *pvParameters)
{
	DA16X_UNUSED_ARG(pvParameters);

	float  per = 0., per_total = 0., per_fixed=0., per_total_fixed=0., pkt_estimated=0.;
	unsigned int rcvd = 0, errored = 0, phy_error_tot = 0, rx_ovfl_tot = 0;
	UNSIGNED vga_gain = 0, lna_gain = 0;

	per_mutex = xEventGroupCreate();
	if (per_mutex == NULL)
	{
		PRINTF("[%s] xEventGroupCreate err( Heap memory check).\n", __func__);
		return;
	}

	while(1)
	{
		xEventGroupWaitBits(per_mutex, 1 , pdTRUE, pdFALSE, portMAX_DELAY);    // Wait
		if (timer_reset_flag)
		{
			total_received_packets = 0; total_errored_packet = 0; timer_count = 0, phy_error_acc = 0, rx_ovfl_acc = 0;
			*((volatile uint32_t *)(MAC_CONTROL_REG1)) = (*((volatile uint32_t *)(MAC_CONTROL_REG1)) + 0x1000 );
			timer_reset_flag = 0;
			continue;
		}
		timer_count++;
		if (per_loop == 1)
		{
			continue;
		}
		else if (per_loop > 1) per_loop--;


		get_per(0, &rcvd, &errored, &phy_error_tot, &rx_ovfl_tot);
		vga_gain = *((unsigned int *)(RWNX_MODEM_AGCSTAT));
		lna_gain = (vga_gain>>5)%0x3;
		vga_gain = vga_gain & 0x1f;

		received_packet = (int)rcvd - total_received_packets;
		errored_packet = (int)errored - total_errored_packet;
		total_received_packets = (int)rcvd ;
		total_errored_packet = (int)errored;

		phy_error = (int)phy_error_tot - phy_error_acc;
		rx_ovfl = (int)rx_ovfl_tot - rx_ovfl_acc;
		phy_error_acc = (int)phy_error_tot;
		rx_ovfl_acc = (int)rx_ovfl_tot;

		if (phy_error<0)
			phy_error=0;
		if (rx_ovfl<0)
			rx_ovfl=0;
		if (received_packet<0)
			received_packet=0;
		if (errored_packet<0)
			errored_packet=0;

		if (received_packet+errored_packet+phy_error+rx_ovfl) {
			per = (float)(errored_packet+phy_error+rx_ovfl) / (float)(received_packet+errored_packet+phy_error+rx_ovfl);
		} else {
			per = 999.0f;
		}

		if (total_received_packets+total_errored_packet+phy_error_acc+rx_ovfl_acc) {
			per_total = (float)(total_errored_packet+phy_error_acc+rx_ovfl_acc) / (float)(total_received_packets+total_errored_packet+phy_error_acc+rx_ovfl_acc);
		} else {
			per_total = 999.0f;
		}

		per_fixed = ((float)pktperduration  - (float)received_packet) / (float)pktperduration;
		pkt_estimated = (float)(timer_count*pktperduration);
		per_total_fixed = (float)(pkt_estimated - (float)total_received_packets) /pkt_estimated;

		PRINTF("%s%s",terminal_clear,terminal_top_left);
		PRINTF("phyerroracc = %8d , phyerror     = %8d\n",phy_error_acc, phy_error);
		PRINTF("errored acc = %8d , errored pkts = %8d\n",total_errored_packet, errored_packet);
		PRINTF("rxovflowacc = %8d , rxovflow     = %8d\n",rx_ovfl_acc, rx_ovfl);
		PRINTF("passed acc  = %8d , passed pkts  = %8d\n",total_received_packets, received_packet);
		PRINTF("rcvd acc    = %8d , rcvd total   = %8d\n",total_received_packets+total_errored_packet+phy_error_acc+rx_ovfl_acc, received_packet+errored_packet+phy_error+rx_ovfl);
		PRINTF("vga_gain    = %8d , lna_gain     = %8d\n",vga_gain, lna_gain);

		if (per_total > 0.1f || per_total == 999.0f)
			PRINTF("%s",terminal_color_red);
		else
			PRINTF("%s",terminal_color_blue);
		PRINTF("per acc     = %1.6f%s ,",(double)per_total, terminal_color_normal);

		if (per > 0.1f || per == 999.0f)
			PRINTF("%s",terminal_color_red);
		else
			PRINTF("%s",terminal_color_blue);
		PRINTF(" per          = %1.6f%s\n",(double)per,terminal_color_normal);

		if (per_total_fixed > 0.1f || per_total_fixed == 999.0f)
			PRINTF("%s",terminal_color_red);
		else
			PRINTF("%s",terminal_color_blue);
		PRINTF("perfixedacc =%1.6f%s ,",(double)per_total_fixed, terminal_color_normal);

		if (per_fixed > 0.1f || per_fixed == 999.0f)
			PRINTF("%s",terminal_color_red);
		else
			PRINTF("%s",terminal_color_blue);
		PRINTF(" per fixed    = %1.6f%s\n\n",(double)per_fixed,terminal_color_normal);

		PRINTF("measure time= %8d , pkt/duration = %8d , throughput   = %3dMbps\n",duration, pktperduration, (unsigned int)received_packet*bytesperpkt*8/1000000);

	}
}

static void per_timer_callback( TimerHandle_t  xTimer )
{
	DA16X_UNUSED_ARG(xTimer);

	xEventGroupSetBits(per_mutex, 1);// Set
}

void cmd_lmac_per(int argc, char *argv[])
{
	static unsigned int is_per_timer_started = 0, is_per_theread_started = 0;;

	if (!cmd_lmac_flag) {
		return ;
	}

	if (argc == 1) {
		if (!is_per_timer_started) {
			PRINTF("START\n");

			if (!is_per_theread_started) {
				xTaskCreate(command_per_thread, "@perthread", (uint16_t)per_thread_stacksize, 0, OS_TASK_PRIORITY_SYSTEM+9,  &per_thread_type);
				is_per_theread_started = 1;
			}

			timer = xTimerCreate("per_tick", pdMS_TO_TICKS( duration ), pdTRUE, (void *)0, per_timer_callback);
			if (timer == NULL) {
				PRINTF(" [%s] timer create error \n", __func__);
			}

			if ( xTimerStart( timer, 0 ) != pdPASS ) {
				/* The timer could not be set into the Active
				state. */
				PRINTF(" [%s] The timer could not be set into the Active state.  0x%x\n", __func__, timer);
			}

			is_per_timer_started = 1;
			per_loop = 0;
			timer_reset_flag = 1;
		} else {
			if (is_per_timer_started) {
				xTimerDelete( timer, 0 );
				is_per_timer_started = 0;
			}
		}
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "start")) {
			if (!is_per_timer_started) {
				PRINTF("START\n");

				if (!is_per_theread_started) {
					xTaskCreate(command_per_thread, "@perthread", (uint16_t)per_thread_stacksize, 0, OS_TASK_PRIORITY_SYSTEM+9,  &per_thread_type);
				    is_per_theread_started = 1;
				}
				per_loop = 0;
				timer_reset_flag = 1;

				timer = xTimerCreate("per_tick", pdMS_TO_TICKS( duration ), pdTRUE, (void *)0, per_timer_callback);
				if (timer == NULL) {
					PRINTF(" [%s] timer create error \n", __func__);
				}

	            if ( xTimerStart( timer, 0 ) != pdPASS ) {
	                /* The timer could not be set into the Active
	                state. */
	            	PRINTF(" [%s] The timer could not be set into the Active state.  0x%x\n", __func__, timer);
	            }
				is_per_timer_started = 1;
			}
		} else if (!strcmp(argv[1], "stop")) {
			if (is_per_timer_started) {
				xTimerDelete( timer, 0 );
				is_per_timer_started = 0;
			}
		} else if (!strcmp(argv[1], "reset")) {
			timer_reset_flag = 1;
		} else if (!strcmp(argv[1], "rifs")) {
			if (*((volatile uint32_t *)(MAC_CONTROL_REG1)) & 0x04000000)
				*((volatile uint32_t *)(MAC_CONTROL_REG1)) = *((volatile uint32_t *)(MAC_CONTROL_REG1)) &(uint32_t)(~(0x04000000));
			else
				*((volatile uint32_t *)(MAC_CONTROL_REG1)) = *((volatile uint32_t *)(MAC_CONTROL_REG1)) + 0x04000000;

		} else if (!strcmp(argv[1], "1")) {
			per_loop = 2;
			if (is_per_timer_started) {
				PRINTF("START\n");
				xTimerDelete( timer, 0 );
			} else {
				if (!is_per_theread_started) {
				    xTaskCreate(command_per_thread, "@perthread", (uint16_t)per_thread_stacksize, 0, OS_TASK_PRIORITY_SYSTEM+9,  &per_thread_type);
				    is_per_theread_started = 1;
				}
			}
			timer_reset_flag = 1;

			timer = xTimerCreate("per_tick", pdMS_TO_TICKS( duration ), pdTRUE, (void *)0, per_timer_callback);
			if (timer == NULL) {
				PRINTF(" [%s] timer create error \n", __func__);
			}

			if ( xTimerStart( timer, 0 ) != pdPASS ) {
				/* The timer could not be set into the Active
				state. */
				PRINTF(" [%s] The timer could not be set into the Active state.  0x%x\n", __func__, timer);
			}

			is_per_timer_started = 1;
		} else {
			PRINTF("Invalid Command\n");
		}
	} else if (argc == 3) {
		if (!strcmp(argv[1], "duration")) {
			duration = atoi(argv[2]);
			if (duration == 0)
				duration = 1000;

			if (is_per_timer_started) {
				xTimerDelete( timer, 0 );

				timer = xTimerCreate("per_tick", pdMS_TO_TICKS( duration ), pdTRUE, (void *)0, per_timer_callback);
				if (timer == NULL) {
					PRINTF(" [%s] timer create error \n", __func__);
				}

	            if ( xTimerStart( timer, 0 ) != pdPASS ) {
	                /* The timer could not be set into the Active
	                state. */
	            	PRINTF(" [%s] The timer could not be set into the Active state.  0x%x\n", __func__, timer);
	            }

				is_per_timer_started = 1;
			}
		} else if (!strcmp(argv[1], "pkt")) {
			pktperduration = (unsigned int)(atoi(argv[2]));
			if (pktperduration == 0)
				pktperduration = 10;
			timer_reset_flag = 1;
		} else if (!strcmp(argv[1], "len")) {
			bytesperpkt = (unsigned int)(atoi(argv[2]));
			if (bytesperpkt == 0)
				bytesperpkt = 1000;
			timer_reset_flag = 1;
		} else if (!strcmp(argv[1], "loop")) {
			per_loop = (unsigned int)(atoi(argv[2]));
			if (is_per_timer_started) {
				PRINTF("START\n");
				xTimerDelete( timer, 0 );
			} else {
				if (!is_per_theread_started) {
				    xTaskCreate(command_per_thread, "@perthread", (uint16_t)per_thread_stacksize, 0, OS_TASK_PRIORITY_SYSTEM+9,  &per_thread_type);
				    is_per_theread_started = 1;
				}
			}
			timer_reset_flag = 1;

			timer = xTimerCreate("per_tick", pdMS_TO_TICKS( duration ), pdTRUE, (void *)0, per_timer_callback);
			if (timer == NULL) {
				PRINTF(" [%s] timer create error \n", __func__);
			}

			if ( xTimerStart( timer, 0 ) != pdPASS ) {
				/* The timer could not be set into the Active
				state. */
				PRINTF(" [%s] The timer could not be set into the Active state.  0x%x\n", __func__, timer);
			}
			is_per_timer_started = 1;

		} else {
			PRINTF("Invalid Command\n");
		}
	}
}

#ifdef SUPPORT_BCN_RX_CANCELLATION
#define REG_PL_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)
#define REG_PL_RD(addr)              (*(volatile uint32_t *)(addr))

#define NXMAC_BCNFRMSTDYNAMIC          0x60B00564
#define NXMAC_TIMESTAMP_0              0x60B00568
#define NXMAC_TIMESTAMP_1              0x60B0056C
#define NXMAC_PARTIALBCNAID            0x60B00570
#define NXMAC_PARTIALBCNRXEN           0x60B00570
#define NXMAC_BCNFRMSTCLR              0x60B00574
#define NXMAC_BCNFRMSTSTAIC            0x60B00578
#define NXMAC_BCNINTMASK               0x60B0057C
#define NXMAC_BCN_RX_CANCELLATION_IRQn phy_intRxBcn_n

/// SLR features
#define NXMAC_FCI_PBR_MON_TSF_DATA_START_L       0x60B00650
#define NXMAC_FCI_PBR_MON_TSF_DATA_START_H       0x60B00654
#define NXMAC_FCI_PBR_MON_TSF_TIM_PARSING_DONE_L 0x60B00658
#define NXMAC_FCI_PBR_MON_TSF_TIM_PARSING_DONE_H 0x60B0065C

#define NXMAC_FCI_MON_TA_RSSI_CFG_TA_L 0x60B00630
#define NXMAC_FCI_MON_TA_RSSI_CFG_TA_H 0x60B00634
#define NXMAC_FCI_MON_TA_RSSI_CFG_CTRL 0x60B00638
#define NXMAC_FCI_MON_TA_RSSI_MON0     0x60B0063C ///< 11b Data
#define NXMAC_FCI_MON_TA_RSSI_MON1     0x60B00640 ///< 11g Data
#define NXMAC_FCI_MON_TA_RSSI_MON2     0x60B00644 ///< 11n Data
#define NXMAC_FCI_MON_TA_RSSI_MONX     0x60B00648

volatile unsigned char bcn_rx_cancellation_polling_en = 0;
volatile unsigned char bcnc_dump_en = 0;
volatile unsigned long bcnc_status = 0;
volatile unsigned long bcnc_dyn_status = 0;
volatile unsigned long bcnc_data_start = 0;
volatile unsigned long bcnc_parsing_done = 0;
volatile unsigned long bcnc_rssi_mon0 = 0;
volatile signed char   bcnc_rssi_mon0_max = 0;
volatile signed char   bcnc_rssi_mon0_min = 0;
volatile signed char   bcnc_rssi_mon0_avg = 0;

unsigned long nxmac_get_bcn_frmst()
{
	return REG_PL_RD(NXMAC_BCNFRMSTSTAIC);
}

unsigned long nxmac_get_bcn_int_mask()
{
	return REG_PL_RD(NXMAC_BCNINTMASK);
}

void nxmac_set_bcn_int_mask(unsigned long mask)
{
	REG_PL_WR(NXMAC_BCNINTMASK, mask);
}

unsigned long long nxmac_get_timestamp()
{
	unsigned long long timestamp;
	timestamp = REG_PL_RD(NXMAC_TIMESTAMP_1);

	return (timestamp << 32LL) | REG_PL_RD(NXMAC_TIMESTAMP_0);
}

unsigned short nxmac_get_aid()
{
	return REG_PL_RD(NXMAC_PARTIALBCNAID);
}

void nxmac_set_aid(unsigned short aid)
{
	unsigned long en;
	en = REG_PL_RD(NXMAC_PARTIALBCNAID) & 0x80000000;
	en |= aid;
	REG_PL_WR(NXMAC_PARTIALBCNAID, en);
}

unsigned long nxmac_get_partial_bcn_rx_en()
{
	return !!(REG_PL_RD(NXMAC_PARTIALBCNRXEN) & 0x80000000);
}

void nxmac_set_partial_bcn_rx_en()
{
	unsigned long bcn_rx_en = REG_PL_RD(NXMAC_PARTIALBCNRXEN) | 0x80000000;
	REG_PL_WR(NXMAC_PARTIALBCNRXEN, bcn_rx_en);
}

void nxmac_reset_partial_bcn_rx_en()
{
	unsigned long bcn_rx_en = REG_PL_RD(NXMAC_PARTIALBCNRXEN) & 0x7fffffff;
	REG_PL_WR(NXMAC_PARTIALBCNRXEN, bcn_rx_en);
}

void nxmac_clear_bcn_frmstclr(unsigned long bcn_frmstclr)
{
	REG_PL_WR(NXMAC_BCNFRMSTCLR, bcn_frmstclr);
}

unsigned long nxmac_fci_pbr_mon_tsf_data_start_l()
{
	return REG_PL_RD(NXMAC_FCI_PBR_MON_TSF_DATA_START_L);
}

unsigned long nxmac_fci_pbr_mon_tsf_data_start_h()
{
	return REG_PL_RD(NXMAC_FCI_PBR_MON_TSF_DATA_START_H);
}

unsigned long nxmac_fci_pbr_mon_tsf_tim_parsing_done_l()
{
	return REG_PL_RD(NXMAC_FCI_PBR_MON_TSF_TIM_PARSING_DONE_L);
}

unsigned long nxmac_fci_pbr_mon_tsf_tim_parsing_done_h()
{
	return REG_PL_RD(NXMAC_FCI_PBR_MON_TSF_TIM_PARSING_DONE_H);
}

unsigned long nxmac_get_fci_mon_ta_rssi_cfg_ta_l()
{
	return REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_TA_L);
}

void nxmac_set_fci_mon_ta_rssi_cfg_ta_l(unsigned long rssi_cfg_ta_l)
{
	REG_PL_WR(NXMAC_FCI_MON_TA_RSSI_CFG_TA_L, rssi_cfg_ta_l);
}

unsigned long nxmac_get_fci_mon_ta_rssi_cfg_ta_h()
{
	return REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_TA_H);
}

void nxmac_set_fci_mon_ta_rssi_cfg_ta_h(unsigned long rssi_cfg_ta_h)
{
	REG_PL_WR(NXMAC_FCI_MON_TA_RSSI_CFG_TA_H, rssi_cfg_ta_h);
}

unsigned long nxmac_get_cfg_monrxtarssi_en()
{
	return (REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL) & 0x00000001);
}

void nxmac_set_cfg_monrxtarssi_en()
{
	unsigned long cfg_monrxtarssi_en =
		REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL) | 0x00000001;
	REG_PL_WR(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL, cfg_monrxtarssi_en);
}

void nxmac_reset_cfg_monrxtarssi_en()
{
	unsigned long cfg_monrxtarssi_en =
		REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL) & 0xfffffffe;
	REG_PL_WR(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL, cfg_monrxtarssi_en);
}

unsigned long nxmac_get_cfg_monrxtarssi_mode_onlyfcsok()
{
	return (REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL) & 0x00000010);
}

void nxmac_set_cfg_monrxtarssi_mode_onlyfcsok()
{
	unsigned long cfg_monrxtarssi_mode_onlyfcsok =
		REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL) | 0x00000010;
	REG_PL_WR(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL, cfg_monrxtarssi_mode_onlyfcsok);
}

void nxmac_reset_cfg_monrxtarssi_mode_onlyfcsok()
{
	unsigned long cfg_monrxtarssi_mode_onlyfcsok =
		REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL) & 0xffffffef;
	REG_PL_WR(NXMAC_FCI_MON_TA_RSSI_CFG_CTRL, cfg_monrxtarssi_mode_onlyfcsok);
}

unsigned long nxmac_get_fci_mon_ta_rssi_mon0()
{
	return REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MON0);
}

unsigned long nxmac_get_fci_mon_ta_rssi_mon1()
{
	return REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MON1);
}

unsigned long nxmac_get_fci_mon_ta_rssi_mon2()
{
	return REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MON2);
}

unsigned long nxmac_get_fci_mon_ta_rssi_monx()
{
	return REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MONX);
}

signed char nxmac_get_mon_monrxtarssi0_rssimax()
{
	return (REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MON0) & 0x00ff0000) >> 16;
}

signed char nxmac_get_mon_monrxtarssi0_rssimin()
{
	return (REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MON0) & 0x0000ff00) >> 8;
}

signed char nxmac_get_mon_monrxtarssi0_rssiavg()
{
	return (REG_PL_RD(NXMAC_FCI_MON_TA_RSSI_MON0) & 0x000000ff);
}

static char* lltoa(long long val, int base)
{
	static char lookup[] = {
			'0', '1', '2', '3', '4', '5', '6', '7', '8',
			'9', 'a', 'b', 'c', 'd', 'e', 'f'};
	static char buf[24] = {0};

	int i = 22;
	int sign = (val < 0);

	if (sign)
		val = -val;

	if (val == 0) {
		buf[22] = '0';
		return &buf[i];
	}

	for (; val && i ; --i, val /= base)
		buf[i] = lookup[val % base];

	if (sign)
		buf[i--] = '-';

	return &buf[i + 1];
}

void _onion_bcn_rx_cancellation_int_handler()
{
	bcnc_dyn_status = REG_PL_RD(0x60b00564);
	bcnc_status = nxmac_get_bcn_frmst();
	nxmac_clear_bcn_frmstclr(bcnc_status);
	bcn_rx_cancellation_polling_en = 1;
	bcnc_data_start = nxmac_fci_pbr_mon_tsf_data_start_l();
	bcnc_parsing_done = nxmac_fci_pbr_mon_tsf_tim_parsing_done_l();

	bcnc_rssi_mon0 = nxmac_get_fci_mon_ta_rssi_mon0();
	bcnc_rssi_mon0_max = nxmac_get_mon_monrxtarssi0_rssimax();
	bcnc_rssi_mon0_min = nxmac_get_mon_monrxtarssi0_rssimin();
	bcnc_rssi_mon0_avg = nxmac_get_mon_monrxtarssi0_rssiavg();
}

static void _tx_lmac_mac_bcn_rx_cancellation(void)
{
	INTR_CNTXT_CALL(_onion_bcn_rx_cancellation_int_handler);
}

void cmd_lmac_bcnc(int argc, char *argv[])
{
	unsigned char status_en = 0;
	unsigned char help_en = 0;
	unsigned char en = 0;
	unsigned char di = 0;
	unsigned char clr_en = 0;
	unsigned char aid_en = 0;
	unsigned char status_polling_en = 0;
	unsigned char interrupt_en = 0;
	unsigned char interrupt_di = 0;
	unsigned char pbr_status_en = 0;
	unsigned char pbr_status_di = 0;
	unsigned char bcn_en = 0;
	unsigned char bcn_di = 0;
	unsigned char aging_en = 0;
	unsigned char rssi_mon_en = 0;
	unsigned char rssi_mon_di = 0;
	unsigned long stclr = 0xffffffff;
	unsigned long aid = 0;
	unsigned long current_st;
	unsigned long current_st_AfterClr;
	unsigned long rmon_ta_l = 0;
	unsigned long rmon_ta_h = 0;

	switch (argc) {
	case 1:
		status_en = 1;
		help_en = 1;
		break;
	case 2:
		if (strcmp(argv[1], "en=0") == 0)
			di = 1;
		else if (strcmp(argv[1], "en=1") == 0)
			en = 1;
		else if (strcmp(argv[1], "p") == 0)
			status_polling_en = 1;
		else if (strcmp(argv[1], "i") == 0)
			interrupt_en = 1;
		else if (strcmp(argv[1], "d") == 0)
			interrupt_di = 1;
		else if (strcmp(argv[1], "si") == 0)
			pbr_status_en = 1;
		else if (strcmp(argv[1], "sd") == 0)
			pbr_status_di = 1;
		else if (strcmp(argv[1], "aging") == 0)
			aging_en = 1;
		else if (strcmp(argv[1], "c") == 0)
			clr_en = 1;
		else if (strcmp(argv[1], "rmon") == 0)
			rssi_mon_di = 1;
		else
			help_en = 1;
		break;
	case 3:
		if (strcmp(argv[1], "c") == 0) {
			clr_en = 1;
			stclr = htoi(argv[2]);
		} else if (strcmp(argv[1], "a") == 0) {
			aid_en = 1;
			aid = atoi(argv[2]);
		} else {
			help_en = 1;
		}
		break;
	case 4:
		if (strcmp(argv[1], "rmon") == 0) {
			rssi_mon_en = 1;
			rmon_ta_l = htoi(argv[2]);
			rmon_ta_h = htoi(argv[3]);
		} else {
			help_en = 1;
		}
		break;
	}

	current_st = nxmac_get_bcn_frmst();

	if (status_polling_en) {
		PRINTF("%s::interrupt\n", argv[0]);
		bcn_rx_cancellation_polling_en = 0;
		bcnc_dump_en = 1;
		bcnc_status = 0;

		nxmac_set_cfg_monrxtarssi_en();
		_sys_nvic_write(NXMAC_BCN_RX_CANCELLATION_IRQn, (void *) _tx_lmac_mac_bcn_rx_cancellation, 0x7);
		nxmac_set_bcn_int_mask(0xffffffff);
		nxmac_set_partial_bcn_rx_en();
		while (bcn_rx_cancellation_polling_en != 1);
		nxmac_set_bcn_int_mask(0x00000000);
		_sys_nvic_write(NXMAC_BCN_RX_CANCELLATION_IRQn, (void *) NULL, 0x7);
		current_st = nxmac_get_bcn_frmst();
		nxmac_reset_partial_bcn_rx_en();
		status_en = 1;
	}

	if (status_en) {
		unsigned long long timestamp;
		timestamp = nxmac_get_timestamp();

		PRINTF("- bcnc: %s\n", lltoa(timestamp, 10));
		PRINTF("  timestamp: 0x%08x%08x\n",
				(unsigned long) (timestamp >> 32),
				(unsigned long) (timestamp & 0xffffffff));
		PRINTF("  current_status: %08x\n", current_st);
		PRINTF("  int_status: %08x\n", bcnc_status);
		PRINTF("  bcnc_dyn_status: %08x\n", bcnc_dyn_status);
		PRINTF("  fcs_error: %d\n", !!(bcnc_status & 0x80));
		PRINTF("  aid: %d\n", nxmac_get_aid());
		PRINTF("  bcn_cancellation_en: %d\n",
				nxmac_get_partial_bcn_rx_en());
		PRINTF("  data_start_l: %d\n", bcnc_data_start);
		PRINTF("  tim_parsing_done_l: %d\n", bcnc_parsing_done);
		PRINTF("  bcn_duration: %d\n",
				bcnc_parsing_done - bcnc_data_start);
		PRINTF("  rmon0: %08x\n", bcnc_rssi_mon0);
		PRINTF("  rmon0_max: %d\n", bcnc_rssi_mon0_max);
		PRINTF("  rmon0_min: %d\n", bcnc_rssi_mon0_min);
		PRINTF("  rmon0_avg: %d\n", bcnc_rssi_mon0_avg);
	}

	if (aging_en) {
		bcn_rx_cancellation_polling_en = 0;
		PRINTF("%s::interrupt\n", argv[0]);

		_sys_nvic_write(NXMAC_BCN_RX_CANCELLATION_IRQn, (void *) _tx_lmac_mac_bcn_rx_cancellation, 0x7);
		nxmac_set_partial_bcn_rx_en();

		while (1) {
			if (bcn_rx_cancellation_polling_en) {
				unsigned long long timestamp;
				timestamp = nxmac_get_timestamp();

				bcn_rx_cancellation_polling_en = 0;

				PRINTF("- bcnc: %s\n", lltoa(timestamp, 10));
				PRINTF("  timestamp: 0x%08x%08x\n",
					(unsigned long) (timestamp >> 32),
					(unsigned long) (timestamp & 0xffffffff));
				PRINTF("  current_status: %08x\n",
						nxmac_get_bcn_frmst());
				PRINTF("  int_status: %08x\n",
						bcnc_status);
				PRINTF("  fcs_error: %d\n", !!(bcnc_status & 0x80));
			}
		}

		nxmac_reset_partial_bcn_rx_en();
		_sys_nvic_write(NXMAC_BCN_RX_CANCELLATION_IRQn, (void *) NULL, 0x7);
		status_en = 1;
	}

	if (di) {
		nxmac_reset_partial_bcn_rx_en();
		PRINTF("%s::en=0", argv[0]);
	}
	if (en) {
		nxmac_set_partial_bcn_rx_en();
		PRINTF("%s::en=1", argv[0]);
	}
	if (clr_en) {
		nxmac_clear_bcn_frmstclr(stclr);
		PRINTF("%s::clear=%08x", argv[0], stclr);
	}
	if (aid_en) {
		nxmac_set_aid(aid);
		PRINTF("%s::aid=%d", argv[0], aid);
	}
	if (interrupt_en) {
		_sys_nvic_write(NXMAC_BCN_RX_CANCELLATION_IRQn, (void *) _tx_lmac_mac_bcn_rx_cancellation, 0x7);
		nxmac_set_bcn_int_mask(0xffffffff);
		PRINTF("%s::interrupt=0", argv[0]);
	}
	if (interrupt_di) {
		nxmac_set_bcn_int_mask(0x00000000);
		_sys_nvic_write(NXMAC_BCN_RX_CANCELLATION_IRQn, (void *) NULL, 0x7);
		PRINTF("%s::interrupt=0", argv[0]);
	}
	if (pbr_status_en) {
		nxmac_set_bcn_int_mask(0xffffffff);
		PRINTF("%s::status=0", argv[0]);
	}
	if (pbr_status_di) {
		nxmac_set_bcn_int_mask(0x00000000);
		PRINTF("%s::status=0", argv[0]);
	}
	if (rssi_mon_en) {
		PRINTF("%s::rmon=1", argv[0]);
		PRINTF("%s::rmon_ta %08x%04x", argv[0], rmon_ta_h, rmon_ta_l);

		nxmac_set_cfg_monrxtarssi_en();
		nxmac_set_cfg_monrxtarssi_mode_onlyfcsok();
		nxmac_set_fci_mon_ta_rssi_cfg_ta_h(rmon_ta_h);
		nxmac_set_fci_mon_ta_rssi_cfg_ta_l(rmon_ta_l);
	}
	if (rssi_mon_di) {
		PRINTF("%s::rmon=0", argv[0]);
		nxmac_reset_cfg_monrxtarssi_en();
	}

	if (help_en) {
		PRINTF("Usage:\n", argv[0]);
		PRINTF("       %s: help & status\n", argv[0]);
		PRINTF("       %s en=1: BCN RX Cancellation Enable\n", argv[0]);
		PRINTF("       %s en=0: BCN RX Cancellation Disable\n", argv[0]);
		PRINTF("       %s c [status]: Status Clear\n", argv[0]);
		PRINTF("       %s a [aid]: Set AID\n", argv[0]);
		PRINTF("       %s i: Interrupt Enable\n", argv[0]);
		PRINTF("       %s d: Interrupt Disable\n", argv[0]);
		PRINTF("       %s si: Status Enable\n", argv[0]);
		PRINTF("       %s sd: Status Disable\n", argv[0]);
		PRINTF("       %s p: Interrupt Test\n", argv[0]);
		PRINTF("       %s aging: Aging Test\n", argv[0]);
		PRINTF("       %s rmon: RSSI MON Disable\n", argv[0]);
		PRINTF("       %s rmon [ta_l] [ta_h]: RSSI MON Enable\n", argv[0]);
	}
}
#endif	// SUPPORT_BCN_RX_CANCELLATION

#ifdef TIMP_CMD_EN
void cmd_timp(int argc, char *argv[])
{
	unsigned char usage_en = 1;
	long long c;

	switch (argc) {
	case 2:
		if (strcmp("u", argv[1]) == 0) {
			long long c = (unsigned long) dpm_get_clk(GET_DPMP());
			enum TIMP_UPDATED_STATUS s;
			c = CLK2US(c);
			c = US2TU(c);

			s = timp_update((unsigned long) c);
			PRINTF("%s::update clk = %d, %02x\n", argv[0],
						(unsigned long) c, s);
			PRINTF("%s::interface_0 updatetd %d, weak_sig: %d\n",
					argv[0],
					TIMP_CHK_UPDATED(TIMP_INTERFACE_0, s),
					TIMP_CHK_WEAK_SIG(TIMP_INTERFACE_0, s));
			PRINTF("%s::interface_1 updatetd %d, weak_sig: %d\n",
					argv[0],
					TIMP_CHK_UPDATED(TIMP_INTERFACE_1, s),
					TIMP_CHK_WEAK_SIG(TIMP_INTERFACE_1, s));
			timp_display_interface_data(s);

			usage_en = 0;
		} else if (strcmp("U", argv[1]) == 0) {
			enum TIMP_UPDATED_STATUS s;
			struct timp_data_tag *p =
				timp_get_interface_current(TIMP_INTERFACE_0);

			s =  timp_update(1);

			PRINTF("%s::update clk = %d, %02x\n", argv[0],
					(unsigned long) c, s);
			PRINTF("%s::interface_0 updatetd %d, weak_sig: %d\n",
					argv[0],
					TIMP_CHK_UPDATED(TIMP_INTERFACE_0, s),
					TIMP_CHK_WEAK_SIG(TIMP_INTERFACE_0, s));
			PRINTF("%s::interface_1 updatetd %d, weak_sig: %d\n",
					argv[0],
					TIMP_CHK_UPDATED(TIMP_INTERFACE_1, s),
					TIMP_CHK_WEAK_SIG(TIMP_INTERFACE_1, s));

			PRINTF("rx_on_time_tu: %d\n", p->rx_on_time_tu);
			PRINTF("tim_count: %d\n", p->tim_count);
			PRINTF("bcn_timeout_count: %d\n", p->bcn_timeout_count);
			usage_en = 0;
		} else if (strcmp("i", argv[1]) == 0) {
			PRINTF("%s::init\n", argv[0]);
			usage_en = 0;
		}
		break;

	case 3:
		if (strcmp("r", argv[1]) == 0) {
			long long c = (unsigned long) dpm_get_clk(GET_DPMP());
			c = CLK2US(c);
			c = US2TU(c);

			timp_reset(atoi(argv[2]), (unsigned long) c);
			PRINTF("%s::reset %d clk = %d\n", argv[0],
					atoi(argv[2]), (unsigned long) c);
			usage_en = 0;
		} else if (strcmp("ww", argv[1]) == 0) {
			if (atoi(argv[2]) == 0) {
				dpm_set_env_timp_chk_wakeup_0(GET_DPMP());
		  		dpm_set_env_timp_weaksig_0(GET_DPMP());
			} else if (atoi(argv[2]) == 1) {
				dpm_set_env_timp_chk_wakeup_1(GET_DPMP());
		  		dpm_set_env_timp_weaksig_1(GET_DPMP());
			} else {
				usage_en = 0;
			}

			PRINTF("%s::weak_signal_%d = on\n", argv[0],
					atoi(argv[2]));
		} else if (strcmp("pw", argv[1]) == 0) {
			if (atoi(argv[2]) == 0) {
				dpm_set_env_timp_chk_wakeup_0(GET_DPMP());
		  		dpm_set_env_timp_update_0(GET_DPMP());
			} else if (atoi(argv[2]) == 1) {
				dpm_set_env_timp_chk_wakeup_1(GET_DPMP());
		  		dpm_set_env_timp_update_1(GET_DPMP());
			} else {
				usage_en = 0;
			}

			PRINTF("%s::update_wakeup_%d = on\n", argv[0],
					atoi(argv[2]));
		} else if (strcmp("w", argv[1]) == 0) {
			if (atoi(argv[2]) == 0)
				dpm_reset_env_timp_chk_wakeup_0(GET_DPMP());
			else if (atoi(argv[2]) == 1)
				dpm_reset_env_timp_chk_wakeup_1(GET_DPMP());
			else
				usage_en = 0;

			PRINTF("%s::wakeup_%d = off\n", argv[0],
					atoi(argv[2]));
		}
		break;

	case 4:
		if (strcmp("p", argv[1]) == 0) {
			timp_set_period((unsigned char) atoi(argv[2]),
					atoi(argv[3]));
			//timp_set_condition(TIMP_INTERFACE_0, &cond);

			PRINTF("%s::set_period %p = %d\n", argv[0],
					atoi(argv[2]), atoi(argv[3]));
			usage_en = 0;
		}
		break;

	case 1:
		break;
	}

	if (usage_en) {
		PRINTF("%s [p] [interface] [period]: set period\n", argv[0]);
		PRINTF("%s [i]                     : timp initialization\n", argv[0]);
		PRINTF("%s [r] [interface]         : timp reset\n", argv[0]);
		PRINTF("%s [u]                     : timp update (Auto)\n", argv[0]);
		PRINTF("%s [U]                     : timp update (Manual)\n", argv[0]);
		PRINTF("%s [ww] [interface]        : Weak signal wakeup\n", argv[0]);
		PRINTF("%s [pw] [interface]        : Period wakeup\n", argv[0]);
		PRINTF("%s [w]  [interface]        : No wakeup\n", argv[0]);
	}
}
#endif	// TIMP_CMD_EN

//-----------------------------------------------------------------------
// Command NET-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_lmac_list[] = {
	{ "LMAC",		CMD_MY_NODE,	cmd_lmac_list,	NULL,			"lmac "			},		// Head
	{ "-------",	CMD_FUNC_NODE,	NULL,			NULL,			"--------------------------------"} ,

	{ "per",		CMD_FUNC_NODE,  NULL,			&cmd_lmac_per,	"lmac per logging"	},
	{ "tx",			CMD_SUB_NODE,	&cmd_lmac_tx_list[0],	NULL,	"lmac tx command"	},
#ifdef SUPPORT_BCN_RX_CANCELLATION
	{ "bcnc",		CMD_FUNC_NODE,	NULL,			cmd_lmac_bcnc,	"lmac bcnc command"},
#endif	// SUPPORT_BCN_RX_CANCELLATION
#ifdef TIMP_CMD_EN
	{ "timp",		CMD_FUNC_NODE,	NULL,			&cmd_timp,		"TIMP"},
#endif	// TIMP_CMD_EN
	{ NULL, 		CMD_NULL_NODE,	NULL,	NULL,	NULL }		// Tail
};

/* EOF */
