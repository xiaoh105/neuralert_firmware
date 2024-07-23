/**
 ****************************************************************************************
 *
 * @file skbuff.h
 *
 * @brief Header file for WiFi MAC Socket Buffer Data Base
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

#ifndef _DIW_SKBUFF_H
#define _DIW_SKBUFF_H
#include "errno.h"
#include "os_con.h"
#include "lwip/pbuf.h"
#include "lwipopts.h"

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Important Variable for Throughput  **/
#define NX_PAYLOAD_LEN		1572	//1600	//1560	/* should same as NX_RX_PAYLOAD_LEN in rwnx_config.h */

#define NX_RXDESC_COUNT	17//20//25//27	/* should same as NX_RXDESC_CNT in rwnx_config.h */
#define NX_TXDESC_COUNT	18//22//24	/* should be same as PRADA_MAX_PACKETS */

#define BRIDGE_PACKET		12

//#define SK_BUFF_HEADER		152
#define SK_BUFF_HEADER		172 // struct rwnx_txhdr : 140 +  mac hdr: 32
#define PAYLOAD_MARGIN_LEN		PBUF_PAYLOAD_MARGIN_LEN	/* additional attached payload */
#define SEPERATE_WPKT

extern QueueHandle_t	nx_data_tx_queue;

#define STACK_TO_UMAC 		((ULONG)0x0001)
#define MGMT_TO_UMAC		((ULONG)0x0002)
#define MGMT_TO_UMAC2		((ULONG)0x0004)
#define L2_PACKET_TO_SP		((ULONG)0x0008)
#define RX_TO_UMAC		((ULONG)0x0010)
#define ACTION_TO_UMAC		((ULONG)0x0020)
#define RX_TO_FORWARD		((ULONG)0x0400) //
#define TX_TO_FORWARD		((ULONG)0x0800)
#define UMAC_TO_ANY		((ULONG)0xffff)
#define ACTION_TO_UMAC_IF0	((ULONG)0x0100)
#define ACTION_TO_UMAC_IF1	((ULONG)0x0200)

#define dev_kfree_wpkt_any kfree_wpkt
#define dev_kfree_wpkt 	kfree_wpkt
#define alloc_wpkt(a,b) dev_alloc_wpkt(a,1)

struct wpktbuff_head
{
	/* These two members must be first. */
	struct wpktbuff	*next;
	struct wpktbuff	*prev;

	__u32		qlen;
#ifdef SKB_LIST_LOCK
	TX_SEMAPHORE lock;//spinlock_t	lock;
#endif
};

/* Sizeof wpktbuff is 116(112 + margin  */
typedef unsigned char *wpktbuff_data_t;
#pragma pack(4)
struct wpktbuff
{
	/* These two members must be first. */
	struct wpktbuff *next;
	struct wpktbuff *prev;
	struct net_device *dev;

	/* This is the control buffer */
	char cb[52] __aligned(8);

	unsigned int len;
	unsigned int data_len;

#if 1
	__u32 priority;


	//kmemcheck_bitfield_begin(flags1);
	__u8 local_df: 1, cloned: 1, ip_summed: 2, nohdr: 1, nfctinfo: 3;
	// Have to check if these are using or not.
	__u8 pkt_type: 3, fclone: 2, ipvs_property: 1, peeked: 1, nf_trace: 1;
	//kmemcheck_bitfield_end(flags1);
#else
	__u16 	priority;
	__u8	pkt_type;
	__u8	dummy;
#endif

	__be16 protocol;
	__u16 queue_mapping;

#ifdef FEATURE_UMAC_MESH
	wpktbuff_data_t transport_header;
	wpktbuff_data_t network_header;
	wpktbuff_data_t mac_header;
#else
	wpktbuff_data_t network_header;
#endif /* FEATURE_UMAC_MESH */

	/* These elements must be at the end, see alloc_wpkt() for details.  */
	wpktbuff_data_t tail;
	wpktbuff_data_t end;
	unsigned char *head;
	unsigned char *data;

    struct pbuf *pbuf;
    unsigned char t_rx_flag;

};
#pragma pack()

void wpkt_sync(struct wpktbuff *wpkt);

extern void kfree_wpkt(struct wpktbuff *wpkt);
extern void kfree_memp_h(struct wpktbuff *wpkt);
extern int pwpkt_expand_head(struct wpktbuff *wpkt,
							 int nhead, int ntail, gfp_t gfp_mask);

#if 0
static inline unsigned int wpkt_end_offset(const struct wpktbuff *wpkt)
{
	return wpkt->end - wpkt->head;
}
#endif
/**
 ****************************************************************************************
 * @brief Check if a queue is empty.
 * @param[in] list Queue head.
 * @return True if the queue is empty, false otherwise.
 ****************************************************************************************
 */
static inline int wpkt_queue_empty(const struct wpktbuff_head *phead)
{
	return phead->next == (struct wpktbuff *)phead;
}

/**
 ****************************************************************************************
 * @brief Check if wpkt is the last entry in the queue.
 * @param[in] list Queue head.
 * @param[in] wpkt Buffer.
 * @return True if @wpkt is the last buffer on the list.
 ****************************************************************************************
 */
static inline bool wpkt_queue_is_last(const struct wpktbuff_head *phead,
									  const struct wpktbuff *wpkt)
{
	return wpkt->next == (struct wpktbuff *)phead;
}

/**
 ****************************************************************************************
 * @brief Return the next packet in the queue.
 * @param[in] list Queue head.
 * @param[in] wpkt Current buffer.
 * @return Next packet in @list after @wpkt.\n
 * It is only valid to call this if wpkt_queue_is_last() evaluates to false.
 ****************************************************************************************
 */
static inline struct wpktbuff *wpkt_queue_next(const struct wpktbuff_head *phead,
											   const struct wpktbuff *wpkt)
{
	/* This BUG_ON may seem severe, but if we just return then we are going to dereference garbage. */
	//BUG_ON(wpkt_queue_is_last(list, wpkt));
	return wpkt->next;
}

/**
 ****************************************************************************************
 * @brief Peek at the head of an &wpktbuff_head.
 * @param[in] list_ List to peek at.
 ****************************************************************************************
 */
static inline struct wpktbuff *wpkt_peek(const struct wpktbuff_head *list_)
{
	struct wpktbuff *wpkt = list_->next;

	if (wpkt == (struct wpktbuff *)list_)
	{
		wpkt = NULL;
	}

	return wpkt;
}

/**
 ****************************************************************************************
 * @brief Peek at the tail of an &wpktbuff_head.
 * @param[in] list_ List to peek at.
 ****************************************************************************************
 */
static inline struct wpktbuff *wpkt_peek_tail(const struct wpktbuff_head *list_)
{
	struct wpktbuff *wpkt = list_->prev;

	if (wpkt == (struct wpktbuff *)list_)
	{
		wpkt = NULL;
	}

	return wpkt;
}

/**
 ****************************************************************************************
 * @brief Get queue length.
 * @param[in] list_ List to measure.
 * @return Length of an &wpktbuff queue.
 ****************************************************************************************
 */
static inline __u32 wpkt_queue_len(const struct wpktbuff_head *list_)
{
	return list_->qlen;
}

/**
 ****************************************************************************************
 * @brief Initialize non-spinlock portions of wpktbuff_head.
 * @param[in] list Queue to initialize.
 ****************************************************************************************
 */
static inline void __wpkt_queue_head_init(struct wpktbuff_head *phead)
{
	phead->prev = phead->next = (struct wpktbuff *)phead;
	phead->qlen = 0;
}

static inline void wpkt_queue_head_init(struct wpktbuff_head *phead)
{
	/* Chang. Guide for stable sk buffer using. 141006 */
#ifdef SKB_LIST_LOCK
	tx_semaphore_create(&phead->lock, "wpkt_queue_lock", 1);
#endif
	__wpkt_queue_head_init(phead);
}

static inline void __wpkt_insert(struct wpktbuff *newsk,
								 struct wpktbuff *prev, struct wpktbuff *next,
								 struct wpktbuff_head *phead)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev  = prev->next = newsk;
	phead->qlen++;
}

static inline void __wpkt_queue_splice(const struct wpktbuff_head *phead,
									   struct wpktbuff *prev,
									   struct wpktbuff *next)
{
	struct wpktbuff *first = phead->next;
	struct wpktbuff *last = phead->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 ****************************************************************************************
 * @brief Join two wpkt lists and reinitialise the emptied list.
 * @param[in] list
 * @param[in] head
 ****************************************************************************************
 */
#ifdef UMAC_SKB_TX_FLOWCONTROL
static inline void wpkt_queue_splice_init(struct wpktbuff_head *phead,
										  struct wpktbuff_head *head)
{
	if (!wpkt_queue_empty(phead))
	{
		__wpkt_queue_splice(phead, (struct wpktbuff *) head, head->next);
		head->qlen += phead->qlen;
		__wpkt_queue_head_init(phead);
	}
}
#endif

/**
 ****************************************************************************************
 * @brief Join two wpkt lists and reinitialise the emptied list.
 * @param[in] list New list to add.
 * @param[in] head Place to add it in the first list.
 * @details Each of the lists is a queue. The list at @list is reinitialised.
 ****************************************************************************************
 */
static inline void wpkt_queue_splice_tail_init(struct wpktbuff_head *phead,
											   struct wpktbuff_head *head)
{
	if (!wpkt_queue_empty(phead))
	{
		__wpkt_queue_splice(phead, head->prev, (struct wpktbuff *) head);
		head->qlen += phead->qlen;
		__wpkt_queue_head_init(phead);
	}
}

static inline void __wpkt_queue_before(struct wpktbuff_head *phead,
									   struct wpktbuff *next,
									   struct wpktbuff *newsk)
{
	__wpkt_insert(newsk, next->prev, next, phead);
}

extern void wpkt_queue_tail(struct wpktbuff_head *phead, struct wpktbuff *newsk);

/**
 ****************************************************************************************
 * @brief Queue a buffer at the list tail.
 * @param[in] list List to use.
 * @param[in] newsk Buffer to queue.
 * @details Queue a buffer at the end of a list.\n
 * This function takes no locks and you must therefore hold required locks before calling it.\n\n
 * A buffer cannot be placed on two lists at the same time.
 ****************************************************************************************
 */
static inline void __wpkt_queue_tail(struct wpktbuff_head *phead,
									 struct wpktbuff *newsk)
{
	__wpkt_queue_before(phead, (struct wpktbuff *)phead, newsk);
}

extern void wpkt_unlink(struct wpktbuff *wpkt, struct wpktbuff_head *phead);

/**
 ****************************************************************************************
 * @brief Remove wpktbuff from list.
 * @details Must be called atomically, and with the list known.
 ****************************************************************************************
 */
static inline void __wpkt_unlink(struct wpktbuff *wpkt, struct wpktbuff_head *phead)
{
	struct wpktbuff *next, *prev;

	phead->qlen--;
	next		= wpkt->next;
	prev		= wpkt->prev;
	wpkt->next	= wpkt->prev = NULL;
	next->prev	= prev;
	prev->next	= next;
}

extern struct wpktbuff *wpkt_dequeue(struct wpktbuff_head *phead);
extern struct wpktbuff *wpkt_dequeue_ps(struct wpktbuff_head *phead);

/**
 ****************************************************************************************
 * @brief Remove from the head of the queue.
 * @param[in] list List to dequeue from.
 ****************************************************************************************
 */
static inline struct wpktbuff *__wpkt_dequeue(struct wpktbuff_head *phead)
{
	struct wpktbuff *wpkt = wpkt_peek(phead);

	if (wpkt)
	{
		__wpkt_unlink(wpkt, phead);
	}

	return wpkt;
}

static inline bool wpkt_is_nonlinear(const struct wpktbuff *wpkt)
{
	return wpkt->data_len;
}

static inline unsigned int wpkt_headlen(const struct wpktbuff *wpkt)
{
	return wpkt->len - wpkt->data_len;
}

static inline unsigned char *wpkt_tail_pointer(const struct wpktbuff *wpkt)
{
	return wpkt->tail;
}

static inline void wpkt_set_tail_pointer(struct wpktbuff *wpkt, const int offset)
{
	wpkt->tail = wpkt->data + offset;
}

extern unsigned char *wpkt_put(struct wpktbuff *wpkt, unsigned int len);

#define wpkt_push(a,b) __wpkt_push(a,b)
static inline unsigned char *__wpkt_push(struct wpktbuff *wpkt, unsigned int len)
{

	wpkt->data -= len;
	wpkt->len  += len;
#ifndef STACK_TO_ZEROCOPY
    wpkt->pbuf->payload = wpkt->data;
    wpkt->pbuf->len = wpkt->len;
#endif

	return wpkt->data;
}

extern unsigned char *wpkt_pull(struct wpktbuff *wpkt, unsigned int len);
static inline unsigned char *__wpkt_pull(struct wpktbuff *wpkt, unsigned int len)
{
	wpkt->len -= len;
    wpkt->data += len;

#ifndef STACK_TO_ZEROCOPY
    wpkt->pbuf->payload = wpkt->data;
    wpkt->pbuf->len = wpkt->len;
#endif
	return wpkt->data;
}

static inline unsigned char *wpkt_pull_inline(struct wpktbuff *wpkt, unsigned int len)
{
	return unlikely(len > wpkt->len) ? NULL : __wpkt_pull(wpkt, len);
}

extern unsigned char *__pwpkt_pull_tail(struct wpktbuff *wpkt, int delta);

static inline int pwpkt_may_pull(struct wpktbuff *wpkt, unsigned int len)
{
	if (likely(len <= wpkt_headlen(wpkt)))
	{
		return 1;
	}

	if (unlikely(len > wpkt->len))
	{
		return 0;
	}

	return __pwpkt_pull_tail(wpkt, len - wpkt_headlen(wpkt)) != NULL;
}

/* Chang, for the umac interface */
static inline int wpkt_add_data(struct wpktbuff *wpkt, char __user *from, int copy)
{
#if 1 /* Check it lator which one is better */
	memcpy(wpkt_put(wpkt, copy), from, copy);
#else
	//unsigned char *temp;
	wpkt_put(wpkt, copy);
	wpkt->data = (unsigned char *)from;
#endif
	return 0;
}

/**
 ****************************************************************************************
 * @brief Bytes at buffer head.
 * @param[in] wpkt Buffer to check.
 * @return Number of bytes of free space at the head of an &wpktbuff.
 ****************************************************************************************
 */
static inline unsigned int wpkt_headroom(const struct wpktbuff *wpkt)
{
	unsigned int temp;
	temp = wpkt->data - wpkt->head;
	return temp;
}

/**
 ****************************************************************************************
 * @brief Bytes at buffer end.
 * @param[in] wpkt Buffer to check.
 * @return Number of bytes of free space at the tail of an wpktbuff.
 ****************************************************************************************
 */
static inline int wpkt_tailroom(const struct wpktbuff *wpkt)
{
	int temp;
	temp = wpkt->end - wpkt->tail;
	return temp;
}

static inline void wpkt_reserve(struct wpktbuff *wpkt, int len)
{
	wpkt->data += len;
	wpkt->tail += len;
}

#ifdef FEATURE_UMAC_MESH
static inline unsigned char *wpkt_transport_header(const struct wpktbuff *wpkt)
{
	return wpkt->transport_header;
}

static inline void wpkt_set_transport_header(struct wpktbuff *wpkt,
					    const int offset)
{
	wpkt->transport_header = wpkt->data + offset;
}
#endif /* FEATURE_UMAC_MESH */

static inline unsigned char *wpkt_network_header(const struct wpktbuff *wpkt)
{
	return wpkt->network_header;
}

static inline void wpkt_set_network_header(struct wpktbuff *wpkt, const int offset)
{
	wpkt->network_header = wpkt->data + offset;
}

static inline int wpkt_network_offset(const struct wpktbuff *wpkt)
{
	return wpkt_network_header(wpkt) - wpkt->data;
}

#ifdef FEATURE_UMAC_MESH
static inline unsigned char *wpkt_mac_header(const struct wpktbuff *wpkt)
{
	return wpkt->mac_header;
}

static inline void wpkt_set_mac_header(struct wpktbuff *wpkt, const int offset)
{
	wpkt->mac_header = wpkt->data + offset;
}
#endif /* FEATURE_UMAC_MESH */

static inline void __wpkt_trim(struct wpktbuff *wpkt, unsigned int len)
{
	if (unlikely(wpkt_is_nonlinear(wpkt)))
	{
		return;
	}

	wpkt->len = len;
	wpkt_set_tail_pointer(wpkt, len);
}

extern void wpkt_trim(struct wpktbuff *wpkt, unsigned int len);
#ifdef FEATURE_AMSDU_AND_DEFRAGEMENT
extern void nx_chain_trim(struct wpktbuff *wpkt, unsigned int len);
#endif

#ifndef FEATURE_AMSDU_AND_DEFRAGEMENT
static inline int __pwpkt_trim(struct wpktbuff *wpkt, unsigned int len)
{
	//if (wpkt->data_len)
	//	return ___pwpkt_trim(wpkt, len);
	__wpkt_trim(wpkt, len);
	return 0;
}
static inline int pwpkt_trim(struct wpktbuff *wpkt, unsigned int len)
{
	return (len < wpkt->len) ? __pwpkt_trim(wpkt, len) : 0;
}
#endif
/**
 ****************************************************************************************
 * @brief Empty a list.
 * @param[in] list List to empty.
 ****************************************************************************************
 */
extern void wpkt_queue_purge(struct wpktbuff_head *phead);
static inline void __wpkt_queue_purge(struct wpktbuff_head *phead)
{
	struct wpktbuff *wpkt;

	while ((wpkt = __wpkt_dequeue(phead)) != NULL)
	{
		kfree_wpkt(wpkt);
	}
}

extern struct wpktbuff *dev_alloc_wpkt(int length, bool wait);
extern struct wpktbuff *dev_alloc_wpkt_rx(int length, bool waitflag);
extern struct wpktbuff *dev_alloc_wpkt_rx_h(int length, bool wait);

static inline int __wpkt_linearize(struct wpktbuff *wpkt)
{
	return __pwpkt_pull_tail(wpkt, wpkt->data_len) ? 0 : -ENOMEM;
}

/**
 ****************************************************************************************
 * @brief Convert paged wpkt to linear one.
 * @param[in] wpkt Buffer to linarize.
 * @return If there is no free memory -ENOMEM is returned,
 * otherwise zero is returned and the old wpkt data released.
 ****************************************************************************************
 */
static inline int wpkt_linearize(struct wpktbuff *wpkt)
{
	return wpkt_is_nonlinear(wpkt) ? __wpkt_linearize(wpkt) : 0;
}

#define wpkt_queue_walk(queue, wpkt) \
	for (wpkt = (queue)->next; \
		wpkt != (struct wpktbuff *)(queue); \
		wpkt = wpkt->next)

#define wpkt_queue_walk_safe(queue, wpkt, tmp) \
	for (wpkt = (queue)->next, tmp = wpkt->next; \
		wpkt != (struct wpktbuff *)(queue); \
		wpkt = tmp, tmp = wpkt->next)

extern int wpkt_copy_bits(const struct wpktbuff *wpkt, int offset,
						  void *to, int len);

static inline void wpkt_copy_from_linear_data_offset(const struct wpktbuff *wpkt,
													 const int offset, void *to,
													 const unsigned int len)
{
	memcpy(to, wpkt->data + offset, len);
}

static inline void wpkt_set_queue_mapping(struct wpktbuff *wpkt, u16 queue_mapping)
{
	wpkt->queue_mapping = queue_mapping;
}

static inline u16 wpkt_get_queue_mapping(const struct wpktbuff *wpkt)
{
	return wpkt->queue_mapping;
}

static inline void wpkt_copy_queue_mapping(struct wpktbuff *to,
										   const struct wpktbuff *from)
{
	to->queue_mapping = from->queue_mapping;
}
#endif	/* _DIW_SKBUFF_H */
