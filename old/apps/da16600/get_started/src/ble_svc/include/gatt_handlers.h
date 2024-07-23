/**
 ****************************************************************************************
 *
 * @file gatt_handlers.h
 *
 * @brief GATTM / GATTC handlers.
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


#ifndef _GATT_HANDLERS_H_
#define _GATT_HANDLERS_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"
#include "gattm_task.h"                
#include "gattc_task.h"                 
#include "../../../../../../core/ble_interface/ble_inc/platform/core_modules/common/api/co_error.h"
#include "att.h"
#include "ble_msg.h"



/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
 
                                                                     
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */



uint8_t gattm_add_svc_rsp_hnd(ke_msg_id_t const msgid,
                          struct gattm_add_svc_rsp const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);

//CFC                          
//uint8_t gattm_add_att_rsp_hnd(ke_msg_id_t const msgid,
//                          struct gattm_add_attribute_rsp const *param,
//                          ke_task_id_t const dest_id,
//                          ke_task_id_t const src_id);

uint8_t gattm_att_set_value_rsp_hnd(ke_msg_id_t const msgid,
                          struct gattm_att_set_value_rsp const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);

//CFC                          
//uint8_t gattc_write_cmd_ind_hnd(ke_msg_id_t const msgid,
//                          struct gattc_write_cmd_ind const *param,
//                          ke_task_id_t const dest_id,
//                          ke_task_id_t const src_id);		
                          
uint8_t gattm_svc_set_permission_rsp_hnd(ke_msg_id_t const msgid,
                          struct gattm_svc_set_permission_rsp const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);
                          
uint8_t gattm_att_set_permission_rsp_hnd(ke_msg_id_t const msgid,
                          struct gattm_att_set_permission_rsp const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);                          

uint8_t gattc_cmp_evt_hnd(ke_msg_id_t const msgid,
                          struct gattc_cmp_evt const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);
                          
uint8_t gattc_att_info_req_ind_hnd (ke_msg_id_t const msgid,
                          struct gattc_att_info_req_ind const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);
                          
uint8_t gattc_read_req_ind_hnd (ke_msg_id_t const msgid,
                          struct gattc_read_req_ind const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id);
                         
uint8_t gattc_write_req_ind_hnd (ke_msg_id_t const msgid,
                          struct gattc_write_req_ind const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id, uint8_t triggered_by_write_cmd);

                          
#endif //_GATT_HANDLERS_

/* EOF */
