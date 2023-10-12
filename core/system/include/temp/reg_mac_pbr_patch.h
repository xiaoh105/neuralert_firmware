/**
 ****************************************************************************************
 *
 * @file     reg_mac_pbr.h
 *
 * @brief    PBR header
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

/**
 * @file reg_mac_pbr.h
 * @brief Definitions of the NXMAC HW block registers and register access functions.
 *
 * @defgroup REG_MAC_PBR
 * @ingroup REG
 * @{
 *
 * @brief Definitions of the NXMAC HW block registers and register access functions.
 */
#ifndef _REG_MAC_PBR_H_
#define _REG_MAC_PBR_H_

#include "reg_access.h"
#include "co_int.h"
#include "compiler.h"

/** @brief Number of registers in the REG_MAC_PL peripheral.
 */
#define REG_MAC_PBR_COUNT 17

#define NXMAC_BCNFRMSTDYNAMIC                   0x60B00564
#define NXMAC_TIMESTAMP_0                       0x60B00568
#define NXMAC_TIMESTAMP_1                       0x60B0056C
#define NXMAC_PARTIALBCNAID                     0x60B00570
#define NXMAC_PARTIALBCNRXEN                    0x60B00570
#define NXMAC_BCNFRMSTCLR                       0x60B00574
#define NXMAC_BCNFRMSTSTAIC                     0x60B00578
#define NXMAC_BCNINTMASK                        0x60B0057C
#define NXMAC_INITDSPARAMSET                    0x60B00588
#define NXMAC_INITSSID0                         0x60B0058C
#define NXMAC_INITSSID1                         0x60B00590
#define NXMAC_INITSSID2                         0x60B00594
#define NXMAC_INITSSID3                         0x60B00598
#define NXMAC_INITSSID4                         0x60B0059C
#define NXMAC_INITSSID5                         0x60B005A0
#define NXMAC_INITSSID6                         0x60B005A4
#define NXMAC_INITSSID7                         0x60B005A8
#define NXMAC_CURRDSPARAMSET                    0x60B005AC
#define NXMAC_CURRSSID0                         0x60B005B0
#define NXMAC_CURRSSID1                         0x60B005B4
#define NXMAC_CURRSSID2                         0x60B005B8
#define NXMAC_CURRSSID3                         0x60B005BC
#define NXMAC_CURRSSID4                         0x60B005C0
#define NXMAC_CURRSSID5                         0x60B005C4
#define NXMAC_CURRSSID6                         0x60B005C8
#define NXMAC_CURRSSID7                         0x60B005CC

#define NXMAC_MON_TA_RSSI_CFG_TA_L              0x60B00630
#define NXMAC_MON_TA_RSSI_CFG_TA_H              0x60B00634
#define NXMAC_MON_TA_RSSI_CFG_CTRL              0x60B00638
#define NXMAC_MON_TA_RSSI_MON0_11B              0x60B0063C
#define NXMAC_MON_TA_RSSI_MON1_11G              0x60B00640
#define NXMAC_MON_TA_RSSI_MON2_11N              0x60B00644
#define NXMAC_MON_TA_RSSI_MONX                  0x60B00648

#define NXMAC_PBR_MON_TSF_DATA_START_L          0x60B00650
#define NXMAC_PBR_MON_TSF_DATA_START_H          0x60B00654
#define NXMAC_PBR_MON_TSF_TIM_PARSING_DONE_L    0x60B00658
#define NXMAC_PBR_MON_TSF_TIM_PARSING_DONE_H    0x60B0065C

#define NXMAC_BCNFRMST_BSSID_MATCH              0x01
#define NXMAC_BCNFRMST_TIMESTAMP_PARSING_DONE   0x02
#define NXMAC_BCNFRMST_DETECTED_BCMC            0x04
#define NXMAC_BCNFRMST_DETECTED_AID             0x08
#define NXMAC_BCNFRMST_TIM_PARSING_DONE         0x10
#define NXMAC_BCNFRMST_CHANGED_SSID             0x20
#define NXMAC_BCNFRMST_CHANGED_DS_PARAMETER_SET 0x40
#define NXMAC_BCNFRMST_FCS_ERROR                0x80

__INLINE uint32_t nxmac_get_bcn_frmst()
{
	return REG_PL_RD(NXMAC_BCNFRMSTSTAIC);
}

__INLINE uint32_t nxmac_get_bcn_int_mask()
{
	return REG_PL_RD(NXMAC_BCNINTMASK);
}

__INLINE void nxmac_set_bcn_int_mask(uint32_t mask)
{
	REG_PL_WR(NXMAC_BCNINTMASK, mask);
}

__INLINE uint64_t nxmac_get_timestamp()
{
	uint64_t timestamp;
	timestamp = REG_PL_RD(NXMAC_TIMESTAMP_1);

	return (timestamp << 32LL) | REG_PL_RD(NXMAC_TIMESTAMP_0);
}

__INLINE unsigned short nxmac_get_aid()
{
	return REG_PL_RD(NXMAC_PARTIALBCNAID);
}

__INLINE void nxmac_set_aid(unsigned short aid)
{
	uint32_t en;
	en = REG_PL_RD(NXMAC_PARTIALBCNAID) & 0x80000000;
	en |= aid;
	REG_PL_WR(NXMAC_PARTIALBCNAID, en);
}

__INLINE uint32_t nxmac_get_partial_bcn_rx_en()
{
	return !!(REG_PL_RD(NXMAC_PARTIALBCNRXEN) & 0x80000000);
}

__INLINE void nxmac_set_partial_bcn_rx_en()
{
	uint32_t bcn_rx_en = REG_PL_RD(NXMAC_PARTIALBCNRXEN) | 0x80000000;
	REG_PL_WR(NXMAC_PARTIALBCNRXEN, bcn_rx_en);
}

__INLINE void nxmac_reset_partial_bcn_rx_en()
{
	uint32_t bcn_rx_en = REG_PL_RD(NXMAC_PARTIALBCNRXEN) & 0x7fffffff;
	REG_PL_WR(NXMAC_PARTIALBCNRXEN, bcn_rx_en);
}

__INLINE void nxmac_clear_bcn_frmstclr(uint32_t bcn_frmstclr)
{
	REG_PL_WR(NXMAC_BCNFRMSTCLR, bcn_frmstclr);
}

__INLINE uint32_t nxmac_get_currdsparamset()
{
	return REG_PL_RD(NXMAC_CURRDSPARAMSET);
}

__INLINE void nxmac_set_initdsparamset(uint32_t initdsparamset)
{
	REG_PL_WR(NXMAC_INITDSPARAMSET, initdsparamset);
}

__INLINE uint32_t nxmac_get_currssid0()
{
	return REG_PL_RD(NXMAC_CURRSSID0);
}

__INLINE void nxmac_set_initssid0(uint32_t initssid0)
{
	REG_PL_WR(NXMAC_INITSSID0, initssid0);
}

__INLINE uint32_t nxmac_get_currssid1()
{
	return REG_PL_RD(NXMAC_CURRSSID1);
}

__INLINE void nxmac_set_initssid1(uint32_t initssid1)
{
	REG_PL_WR(NXMAC_INITSSID1, initssid1);
}

__INLINE uint32_t nxmac_get_currssid2()
{
	return REG_PL_RD(NXMAC_CURRSSID2);
}

__INLINE void nxmac_set_initssid2(uint32_t initssid2)
{
	REG_PL_WR(NXMAC_INITSSID2, initssid2);
}

__INLINE uint32_t nxmac_get_currssid3()
{
	return REG_PL_RD(NXMAC_CURRSSID3);
}

__INLINE void nxmac_set_initssid3(uint32_t initssid3)
{
	REG_PL_WR(NXMAC_INITSSID3, initssid3);
}

__INLINE uint32_t nxmac_get_currssid4()
{
	return REG_PL_RD(NXMAC_CURRSSID4);
}

__INLINE void nxmac_set_initssid4(uint32_t initssid4)
{
	REG_PL_WR(NXMAC_INITSSID4, initssid4);
}

__INLINE uint32_t nxmac_get_currssid5()
{
	return REG_PL_RD(NXMAC_CURRSSID5);
}

__INLINE void nxmac_set_initssid5(uint32_t initssid5)
{
	REG_PL_WR(NXMAC_INITSSID5, initssid5);
}

__INLINE uint32_t nxmac_get_currssid6()
{
	return REG_PL_RD(NXMAC_CURRSSID6);
}

__INLINE void nxmac_set_initssid6(uint32_t initssid6)
{
	REG_PL_WR(NXMAC_INITSSID6, initssid6);
}

__INLINE uint32_t nxmac_get_currssid7()
{
	return REG_PL_RD(NXMAC_CURRSSID7);
}

__INLINE void nxmac_set_initssid7(uint32_t initssid7)
{
	REG_PL_WR(NXMAC_INITSSID7, initssid7);
}

__INLINE uint32_t nxmac_get_pbr_mon_tsf_data_start_l()
{
	return REG_PL_RD(NXMAC_PBR_MON_TSF_DATA_START_L);
}

__INLINE uint32_t nxmac_get_pbr_mon_tsf_data_start_h()
{
	return REG_PL_RD(NXMAC_PBR_MON_TSF_DATA_START_H);
}
//
/// @}

#endif // _REG_MAC_PBR_H_

/// @}

/* EOF */
