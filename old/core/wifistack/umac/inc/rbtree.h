/**
 ****************************************************************************************
 *
 * @file rbtree.h
 *
 * @brief Header file for WiFi MAC Protocol RoundRobin Tree
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

#ifndef	_DIW_RBTREE_H
#define	_DIW_RBTREE_H

#include <stddef.h>

struct diw_rb_node
{
	unsigned long  diw_rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct diw_rb_node *rb_right;
	struct diw_rb_node *rb_left;
};

struct diw_rb_root
{
	struct diw_rb_node *diw_rb_node;
};

#define diw_rb_parent(r) ((struct diw_rb_node *)((r)->diw_rb_parent_color & ~3))
#define diw_rb_color(r) ((r)->diw_rb_parent_color & 1)
#define diw_rb_is_red(r) (!diw_rb_color(r))
#define diw_rb_is_black(r) diw_rb_color(r)
#define diw_rb_set_red(r) do {(r)->diw_rb_parent_color &= ~1;} while (0)
#define diw_rb_set_black(r) do {(r)->diw_rb_parent_color |= 1;} while (0)

static inline void diw_rb_set_parent(struct diw_rb_node *rb, struct diw_rb_node *p)
{
	rb->diw_rb_parent_color = (rb->diw_rb_parent_color & 3) | (unsigned long)p;
}
static inline void diw_rb_set_color(struct diw_rb_node *rb, int color)
{
	rb->diw_rb_parent_color = (rb->diw_rb_parent_color & ~1) | color;
}

#define RB_ROOT	(struct diw_rb_root) { NULL, }
#define	diw_rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)	((root)->diw_rb_node == NULL)
#define RB_EMPTY_NODE(node)	(diw_rb_parent(node) == node)
#define RB_CLEAR_NODE(node)	(diw_rb_set_parent(node, node))

extern void diw_rb_insert_color(struct diw_rb_node *, struct diw_rb_root *);
extern void diw_rb_erase(struct diw_rb_node *, struct diw_rb_root *);

typedef void (*rb_augment_f)(struct diw_rb_node *node, void *data);

static inline void diw_rb_link_node(struct diw_rb_node *node, struct diw_rb_node *parent,
									struct diw_rb_node **rb_link)
{
	node->diw_rb_parent_color = (unsigned long )parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

#endif	/* _DIW_RBTREE_H */
