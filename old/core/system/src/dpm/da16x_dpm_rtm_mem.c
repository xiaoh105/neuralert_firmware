/**
 * @file
 * Dynamic memory manager for RTM in dpm
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 * Copyright (c) 2019-2022 Modified by Renesas Electronics.
 */

#include "da16x_dpm_rtm_mem.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/err.h"

#include <string.h>
#include "common_def.h"

/* This is overridable for tests only... */
#ifndef DA16X_DPM_RTM_MEM_ILLEGAL_FREE
	#define DA16X_DPM_RTM_MEM_ILLEGAL_FREE(msg)         LWIP_ASSERT(msg, 0)
#endif

#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK

	#define DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE	16
	#if DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE > 0
		#define DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED    LWIP_MEM_ALIGN_SIZE(DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE)
	#else
		#define DA16X_DPM_RTM_MEM_SANITY_REGION_BEFOREALIGNED    0
	#endif /* DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE */
	#ifndef DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER
		#define DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER   16
	#endif /* DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER*/
	#if DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER > 0
		#define DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED     LWIP_MEM_ALIGN_SIZE(DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER)
	#else
		#define DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED     0
	#endif /* DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER*/

	#define DA16X_DPM_RTM_MEM_SANITY_OFFSET   DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED
	#define DA16X_DPM_RTM_MEM_SANITY_OVERHEAD (DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED + DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED)
#else
	#define DA16X_DPM_RTM_MEM_SANITY_OFFSET   0
	#define DA16X_DPM_RTM_MEM_SANITY_OVERHEAD 0
#endif

int  da16x_dpm_rtm_mem_alloc_fail_cnt = 0;
int  da16x_dpm_rtm_mem_alloc_illegal_cnt = 0;

#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
/**
 * Check if a mep element was victim of an overflow or underflow
 * (e.g. the restricted area after/before it has been altered)
 *
 * @param p the mem element to check
 * @param size allocated size of the element
 * @param descr1 description of the element source shown on error
 * @param descr2 description of the element source shown on error
 */
void
da16x_dpm_rtm_mem_overflow_check_raw(void *p, size_t size, const char *descr1,
									 const char *descr2)
{
#if DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED || DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED
	u16_t k;
	u8_t *m;

#if DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED > 0
	m = (u8_t *)p + size;
	for (k = 0; k < DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED; k++) {
		if (m[k] != 0xcd) {
			char errstr[128];
			snprintf(errstr, sizeof(errstr), "detected mem overflow in %s%s", descr1,
					 descr2);
			LWIP_ASSERT(errstr, 0);
		}
	}
#endif /* DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED > 0 */

#if DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED > 0
	m = (u8_t *)p - DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED;
	for (k = 0; k < DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED; k++) {
		if (m[k] != 0xcd) {
			char errstr[128];
			snprintf(errstr, sizeof(errstr), "detected mem underflow in %s%s", descr1,
					 descr2);
			LWIP_ASSERT(errstr, 0);
		}
	}
#endif /* DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED > 0 */
#else
	LWIP_UNUSED_ARG(p);
	LWIP_UNUSED_ARG(desc);
	LWIP_UNUSED_ARG(descr);
#endif
}

/**
 * Initialize the restricted area of a mem element.
 */
void
da16x_dpm_rtm_mem_overflow_init_raw(void *p, size_t size)
{
#if DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED > 0 || DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED > 0
	u8_t *m;
#if DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED > 0
	m = (u8_t *)p - DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED;
	memset(m, 0xcd, DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED);
#endif
#if DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED > 0
	m = (u8_t *)p + size;
	memset(m, 0xcd, DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED);
#endif
#else /* DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED > 0 || DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED > 0 */
	LWIP_UNUSED_ARG(p);
	LWIP_UNUSED_ARG(desc);
#endif /* DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED > 0 || DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED > 0 */
}
#endif /* DA16X_DPM_RTM_MEM_OVERFLOW_CHECK */

/* lwIP replacement for your libc malloc() */

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_STRUCT_DA16X_DPM_RTM_MEM, which automatically aligns.
 */
struct da16x_dpm_rtm_mem {
	/** index (-> rtm[next]) of the next struct */
	da16x_dpm_rtm_mem_size_t next;
	/** index (-> rtm[prev]) of the previous struct */
	da16x_dpm_rtm_mem_size_t prev;
	/** 1: this area is used; 0: this area is unused */
	u8_t used;
#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
	/** this keeps track of the dpm allocation size for guard checks */
	da16x_dpm_rtm_mem_size_t dpm_size;
#endif
};

/** All allocated blocks will be DA16X_DPM_RTM_MIN_SIZE bytes big, at least!
 * DA16X_DPM_RTM_MIN_SIZE can be overridden to suit your needs. Smaller values save space,
 * larger values could prevent too small blocks to fragment the RAM too much. */
#ifndef DA16X_DPM_RTM_MIN_SIZE
	#define DA16X_DPM_RTM_MIN_SIZE             12
#endif /* DA16X_DPM_RTM_MIN_SIZE */

static da16x_dpm_rtm_mem_size_t rtm_size;

/* some alignment macros: we define them here for better source code layout */
#define DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED		LWIP_MEM_ALIGN_SIZE(DA16X_DPM_RTM_MIN_SIZE)
#define SIZEOF_STRUCT_DA16X_DPM_RTM_MEM		LWIP_MEM_ALIGN_SIZE(sizeof(struct da16x_dpm_rtm_mem))
#define DA16X_DPM_RTM_MEM_SIZE_ALIGNED			LWIP_MEM_ALIGN_SIZE(rtm_size)

/** pointer to the heap (rtm_heap): for alignment, rtm is now a pointer instead of an array */
static u8_t *rtm;
/** the last entry, always unused! */
static struct da16x_dpm_rtm_mem *rtm_end;

/** concurrent access protection */
#if !NO_SYS
	static sys_mutex_t da16x_dpm_rtm_mem_mutex;
#endif

#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT

	static volatile u8_t da16x_dpm_rtm_mem_free_count;

	/* Allow da16x_dpm_rtm_mem_free from other (e.g. interrupt) context */
	#define DA16X_DPM_RTM_MEM_FREE_DECL_PROTECT()  SYS_ARCH_DECL_PROTECT(lev_free)
	#define DA16X_DPM_RTM_MEM_FREE_PROTECT()       SYS_ARCH_PROTECT(lev_free)
	#define DA16X_DPM_RTM_MEM_FREE_UNPROTECT()     SYS_ARCH_UNPROTECT(lev_free)
	#define DA16X_DPM_RTM_MEM_ALLOC_DECL_PROTECT() SYS_ARCH_DECL_PROTECT(lev_alloc)
	#define DA16X_DPM_RTM_MEM_ALLOC_PROTECT()      SYS_ARCH_PROTECT(lev_alloc)
	#define DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT()    SYS_ARCH_UNPROTECT(lev_alloc)
	#define DA16X_DPM_RTM_MEM_LFREE_VOLATILE       volatile

#else /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

	/* Protect the heap only by using a mutex */
	#define DA16X_DPM_RTM_MEM_FREE_DECL_PROTECT()
	#define DA16X_DPM_RTM_MEM_FREE_PROTECT()    sys_mutex_lock(&da16x_dpm_rtm_mem_mutex)
	#define DA16X_DPM_RTM_MEM_FREE_UNPROTECT()  sys_mutex_unlock(&da16x_dpm_rtm_mem_mutex)
	/* da16x_dpm_rtm_mem_malloc is protected using mutex AND DA16X_DPM_RTM_MEM_ALLOC_PROTECT */
	#define DA16X_DPM_RTM_MEM_ALLOC_DECL_PROTECT()
	#define DA16X_DPM_RTM_MEM_ALLOC_PROTECT()
	#define DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT()
	#define DA16X_DPM_RTM_MEM_LFREE_VOLATILE

#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

/** pointer to the lowest free block, this is used for faster search */
static struct da16x_dpm_rtm_mem *DA16X_DPM_RTM_MEM_LFREE_VOLATILE lfree;

#if DA16X_DPM_RTM_MEM_SANITY_CHECK
	static void da16x_dpm_rtm_mem_sanity(void);
	#define DA16X_DPM_RTM_MEM_SANITY() da16x_dpm_rtm_mem_sanity()
#else
	#define DA16X_DPM_RTM_MEM_SANITY()
#endif

#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
static void
da16x_dpm_rtm_mem_overflow_init_element(struct da16x_dpm_rtm_mem *mem,
										da16x_dpm_rtm_mem_size_t dpm_size)
{
	void *p = (u8_t *)mem + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
			  DA16X_DPM_RTM_MEM_SANITY_OFFSET;
	mem->dpm_size = dpm_size;
	da16x_dpm_rtm_mem_overflow_init_raw(p, dpm_size);
}

static void
da16x_dpm_rtm_mem_overflow_check_element(struct da16x_dpm_rtm_mem *mem)
{
	void *p = (u8_t *)mem + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
			  DA16X_DPM_RTM_MEM_SANITY_OFFSET;
	da16x_dpm_rtm_mem_overflow_check_raw(p, mem->dpm_size, "heap", "");
}
#else /* DA16X_DPM_RTM_MEM_OVERFLOW_CHECK */
#define da16x_dpm_rtm_mem_overflow_init_element(mem, size)
#define da16x_dpm_rtm_mem_overflow_check_element(mem)
#endif /* DA16X_DPM_RTM_MEM_OVERFLOW_CHECK */

static struct da16x_dpm_rtm_mem *
ptr_to_mem(da16x_dpm_rtm_mem_size_t ptr)
{
	return (struct da16x_dpm_rtm_mem *)(void *)&rtm[ptr];
}

static da16x_dpm_rtm_mem_size_t
mem_to_ptr(void *mem)
{
	return (da16x_dpm_rtm_mem_size_t)((u8_t *)mem - rtm);
}

/**
 * "Plug holes" by combining adjacent empty struct da16x_dpm_rtm_mems.
 * After this function is through, there should not exist
 * one empty struct da16x_dpm_rtm_mem pointing to another empty struct da16x_dpm_rtm_mem.
 *
 * @param mem this points to a struct da16x_dpm_rtm_mem which just has been freed
 * @internal this function is only called by da16x_dpm_rtm_mem_free() and da16x_dpm_rtm_mem_trim()
 *
 * This assumes access to the heap is protected by the calling function
 * already.
 */
static void
da16x_dpm_rtm_mem_plug_holes(struct da16x_dpm_rtm_mem *mem)
{
	struct da16x_dpm_rtm_mem *nmem;
	struct da16x_dpm_rtm_mem *pmem;

	LWIP_ASSERT("da16x_dpm_rtm_mem_plug_holes: mem >= rtm", (u8_t *)mem >= rtm);
	LWIP_ASSERT("da16x_dpm_rtm_mem_plug_holes: mem < rtm_end",
				(u8_t *)mem < (u8_t *)rtm_end);
	LWIP_ASSERT("da16x_dpm_rtm_mem_plug_holes: mem->used == 0", mem->used == 0);

	/* plug hole forward */
	LWIP_ASSERT("da16x_dpm_rtm_mem_plug_holes: mem->next <= DA16X_DPM_RTM_MEM_SIZE_ALIGNED",
				mem->next <= DA16X_DPM_RTM_MEM_SIZE_ALIGNED);

	nmem = ptr_to_mem(mem->next);
	if (mem != nmem && nmem->used == 0 && (u8_t *)nmem != (u8_t *)rtm_end) {
		/* if mem->next is unused and not end of rtm, combine mem and mem->next */
		if (lfree == nmem) {
			lfree = mem;
		}
		mem->next = nmem->next;
		if (nmem->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED) {
			ptr_to_mem(nmem->next)->prev = mem_to_ptr(mem);
		}
	}

	/* plug hole backward */
	pmem = ptr_to_mem(mem->prev);
	if (pmem != mem && pmem->used == 0) {
		/* if mem->prev is unused, combine mem and mem->prev */
		if (lfree == mem) {
			lfree = pmem;
		}
		pmem->next = mem->next;
		if (mem->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED) {
			ptr_to_mem(mem->next)->prev = mem_to_ptr(pmem);
		}
	}
}

/**
 * Zero the heap and initialize start, end and lowest-free
 */
void
da16x_dpm_rtm_mem_init(void *mem_ptr, size_t mem_len)
{
	struct da16x_dpm_rtm_mem *mem;

	LWIP_ASSERT("Sanity check alignment",
				(SIZEOF_STRUCT_DA16X_DPM_RTM_MEM & (MEM_ALIGNMENT - 1)) == 0);

	mem_len -= SIZEOF_STRUCT_DA16X_DPM_RTM_MEM;

	/* align the heap */
	rtm = (u8_t *)LWIP_MEM_ALIGN(mem_ptr);
	rtm_size = mem_len;
	/* initialize the start of the heap */
	mem = (struct da16x_dpm_rtm_mem *)(void *)rtm;
	mem->next = DA16X_DPM_RTM_MEM_SIZE_ALIGNED;
	mem->prev = 0;
	mem->used = 0;
	/* initialize the end of the heap */
	rtm_end = ptr_to_mem(DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
	rtm_end->used = 1;
	rtm_end->next = DA16X_DPM_RTM_MEM_SIZE_ALIGNED;
	rtm_end->prev = DA16X_DPM_RTM_MEM_SIZE_ALIGNED;
	DA16X_DPM_RTM_MEM_SANITY();

	/* initialize the lowest-free pointer to the start of the heap */
	lfree = (struct da16x_dpm_rtm_mem *)(void *)rtm;

	if (sys_mutex_new(&da16x_dpm_rtm_mem_mutex) != ERR_OK) {
		LWIP_ASSERT("failed to create da16x_dpm_rtm_mem_mutex", 0);
	}
}

void
da16x_dpm_rtm_mem_recovery(void *mem_ptr, size_t mem_len, void *lfree_ptr)
{
	LWIP_ASSERT("Sanity check alignment",
				(SIZEOF_STRUCT_DA16X_DPM_RTM_MEM & (MEM_ALIGNMENT - 1)) == 0);

	mem_len -= SIZEOF_STRUCT_DA16X_DPM_RTM_MEM;

	/* align the heap */
	rtm = (u8_t *)LWIP_MEM_ALIGN(mem_ptr);
	rtm_size = mem_len;

	/* initialize the end of the heap */
	rtm_end = ptr_to_mem(DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
	rtm_end->used = 1;
	rtm_end->next = DA16X_DPM_RTM_MEM_SIZE_ALIGNED;
	rtm_end->prev = DA16X_DPM_RTM_MEM_SIZE_ALIGNED;

	/* initialize the lowest-free pointer to the start of the heap */
	lfree = (struct da16x_dpm_rtm_mem *)lfree_ptr;

	if (sys_mutex_new(&da16x_dpm_rtm_mem_mutex) != ERR_OK) {
		LWIP_ASSERT("failed to create da16x_dpm_rtm_mem_mutex", 0);
	}
}

void *
da16x_dpm_rtm_mem_get_free_ptr()
{
	return (void *)lfree;
}


/* Check if a struct da16x_dpm_rtm_mem is correctly linked.
 * If not, double-free is a possible reason.
 */
static int
da16x_dpm_rtm_mem_link_valid(struct da16x_dpm_rtm_mem *mem)
{
	struct da16x_dpm_rtm_mem *nmem, *pmem;
	da16x_dpm_rtm_mem_size_t rmem_idx;
	rmem_idx = mem_to_ptr(mem);
	nmem = ptr_to_mem(mem->next);
	pmem = ptr_to_mem(mem->prev);
	if ((mem->next > DA16X_DPM_RTM_MEM_SIZE_ALIGNED)
		|| (mem->prev > DA16X_DPM_RTM_MEM_SIZE_ALIGNED) ||
		((mem->prev != rmem_idx) && (pmem->next != rmem_idx)) ||
		((nmem != rtm_end) && (nmem->prev != rmem_idx))) {
		return 0;
	}
	return 1;
}

#if DA16X_DPM_RTM_MEM_SANITY_CHECK
static void
da16x_dpm_rtm_mem_sanity(void)
{
	struct da16x_dpm_rtm_mem *mem;
	u8_t last_used;

	/* begin with first element here */
	mem = (struct da16x_dpm_rtm_mem *)rtm;
	LWIP_ASSERT("heap element used valid", (mem->used == 0) || (mem->used == 1));
	last_used = mem->used;
	LWIP_ASSERT("heap element prev ptr valid", mem->prev == 0);
	LWIP_ASSERT("heap element next ptr valid",
				mem->next <= DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
	LWIP_ASSERT("heap element next ptr aligned",
				LWIP_MEM_ALIGN(ptr_to_mem(mem->next) == ptr_to_mem(mem->next)));

	/* check all elements before the end of the heap */
	for (mem = ptr_to_mem(mem->next);
		 ((u8_t *)mem > rtm) && (mem < rtm_end);
		 mem = ptr_to_mem(mem->next)) {
		LWIP_ASSERT("heap element aligned", LWIP_MEM_ALIGN(mem) == mem);
		LWIP_ASSERT("heap element prev ptr valid",
					mem->prev <= DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
		LWIP_ASSERT("heap element next ptr valid",
					mem->next <= DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
		LWIP_ASSERT("heap element prev ptr aligned",
					LWIP_MEM_ALIGN(ptr_to_mem(mem->prev) == ptr_to_mem(mem->prev)));
		LWIP_ASSERT("heap element next ptr aligned",
					LWIP_MEM_ALIGN(ptr_to_mem(mem->next) == ptr_to_mem(mem->next)));

		if (last_used == 0) {
			/* 2 unused elements in a row? */
			LWIP_ASSERT("heap element unused?", mem->used == 1);
		} else {
			LWIP_ASSERT("heap element unused member", (mem->used == 0) || (mem->used == 1));
		}

		LWIP_ASSERT("heap element link valid", da16x_dpm_rtm_mem_link_valid(mem));

		/* used/unused altering */
		last_used = mem->used;
	}
	LWIP_ASSERT("heap end ptr sanity",
				mem == ptr_to_mem(DA16X_DPM_RTM_MEM_SIZE_ALIGNED));
	LWIP_ASSERT("heap element used valid", mem->used == 1);
	LWIP_ASSERT("heap element prev ptr valid",
				mem->prev == DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
	LWIP_ASSERT("heap element next ptr valid",
				mem->next == DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
}
#endif /* DA16X_DPM_RTM_MEM_SANITY_CHECK */

/**
 * Put a struct da16x_dpm_rtm_mem back on the heap
 *
 * @param rmem is the data portion of a struct da16x_dpm_rtm_mem as returned by a previous
 *             call to da16x_dpm_rtm_mem_malloc()
 */
void
da16x_dpm_rtm_mem_free(void *rmem)
{
	struct da16x_dpm_rtm_mem *mem;
	DA16X_DPM_RTM_MEM_FREE_DECL_PROTECT();

	if (rmem == NULL) {
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
					"da16x_dpm_rtm_mem_free(p == NULL) was called.\n");
		return;
	}
	if ((((mem_ptr_t)rmem) & (MEM_ALIGNMENT - 1)) != 0) {
		DA16X_DPM_RTM_MEM_ILLEGAL_FREE("da16x_dpm_rtm_mem_free: sanity check alignment");
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE,
					"da16x_dpm_rtm_mem_free: sanity check alignment\n");
		/* protect mem stats from concurrent access */
		da16x_dpm_rtm_mem_alloc_illegal_cnt++;
		return;
	}

	/* Get the corresponding struct da16x_dpm_rtm_mem: */
	/* cast through void* to get rid of alignment warnings */
	mem = (struct da16x_dpm_rtm_mem *)(void *)((u8_t *)rmem -
			(SIZEOF_STRUCT_DA16X_DPM_RTM_MEM + DA16X_DPM_RTM_MEM_SANITY_OFFSET));

	if ((u8_t *)mem < rtm
		|| (u8_t *)rmem + DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED > (u8_t *)rtm_end) {
		DA16X_DPM_RTM_MEM_ILLEGAL_FREE("da16x_dpm_rtm_mem_free: illegal memory");
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE,
					"da16x_dpm_rtm_mem_free: illegal memory\n");
		/* protect mem stats from concurrent access */
		da16x_dpm_rtm_mem_alloc_illegal_cnt++;
		return;
	}
#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
	da16x_dpm_rtm_mem_overflow_check_element(mem);
#endif
	/* protect the heap from concurrent access */
	DA16X_DPM_RTM_MEM_FREE_PROTECT();
	/* mem has to be in a used state */
	if (!mem->used) {
		DA16X_DPM_RTM_MEM_ILLEGAL_FREE("da16x_dpm_rtm_mem_free: illegal memory: double free");
		DA16X_DPM_RTM_MEM_FREE_UNPROTECT();
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE,
					"da16x_dpm_rtm_mem_free: illegal memory: double free?\n");
		/* protect mem stats from concurrent access */
		da16x_dpm_rtm_mem_alloc_illegal_cnt++;
		return;
	}

	if (!da16x_dpm_rtm_mem_link_valid(mem)) {
		DA16X_DPM_RTM_MEM_ILLEGAL_FREE("da16x_dpm_rtm_mem_free: illegal memory: non-linked: double free");
		DA16X_DPM_RTM_MEM_FREE_UNPROTECT();
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE,
					"da16x_dpm_rtm_mem_free: illegal memory: non-linked: double free?\n");
		/* protect mem stats from concurrent access */
		da16x_dpm_rtm_mem_alloc_illegal_cnt++;
		return;
	}

	//PRINTF(GREEN_COLOR " [%s] mem: 0x%x used:%d next:0x%x prev:0x%x \r\n" CLEAR_COLOR,
	//									__func__, mem, mem->used, mem->next, mem->prev);

	/* mem is now unused. */
	mem->used = 0;

	if (mem < lfree) {
		/* the newly freed struct is now the lowest */
		lfree = mem;
	}

	/* finally, see if prev or next are free also */
	da16x_dpm_rtm_mem_plug_holes(mem);
	DA16X_DPM_RTM_MEM_SANITY();
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
	da16x_dpm_rtm_mem_free_count = 1;
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
	DA16X_DPM_RTM_MEM_FREE_UNPROTECT();
}

/**
 * Shrink memory returned by da16x_dpm_rtm_mem_malloc().
 *
 * @param rmem pointer to memory allocated by da16x_dpm_rtm_mem_malloc the is to be shrinked
 * @param new_size required size after shrinking (needs to be smaller than or
 *                equal to the previous size)
 * @return for compatibility reasons: is always == rmem, at the moment
 *         or NULL if newsize is > old size, in which case rmem is NOT touched
 *         or freed!
 */
void *
da16x_dpm_rtm_mem_trim(void *rmem, da16x_dpm_rtm_mem_size_t new_size)
{
	da16x_dpm_rtm_mem_size_t size, newsize;
	da16x_dpm_rtm_mem_size_t ptr, ptr2;
	struct da16x_dpm_rtm_mem *mem, *mem2;
	/* use the FREE_PROTECT here: it protects with sem OR SYS_ARCH_PROTECT */
	DA16X_DPM_RTM_MEM_FREE_DECL_PROTECT();

	/* Expand the size of the allocated memory region so that we can
	   adjust for alignment. */
	newsize = (da16x_dpm_rtm_mem_size_t)LWIP_MEM_ALIGN_SIZE(new_size);
	if (newsize < DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED) {
		/* every data block must be at least DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED long */
		newsize = DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED;
	}
#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
	newsize += DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED +
			   DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED;
#endif
	if ((newsize > DA16X_DPM_RTM_MEM_SIZE_ALIGNED) || (newsize < new_size)) {
		return NULL;
	}

	LWIP_ASSERT("da16x_dpm_rtm_mem_trim: legal memory",
				(u8_t *)rmem >= (u8_t *)rtm &&
				(u8_t *)rmem < (u8_t *)rtm_end);

	if ((u8_t *)rmem < (u8_t *)rtm || (u8_t *)rmem >= (u8_t *)rtm_end) {
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE,
					"da16x_dpm_rtm_mem_trim: illegal memory\n");
		/* protect mem stats from concurrent access */
		da16x_dpm_rtm_mem_alloc_illegal_cnt++;
		return rmem;
	}
	/* Get the corresponding struct da16x_dpm_rtm_mem ... */
	/* cast through void* to get rid of alignment warnings */
	mem = (struct da16x_dpm_rtm_mem *)(void *)((u8_t *)rmem -
			(SIZEOF_STRUCT_DA16X_DPM_RTM_MEM + DA16X_DPM_RTM_MEM_SANITY_OFFSET));
#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
	da16x_dpm_rtm_mem_overflow_check_element(mem);
#endif
	/* ... and its offset pointer */
	ptr = mem_to_ptr(mem);

	size = (da16x_dpm_rtm_mem_size_t)((da16x_dpm_rtm_mem_size_t)(
										  mem->next - ptr) - (SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
												  DA16X_DPM_RTM_MEM_SANITY_OVERHEAD));
	LWIP_ASSERT("da16x_dpm_rtm_mem_trim can only shrink memory", newsize <= size);
	if (newsize > size) {
		/* not supported */
		return NULL;
	}
	if (newsize == size) {
		/* No change in size, simply return */
		return rmem;
	}

	/* protect the heap from concurrent access */
	DA16X_DPM_RTM_MEM_FREE_PROTECT();

	mem2 = ptr_to_mem(mem->next);
	if (mem2->used == 0) {
		/* The next struct is unused, we can simply move it at little */
		da16x_dpm_rtm_mem_size_t next;
		LWIP_ASSERT("invalid next ptr", mem->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
		/* remember the old next pointer */
		next = mem2->next;
		/* create new struct da16x_dpm_rtm_mem which is moved directly after the shrinked mem */
		ptr2 = (da16x_dpm_rtm_mem_size_t)(ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
										  newsize);
		if (lfree == mem2) {
			lfree = ptr_to_mem(ptr2);
		}
		mem2 = ptr_to_mem(ptr2);
		mem2->used = 0;
		/* restore the next pointer */
		mem2->next = next;
		/* link it back to mem */
		mem2->prev = ptr;
		/* link mem to it */
		mem->next = ptr2;
		/* last thing to restore linked list: as we have moved mem2,
		 * let 'mem2->next->prev' point to mem2 again. but only if mem2->next is not
		 * the end of the heap */
		if (mem2->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED) {
			ptr_to_mem(mem2->next)->prev = ptr2;
		}
		/* no need to plug holes, we've already done that */
	} else if (newsize + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
			   DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED <= size) {
		/* Next struct is used but there's room for another struct da16x_dpm_rtm_mem with
		 * at least DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED of data.
		 * Old size ('size') must be big enough to contain at least 'newsize' plus a struct da16x_dpm_rtm_mem
		 * ('SIZEOF_STRUCT_DA16X_DPM_RTM_MEM') with some data ('DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED').
		 * @todo we could leave out DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED. We would create an empty
		 *       region that couldn't hold data, but when mem->next gets freed,
		 *       the 2 regions would be combined, resulting in more free memory */
		ptr2 = (da16x_dpm_rtm_mem_size_t)(ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
										  newsize);
		LWIP_ASSERT("invalid next ptr", mem->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
		mem2 = ptr_to_mem(ptr2);
		if (mem2 < lfree) {
			lfree = mem2;
		}
		mem2->used = 0;
		mem2->next = mem->next;
		mem2->prev = ptr;
		mem->next = ptr2;
		if (mem2->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED) {
			ptr_to_mem(mem2->next)->prev = ptr2;
		}
		/* the original mem->next is used, so no need to plug holes! */
	}
	/* else {
	   next struct da16x_dpm_rtm_mem is used but size between mem and mem2 is not big enough
	   to create another struct da16x_dpm_rtm_mem
	   -> don't do anyhting.
	   -> the remaining space stays unused since it is too small
	   } */
#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
	da16x_dpm_rtm_mem_overflow_init_element(mem, new_size);
#endif
	DA16X_DPM_RTM_MEM_SANITY();
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
	da16x_dpm_rtm_mem_free_count = 1;
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
	DA16X_DPM_RTM_MEM_FREE_UNPROTECT();
	return rmem;
}

/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size_in is the minimum size of the requested block in bytes.
 * @return pointer to allocated memory or NULL if no free memory was found.
 *
 * Note that the returned value will always be aligned (as defined by MEM_ALIGNMENT).
 */
void *
da16x_dpm_rtm_mem_malloc(da16x_dpm_rtm_mem_size_t size_in)
{
	da16x_dpm_rtm_mem_size_t ptr, ptr2, size;
	struct da16x_dpm_rtm_mem *mem, *mem2;
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
	u8_t local_mem_free_count = 0;
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
	DA16X_DPM_RTM_MEM_ALLOC_DECL_PROTECT();

	if (size_in == 0) {
		return NULL;
	}

	/* Expand the size of the allocated memory region so that we can
	   adjust for alignment. */
	size = (da16x_dpm_rtm_mem_size_t)LWIP_MEM_ALIGN_SIZE(size_in);
	if (size < DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED) {
		/* every data block must be at least DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED long */
		size = DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED;
	}
#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
	size += DA16X_DPM_RTM_MEM_SANITY_REGION_BEFORE_ALIGNED +
			DA16X_DPM_RTM_MEM_SANITY_REGION_AFTER_ALIGNED;
#endif
	if ((size > DA16X_DPM_RTM_MEM_SIZE_ALIGNED) || (size < size_in)) {
		return NULL;
	}

	/* protect the heap from concurrent access */
	sys_mutex_lock(&da16x_dpm_rtm_mem_mutex);
	DA16X_DPM_RTM_MEM_ALLOC_PROTECT();
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
	/* run as long as a da16x_dpm_rtm_mem_free disturbed da16x_dpm_rtm_mem_malloc or da16x_dpm_rtm_mem_trim */
	do {
		local_mem_free_count = 0;
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

		/* Scan through the heap searching for a free block that is big enough,
		 * beginning with the lowest free block.
		 */
		for (ptr = mem_to_ptr(lfree); ptr < DA16X_DPM_RTM_MEM_SIZE_ALIGNED - size;
			 ptr = ptr_to_mem(ptr)->next) {
			mem = ptr_to_mem(ptr);
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
			da16x_dpm_rtm_mem_free_count = 0;
			DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT();
			/* allow da16x_dpm_rtm_mem_free_count or da16x_dpm_rtm_mem_trim to run */
			DA16X_DPM_RTM_MEM_ALLOC_PROTECT();
			if (da16x_dpm_rtm_mem_free_count != 0) {
				/* If da16x_dpm_rtm_mem_free_count or da16x_dpm_rtm_mem_trim have run, we have to restart since they
				   could have altered our current struct da16x_dpm_rtm_mem. */
				local_mem_free_count = 1;
				break;
			}
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

			if ((!mem->used) &&
				(mem->next - (ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM)) >= size) {
				/* mem is not used and at least perfect fit is possible:
				 * mem->next - (ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM) gives us the 'dpm data size' of mem */

				if (mem->next - (ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM) >=
					(size + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
					 DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED)) {
					/* (in addition to the above, we test if another struct da16x_dpm_rtm_mem (SIZEOF_STRUCT_DA16X_DPM_RTM_MEM) containing
					 * at least DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED of data also fits in the 'dpm data space' of 'mem')
					 * -> split large block, create empty remainder,
					 * remainder must be large enough to contain DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED data: if
					 * mem->next - (ptr + (2*SIZEOF_STRUCT_DA16X_DPM_RTM_MEM)) == size,
					 * struct da16x_dpm_rtm_mem would fit in but no data between mem2 and mem2->next
					 * @todo we could leave out DA16X_DPM_RTM_MEM_MIN_SIZE_ALIGNED. We would create an empty
					 *       region that couldn't hold data, but when mem->next gets freed,
					 *       the 2 regions would be combined, resulting in more free memory
					 */
					ptr2 = (da16x_dpm_rtm_mem_size_t)(ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM + size);
					LWIP_ASSERT("invalid next ptr", ptr2 != DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
					/* create mem2 struct */
					mem2 = ptr_to_mem(ptr2);
					mem2->used = 0;
					mem2->next = mem->next;
					mem2->prev = ptr;
					/* and insert it between mem and mem->next */
					mem->next = ptr2;
					mem->used = 1;

					if (mem2->next != DA16X_DPM_RTM_MEM_SIZE_ALIGNED) {
						ptr_to_mem(mem2->next)->prev = ptr2;
					}
				} else {
					/* (a mem2 struct does no fit into the dpm data space of mem and mem->next will always
					 * be used at this point: if not we have 2 unused structs in a row, da16x_dpm_rtm_mem_plug_holes should have
					 * take care of this).
					 * -> near fit or exact fit: do not split, no mem2 creation
					 * also can't move mem->next directly behind mem, since mem->next
					 * will always be used at this point!
					 */
					mem->used = 1;
				}
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
mem_malloc_adjust_lfree:
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
				if (mem == lfree) {
					struct da16x_dpm_rtm_mem *cur = lfree;
					/* Find next free block after mem and update lowest free pointer */
					while (cur->used && cur != rtm_end) {
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
						da16x_dpm_rtm_mem_free_count = 0;
						DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT();
						/* prevent high interrupt latency... */
						DA16X_DPM_RTM_MEM_ALLOC_PROTECT();
						if (da16x_dpm_rtm_mem_free_count != 0) {
							/* If da16x_dpm_rtm_mem_free_count or da16x_dpm_rtm_mem_trim have run, we have to restart since they
							   could have altered our current struct da16x_dpm_rtm_mem or lfree. */
							goto mem_malloc_adjust_lfree;
						}
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
						cur = ptr_to_mem(cur->next);
					}
					lfree = cur;
					LWIP_ASSERT("da16x_dpm_rtm_mem_malloc: !lfree->used", ((lfree == rtm_end)
								|| (!lfree->used)));
				}
				DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT();
				sys_mutex_unlock(&da16x_dpm_rtm_mem_mutex);
				LWIP_ASSERT("da16x_dpm_rtm_mem_malloc: allocated memory not above rtm_end.",
							(mem_ptr_t)mem + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM + size <= (mem_ptr_t)rtm_end);
				LWIP_ASSERT("da16x_dpm_rtm_mem_malloc: allocated memory properly aligned.",
							((mem_ptr_t)mem + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM) % MEM_ALIGNMENT == 0);
				LWIP_ASSERT("da16x_dpm_rtm_mem_malloc: sanity check alignment",
							(((mem_ptr_t)mem) & (MEM_ALIGNMENT - 1)) == 0);

#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
				da16x_dpm_rtm_mem_overflow_init_element(mem, size_in);
#endif
				DA16X_DPM_RTM_MEM_SANITY();
				return (u8_t *)mem + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM +
					   DA16X_DPM_RTM_MEM_SANITY_OFFSET;
			}
		}
#if DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
		/* if we got interrupted by a da16x_dpm_rtm_mem_free_count, try again */
	} while (local_mem_free_count != 0);
#endif /* DA16X_DPM_RTM_MEM_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
	da16x_dpm_rtm_mem_alloc_fail_cnt++;
	DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT();
	sys_mutex_unlock(&da16x_dpm_rtm_mem_mutex);
	//LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SERIOUS, "da16x_dpm_rtm_mem_malloc: could not allocate %"S16_F" bytes\n", (s16_t)size);
	return NULL;
}

void da16x_dpm_rtm_mem_status()
{
	da16x_dpm_rtm_mem_size_t ptr;
	struct da16x_dpm_rtm_mem *mem;
	DA16X_DPM_RTM_MEM_ALLOC_DECL_PROTECT();
	da16x_dpm_rtm_mem_size_t used_mem = 0;
	da16x_dpm_rtm_mem_size_t free_mem = 0;
	da16x_dpm_rtm_mem_size_t node_count = 0;

	PRINTF("\r\n");

	/* Scan through the heap searching for a free block that is big enough,
	 * beginning with the lowest free block.
	 */
	PRINTF(CYAN_COLOR "\r\n << DPM RTM MEM STATUS >> \r\n" CLEAR_COLOR);

	/* protect the heap from concurrent access */
	sys_mutex_lock(&da16x_dpm_rtm_mem_mutex);
	DA16X_DPM_RTM_MEM_ALLOC_PROTECT();

	for (ptr = mem_to_ptr(rtm); ptr < DA16X_DPM_RTM_MEM_SIZE_ALIGNED;
		 ptr = ptr_to_mem(ptr)->next) {
		mem = ptr_to_mem(ptr);
		PRINTF(" mem: 0x%x used:%d next:0x%x prev:0x%x size:%d \r\n", mem, mem->used,
			   mem->next, mem->prev, mem->next - (ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM));

		node_count++;

		if (!mem->used)  {
			free_mem += (mem->next - (ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM));
		} else if (mem->used) {
			used_mem += (mem->next - (ptr + SIZEOF_STRUCT_DA16X_DPM_RTM_MEM));
		} else {
			PRINTF(RED_COLOR " [%s] Fail check \r\n" CLEAR_COLOR, __func__);
		}
	}

	DA16X_DPM_RTM_MEM_ALLOC_UNPROTECT();
	sys_mutex_unlock(&da16x_dpm_rtm_mem_mutex);

	PRINTF(CYAN_COLOR " Total mem  : %d \r\n" CLEAR_COLOR, DA16X_DPM_RTM_MEM_SIZE_ALIGNED);
	PRINTF(CYAN_COLOR " node cnt   : %d \r\n" CLEAR_COLOR, node_count);
	PRINTF(CYAN_COLOR " error_cnt  : %d \r\n" CLEAR_COLOR,
		   da16x_dpm_rtm_mem_alloc_fail_cnt);
	PRINTF(CYAN_COLOR " illegal cnt: %d \r\n" CLEAR_COLOR,
		   da16x_dpm_rtm_mem_alloc_illegal_cnt);
	PRINTF(CYAN_COLOR " Used_mem   : %d \r\n" CLEAR_COLOR, used_mem);
	PRINTF(CYAN_COLOR " Free mem   : %d \r\n" CLEAR_COLOR, free_mem);

	return;
}

/**
 * Contiguously allocates enough space for count objects that are size bytes
 * of memory each and returns a pointer to the allocated memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *
da16x_dpm_rtm_mem_calloc(da16x_dpm_rtm_mem_size_t count,
						 da16x_dpm_rtm_mem_size_t size)
{
	void *p;
	size_t alloc_size = (size_t)count * (size_t)size;

	if ((size_t)(da16x_dpm_rtm_mem_size_t)alloc_size != alloc_size) {
		LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
					"da16x_dpm_rtm_mem_calloc: could not allocate %"SZT_F" bytes\n", alloc_size);
		return NULL;
	}

	/* allocate 'count' objects of size 'size' */
	p = da16x_dpm_rtm_mem_malloc((da16x_dpm_rtm_mem_size_t)alloc_size);
	if (p) {
		/* zero the memory */
		memset(p, 0, alloc_size);
	}
	return p;
}
