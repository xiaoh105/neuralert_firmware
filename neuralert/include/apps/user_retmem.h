/**
 ****************************************************************************************
 *
 * @file user_retmem.h
 *
 * @brief Header file for the example for an implementation of user retention memory API.
 *
 * Copyright (c) 2016-2021 Dialog Semiconductor. All rights reserved.
 *
 * This software ("Software") is owned by Dialog Semiconductor.
 *
 * By using this Software you agree that Dialog Semiconductor retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Dialog Semiconductor is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Dialog Semiconductor products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * DIALOG SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#ifndef USER_MAIN_INCLUDE_USER_RETMEM_H_
#define USER_MAIN_INCLUDE_USER_RETMEM_H_

/**
 ****************************************************************************************
 * @brief Initialize User Retention Memory
 *
 * @return void.
 ****************************************************************************************
 */
void user_retmem_init(void);

/**
 ****************************************************************************************
 * @brief De-initialize User Retention Memory
 *
 * @return void.
 ****************************************************************************************
 */
void user_retmem_deinit(void);

/**
 ****************************************************************************************
 * @brief User Retention Memory Allocation
 *
 * @Param[in] name 		 	a name of the memory
 * @Param[in] memory_ptr	a pointer of the memory
 * @Param[in] memory_size 	a size of the memory
 *
 * @return unsigned int 	0 or error code
 ****************************************************************************************
 */
unsigned int user_retmmem_allocate(char *name,
								   void **memory_ptr,
								   unsigned long memory_size);

/**
 ****************************************************************************************
 * @brief User Retention Memory Release
 *
 * @Param[in] name 		 	a name of the memory
 *
 * @return unsigned int 	0 or error code
 ****************************************************************************************
 */
unsigned int user_retmmem_release(char *name);

/**
 ****************************************************************************************
 * @brief Get the pointer of the User Retention Memory
 *
 * @Param[in] name 		 	a name of the memory
 * @Param[in] memory_ptr	a pointer of the memory
 *
 * @return unsigned int 	the size of the memory
 ****************************************************************************************
 */
unsigned int user_retmmem_get(char *name, unsigned char **data);

/**
 ****************************************************************************************
 * @brief Print out the status of the User Retention Memory
 *
 * @return void.
 ****************************************************************************************
 */
void user_retmmem_info_get(void);

#endif /* USER_MAIN_INCLUDE_USER_RETMEM_H_ */
