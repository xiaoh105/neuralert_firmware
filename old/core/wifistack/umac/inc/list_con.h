/**
 ****************************************************************************************
 *
 * @file list_con.h
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

#ifndef _LIST_CON_H
#define _LIST_CON_H

#include "os_con.h"

static inline void __diw_list_add(struct diw_list_head *new,
				  struct diw_list_head *prev,
				  struct diw_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct diw_list_head *new, struct diw_list_head *head)
{
	__diw_list_add(new, head, head->next);
}

static inline void list_add_tail(struct diw_list_head *new, struct diw_list_head *head)
{
	__diw_list_add(new, head->prev, head);
}

static inline void __diw_list_del(struct diw_list_head *prev, struct diw_list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void __diw_list_del_entry(struct diw_list_head *diwentry)
{
	__diw_list_del(diwentry->prev, diwentry->next);
}

#define LIST_POISON1  NULL
#define LIST_POISON2  NULL

static inline void list_del(struct diw_list_head *diwentry)
{
	__diw_list_del(diwentry->prev, diwentry->next);
	diwentry->next = (struct diw_list_head *)LIST_POISON1;
	diwentry->prev = (struct diw_list_head *)LIST_POISON2;
}

static inline void list_del_init(struct diw_list_head *diwentry)
{
	__diw_list_del_entry(diwentry);
	INIT_LIST_HEAD(diwentry);
}

static inline void list_move(struct diw_list_head *diwlist, struct diw_list_head *head)
{
	__diw_list_del_entry(diwlist);
	list_add(diwlist, head);
}

static inline void list_move_tail(struct diw_list_head *diwlist,
								  struct diw_list_head *head)
{
	__diw_list_del_entry(diwlist);
	list_add_tail(diwlist, head);
}

static inline int list_empty(const struct diw_list_head *head)
{
	return head->next == head;
}

static inline void list_rotate_left(struct diw_list_head *head)
{
	struct diw_list_head *first;

	if (!list_empty(head))
	{
		first = head->next;
		list_move_tail(first, head);
	}
}

static inline void __list_splice(const struct diw_list_head *diwlist,
								 struct diw_list_head *prev,
								 struct diw_list_head *next)
{
	struct diw_list_head *first = diwlist->next;
	struct diw_list_head *last = diwlist->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void list_splice_init(struct diw_list_head *diwlist,
									struct diw_list_head *head)
{
	if (!list_empty(diwlist))
	{
		__list_splice(diwlist, head, head->next);
		INIT_LIST_HEAD(diwlist);
	}
}

#ifdef FEATURE_UMAC_MESH
#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct diw_hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HLIST_NODE(struct diw_hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int hlist_unhashed(const struct diw_hlist_node *h)
{
	return !h->pprev;
}

static inline int hlist_empty(const struct diw_hlist_head *h)
{
	return !h->first;
}
#endif /* FEATURE_UMAC_MESH */

static inline void __hlist_del(struct diw_hlist_node *n)
{
	struct diw_hlist_node *next = n->next;
	struct diw_hlist_node **pprev = n->pprev;
	*pprev = next;

	if (next)
	{
		next->pprev = pprev;
	}
}

static inline void hlist_del(struct diw_hlist_node *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

#define hlist_del_rcu(a) hlist_del(a)

static inline void hlist_add_head(struct diw_hlist_node *n, struct diw_hlist_head *h)
{
	struct diw_hlist_node *first = h->first;
	n->next = first;

	if (first)
	{
		first->pprev = &n->next;
	}

	h->first = n;
	n->pprev = &h->first;
}

#define hlist_for_each_entry(cur, head, member)				\
	for (cur = hlist_entry_safe((head)->first, typeof(*(cur)), member);\
	     cur;							\
	     cur = hlist_entry_safe((cur)->member.next, typeof(*(cur)), member))

#ifdef FEATURE_UMAC_MESH
#define hlist_first_rcu(head)	(*((struct diw_hlist_node __rcu **)(&(head)->first)))
#define hlist_next_rcu(node)	(*((struct diw_hlist_node __rcu **)(&(node)->next)))
#define hlist_pprev_rcu(node)	(*((struct diw_hlist_node __rcu **)((node)->pprev)))	

static inline void hlist_add_head_rcu(struct diw_hlist_node *n,
					struct diw_hlist_head *h)
{
	struct diw_hlist_node *first = h->first;

	n->next = first;
	n->pprev = &h->first;
	hlist_first_rcu(h) = n;
	if (first)
		first->pprev = &n->next;
}

#define hlist_for_each_entry_rcu(cur, head, member) \
	for (cur = hlist_entry_safe(hlist_first_rcu(head), typeof(*(cur)), member); \
		 cur; \
		 cur = hlist_entry_safe(hlist_next_rcu(&(cur)->member), typeof(*(cur)), member))	     
#endif /* FEATURE_UMAC_MESH */

#endif /* _LIST_CON_H */
