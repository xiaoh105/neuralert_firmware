/**
 ****************************************************************************************
 *
 * @file sys_sort.c
 *
 * @brief Sorting algorithm
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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oal.h"
#include "sal.h"
#include "driver.h"
#include "library/library.h"


/******************************************************************************
 *
 *	MACRO
 *
 ******************************************************************************/

#define	SORT_THRESHOLD		8

#define	SORT_MEMCPY(...)	SAL_MEMCPY( __VA_ARGS__ )

/******************************************************************************
 *  SwapSort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


static void SwapSort(void *a, size_t size, void *temp, sort_comp_func *comp)
{

	if( comp((void *)((UINT32)a + size), a) < 0 ){
		SORT_MEMCPY(temp, (void *)((UINT32)a + size), size);
		SORT_MEMCPY((void *)((UINT32)a + size), a, size);
		SORT_MEMCPY(a, temp, size);
	}
}

#ifdef	SORT_FULL_LIB
/******************************************************************************
 *  da16x_shellsort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void da16x_shellsortInner(void *a, size_t num, size_t size, void *temp, sort_comp_func *comp)
{
	int h, i, j;
	void *t;

	t = temp;

	for (h = num >> 1; h > 0; h = h >> 1 ) {
		for (i = h; i < num; i++) {
			SORT_MEMCPY(t, (void *)((UINT32)a + (i*size)), size);
			//t = a[i*size];

			for (j = i
			     ; (j >= h)
				&&  (comp(t, (void *)((UINT32)a + ((j-h)*size))) < 0)
			     ; j -= h) {

				SORT_MEMCPY((void *)((UINT32)a + (j*size)), (void *)((UINT32)a + ((j-h)*size)), size);
				//a[j*size] = a[(j - h)*size];
			}

			SORT_MEMCPY((void *)((UINT32)a + (j*size)), t, size);
			//a[j*size] = t;
		}
	}
}

int da16x_shellsort(void *item, size_t num, size_t size, sort_comp_func *comp)
{
	if( num <= 1 ){
		return 0;
	}
	if( size > SORT_TEMPSIZE ){
		return -1;
	}

	{
		UINT32 *sortbuf = (UINT32 *)SAL_MALLOC(sizeof(UINT32)*SORT_TEMPSIZE);
		if( sortbuf == NULL ){
			return -1;
		}
		da16x_shellsortInner(item, num, size, sortbuf, comp);
		SAL_FREE(sortbuf);
	}
	return 1;
}

#endif	//SORT_FULL_LIB

/******************************************************************************
 *  da16x_quicksort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int da16x_qspartition( void *a, int l, int r, size_t size, void *pivot, sort_comp_func *comp)
{

	void *t;
	int i, j;

	t =  (void *)((UINT32)pivot + size);

	SORT_MEMCPY(pivot, (void *)((UINT32)a + ((size_t)l*size)), size);
	//pivot = a[(l*size)];

	i = l; j = r+1;

	while( 1)
	{
		do{
			++i;
		}while( comp((void *)((UINT32)a + ((size_t)i*size)), pivot) <= 0 && i <= r );

		do{
			--j;
		}while( comp((void *)((UINT32)a + ((size_t)j*size)), pivot) > 0 );

		if( i >= j ) break;

		SORT_MEMCPY(t, (void *)((UINT32)a + ((size_t)i*size)), size);
		//t = a[i*size];
		SORT_MEMCPY((void *)((UINT32)a + ((size_t)i*size)), (void *)((UINT32)a + ((size_t)j*size)), size);
		//a[i*size] = a[j*size];
		SORT_MEMCPY((void *)((UINT32)a + ((size_t)j*size)), t, size);
		//a[j*size] = t;
	}

	SORT_MEMCPY(t, (void *)((UINT32)a + ((size_t)l*size)), size);
	//t = a[l*size];
	SORT_MEMCPY((void *)((UINT32)a + ((size_t)l*size)), (void *)((UINT32)a + ((size_t)j*size)), size);
	//a[l*size] = a[j*size];
	SORT_MEMCPY((void *)((UINT32)a + ((size_t)j*size)), t, size);
	//a[j*size] = t;

	return j;
}

void da16x_quicksortInner( void *a, int l, int r, size_t size, void *temp, sort_comp_func *comp)
{
	if( l < r )
	{
		int j;
		// divide and conquer
		j = da16x_qspartition( a, l, r, size, temp, comp);

		da16x_quicksortInner( a, l, j-1, size, temp, comp);
		da16x_quicksortInner( a, j+1, r, size, temp, comp);
	}
}

int da16x_quicksort(void *item, size_t num, size_t size, sort_comp_func *comp)
{
	if( num <= 1 ){
		return 0;
	}
	if( (size<<1) > SORT_TEMPSIZE ){
		return -1;
	}

	{
		UINT32 *sortbuf = (UINT32 *)SAL_MALLOC(sizeof(UINT32)*SORT_TEMPSIZE);
		if( sortbuf == NULL ){
			return -1;
		}
		da16x_quicksortInner(item, 0, (int)num, size, sortbuf, comp);
		SAL_FREE(sortbuf);
	}
	return 1;
}

#ifdef	SORT_FULL_LIB
/******************************************************************************
 *  da16x_heapsort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void da16x_heapsortInner(void *a, int num, size_t size, void *temp, sort_comp_func *comp)
{
	unsigned int n, i, parent, child;
	void *t;

	n = (unsigned int)num;
	i = (n >> 1);
	t = temp;

	for (;;) {
		if (i > 0) {
			i--;
			SORT_MEMCPY(t, (void *)((UINT32)a + (i*size)), size);
			//t = a[i*size];
		} else {
			n--;
			if (n == 0) return;

			SORT_MEMCPY(t, (void *)((UINT32)a + (n*size)), size);
			//t = a[n*size];
			SORT_MEMCPY((void *)((UINT32)a + (n*size)), (a), size);
			//a[n*size] = a[0];
		}

		parent = i;
		child = (i<<1) + 1;

		while (child < n) {

			if ((child + 1) < n
				&&  comp((void *)((UINT32)a + ((child + 1)*size)), (void *)((UINT32)a + (child*size))) > 0 ){
				child++;
			}

			if( comp((void *)((UINT32)a + (child*size)), t) > 0 ) {

				SORT_MEMCPY((void *)((UINT32)a + (parent*size)), (void *)((UINT32)a + (child*size)), size);
				//a[parent*size] = a[child*size];
				parent = child;
				//child = parent*2-1; /* Find the next child */
				child = (parent<<1)+1;
			} else {
				break;
			}
		}

		SORT_MEMCPY((void *)((UINT32)a + (parent*size)), t, size);
		//a[parent*size] = t;
	}
}

int da16x_heapsort(void *item, int num, size_t size, sort_comp_func *comp)
{
	if( num <= 1 ){
		return 0;
	}
	if( (size) > SORT_TEMPSIZE ){
		return -1;
	}

	{
		UINT32 *sortbuf = (UINT32 *)SAL_MALLOC(sizeof(UINT32)*SORT_TEMPSIZE);
		if( sortbuf == NULL ){
			return -1;
		}
		da16x_heapsortInner(item, num, size, sortbuf, comp);
		SAL_FREE(sortbuf);
	}
	return 1;
}

#endif	//SORT_FULL_LIB

/******************************************************************************
 *  da16x_mergesort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void da16x_MergeCore(void *a, int high, int mid, size_t size, UINT32 *b, sort_comp_func *comp)
{
	int i, j, k;

	for (i = 0, j = mid, k = 0; k < high; k++) {
		if( j == high ){
			SORT_MEMCPY((void *)((UINT32)b + ((size_t)k*size)), (void *)((UINT32)a + ((size_t)i*size)), size);
			//b[k*size] = a[i*size];
			i++;
		}else{
			if( i == mid ){
				SORT_MEMCPY((void *)((UINT32)b + ((size_t)k*size)), (void *)((UINT32)a + ((size_t)j*size)), size);
				//b[k*size] = a[j*size];
				j++;
			}else {

				if( comp((void *)((UINT32)a+((size_t)j*size)), (void *)((UINT32)a+((size_t)i*size))) < 0 ){
					SORT_MEMCPY((void *)((UINT32)b + ((size_t)k*size)), (void *)((UINT32)a + ((size_t)j*size)), size);
					//b[k*size] = a[j*size];
					j++;
				}else{
					SORT_MEMCPY((void *)((UINT32)b + ((size_t)k*size)), (void *)((UINT32)a + ((size_t)i*size)), size);
					//b[k*size] = a[i*size];
					i++;
				}
			}
		}
	}

	for (i = 0; i < high; i++) {
		SORT_MEMCPY((void *)((UINT32)a + ((size_t)i*size)), (void *)((UINT32)b + ((size_t)i*size)), size);
		//a[i*size] = b[i*size];
	}
}

void da16x_mergesortInner(void *a, int high, size_t size, UINT32 *tmp, sort_comp_func *comp)
{
	int m = high>>1;

	if( high <= 1 ){
		return;
	}

	if( high < SORT_THRESHOLD ){
		da16x_insertsortInner(a, high, size, tmp, comp);
		return;
	}

	m = high >> 1;

	da16x_mergesortInner(a, m, size, tmp, comp);
	da16x_mergesortInner((void *)((UINT32)a + ((UINT32)m*size)), (high-m), size, tmp, comp);
	da16x_MergeCore(a, high, m, size, tmp, comp);
}

/******************************************************************************
 *  da16x_insertsort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void da16x_insertsortInner(void *a, int num, size_t size, UINT32 *tmp, sort_comp_func *comp)
{
	int i, j;
	void *t;

	if( num == 2 ){
		SwapSort(a, size, tmp, comp);
		return;
	}

	t = tmp;

	for(i = 1; i < num; ++i) {
		SORT_MEMCPY(t, (void *)((UINT32)a + ((size_t)i*size)), size);
		//t = a[i*size];

		j = i;

		while(j > 0 && (comp(t, (void *)((UINT32)a+((size_t)(j-1)*size))) < 0)) {
			SORT_MEMCPY((void *)((UINT32)a + ((size_t)j*size)), (void *)((UINT32)a + ((size_t)(j-1)*size)), size);
			//a[j*size] = a[(j - 1)*size];
			--j;
		}

		if( j != i ){
			SORT_MEMCPY((void *)((UINT32)a + ((size_t)j*size)), t, size);
			//a[j*size] = t;
		}
	}
}

int da16x_insertsort(void *a, int num, size_t size, sort_comp_func *comp)
{
	if( num <= 1 ){
		return 0;
	}
	if( (size) > SORT_TEMPSIZE ){
		return -1;
	}

	{
		UINT32 *sortbuf = (UINT32 *)SAL_MALLOC(sizeof(UINT32)*SORT_TEMPSIZE);
		if( sortbuf == NULL ){
			return -1;
		}
		da16x_insertsortInner(a, num, size, sortbuf, comp);
		SAL_FREE(sortbuf);
	}
	return 1;
}

/******************************************************************************
 *  da16x_mquicksort ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void da16x_mquicksortInner( void *a, int l, int r, size_t size, void *temp, sort_comp_func *comp)
{
	if( l < r )
	{
		int j;
		// divide and conquer
		j = da16x_qspartition( a, l, r, size, temp, comp);

		if( (j - l) < SORT_THRESHOLD ){
			da16x_insertsortInner( (void *)((UINT32)a+((UINT32)l*size)), (j-l), size, temp, comp );
		}else{
			da16x_mquicksortInner( a, l, (j-1), size, temp, comp);
		}

		da16x_mquicksortInner( a, (j+1), r, size, temp, comp);
	}
}

int da16x_mquicksort(void *item, size_t num, size_t size, sort_comp_func *comp)
{
	if( num <= 1 ){
		return 0;
	}
	if( (size<<1) > SORT_TEMPSIZE ){
		return -1;
	}

	{
		UINT32 *sortbuf = (UINT32 *)SAL_MALLOC(sizeof(UINT32)*SORT_TEMPSIZE);
		if( sortbuf == NULL ){
			return -1;
		}
		da16x_mquicksortInner(item, 0, (int)num, size, sortbuf, comp);
		SAL_FREE(sortbuf);
	}
	return 1;
}

/******************************************************************************
 *  da16x_crc8ccitt ( )
 *
 *  Purpose : x8 + x2 + x + 1
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static const UINT8 da16x_crc8ccitt_table[256] = {
/* CRC-8-CCITT : x^8 + x^2 + x + 1 */
	0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
	0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
	0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
	0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
	0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
	0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
	0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
	0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
	0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
	0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
	0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
	0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
	0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
	0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
	0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
	0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
	0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
	0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
	0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
	0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
	0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
	0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
	0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
	0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
	0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
	0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
	0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
	0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
	0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
	0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
	0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
	0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

UINT8 da16x_crc8ccitt(void *data, size_t siz, UINT8 seed)
{
	UINT8 val = seed;

	UINT8 * pos = (UINT8 *) data;
	UINT8 * end = pos + siz;

	while (pos < end) {
		val = da16x_crc8ccitt_table[val ^ *pos];
		pos++;
	}

	return val;
}

/* EOF */
