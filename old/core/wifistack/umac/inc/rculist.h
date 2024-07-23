/**
 ****************************************************************************************
 *
 * @file rculist.h
 *
 * @brief Header file
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

#ifndef _DIW_RCULIST_H
#define _DIW_RCULIST_H

#include <list_con.h>
//#include <os_con.h>

#define list_next_rcu(list)	(*((struct diw_list_head __rcu **)(&(list)->next)))

#ifndef CONFIG_DEBUG_LIST
static inline void __diw_list_add_rcu(struct diw_list_head *new,
									  struct diw_list_head *prev,
									  struct diw_list_head *next)
{
	new->next = next;
	new->prev = prev;
	list_next_rcu(prev) = new;
	next->prev = new;
}
#else
extern void __diw_list_add_rcu(struct diw_list_head *new,
							   struct diw_list_head *prev,
							   struct diw_list_head *next);
#endif

static inline void diw_list_add_rcu(struct diw_list_head *new,
									struct diw_list_head *head)
{
	__diw_list_add_rcu(new, head, head->next);
}

static inline void diw_list_add_tail_rcu(struct diw_list_head *new,
		struct diw_list_head *head)
{
	__diw_list_add_rcu(new, head->prev, head);
}

static inline void diw_list_del_rcu(struct diw_list_head *entry)
{
	__diw_list_del_entry(entry);
	entry->prev = LIST_POISON2;
}

#endif
