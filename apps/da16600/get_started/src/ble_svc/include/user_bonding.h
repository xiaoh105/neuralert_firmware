/**
 ****************************************************************************************
 *
 * @file user_bonding.h
 *
 * @brief bonding - header file
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

#ifndef USER_BONDING_H_
#define USER_BONDING_H_

#define MAX_BOND_PEER  (10)

#define BOND_INFO_CURRENT_INDEX_NONE (0xFF)

#include "gap.h"
#include "ble_msg.h"

/**
 ****************************************************************************************
 * @brief   Clear the Identiry Resolving Key array. The array holding the irks of all the
 *          bonded hosts is cleared.
 * @param   {none}
 * @returns {none}
 ****************************************************************************************
 */
void user_bonding_clear_irks(void);

/**
 ****************************************************************************************
 * @brief Deselect the bonded host. The index in the list of bonded host entries is 
 *        invalidated
 * @param   {none}
 * @returns {none}
 ****************************************************************************************
 */
void user_bonding_deselect_bonded_device(void);

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
uint8_t user_bonding_get_bond_info_current_index(void);
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
void user_bonding_init(void);

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
void user_resolve_random_address(uint8_t * addr, uint8_t *irks, uint16_t counter);

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
uint8_t user_find_bonded_public( uint8_t * addr);

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
bool user_resolve_addr_rand(uint8_t * addr, uint8_t for_security_request);

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
uint8_t user_resolved_rand_addr_success(struct gap_sec_key * irk, uint8_t do_not_store_bonding_data);

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
bool user_store_bond_data(void);

unsigned int user_read_bond_data_from_sflash(void);

unsigned int user_store_bond_data_in_sflash(void);

#endif //USER_BONDING_H_

/* EOF */
