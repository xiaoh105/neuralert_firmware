/**
 ****************************************************************************************
 *
 * @file os_con.h
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

#ifndef _OS_CON_H
#define _OS_CON_H

#include "FreeRTOS.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include "features.h"
#include "target.h"
#include "da16200_system.h"
#ifdef SUPPORT_FREERTOS
#include "timers.h"
#endif

#ifdef	BUILD_OPT_FC9000_ROMNDK
#include "rom_schd_mem.h"
#endif	/* BUILD_OPT_FC9000_ROMNDK */

#define inline /* no inline */

#undef	__must_check
#undef	__used
#define __must_check /* no warn_unused_result */
#define __used /* no used */

#define likely(x) (x)
#define unlikely(x) (x)

/* defined in compiler.h */

#ifdef __CHECKER__
# define __user		__attribute__((noderef, address_space(1)))
# define __force	__attribute__((force))
# define __acquires(x)	__attribute__((context(x,0,1)))
# define __releases(x)	__attribute__((context(x,1,0)))
# define __acquire(x)	__context__(x,1)
# define __release(x)	__context__(x,-1)
#ifdef CONFIG_SPARSE_RCU_POINTER
# define __rcu		__attribute__((noderef, address_space(4)))
#else
# define __rcu
#endif
#else
# define __user
# define __force
# define __acquires(x)
# define __releases(x)
# define __acquire(x) (void)0
# define __release(x) (void)0
# define __rcu
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

/* sysv */
//#ifndef SUPPORT_FREERTOS
typedef 		unsigned char u8;
typedef 		unsigned short	u16;
typedef 		unsigned int	u32;
typedef 		unsigned long long	u64;
//#endif	/*SUPPORT_FREERTOS*/
typedef 		signed char s8;
typedef 		signed short s16;
typedef 		signed int s32;
typedef 		signed long 	long	s64;

typedef		u8		__u8;
typedef		u16		__u16;
typedef		u32		__u32;
typedef 	s32		__s32;
typedef 	unsigned long long 	__u64;

#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif

#ifdef __CHECK_ENDIAN__
#define __bitwise __bitwise__
#else
#define __bitwise
#endif

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
//typedef __u64 __bitwise __be64;

/* kernel.h */
#undef	INT_MAX
#undef	INT_MIN

#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)

//#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define DIW_BITS_PER_LONG 32 //Check

//#define __compiler_offsetof(a,b) __builtin_offsetof(a,b)
#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

//#define BIT_MASK(nr)		(1UL << ((nr) % DIW_BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / DIW_BITS_PER_LONG)
#define BITS_PER_BYTE		8
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))


struct rcu_head
{
	struct rcu_head *next;
	void (*func)(struct rcu_head *head);
};

struct diw_list_head
{
	struct diw_list_head *next, *prev;
};

struct diw_hlist_head
{
	struct diw_hlist_node *first;
};

struct diw_hlist_node
{
	struct diw_hlist_node *next, **pprev;
};

#define HZ 100 //100

typedef unsigned __bitwise__ gfp_t;

#define MAX_ERRNO	4095
#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

void *ERR_PTR(long error);
long PTR_ERR(const void *ptr);
long IS_ERR(const void *ptr);

typedef struct
{
	long long counter;
} atomic64_t;

typedef struct
{
	/*volatile*/ int counter;
} atomic_t;

#define container_of(ptr, type, member) (type *)( (char *)ptr - offsetof(type,member) )

struct notifier_block
{
	int (*notifier_call)(struct notifier_block *, unsigned long, void *);
	struct notifier_block __rcu *next;
	int priority;
};

#define DECLARE_BITMAP(name,bits) unsigned long name[BITS_TO_LONGS(bits)]

#define ___constant_swab16(z) ((__u16)(			\
							   (((__u16)(z) & (__u16)0x00ffU) << 8) |		\
							   (((__u16)(z) & (__u16)0xff00U) >> 8)))

#define ___constant_swab32(z) ((__u32)(			\
							   (((__u32)(z) & (__u32)0x000000ffUL) << 24) |	\
							   (((__u32)(z) & (__u32)0x0000ff00UL) <<  8) |	\
							   (((__u32)(z) & (__u32)0x00ff0000UL) >>  8) |	\
							   (((__u32)(z) & (__u32)0xff000000UL) >> 24)))

#define __swab16(z)		___constant_swab16(z)
#define __swab32(z)		___constant_swab32(z)

//#define __cpu_to_le64(z) ((__force __le64)(__u64)(z))
#define __le64_to_cpu(z) ((__force __u64)(__le64)(z))
#define __cpu_to_le32(z) ((__force __le32)(__u32)(z))
#define __le32_to_cpu(z) ((__force __u32)(__le32)(z))
#define __cpu_to_le16(z) ((__force __le16)(__u16)(z))
#define __le16_to_cpu(z) ((__force __u16)(__le16)(z))
#define __cpu_to_be32(z) ((__force __be32)__swab32((z)))
#define __be32_to_cpu(z) __swab32((__force __u32)(__be32)(z))
#define __cpu_to_be16(z) ((__force __be16)__swab16((z)))
#define __be16_to_cpu(z) __swab16((__force __u16)(__be16)(z))

/* generic.h */
//#define cpu_to_le64 __cpu_to_le64
#define cpu_to_le32 __cpu_to_le32
#define cpu_to_le16 __cpu_to_le16
#define cpu_to_be16 __cpu_to_be16
#define le64_to_cpu __le64_to_cpu
#define le32_to_cpu __le32_to_cpu
#define le16_to_cpu __le16_to_cpu

#undef BUILD_BUG_ON
#define BUILD_BUG_ON(condition)

#undef IS_ENABLED
#define IS_ENABLED(a) (a)

#undef min_t
#define min_t(type, x, y) 			(type)x < (type)y ? (type)x: (type)y;

#undef max_t
#define max_t(type, x, y) 			(type)x > (type)y ? (type)x: (type)y;


#ifndef SUPPORT_FREERTOS
#undef mutex_lock
#define mutex_lock(a) tx_mutex_get(a, TX_WAIT_FOREVER)

#undef mutex_unlock
#define mutex_unlock(a) tx_mutex_put(a)


#undef mutex_destroy
#define mutex_destroy(a) tx_mutex_delete(a)


#undef spin_lock_irqsave
#define spin_lock_irqsave(a) tx_semaphore_get(a,TX_WAIT_FOREVER)

#undef spin_unlock_irqrestore
#define spin_unlock_irqrestore(a) tx_semaphore_put(a)

#undef spin_lock_bh
#define spin_lock_bh(a) tx_semaphore_get(a,TX_WAIT_FOREVER)

#undef spin_unlock_bh
#define spin_unlock_bh(a) tx_semaphore_put(a)

#undef spin_lock
#define spin_lock(a) tx_semaphore_get(a,TX_WAIT_FOREVER)

#undef spin_unlock
#define spin_unlock(a) tx_semaphore_put(a)

#endif	/*SUPPORT_FREERTOS*/

#undef GFP_ATOMIC
#define GFP_ATOMIC      NULL

#undef GFP_KERNEL
#define GFP_KERNEL      NULL

/*********************************************/
#undef debugfs_remove_recursive
#define debugfs_remove_recursive(a,b)

#undef debugfs_create_file
#define debugfs_create_file(a,b,c,d,e)

#undef debugfs_create_dir
#define debugfs_create_dir(a,b) NULL
/**********************************************/

#undef atomic_read
#if 0 //removed volatile
#define atomic_read(v)  (*(volatile int *)&(v)->counter)
#define atomic64_read(v)        (*(volatile long long *)&(v)->counter)
#else
#define atomic_read(v)  (*( int *)&(v)->counter)
#define atomic64_read(v)        (*( long long *)&(v)->counter)
#endif

#undef USHRT_MAX
#define USHRT_MAX       ((u16)(~0U))

#ifndef BIT_MASK
#define BIT_MASK(nr)    (0x01 << (nr%32))
#endif
#ifndef set_bit
#define set_bit(nr, addr)   *addr |= BIT_MASK(nr)
#endif
#ifndef clear_bit
#define clear_bit(nr, addr) *addr &= ~BIT_MASK(nr)
#endif
#ifndef test_bit
#define test_bit(nr, addr)  (*addr & BIT_MASK(nr))?1:0
#endif

#if 0
void __set_bit(int nr, volatile unsigned long *addr);
void __clear_bit(int nr, volatile unsigned long *addr);
void change_bit(int nr, volatile unsigned long *addr);
int test_and_set_bit(int nr, volatile unsigned long *addr);
int test_and_clear_bit(int nr, volatile unsigned long *addr);
//int test_and_change_bit(int nr, volatile unsigned long *addr);
#else /* removed volatile */
void __set_bit(int nr, uint32_t *addr);
void __clear_bit(int nr, uint32_t *addr);
void change_bit(int nr, uint32_t *addr);
int test_and_set_bit(int nr, uint32_t *addr);
int test_and_clear_bit(int nr, uint32_t *addr);
#endif

typedef long __kernel_time_t;
typedef long __kernel_suseconds_t;

/*	//duplicated
struct timespec
{
	__kernel_time_t		tv_sec;
	long				tv_nsec;
};
*/

union ktime
{
	s64 tv64;
	struct
	{
		s32	nsec, sec;
	} tv;
};
typedef union ktime ktime_t;

//#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC   1000L
#define USEC_PER_SEC    1000000L
//#define NSEC_PER_SEC    1000000000L

void ktime_get_ts(struct timespec *ts);

#define do_posix_clock_monotonic_gettime(ts) ktime_get_ts(ts)

unsigned long find_first_bit(const unsigned long *addr, unsigned long size);
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
							unsigned long offset);

#define diw_for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
			(bit) < (size);					\
			(bit) = find_next_bit((addr), (size), (bit) + 1))

#define CRC_BE_BITS 32
#define CRCPOLY_BE 0x04c11db7
#define BE_TABLE_ROWS (CRC_BE_BITS/8)

u32 crc32_be_generic(u32 crc, unsigned char const *p, size_t len,
					 const u32 (*tab)[256], u32 polynomial);
u32 crc32_be(u32 crc, unsigned char const *p, size_t len);
u32 crc32_body(u32 crc, unsigned char const *buf, size_t len, const u32 (*tab)[256]);

#ifdef FEATURE_UMAC_MESH
/* Rotate a 32-bit value left */
static inline __u32 rol32(__u32 word, unsigned int shift)
{
	return (word << shift) | (word >> (32 - shift));
}
#endif	//FEATURE_UMAC_MESH

typedef int ssize_t; //check
typedef long long  loff_t;
typedef u32 dma_addr_t;


void INIT_LIST_HEAD(struct diw_list_head *list);

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct diw_list_head name = LIST_HEAD_INIT(name)

#undef list_entry
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#undef list_first_entry
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#undef list_last_entry
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#undef list_entry_rcu
#define list_entry_rcu(ptr, type, member) \
	((type *)( (char *)ptr - offsetof(type,member) ))


#undef list_for_each_entry_rcu
#define list_for_each_entry_rcu(cur, head, member) \
	for (cur = list_entry_rcu((head)->next, typeof(*cur), member); \
		&cur->member != (head); \
		cur = list_entry_rcu(cur->member.next, typeof(*cur), member))

#undef list_for_each_entry
#define list_for_each_entry(cur, head, member) \
	for (cur = list_entry_rcu((head)->next, typeof(*cur), member); \
		&cur->member != (head); \
		cur = list_entry_rcu(cur->member.next, typeof(*cur), member))

#undef list_for_each_entry_safe
#define list_for_each_entry_safe(cur, n, head, member) \
	for (cur = list_entry((head)->next, typeof(*cur), member), \
		n = list_entry(cur->member.next, typeof(*cur), member);	\
		&cur->member != (head); \
		cur = n, n = list_entry(n->member.next, typeof(*n), member))

#ifdef FEATURE_UMAC_MESH
#undef hlist_first_rcu
#define hlist_first_rcu(head)	(*((struct diw_hlist_node __rcu **)(&(head)->first)))

#undef hlist_next_rcu
#define hlist_next_rcu(node)	(*((struct diw_hlist_node __rcu **)(&(node)->next)))

#undef hlist_entry
#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#undef hlist_entry_safe
#define hlist_entry_safe(ptr, type, member) \
	ptr ? hlist_entry(ptr, type, member) : NULL

#undef list_for_each_safe
#define list_for_each_safe(cur, n, head) \
	for (cur = (head)->next, n = cur->next; cur != (head); \
		cur = n, n = cur->next)

#undef hlist_for_each_entry_rcu
#define hlist_for_each_entry_rcu(cur, head, member) \
	for (cur = hlist_entry_safe (hlist_first_rcu(head), typeof(*(cur)), member);	 \
		 cur;							\
		 cur = hlist_entry_safe(hlist_next_rcu(&(cur)->member), typeof(*(cur)), member))
#else
#undef hlist_entry
#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#undef hlist_entry_safe
#define hlist_entry_safe(ptr, type, member) ptr ? hlist_entry(ptr, type, member) : NULL
#endif /* FEATURE_UMAC_MESH */

#ifdef FEATURE_UMAC_MESH
#undef hlist_for_each
#define hlist_for_each(cur, head) \
	for (cur = (head)->first; cur ; cur = cur->next)

#undef hlist_for_each_safe
#define hlist_for_each_safe(cur, n, head) hlist_for_each(cur, head)
#endif /* FEATURE_UMAC_MESH */

#undef hlist_for_each_entry
#define hlist_for_each_entry(cur, head, member) \
	for (cur = hlist_entry_safe((head)->first, typeof(*(cur)), member); \
		cur; \
		cur = hlist_entry_safe((cur)->member.next, typeof(*(cur)), member))

#ifdef FEATURE_UMAC_MESH
#undef hlist_for_each_entry_safe
#define hlist_for_each_entry_safe(cur, n, head, member) hlist_for_each_entry(cur, head, member)
#endif /* FEATURE_UMAC_MESH */

struct work_struct;
typedef void (*work_func_t)(struct work_struct *work);

struct work_struct
{
	unsigned long data;
	//struct diw_list_head entry;  /* Chang. Not used */
#ifdef FEATURE_NEW_WORK_LIST
	struct diw_list_head list;
#endif
	work_func_t func;
};


struct delayed_work
{
	struct work_struct work;

#ifdef SUPPORT_FREERTOS
	TimerHandle_t timer;
#else
	TX_TIMER timer;
#endif
};


#define __INIT_WORK(_work, _func, _onstack)	\
	do {									\
		(_work)->func = (_func); 			\
		(_work)->data = 0;					\
		INIT_LIST_HEAD(&(_work)->list);		\
	} while (0)

#define INIT_WORK(_work, _func) __INIT_WORK((_work), (_func), 0)
#define __INIT_DELAYED_WORK(_work, _func, _tflags) INIT_WORK(&(_work)->work, (_func))
#define INIT_DELAYED_WORK(_work, _func) __INIT_DELAYED_WORK(_work, _func, 0)

#define __init
//#define __exit
//#define PM_QOS_NETWORK_LATENCY 2  //pm_qos.h
#ifndef SUPPORT_FREERTOS

typedef unsigned short __kernel_sa_family_t;
typedef __kernel_sa_family_t    sa_family_t;

/// Socket Address.
struct sockaddr
{
	/// Address family. AF_xxx.
	sa_family_t sa_family;
	/// 14 Bytes address of protocol.
	uint8_t sa_data[14];
};
#endif	/*SUPPORT_FREERTOS*/

/* wyyang 140829 for timer function add */
#ifdef SUPPORT_FREERTOS
void mod_timer(TimerHandle_t xTimer, unsigned long expires);
int del_timer(TimerHandle_t xTimer);
int  del_timer_sync(TimerHandle_t xTimer);

UINT  umac_timer_info_get(TimerHandle_t xTimer, CHAR **name, UINT *active,
						  ULONG *remaining_ticks,
						  ULONG *reschedule_ticks, TimerHandle_t *next_timer);
void deactivate_timer(TimerHandle_t xTimer);

#else
void mod_timer(TX_TIMER *tx_timer, unsigned long tx_expires);
int del_timer(TX_TIMER *timer_ptr);
int  del_timer_sync(TX_TIMER *timer_ptr);

UINT  umac_timer_info_get(TX_TIMER *timer_ptr, CHAR **name, UINT *active,
						  ULONG *remaining_ticks,
						  ULONG *reschedule_ticks, TX_TIMER **next_timer);
void deactivate_timer(TX_TIMER *timer_ptr);

#endif	//SUPPORT_FREERTOS

#define time_after(a,b) ((long)((b) - (a)) < 0)
#define time_before(a,b) time_after(b,a)

/* time_is_after_jiffies(a) return true if a is after jiffies */
#ifdef SUPPORT_FREERTOS
#define time_is_after_jiffies(timeout) time_before(xTaskGetTickCount(), timeout)
#else
#define time_is_after_jiffies(timeout) time_before(tx_time_get(), timeout)
#endif	/*SUPPORT_FREERTOS*/

#define round_jiffies_up(a) a
#define round_jiffies_relative(a) a
#define msecs_to_jiffies(a) a/10
#define usecs_to_jiffies(a) a/10000
#define jiffies_to_msecs(a) a *10

//#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while(0)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define might_sleep()

/* Chang. 140915 */
#define small_const_nbits(nbits) \
	(/* __builtin_constant_p*/(nbits) && (nbits) <= DIW_BITS_PER_LONG)
void bitmap_zero(unsigned long *dst, int nbits);

bool is_equal_ether_addr(const u8 *addr1, const u8 *addr2);
bool is_broadcast_ether_addr(const u8 *addr);
bool is_multicast_ether_addr(const u8 *addr);
bool umac_is_zero_ether_addr(const u8 *addr);
bool is_valid_ether_addr(const u8 *addr);
void eth_broadcast_addr(u8 *addr);

#ifdef FEATURE_UMAC_MESH
void eth_zero_addr(u8 *addr);
#endif /* FEATURE_UMAC_MESH */

int atomic_sub(int i, atomic_t *v);
int atomic_add(int i, atomic_t *v);
int atomic64_add(long long i, atomic64_t *v);
void atomic_dec(atomic_t *v);
void atomic_inc(atomic_t *v);

#define atomic_inc_return(v)	atomic_add(1,(v))
#define atomic_dec_return(v)    atomic_sub(1,(v))
#define atomic64_inc_return(v)  atomic64_add(1,(v))

#define atomic_set(v, i) (((v)->counter) = (i))

#define MBM_TO_DBM(gain) ((gain) / 100)
#undef	ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN(x, a)     (((x) + (a) - 1) & ~((a) - 1))

#define ASSERT_RTNL()
#define printk(fmt, ...)

unsigned long __ffs(unsigned long word);
int fls(int x);
int ffs(int x);

unsigned int __arch_hweight8(unsigned int z);
unsigned int __arch_hweight16(unsigned int z);

#define __const_hweight8(z)		\
	((!!((z) & (1ULL << 0))) +	\
	 (!!((z) & (1ULL << 1))) +	\
	 (!!((z) & (1ULL << 2))) +	\
	 (!!((z) & (1ULL << 3))) +	\
	 (!!((z) & (1ULL << 4))) +	\
	 (!!((z) & (1ULL << 5))) +	\
	 (!!((z) & (1ULL << 6))) +	\
	 (!!((z) & (1ULL << 7)))	)

#define __const_hweight16(z) (unsigned int)(__const_hweight8(z)  + __const_hweight8((z)  >> 8 ))
#define __const_hweight32(z) (__const_hweight16(z) + __const_hweight16((z) >> 16))
#define __const_hweight64(z) (__const_hweight32(z) + __const_hweight32((z) >> 32))

#define hweight8(z)  ((z) ? __const_hweight8(z)  : __arch_hweight8(z))
#define hweight16(z) ((z) ? __const_hweight16(z) : __arch_hweight16(z))

#define BITMAP_LAST_WORD_MASK(nbits) \
	( \
		((nbits) % DIW_BITS_PER_LONG) ? \
		(1UL<<((nbits) % DIW_BITS_PER_LONG))-1 : ~0UL \
	)

int bitmap_empty(const unsigned long *src, int nbits);

int __init i3ed11_init(void);

void prandom_bytes(u8 *buf, int bytes);

#define null_printf(a...) do {} while (0)

#ifdef  BUILD_OPT_FC9000_ROMLIB
#include "rom_schd_mem.h"
#else
//#include "schd_mem.h"
#endif

extern void *umac_heap_malloc(size_t size);
extern void umac_heap_free(void *f);
extern void *supp_malloc(size_t size);
extern void supp_free(void *pointmem);

#define		UMAC_HP_MALLOC(...)	umac_heap_malloc( __VA_ARGS__ )
#define		UMAC_HP_FREE(...)	umac_heap_free( __VA_ARGS__ )
#define		SUPP_MALLOC(...)	supp_malloc( __VA_ARGS__ )
#define		SUPP_FREE(...)		supp_free( __VA_ARGS__ )

#ifdef DPM_PORT
#define		UMAC_DPM_MALLOC(...)	umac_heap_malloc( __VA_ARGS__ )
#define		UMAC_DPM_FREE(...)	umac_heap_free( __VA_ARGS__ )
#endif



struct umac_params
{
	unsigned short listen_int;
	unsigned short tx_wpkt_timeout;
	unsigned short rx_wpkt_timeout;
	unsigned short um_tx_wpkt_timeout;
	int ampdu_tx_flag;
	int ampdu_rx_flag;
	int tkip_hw_select;
	int retry_long;
	int retry_short;
};

extern struct umac_params umac_mod_params;

#ifdef FEATURE_UMAC_MESH
/* An arbitrary initial parameter */
#define JHASH_INITVAL		0xdeadbeef

/* __jhash_final - final mixing of 3 32-bit values (a,b,c) into c */
#define __jhash_final(a, b, c)			\
{						\
	c ^= b; c -= rol32(b, 14);		\
	a ^= c; a -= rol32(c, 11);		\
	b ^= a; b -= rol32(a, 25);		\
	c ^= b; c -= rol32(b, 16);		\
	a ^= c; a -= rol32(c, 4);		\
	b ^= a; b -= rol32(a, 14);		\
	c ^= b; c -= rol32(b, 24);		\
}

/* jhash_3words - hash exactly 3, 2 or 1 word(s) */
static inline u32 jhash_3words(u32 a, u32 b, u32 c, u32 initval)
{
	a += JHASH_INITVAL;
	b += JHASH_INITVAL;
	c += initval;

	__jhash_final(a, b, c);

	return c;
}

static inline u32 jhash_2words(u32 a, u32 b, u32 initval)
{
	return jhash_3words(a, b, 0, initval);
}
#endif /* FEATURE_UMAC_MESH */
#endif	/* _OS_CON_H */
