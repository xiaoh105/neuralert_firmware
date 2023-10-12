/**
 ****************************************************************************************
 *
 * @file romac_primitives_patch.h
 *
 * @brief DA16200 romac patch
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



#ifndef __romac_primitives_h__
#define __romac_primitives_h__

/**
 * @file     romac_primitives.h
 * @brief    Function entry point for RoMAC
 * @details  This header file is used in RTOS & PTIM.
             LMAC Symbols should not be included in this header file.
 * @defgroup ROMAC_COM_PRI
 * @ingroup  ROMAC_COM
 * @{
 */
#ifndef TIM_SIMULATION
#include "oal.h"
#include "sal.h"
#endif
#include "hal.h"
#include <stdio.h>
#include <stdbool.h>

#include "dpmrtc_patch.h"
#include "dpmsche_patch.h"
#include "timp_patch.h"

//#include "phy.h"
#ifndef BUILD_OPT_DA16200_ROMAC
#include "da16x_system.h"
#endif

#ifdef	SUPPORT_MEM_PROFILER
#define	SUPPORT_ROMACMEM_PROFILE
#else	//SUPPORT_MEM_PROFILER
#undef	SUPPORT_ROMACMEM_PROFILE
#endif	//SUPPORT_MEM_PROFILER

/******************************************************************************
 *
 *  Retargeted functions
 *
 ******************************************************************************/

typedef		struct	{
	void	(*retarget_dump)(UINT16 tag, void *srcdata, UINT16 len);
	void	(*retarget_text)(UINT16 tag, void *srcdata, UINT16 len);
	void	(*retarget_vprint)(UINT16 tag, const char *format, va_list arg);
	int	(*retarget_getchar)(UINT32 mode );
#ifdef	SUPPORT_HALMEM_PROFILE
	void 	*(*profile_malloc)(char *name, int line, size_t size);
	void 	(*profile_free)(char *name, int line, void *f);
#else	//SUPPORT_HALMEM_PROFILE
	void 	*(*raw_malloc)(size_t size);
	void 	(*raw_free)(void *f);
#endif	//SUPPORT_HALMEM_PROFILE

	UINT32  (*check_crc)(void *data, size_t size, UINT32 seed);

	/* DPM ENTRY DEFINITIONS */
	/* RomMAC Interface */
	int                (*ramlib_check)(void);
	void               (*mac_data_print)(struct dpm_param *dpmp,
					long long rtclk, void *hd);

	/* DPM Power Sequence */
	void               (*power_up_step1)(unsigned long prep_bit,
					unsigned long calen);
	void               (*power_up_step2)(unsigned long prep_bit,
					unsigned long calen);
	void               (*power_up_step3)(unsigned long prep_bit,
					unsigned long calen,
					unsigned long cal_addr);
	void               (*power_up_step4)(unsigned long prep_bit);
	void               (*prepare_for_power_down)(unsigned long prep_bit);
	void               (*power_xip_on)(long wait_clk);
	void               (*power_xip_off)(void);
	void               (*power_fadc_cal)(unsigned long prep_bit,
					unsigned long cal_addr);

	/* DPM main function */
	int                (*sys_ready)(struct dpm_param *dpmp);
	int                (*sys_chk)(struct dpm_param *dpmp);
	int                (*sys_fullboot)(struct dpm_param *dpmp,
					unsigned int boot_mem,
					unsigned int rtos_off);
	long long          (*sys_sleep)(struct dpm_param *dpmp,
					unsigned long ptim_addr,
					long long rtc_us);
	void               (*sys_stop)(struct dpm_param *dpmp);

	void               (*prep_measure)(struct dpm_param *dpmp);
	void               (*prep_post_measure)(struct dpm_param *dpmp);
	long               (*prep_get_next_preptime)(struct dpm_param *dpmp);
	long               (*prep_get_post_preptime)(struct dpm_param *dpmp);
#ifdef TIMP_EN
	/* TIMP */
	void               (*timp_set_backup_addr)(struct dpm_param *dpmp, void *backup_addr);
	void               (*timp_set_period)(enum TIMP_INTERFACE no, unsigned long period);
	void               (*timp_set_condition)(enum TIMP_INTERFACE no, struct timp_data_tag *c);
	void               (*timp_init)(void *back_addr, void *buf);
	void               (*timp_reset)(enum TIMP_INTERFACE no, unsigned long rtc_tu);
	void               (*timp_reset_buf)();
	enum TIMP_UPDATED_STATUS (*timp_update)(unsigned long rtc_tu);
	struct timp_performance_tag *(*timp_get_performance)(void);
	struct timp_data_tag *(*timp_get_interface_data)(enum TIMP_INTERFACE no);
	struct timp_data_tag *(*timp_get_interface_current)(enum TIMP_INTERFACE no);
	struct timp_data_tag *(*timp_get_condition)(enum TIMP_INTERFACE no);
	//void               (*timp_display_interface_data)(enum TIMP_UPDATED_STATUS updated, unsigned long timd_feature);
	void               (*timp_check)(struct dpm_param *dpmp);
	void               (*timp_update_data)(struct timp_data_tag *d,
			struct timp_performance_tag *timp);
	enum TIMP_UPDATED_STATUS (*timp_chk_cond)(enum TIMP_INTERFACE no,
			struct timp_data_tag *c,
			struct timp_data_tag *d);
#endif
	/* Schedule */
	void               (*sche_init)(struct dpm_param *dpmp);
	void               (*sche_delete_all_timer)(struct dpm_param *dpmp);
	void               (*sche_delete)(struct dpm_param *node,
					enum DPMSCHE_FUNCTION func);
	int                (*sche_do)(struct dpm_param *dpmp);
	int                (*sche_set_timer)(struct dpm_param *dpmp,
					enum DPMSCHE_PREP prep,
					enum DPMSCHE_FUNCTION func,
					/*enum DPMSCHE_ATTR attr,*/
					unsigned long interval,
					unsigned long half,
					unsigned long arbitrary,
					int align);
	long long          (*sche_get_sleep_time)(struct dpm_param *dpmp);
	unsigned long long (*sche_get_func)(struct dpm_param *dpmp);
	void               (*sche_plus_time)(struct dpm_schedule_node *a,
					struct dpm_schedule_node *b,
					struct dpm_schedule_node *c);
	void                (*sche_minus_time)(struct dpm_schedule_node *a,
					struct dpm_schedule_node *b,
					struct dpm_schedule_node *c);
	void                (*sche_update_time)(struct dpm_schedule_node *a,
					struct dpm_schedule_node *node0);
	void                (*sche_chk_dtim_cnt)(struct dpm_param *dpmp,
					int dtim_cnt);

	void                (*aptrk_init)(struct dpm_param *dpmp);
	void                (*aptrk_reset)(struct dpm_param *dpmp);
	void                (*aptrk_measure)(struct dpm_param *dpmp,
					long long timestamp,
					long long bcn_clk,
					long cca, long tsflo);
	long                (*aptrk_get_cca)(struct dpm_param *dpmp);
	long                (*aptrk_get_tbtt)(struct dpm_param *dpmp);
	long                (*aptrk_get_timeout)(struct dpm_param *dpmp);

	/* MAC SW */
	unsigned long (*rommac_main)(struct dpm_param *dpmp);
	int           (*ps_init)(struct dpm_param *dpmp);
	int           (*tim_sleep)(void);
	void          (*tim_doze)(void);
	bool          (*sleep_check)(void);
	void          (*tim_bcn_timeout)(void *env);
	void          (*tim_machw_init)(void);
	void          (*tim_check)(struct dpm_param *dpmp,
					int interface_no);
	void          (*tim_txcfm)(void *pvif_entry);

	/* Function Set */
	void          (*tim_set_func)(void *vif);
	int           (*set_func_tim)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_bcmc)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_ps)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_uc)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_ka)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_autoarp)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_deauthchk)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_arpreq)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_udph)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_tcpka)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*set_func_arpresp)(struct dpm_param *dpmp, void *pvif_entry);

	/* Function Check */
	void          (*tim_do_func)();
	int           (*do_func_deauth)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_tim)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_bcmc)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_uc)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_ps)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_autoarp)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_udph)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_arpreq)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_arpresp)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_ka)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_setps)(struct dpm_param *dpmp, void *pvif_entry);
	int           (*do_func_tcpka)(struct dpm_param *dpmp, void *pvif_entry);

	/* BCN Rx */
	int           (*rx_evt)(struct dpm_param *dpmp, void *pvif_entry,
			void *phd);
	unsigned long (*chk_data)(struct dpm_param *dpmp, void *phd);
	unsigned long (*chk_data_bc)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_mc)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_matched_mc)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_uc)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_mgmt)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_ctrl)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_data)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_deauth)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_drop_null)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_arp)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_uc_drop_action)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_uc_drop_action_no_ack)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_drop_ipv6)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_ip)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_drop_bc_ip)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_drop_mc_ip)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_matched_mc_ip)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_drop_other_ip)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_uc_ip)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);
	unsigned long (*chk_data_udp_service)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_tcp_service)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_eapol)(struct dpm_param *dpmp,
			void *phd, unsigned char *machdr,
			unsigned char *payload, int len);
	unsigned long (*chk_data_arp_resp)(struct dpm_param *dpmp, void *phd,
			unsigned char *machdr, unsigned char *payload, int len);

	/* PHY */
	void          (*phy_init)(/*struct phy_cfg_tag*/ void *cfg);
	void          (*phy_set_channel)(uint8_t band, uint8_t type,
			uint16_t prim20_freq,
			uint16_t center1_freq,
			uint16_t center2_freq,
			uint8_t index);
	void          (*phy_get_channel)(
			/*struct phy_channel_info*/ void *info,
			uint8_t index);
	void          (*phy_set_conf)(void *data, int len);

	/* TX */
	unsigned long (*send_data)(struct dpm_param *dpmp,
			unsigned long thd,
			int timeout);
	void          (*send_ps_poll)(struct dpm_param *dpmp);
	unsigned long (*get_null_frame)(struct dpm_param *dpmp);
	unsigned long (*get_ps_poll_frame)(struct dpm_param *dpmp);
	unsigned long (*get_arp_frame)(struct dpm_param *dpmp,
			enum DPM_ARP_TY ty);
	unsigned long (*get_arpreq_frame)(struct dpm_param *dpmp,
			enum DPM_DEAUTH_CHK_TY ty);
	unsigned long (*get_udph_frame)(struct dpm_param *dpmp,
			unsigned char ty, int len);
	unsigned long (*get_tcpka_frame)(struct dpm_param *dpmp,
			unsigned char ty, int len);

} ROMAC_PRIMITIVE_TYPE;

extern void	init_romac_primitives(const ROMAC_PRIMITIVE_TYPE *primitive);
const ROMAC_PRIMITIVE_TYPE *romac_get_entry(void);
extern ROMAC_PRIMITIVE_TYPE *romac_primitive;
extern struct dpm_param *dpmp_base_addr;

#define TENTRY() romac_primitive

/**
 * @def   GET_DPMP
 * @brief Use GET_DPMP to obtain DPMP pointers.
 */
#ifdef BUILD_OPT_DA16200_ROMAC
#define GET_DPMP()   (dpmp_base_addr)
#else
#define GET_DPMP()   ((struct dpm_param *) (DA16X_RTM_VAR_BASE))
#endif

//--------------------------------------------------------------------
//	Memory
//--------------------------------------------------------------------

#ifdef	SUPPORT_ROMACMEM_PROFILE

extern void *profile_embromac_malloc(char *name, int line, size_t size);
extern void profile_embromac_free(char *name, int line, void *f);

#define	romac_MALLOC(...)	profile_embromac_malloc( __func__, __LINE__, __VA_ARGS__ )
#define	romac_FREE(...)		profile_embromac_free(  __func__, __LINE__, __VA_ARGS__ )

#else	//SUPPORT_ROMACMEM_PROFILE

extern void *embromac_malloc(size_t size);
extern void embromac_free(void *f);

#define	romac_MALLOC(...)	embromac_malloc( __VA_ARGS__ )
#define	romac_FREE(...)		embromac_free( __VA_ARGS__ )

#endif	//SUPPORT_ROMACMEM_PROFILE

//--------------------------------------------------------------------
//	Console
//--------------------------------------------------------------------

extern void	embromac_dump(UINT16 tag, void *srcdata, UINT16 len);
extern void	embromac_text(UINT16 tag, void *srcdata, UINT16 len);
extern void	embromac_vprint(UINT16 tag, const char *format, va_list arg);
extern void	embromac_print(UINT16 tag, const char *fmt,...);
extern int	embromac_getchar(UINT32 mode );
extern void 	embromac_memset(void * dest, int value, size_t num);

/// @} end of group ROMAC_COM_PRI

#endif /* __romac_primitives_h__ */
