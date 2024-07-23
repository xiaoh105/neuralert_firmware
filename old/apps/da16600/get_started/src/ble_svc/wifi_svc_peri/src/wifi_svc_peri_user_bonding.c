/**
 ****************************************************************************************
 *
 * @file user_bonding.c
 *
 * @brief bonding
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

#if defined (__BLE_PERI_WIFI_SVC_PERIPHERAL__)

#if defined (__MULTI_BONDING_SUPPORT__)

#include "../../include/da14585_config.h"
#include "../../include/app.h"
#include "../../include/user_bonding.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "da16_gtl_features.h"

struct gapc_ltk rand_ltk;

struct app_sec_env_tag temp_bond_info; // current bond info
struct app_sec_env_tag bond_info[MAX_BOND_PEER]; // Security DB
struct gap_sec_key temp_irks[MAX_BOND_PEER]; // 

uint8_t bond_info_current_index;

extern dpm_boot_type_t dpm_boot_type;

/**
 ****************************************************************************************
 * @brief   Clear the Identiry Resolving Key array. The array holding the irks of all the
 *          bonded hosts is cleared.
 * @param   {none}
 * @returns {none}
 ****************************************************************************************
 */
void user_bonding_clear_irks(void)
{
    memset(temp_irks, 0, MAX_BOND_PEER * sizeof(struct gap_sec_key));  
}

/**
 ****************************************************************************************
 * @brief Deselect the bonded host. The index in the list of bonded host entries is 
 *        invalidated
 * @param   {none}
 * @returns {none}
 ****************************************************************************************
 */
void user_bonding_deselect_bonded_device(void)
{
    bond_info_current_index = BOND_INFO_CURRENT_INDEX_NONE;
}

/**
 ****************************************************************************************
 * @brief Returns the index of the currently bonded hosts in the list of bonded hosts.
 *        
 * @param   {none}
 * @returns The index of the currently bonded host in the list of bonded hosts.
 *          BOND_INFO_CURRENT_INDEX_NONE if the currently connected host is not found
 *          in the list of bonded hosts (unknown host connected)
 ****************************************************************************************
 */
uint8_t user_bonding_get_bond_info_current_index(void)
{
    return bond_info_current_index;
}

/**
 ****************************************************************************************
 * @brief Initialized the user_bonding structures.
 *        The following are cleared:
 *         1) The bond_info structure
 *         2) The irks array
 *         3) the bond_info_current_index
 *        
 * @param   {none}
 * @returns {none}
 ****************************************************************************************
 */
uint8_t empty_key[KEY_LEN];
void user_bonding_init(void)
{
    memset(bond_info, 0, sizeof(bond_info));
    user_bonding_clear_irks();
    user_bonding_deselect_bonded_device();

	if (dpm_boot_type <= DPM_BOOT_TYPE_NORMAL)
	{
		// load bond_info[] from sflash
		user_read_bond_data_from_sflash();
	}

	memset((void*)empty_key, 0xFF, KEY_LEN);
	if (memcmp((uint8_t*)(bond_info[0].ltk.key), (uint8_t*)empty_key, KEY_LEN) == 0)
	{
		// bond_info has not written before, init bond_info[] with 0x00
		memset(bond_info, 0, sizeof(bond_info));
	}
}

/**
 ****************************************************************************************
 * @brief Sends a request to retrieve the irk of a host which uses random address.
 *        
 *
 * @param    addr : The address of the host (random address)
 * @param    irks : The list of the irks of all the bonded hosts
 * @param counter : The count of the irks provided
 * @returns {none}
 ****************************************************************************************
 */
void user_resolve_random_address(uint8_t * addr, uint8_t *irks, uint16_t counter)
{

    struct gapm_resolv_addr_cmd *cmd = BleMsgAlloc(GAPM_RESOLV_ADDR_CMD, 
                                            TASK_ID_GAPM, TASK_ID_GTL,
                                             sizeof(struct gapm_resolv_addr_cmd) + (counter * sizeof(struct gap_sec_key)));
    
    cmd->operation = GAPM_RESOLV_ADDR; // GAPM requested operation
    cmd->nb_key = counter; // Number of provided IRK 
    memcpy( &cmd->addr, addr, 6); // Resolvable random address to solve
    memcpy(cmd->irk, irks, counter * sizeof(struct gap_sec_key));

    // Send the message
    BleSendMsg(cmd);
}

/**
 ****************************************************************************************
 * @brief Retrieves the index of the bonded data for a host with public address.
 *
 * @param   addr : The address of the host (public address)
 * @returns The index of the host with the provided address in the list of bonded hosts.
 *          BOND_INFO_CURRENT_INDEX_NONE no host with the provided address is not found
 *          in the list of bonded hosts (unknown host connected)
 ****************************************************************************************
 */
uint8_t user_find_bonded_public( uint8_t * addr)
{
    for (uint8_t i=0; i<MAX_BOND_PEER; i++)
    {
        if (bond_info[i].auth &&
            ((bond_info[i].peer_addr_type == ADDR_PUBLIC) || ( (bond_info[i].peer_addr_type == ADDR_RAND) && ((bond_info[i].peer_addr.addr[BD_ADDR_LEN-1] & GAP_STATIC_ADDR) == GAP_STATIC_ADDR))) &&
            (!memcmp(bond_info[i].peer_addr.addr, addr, 6)))
        {
            bond_info_current_index = i;
            return i;
        }
    } 
    return BOND_INFO_CURRENT_INDEX_NONE; // No host with the provided address is found
}

extern uint8_t addr_resolv_req_from_gapc_bond_ind;

/**
 ****************************************************************************************
 * @brief Prepares the irks array (with irks for all bonded hosts) and calls the 
 *        user_resolve_random_address() to request resolution for the provided random
 *        address
 *
 * @param     addr : The address of the host (random address)
 * @returns   true : if at least one irk is found. The request is sent
 *           false : if no irk is found. The request is not sent
 ****************************************************************************************
 */
bool user_resolve_addr_rand(uint8_t * addr, uint8_t is_in_bonding_process)
{
    uint8_t counter = 0;
    
    addr_resolv_req_from_gapc_bond_ind = is_in_bonding_process;
    
    for (uint8_t i=0; i<MAX_BOND_PEER; i++)
    { 
        // Prepare the array containing the irks for all the bonded hosts
        if (bond_info[i].auth && (bond_info[i].peer_addr_type == ADDR_RAND) )
        {
            memcpy(&temp_irks[counter], &bond_info[i].irk, sizeof(struct gap_sec_key));
            counter++; 
			bond_info_current_index = i;
        }
    }
    
    if (counter)
    {
        // Request resolution for the provided random address
        user_resolve_random_address(addr, (uint8_t *)&temp_irks, counter);
        return true;
    }
    return false;
}


/**
 ****************************************************************************************
 * @brief Retrieves the index in the list of the bonded hosts after successful resolution 
 *        see: user_resolve_random_address() and user_resolve_addr_rand()
 *
 * @param   irk : the resolved Identity Resolution key (provided by the stack)
 * @returns The index of the host which has the provided irk in the list of bonded hosts.
 *          BOND_INFO_CURRENT_INDEX_NONE if the host is not found in the list of bonded
 *          hosts (unknown host connected)
 ****************************************************************************************
 */
uint8_t user_resolved_rand_addr_success(struct gap_sec_key * irk, uint8_t store_bonding_data)
{
    for (uint8_t i=0; i<MAX_BOND_PEER; i++)
    { 
        if (bond_info[i].auth && (bond_info[i].peer_addr_type == ADDR_RAND) )
        {
            if (!memcmp(&bond_info[i].irk, irk, sizeof(struct gap_sec_key)))
            {
                bond_info_current_index = i;
                if (store_bonding_data == true)
                {
                    user_store_bond_data();
                }
                return i;
            }
        }
    }
    
    return BOND_INFO_CURRENT_INDEX_NONE;
}

/**
 ****************************************************************************************
 * @brief Store bond data in serial flash
 *        
 * @param   {none}
 * @returns true : if flash write success
 *          false : if flash write not successful
 ****************************************************************************************
 */
UINT user_store_bond_data_in_sflash(void)
{
		return ble_sflash_write(SFLASH_USER_AREA_BLE_SECURITY_DB, 
					   	  (UCHAR*)bond_info, 
					      sizeof(bond_info));	
}



/**
 ****************************************************************************************
 * @brief Read bond data from serial flash
 *        
 * @param   {none}
 * @returns true : if flash read success
 *          false : if flash read not successful
 ****************************************************************************************
 */
UINT user_read_bond_data_from_sflash(void)
{
		return ble_sflash_read(SFLASH_USER_AREA_BLE_SECURITY_DB, 
					   	  (UCHAR*)bond_info, 
					      sizeof(bond_info));	
}


/**
 ****************************************************************************************
 * @brief Stores the bonding data for the currently connected host.
 *        The bonding data for the currently connected host is stored in the list of
 *        bonded hosts.
 *        
 * @param   {none}
 * @returns true : if the entry for the currently connected host is found (already bonded)
 *          false : if the entry for the currently connected host is found (unknown host)
 ****************************************************************************************
 */
bool user_store_bond_data(void) 
{
    if (bond_info_current_index != 0xff)
    {
        memcpy ((uint8_t *)&bond_info[bond_info_current_index],(uint8_t *) &temp_bond_info, sizeof(struct app_sec_env_tag));
		if (user_store_bond_data_in_sflash() == TRUE)
		{
        	return true;
		}
    }
    else
    {
        for (uint8_t i=0; i<MAX_BOND_PEER; i++)
        { 
            if (!bond_info[i].auth)
            {
                memcpy ((uint8_t *) &bond_info[i],(uint8_t *) &temp_bond_info, sizeof(struct app_sec_env_tag)); 
				bond_info_current_index = i;

				if (user_store_bond_data_in_sflash() == TRUE)
				{
					return true;
				}
            }
                
        }
    }
        
    return false;
}

#endif /* __MULTI_BONDING_SUPPORT__ */

#endif /* __BLE_PERI_WIFI_SVC_PERIPHERAL__ */

/* EOF */
