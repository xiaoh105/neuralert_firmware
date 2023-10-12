/**
 ****************************************************************************************
 *
 * @file util_func.h
 *
 * @brief util function for mqtt client
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

#ifndef	__UTIL_FUNC_H__
#define	__UTIL_FUNC_H__

UINT8 ut_is_task_running(TaskStatus_t* task_status);

void ut_sock_reset (int* sock);

void ut_sock_convert2tv(struct timeval* tv, ULONG ultime);

void ut_sock_convert2ultime(struct timeval* tv, ULONG* ultime);

void ut_sock_opt_get_timeout(mosq_sock_t sock, ULONG* timeval);

int ut_sock_opt_set_timeout (mosq_sock_t sock, ULONG timeval, UINT8 for_recv);

void ut_mqtt_sem_create(mqtt_mutex_t* mq_mutex);

void ut_mqtt_sem_delete(mqtt_mutex_t* mq_mutex);

void ut_mqtt_sem_take_recur(mqtt_mutex_t* mq_mutex, TickType_t wait_opt);

int ut_mqtt_is_topic_existing(char* topic);

#endif /* __UTIL_FUNC_H__ */
