/**
 ****************************************************************************************
 *
 * @file rf_meas_api.c
 *
 * @brief APIs for RF measurement
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
#include "da16x_types.h"
#include "da16x_system.h"
#include "rf_meas_api.h"
#include "dpmrtm_patch.h"
#include "romac_primitives_patch.h"

/// MAC address structure.
struct mac_addr
{
    /// Array of 16-bit words that make up the MAC address.
    uint16_t array[3];
};
/// Structure containing the information about the PHY channel that was used for this RX
struct phy_channel_info
{
    /// PHY channel information 1
    uint32_t info1;
    /// PHY channel information 2
    uint32_t info2;
};

extern void phy_get_channel(struct phy_channel_info *info, uint8_t index);
extern int read_nvram_int(const char *name, int *_val);

uint32_t gpio_backup = 0;

#define BTCOEX_ENABLE  				 0x60920004
#define BTCOEX_PRIORITY				 0x60920028

#define NXMAC_MON_TA_RSSI_CFG_TA_L              0x60B00630
#define NXMAC_MON_TA_RSSI_CFG_TA_H              0x60B00634
#define NXMAC_MON_TA_RSSI_CFG_CTRL              0x60B00638
#define NXMAC_MON_TA_RSSI_MON0_11B              0x60B0063C
#define NXMAC_MON_TA_RSSI_MON1_11G              0x60B00640
#define NXMAC_MON_TA_RSSI_MON2_11N              0x60B00644
#define NXMAC_MON_TA_RSSI_MONX                  0x60B00648

/*
 * MACROS
 */
/// Macro to read a platform register
#define	REG_PL_RD(addr)			(*(volatile uint32_t *)(addr))
#define	REG_PL_RD_MACPI(addr)	(*(volatile uint32_t *)(addr))

/// Macro to write a platform register
#define	REG_PL_WR(addr, value)			(*(volatile uint32_t *)(addr)) = (value)
#define	REG_PL_WR_MACPI(addr, value)	(*(volatile uint32_t *)(addr)) = (value)


// FCI_COEX_CFG1 Register
#define PolarityInv_BtAct       (1 << 18)
#define PolarityInv_BtPri       (1 << 17)
#define PolarityInv_WlanAct     (1 << 16)
#define WlanRx2WlAct_MapEn      (1 << 13)
#define WlanTx2WlAct_MapEn      (1 << 12)
#define BtAct2tx_OnlyPri        (1 << 10)
#define BtAct2tx_MapEn          (1 << 8)
#define ForceVal_WlanAct        (1 << 6)
#define ForceVal_BtAct          (1 << 4)
#define ForceEn_WlanAct         (1 << 2)
#define ForceEn_BtAct           (1 << 0)

// RW_PTA_CONFIG
#define no_sim_tx               (1 << 2)
#define pta_enable              (1 << 0)


void rf_meas_btcoex(uint8_t enable, uint8_t priority, uint8_t polarity)
{
       struct dpm_param *dpmp = GET_DPMP();
       if (enable)
       {
              REG_PL_WR(BTCOEX_ENABLE , (no_sim_tx | pta_enable));
              switch (priority)
              {
              case 0: /* BT */
#ifdef __SUPPORT_BTCOEX_1PIN__
                REG_PL_WR(BTCOEX_PRIORITY, (ForceEn_BtAct | ForceEn_WlanAct | ForceVal_BtAct | BtAct2tx_MapEn | BtAct2tx_OnlyPri | WlanTx2WlAct_MapEn));
#else
                REG_PL_WR(BTCOEX_PRIORITY, (WlanTx2WlAct_MapEn| BtAct2tx_MapEn| ForceEn_WlanAct));

#endif   /* __SUPPORT_BTCOEX_1PIN__  */
                break;

              case 1: /* SAME */
                REG_PL_WR(BTCOEX_PRIORITY, (BtAct2tx_MapEn | WlanTx2WlAct_MapEn));
                break;

              case 2: /* Wifi */
                REG_PL_WR(BTCOEX_PRIORITY, WlanTx2WlAct_MapEn);
                break;
              case 3: /* Wifi */
                REG_PL_WR(BTCOEX_PRIORITY, ForceVal_WlanAct | ForceEn_WlanAct);
                break;
              default:
                break;
              }

              if(polarity)
              {
                REG_PL_WR(BTCOEX_PRIORITY, REG_PL_RD(BTCOEX_PRIORITY) | (PolarityInv_WlanAct | PolarityInv_BtPri | PolarityInv_BtAct));
              }else{
            	REG_PL_WR(BTCOEX_PRIORITY, REG_PL_RD(BTCOEX_PRIORITY) & ~(PolarityInv_WlanAct | PolarityInv_BtPri | PolarityInv_BtAct));
              }
	   }
       else
       {
         REG_PL_WR(BTCOEX_ENABLE , 0x0);
       }

       // BTCOEX feature
       if (enable)
              dpm_set_env_feature(dpmp, dpm_get_env_feature(dpmp) | DPM_FEATURE_BTCOEX);
       else
              dpm_set_env_feature(dpmp, dpm_get_env_feature(dpmp) & (~DPM_FEATURE_BTCOEX));
       // Set BTCOEX PRIORITY for PTIM.
       dpm_set_env_btcoex_priority(dpmp, REG_PL_RD(BTCOEX_PRIORITY));
}


static void str2mac(struct mac_addr *addr, char *str_addr)
{
	unsigned int tmp[6];

	sscanf(str_addr, "%02x:%02x:%02x:%02x:%02x:%02x", &tmp[0], &tmp[1], &tmp[2],
				   &tmp[3], &tmp[4], &tmp[5]);

	addr->array[0] = (uint16_t)(tmp[1]<<8|tmp[0]);
	addr->array[1] = (uint16_t)(tmp[3]<<8|tmp[2]);
	addr->array[2] = (uint16_t)(tmp[5]<<8|tmp[4]);
}

void rf_meas_monRxTaRssi_set(uint8_t enable,
							uint8_t OnlyFcsOk,
							uint8_t IgnoreTa,
							uint8_t HoldReq,
							uint8_t IIR_Alpha,
							char *addr)
{
	uint32_t ctrl;
	struct mac_addr mac_addr;

	if (addr != NULL) {
		str2mac(&mac_addr, addr);
		REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_TA_L, mac_addr.array[0] | (((uint32_t)mac_addr.array[1]) << 16));
		REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_TA_H, mac_addr.array[2]);
	}

	ctrl = (uint32_t)(IIR_Alpha<<16 | HoldReq <<12 | IgnoreTa << 8 | OnlyFcsOk << 4 | enable);
	REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_CTRL, ctrl);
}

// Set with addr val(Not string)
void rf_meas_monRxTaRssi_set_v(uint8_t enable,
								uint8_t OnlyFcsOk,
								uint8_t IgnoreTa,
								uint8_t HoldReq,
								uint8_t IIR_Alpha,
								unsigned char *addr)
{
	uint32_t ctrl, tmp[6], i;
	uint32_t mac_addr[2];
    
	if (addr != NULL) {
		for (i = 0; i < 6; i++) {
			tmp[i] = addr[i];
		}

		mac_addr[0] = tmp[0] | tmp[1]<<8 | tmp[2] << 16 | tmp[3]<< 24;
		mac_addr[1] = tmp[4] | tmp[5] << 8;

		REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_TA_L, mac_addr[0]);
		REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_TA_H, mac_addr[1] & 0xFFFF);      
	}

	ctrl = (uint32_t)(IIR_Alpha << 16 | HoldReq << 12 | IgnoreTa << 8 | OnlyFcsOk << 4 | enable);
	REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_CTRL, ctrl);
}

void rf_meas_monRxTaRssi_get(struct MonRxTaRssi *meas)
{
	uint32_t reg;

	reg = REG_PL_RD(NXMAC_MON_TA_RSSI_MON0_11B);
	meas->b.avg  = (int8_t)((reg      ) & 0xff);
	meas->b.min  = (int8_t)((reg >>  8) & 0xff);
	meas->b.max  = (int8_t)((reg >> 16) & 0xff);
	meas->b.cnt  = (reg >> 24) & 0x7f;
	meas->b.flag = (uint8_t)(reg >> 31);

	reg = REG_PL_RD(NXMAC_MON_TA_RSSI_MON1_11G);
	meas->g.avg  = (int8_t)((reg      ) & 0xff);
	meas->g.min  = (int8_t)((reg >>  8) & 0xff);
	meas->g.max  = (int8_t)((reg >> 16) & 0xff);
	meas->g.cnt  = (reg >> 24) & 0x7f;
	meas->g.flag = (uint8_t)((reg >> 31) & 0x1);

	reg = REG_PL_RD(NXMAC_MON_TA_RSSI_MON2_11N);
	meas->n.avg  = (int8_t)((reg      ) & 0xff);
	meas->n.min  = (int8_t)((reg >>  8) & 0xff);
	meas->n.max  = (int8_t)((reg >> 16) & 0xff);
	meas->n.cnt  = (reg >> 24) & 0x7f;
	meas->n.flag = (uint8_t)((reg >> 31) & 0x1);

	meas->cur_rssi = (int8_t)(REG_PL_RD(NXMAC_MON_TA_RSSI_MONX) & 0xff);
}

#define NXMAC_MON_TA_CFO_CFG_CTRL           0x60B00660
#define NXMAC_MON_TA_CFO_CFG_MON_DSSS       0x60B00668
#define NXMAC_MON_TA_CFO_CFG_MON_OFDM       0x60B0066C
void rf_meas_monRxTaCfo_set(uint8_t enable, uint8_t OnlyFcsOk, uint8_t IgnoreTa, uint8_t HoldReq, char *addr)
{
	uint32_t ctrl;
	struct mac_addr mac_addr;

	if (addr != NULL)
	{
		str2mac(&mac_addr, addr);
		REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_TA_L, mac_addr.array[0] | (((uint32_t)mac_addr.array[1]) << 16));
		REG_PL_WR(NXMAC_MON_TA_RSSI_CFG_TA_H, mac_addr.array[2]);
	}

	ctrl = (uint32_t)(HoldReq <<12 | IgnoreTa << 8 | OnlyFcsOk << 4 | enable);
	REG_PL_WR(NXMAC_MON_TA_CFO_CFG_CTRL, ctrl);
}

static uint32_t get_current_freq()
{
	struct phy_channel_info phy_info;

#define	PHY_PRIM	0
	phy_get_channel(&phy_info, PHY_PRIM);

	return phy_info.info2 &0xffff;
}

void rf_meas_monRxTaCfo_get(struct MonRxTaCfo *meas)
{
	uint32_t reg;
	int8_t temp;
	float temp2;

	reg = REG_PL_RD(NXMAC_MON_TA_CFO_CFG_MON_DSSS);
	meas->dsss.cnt	= reg & 0x7F;
	meas->dsss.flag = (reg >> 7) & 0x1;
	temp = (int8_t)((reg >> 8) & 0xff);
	meas->dsss.cfo = (float)(-0.7 * temp);

	reg = REG_PL_RD(NXMAC_MON_TA_CFO_CFG_MON_OFDM);
	meas->ofdm.cnt = reg & 0x7F;
	meas->ofdm.flag = (reg >> 7) & 0x1;
	temp2 = (float)((reg >> 8) & 0x3FFFFF);

	PRINTF("temp2 %f\n", (double)temp2);

	if (temp2 > 0x1FFFFF) {
		temp2 = temp2 - 0x400000;
	}

	/* ofdm.cfo = temp2/2^25*20000000/(freq*1000000)*1000000 */
	/*          = temp2/freq/2^25*20000000                   */
	/*          = temp2/freq/0.596046448                     */
	if(get_current_freq() != 0) {
		meas->ofdm.cfo = (float)(temp2/(float)get_current_freq()*0.596046448f);
	} else {
		meas->ofdm.cfo = 0.0f;
	}

}

////////////////////////////////////////////////////////////////////////
/* CMD */
////////////////////////////////////////////////////////////////////////
char *str_pri[3] = {
	"Priority BT > WiFi",
	"Priority BT = WiFi",
	"Priority BT < WiFi"
};

void  cmd_rf_meas_btcoex(int argc, char* argv[])
{
	uint8_t enable = 0;
	uint8_t priority = 0;
	uint8_t gpio = 0;

	switch (argc)
	{
		case 4:
			gpio = (uint8_t)atoi(argv[3]);
			/* fall through */
			//no break
		case 3:
			priority = (uint8_t)atoi(argv[2]);
			/* fall through */
			//no break
		case 2:
			enable = (uint8_t)atoi(argv[1]);
			rf_meas_btcoex(enable, priority, gpio);

			if (enable)
			{
				PRINTF("btcoex enable, %s, %s\n",
				    	str_pri[priority],
				    	gpio?"gpio setting on":"gpio setting off");
			}
			else
			{
				PRINTF("btcoex disable\n");
			}
			break;

		default:
			PRINTF("btcoex [enable][priority][gpio_setting]\n");
			break;
	}
}

char *str_cmd_rssi_set = "MonRxTaRssi.set [enable] [OnlyFcsOk] [IgnoreTa] [HoldReq] [IIR Alpha] [Target Address]";
char *str_cmd_rssi_get = "MonRxTaRssi.get";
void  cmd_rf_meas_MonRxTaRssiSet(int argc, char* argv[])
{
	uint8_t enable = 0;
	uint8_t onlyFcsOk = 0;
	uint8_t ignoreTa = 0;
	uint8_t holdReq = 0;
	uint8_t iirAlpha = 0;
	char *addr = NULL;

	switch(argc)
	{
		case 7:
			addr = argv[6];
			/* fall through */
			//no break
		case 6:
			iirAlpha = (uint8_t)atoi(argv[5]);
			/* fall through */
			//no break
		case 5:
			holdReq = (uint8_t)atoi(argv[4]);
			/* fall through */
			//no break
		case 4:
			ignoreTa = (uint8_t)atoi(argv[3]);
			/* fall through */
			//no break
		case 3:
			onlyFcsOk = (uint8_t)atoi(argv[2]);
			/* fall through */
			//no break
		case 2:
			enable = (uint8_t)atoi(argv[1]);
			rf_meas_monRxTaRssi_set(enable, onlyFcsOk, ignoreTa, holdReq, iirAlpha, addr);

			if (enable)
			{
				PRINTF("MonRxTaRssi enable: onlyFcsOk=%d, ignoreTa=%d, holdReq=%d, iirAlpha=%d \n",
				     	onlyFcsOk,ignoreTa,holdReq,iirAlpha);
				if (addr)
					PRINTF("MAC Address %s\n", addr);
			}
			else
			{
				PRINTF("MonRxTaRssi disable\n");
			}
			break;

		default:
			PRINTF("%s\n", str_cmd_rssi_set);
			break;
	}
}

void  cmd_rf_meas_MonRxTaRssiGet(int argc, char* argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	struct MonRxTaRssi meas;

	rf_meas_monRxTaRssi_get(&meas);

	PRINTF("TYPE\tAVG\tMIN\tMAX\tCNT\tWrap\n");
	PRINTF("11b\t%d\t%d\t%d\t%d\t%d\n", meas.b.avg, meas.b.min, meas.b.max, meas.b.cnt, meas.b.flag);
	PRINTF("11g\t%d\t%d\t%d\t%d\t%d\n", meas.g.avg, meas.g.min, meas.g.max, meas.g.cnt, meas.g.flag);
	PRINTF("11n\t%d\t%d\t%d\t%d\t%d\n", meas.n.avg, meas.n.min, meas.n.max, meas.n.cnt, meas.n.flag);
	PRINTF("\nCurrent RSSI %d\n", meas.cur_rssi);

}

void  cmd_rf_meas_MonRxTaRssi(int argc, char* argv[])
{
	if (!strcmp("set", argv[0])) {
		 cmd_rf_meas_MonRxTaRssiSet(argc, argv);
	} else if (!strcmp("get", argv[0])) {
		 cmd_rf_meas_MonRxTaRssiGet(argc, argv);
	} else {
		PRINTF("%s\n%s\n", str_cmd_rssi_set, str_cmd_rssi_get);
	}
}


char *str_cmd_cfo_set = "MonRxTaCfo.set [enable] [OnlyFcsOk] [IgnoreTa] [HoldReq] [Target Address]";
char *str_cmd_cfo_get = "MonRxTaCfo.get";

void  cmd_rf_meas_MonRxTaCfoSet(int argc, char* argv[])
{
	uint8_t enable = 0;
	uint8_t onlyFcsOk = 0;
	uint8_t ignoreTa = 0;
	uint8_t holdReq = 0;
	char *addr = NULL;

	switch (argc)
	{
		case 6:
			addr = argv[5];
			/* fall through */
			//no break
		case 5:
			holdReq = (uint8_t)atoi(argv[4]);
			/* fall through */
			//no break
		case 4:
			ignoreTa = (uint8_t)atoi(argv[3]);
			/* fall through */
			//no break
		case 3:
			onlyFcsOk = (uint8_t)atoi(argv[2]);
			/* fall through */
			//no break
		case 2:
			enable = (uint8_t)atoi(argv[1]);
			rf_meas_monRxTaCfo_set(enable, onlyFcsOk, ignoreTa, holdReq, addr);

			if (enable) {
				PRINTF("MonRxTaCfo enable: onlyFcsOk=%d, ignoreTa=%d, holdReq=%d\n",
				     	onlyFcsOk, ignoreTa, holdReq);
				if (addr)
					PRINTF("MAC Address %s\n", addr);
			} else {
				PRINTF("MonRxTaCfo disable\n");
			}

			break;

		default:
			PRINTF("%s\n", str_cmd_cfo_set);
			break;
	}
}

void  cmd_rf_meas_MonRxTaCfoGet(int argc, char* argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	struct MonRxTaCfo meas;

	rf_meas_monRxTaCfo_get(&meas);

	PRINTF("TYPE\tCNT\tFLAG\tCFO\n");
	PRINTF("DSSS\t%d\t%d\t%8.5f\n", meas.dsss.cnt, meas.dsss.flag, (double)meas.dsss.cfo);
	PRINTF("OFDM\t%d\t%d\t%8.5f\n", meas.ofdm.cnt, meas.ofdm.flag, (double)meas.ofdm.cfo);
}

void  cmd_rf_meas_MonRxTaCfo(int argc, char* argv[])
{
	if (!strcmp("set", argv[0])) {
		cmd_rf_meas_MonRxTaCfoSet(argc, argv);
	} else if (!strcmp("get", argv[0])) {
		cmd_rf_meas_MonRxTaCfoGet(argc, argv);
	} else {
		PRINTF("%s\n%s\n", str_cmd_cfo_set, str_cmd_cfo_get);
	}
}

void initialize_bt_coex(void)
{
	int	coex_val;

#ifndef __SUPPORT_BTCOEX_1PIN__
	_da16x_io_pinmux(PIN_EMUX, EMUX_BT);
#endif /* !__SUPPORT_BTCOEX_1PIN__ */
	_da16x_io_pinmux(PIN_FMUX, FMUX_GPIOBT);

	if (read_nvram_int("COEX_1_1_0", &coex_val) == 0) {
		// read successful
		if (coex_val == 1) {
			rf_meas_btcoex(ENABLE_COEX, BTCOEX_PRIO_1, 0);
			PRINTF("rf_meas_btcoex(1, 1, 0) \n");
		} else if (coex_val == 0) {
			rf_meas_btcoex(ENABLE_COEX, BTCOEX_PRIO_0, 0);
			PRINTF("rf_meas_btcoex(1, 0, 0) \n");
		} else if (coex_val == 3) {
			rf_meas_btcoex(ENABLE_COEX, BTCOEX_PRIO_3, 0);
			PRINTF("rf_meas_btcoex(1, 3, 0) \n");
		}
	} else {
		// if btcoex in nvram is not set, the below code execute
		rf_meas_btcoex(ENABLE_COEX, __DEFAULT_COEX_PRIORITY__, 0);
		PRINTF("by default, rf_meas_btcoex(1, 0, 0) \n");
	}
}

/* EOF */
