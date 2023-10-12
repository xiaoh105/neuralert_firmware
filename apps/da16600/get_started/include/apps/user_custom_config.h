/**
****************************************************************************************
 *
 * @file user_custom_config.h
 *
 * @brief User Configuration file.
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

#ifndef USER_CUSTOM_CONFIG__H_
#define USER_CUSTOM_CONFIG__H_

/* ================================================
 * BLE: Peripheral Role(Slave).
 * WiFi service.
 * ================================================
 */
#define  __BLE_PERI_WIFI_SVC__

/* ================================================
 * BLE: Peripheral Role(Slave).
 * TCP DPM sample.
 * ================================================
 */
#undef  __BLE_PERI_WIFI_SVC_TCP_DPM__

/* ================================================
 * BLE: Peripheral Role(Slave).
 * BLE chip peripheral samples.
 * ================================================
 */
#undef  __BLE_PERI_WIFI_SVC_PERIPHERAL__

/* ================================================
 * BLE: Central Role(Master).
 * Sensor Gateway sample.
 * ================================================
 */
#undef  __BLE_CENT_SENSOR_GW__


#if defined(__BLE_PERI_WIFI_SVC__) || defined(__BLE_PERI_WIFI_SVC_TCP_DPM__) || defined(__BLE_CENT_SENSOR_GW__)
	#undef __CFG_ENABLE_BLE_HW_RESET__		// Enable H/W reset of BLE
#endif

#endif /* USER_CUSTOM_CONFIG__H_ */
